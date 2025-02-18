// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2011-2014, Intel Corporation.
 * Copyright (c) 2017-2021 Christoph Hellwig.
 */
#ifdef HAVE_BIO_INTEGRITY_H
#include <linux/bio-integrity.h>
#endif
#ifdef HAVE_BLK_INTEGRITY_H
#include <linux/blk-integrity.h>
#endif
#include <linux/ptrace.h>	/* for force_successful_syscall_return */
#include <linux/nvme_ioctl.h>
#ifdef HAVE_IO_URING_CMD_H
#include <linux/io_uring/cmd.h>
#endif
#include "nvme.h"

enum {
	NVME_IOCTL_VEC		= (1 << 0),
	NVME_IOCTL_PARTITION	= (1 << 1),
};

static bool nvme_cmd_allowed(struct nvme_ns *ns, struct nvme_command *c,
#ifdef HAVE_BDEV_IS_PARTITION
		unsigned int flags, bool open_for_write)
#else
		bool open_for_write)
#endif
{
	u32 effects;

#ifdef HAVE_BDEV_IS_PARTITION
	/*
	 * Do not allow unprivileged passthrough on partitions, as that allows an
	 * escape from the containment of the partition.
	 */
	if (flags & NVME_IOCTL_PARTITION)
		goto admin;
#endif

	/*
	 * Do not allow unprivileged processes to send vendor specific or fabrics
	 * commands as we can't be sure about their effects.
	 */
	if (c->common.opcode >= nvme_cmd_vendor_start ||
	    c->common.opcode == nvme_fabrics_command)
		goto admin;

	/*
	 * Do not allow unprivileged passthrough of admin commands except
	 * for a subset of identify commands that contain information required
	 * to form proper I/O commands in userspace and do not expose any
	 * potentially sensitive information.
	 */
	if (!ns) {
		if (c->common.opcode == nvme_admin_identify) {
			switch (c->identify.cns) {
			case NVME_ID_CNS_NS:
			case NVME_ID_CNS_CS_NS:
			case NVME_ID_CNS_NS_CS_INDEP:
			case NVME_ID_CNS_CS_CTRL:
			case NVME_ID_CNS_CTRL:
				return true;
			}
		}
		goto admin;
	}

	/*
	 * Check if the controller provides a Commands Supported and Effects log
	 * and marks this command as supported.  If not reject unprivileged
	 * passthrough.
	 */
	effects = nvme_command_effects(ns->ctrl, ns, c->common.opcode);
	if (!(effects & NVME_CMD_EFFECTS_CSUPP))
		goto admin;

	/*
	 * Don't allow passthrough for command that have intrusive (or unknown)
	 * effects.
	 */
	if (effects & ~(NVME_CMD_EFFECTS_CSUPP | NVME_CMD_EFFECTS_LBCC |
			NVME_CMD_EFFECTS_UUID_SEL |
			NVME_CMD_EFFECTS_SCOPE_MASK))
		goto admin;

	/*
	 * Only allow I/O commands that transfer data to the controller or that
	 * change the logical block contents if the file descriptor is open for
	 * writing.
	 */
	if ((nvme_is_write(c) || (effects & NVME_CMD_EFFECTS_LBCC)) &&
	    !open_for_write)
		goto admin;

	return true;
admin:
	return capable(CAP_SYS_ADMIN);
}

/*
 * Convert integer values from ioctl structures to user pointers, silently
 * ignoring the upper bits in the compat case to match behaviour of 32-bit
 * kernels.
 */
static void __user *nvme_to_user_ptr(uintptr_t ptrval)
{
#if defined(HAVE_IN_COMPAT_SYSCALL) && defined(HAVE_COMPAT_UPTR_T)
	if (in_compat_syscall())
		ptrval = (compat_uptr_t)ptrval;
#endif
	return (void __user *)ptrval;
}

#ifndef HAVE_BIO_INTEGRITY_MAP_USER
static void *nvme_add_user_metadata(struct request *req, void __user *ubuf,
	       unsigned len, u32 seed)
{
       struct bio_integrity_payload *bip;
       int ret = -ENOMEM;
       void *buf;
       struct bio *bio = req->bio;

       buf = kmalloc(len, GFP_KERNEL);
       if (!buf)
	       goto out;

       if (req_op(req) == REQ_OP_DRV_OUT) {
	       ret = -EFAULT;
	       if (copy_from_user(buf, ubuf, len))
		       goto out_free_meta;
       } else {
	       memset(buf, 0, len);
       }

       bip = bio_integrity_alloc(bio, GFP_KERNEL, 1);
       if (IS_ERR(bip)) {
	       ret = PTR_ERR(bip);
	       goto out_free_meta;
       }

/* Using 5e70d0acf082 instead of 80814b8e359 as it has no new symbols */
#ifndef HAVE_PCIE_CAPABILITY_CLEAR_AND_SET_WORD_LOCKED
	bip->bip_iter.bi_size = len;
#endif
       bip->bip_iter.bi_sector = seed;
       ret = bio_integrity_add_page(bio, virt_to_page(buf), len,
		       offset_in_page(buf));
       if (ret != len) {
	       ret = -ENOMEM;
	       goto out_free_meta;
       }

       req->cmd_flags |= REQ_INTEGRITY;
       return buf;
out_free_meta:
       kfree(buf);
out:
       return ERR_PTR(ret);
}

static int nvme_finish_user_metadata(struct request *req, void __user *ubuf,
	       void *meta, unsigned len, int ret)
{
       if (!ret && req_op(req) == REQ_OP_DRV_IN &&
	   copy_to_user(ubuf, meta, len))
	       ret = -EFAULT;
       kfree(meta);
       return ret;
}
#endif

static struct request *nvme_alloc_user_request(struct request_queue *q,
		struct nvme_command *cmd,
#ifdef HAVE_BLK_OPF_T
		blk_opf_t rq_flags, blk_mq_req_flags_t blk_flags)
#else
		unsigned int rq_flags, blk_mq_req_flags_t blk_flags)
#endif
{
	struct request *req;

	req = blk_mq_alloc_request(q, nvme_req_op(cmd) | rq_flags, blk_flags);
	if (IS_ERR(req))
		return req;
	nvme_init_request(req, cmd);
	nvme_req(req)->flags |= NVME_REQ_USERCMD;
	return req;
}

static int nvme_map_user_request(struct request *req, u64 ubuffer,
		unsigned bufflen, void __user *meta_buffer, unsigned meta_len,
		u32 meta_seed,
#ifdef HAVE_IO_URING_CMD_H
		struct io_uring_cmd *ioucmd,
#endif
#ifndef HAVE_BIO_INTEGRITY_MAP_USER
		void **metap,
#endif
		unsigned int flags)
{
	struct request_queue *q = req->q;
	struct nvme_ns *ns = q->queuedata;
#ifdef HAVE_ENUM_BIO_REMAPPED
	struct block_device *bdev = ns ? ns->disk->part0 : NULL;
#else
	struct gendisk *disk = ns ? ns->disk : NULL;
#endif
	struct bio *bio = NULL;
#ifndef HAVE_BIO_INTEGRITY_MAP_USER
	void *meta = NULL;
#endif
	int ret;

#ifdef HAVE_NVME_IOCTL_IO64_CMD_VEC
#ifdef HAVE_BLK_RQ_MAP_USER_IO
#ifdef HAVE_IO_URING_CMD_H
	if (ioucmd && (ioucmd->flags & IORING_URING_CMD_FIXED)) {
		struct iov_iter iter;

		/* fixedbufs is only for non-vectored io */
		if (WARN_ON_ONCE(flags & NVME_IOCTL_VEC))
			return -EINVAL;
		ret = io_uring_cmd_import_fixed(ubuffer, bufflen,
				rq_data_dir(req), &iter, ioucmd);
		if (ret < 0)
			goto out;
		ret = blk_rq_map_user_iov(q, req, NULL, &iter, GFP_KERNEL);
	} else {
		ret = blk_rq_map_user_io(req, NULL, nvme_to_user_ptr(ubuffer),
				bufflen, GFP_KERNEL, flags & NVME_IOCTL_VEC, 0,
				0, rq_data_dir(req));
	}
#else
	ret = blk_rq_map_user_io(req, NULL, nvme_to_user_ptr(ubuffer), bufflen,
			GFP_KERNEL, flags & NVME_IOCTL_VEC, 0, 0, rq_data_dir(req));
#endif
#else
	if (!((flags & NVME_IOCTL_VEC)))
		ret = blk_rq_map_user(q, req, NULL,  nvme_to_user_ptr(ubuffer), bufflen,
			GFP_KERNEL);
	else {
		struct iovec fast_iov[UIO_FASTIOV];
		struct iovec *iov = fast_iov;
		struct iov_iter iter;

		ret = import_iovec(rq_data_dir(req), nvme_to_user_ptr(ubuffer), bufflen,
				UIO_FASTIOV, &iov, &iter);
		if (ret < 0)
			goto out;
		ret = blk_rq_map_user_iov(q, req, NULL, &iter,
				GFP_KERNEL);
		kfree(iov);
	}
#endif /* HAVE_BLK_RQ_MAP_USER_IO */
#else
	ret = blk_rq_map_user(q, req, NULL, nvme_to_user_ptr(ubuffer), bufflen,
			GFP_KERNEL);
#endif /* HAVE_NVME_IOCTL_IO64_CMD_VEC */

	if (ret)
		goto out;

	bio = req->bio;
#ifdef HAVE_BIO_INTEGRITY_MAP_USER
	if (bdev) {
		bio_set_dev(bio, bdev);
		if (meta_buffer && meta_len) {
#ifdef HAVE_BLK_RQ_INTEGRITY_MAP_USER
			ret = blk_rq_integrity_map_user(req, meta_buffer, meta_len,
							meta_seed);
#else
			ret = bio_integrity_map_user(bio, meta_buffer, meta_len,
						     meta_seed);
#endif
			if (ret)
				goto out_unmap;
#ifndef HAVE_BLK_RQ_INTEGRITY_MAP_USER
			req->cmd_flags |= REQ_INTEGRITY;
#endif
		}
	}
#else
#ifdef HAVE_BIO_BI_DISK
	bio->bi_disk = disk;
	if (disk && meta_buffer && meta_len) {
#elif defined HAVE_ENUM_BIO_REMAPPED
	if (bdev)
		bio_set_dev(bio, bdev);

	if (bdev && meta_buffer && meta_len) {
#else
	if (disk) {
		bio->bi_bdev = bdget_disk(disk, 0);
		if (!bio->bi_bdev) {
			ret = -ENODEV;
			goto out_unmap;
		}
	}
	if (disk && meta_buffer && meta_len) {
#endif /* HAVE_BIO_BI_DISK */
		meta = nvme_add_user_metadata(req, meta_buffer, meta_len,
					meta_seed);

		if (IS_ERR(meta)) {
			ret = PTR_ERR(meta);
			goto out_unmap;
		}
		*metap = meta;
	}
#endif /* HAVE_BIO_INTEGRITY_MAP_USER */

	return ret;

out_unmap:
	if (bio)
		blk_rq_unmap_user(bio);
out:
	blk_mq_free_request(req);
	return ret;
}

static int nvme_submit_user_cmd(struct request_queue *q,
		struct nvme_command *cmd, u64 ubuffer, unsigned bufflen,
		void __user *meta_buffer, unsigned meta_len, u32 meta_seed,
		u64 *result, unsigned timeout, unsigned int flags)
{
#if defined(HAVE_BLK_EXECUTE_RQ_2_PARAM) || defined(HAVE_BLK_EXECUTE_RQ_3_PARAM)
	struct nvme_ns *ns = q->queuedata;
#endif
	struct nvme_ctrl *ctrl;
	struct request *req;
#ifndef HAVE_BIO_INTEGRITY_MAP_USER
	void *meta = NULL;
#endif
	struct bio *bio;
#if defined(HAVE_BLK_EXECUTE_RQ_2_PARAM) || defined(HAVE_BLK_EXECUTE_RQ_3_PARAM)
	u32 effects;
#endif
	int ret;

	req = nvme_alloc_user_request(q, cmd, 0, 0);
	if (IS_ERR(req))
		return PTR_ERR(req);

	req->timeout = timeout;
	if (ubuffer && bufflen) {
		ret = nvme_map_user_request(req, ubuffer, bufflen, meta_buffer,
				meta_len, meta_seed,
#ifdef HAVE_IO_URING_CMD_H
				NULL,
#endif
#ifndef HAVE_BIO_INTEGRITY_MAP_USER
				&meta,
#endif
				flags);

		if (ret)
			return ret;
	}

	bio = req->bio;
	ctrl = nvme_req(req)->ctrl;

#if defined(HAVE_BLK_EXECUTE_RQ_2_PARAM) || defined(HAVE_BLK_EXECUTE_RQ_3_PARAM)
	effects = nvme_passthru_start(ctrl, ns, cmd->common.opcode);
#if defined(HAVE_BLK_EXECUTE_RQ_2_PARAM)
	ret = nvme_execute_rq(req, false);
#else
	ret = nvme_execute_rq(NULL, req, false);
#endif
#else
	nvme_execute_rq(req);
	if (nvme_req(req)->flags & NVME_REQ_CANCELLED)
		ret = -EINTR;
	else
		ret = nvme_req(req)->status;
#endif
	if (result)
		*result = le64_to_cpu(nvme_req(req)->result.u64);
#ifndef HAVE_BIO_INTEGRITY_MAP_USER
	if (meta) {
		ret = nvme_finish_user_metadata(req, meta_buffer, meta,
						meta_len, ret);
	}
#endif
#if defined HAVE_BIO_BI_DISK || defined HAVE_ENUM_BIO_REMAPPED
	if (bio)
		blk_rq_unmap_user(bio);
#else
	if (bio) {
		struct nvme_ns *ns = q->queuedata;
		struct gendisk *disk = ns ? ns->disk : NULL;
		if (disk && bio->bi_bdev)
			bdput(bio->bi_bdev);
		blk_rq_unmap_user(bio);
	}
#endif
	blk_mq_free_request(req);

#if defined(HAVE_BLK_EXECUTE_RQ_2_PARAM) || defined(HAVE_BLK_EXECUTE_RQ_3_PARAM)
	if (effects)
		nvme_passthru_end(ctrl, ns, effects, cmd, ret);
#endif

	return ret;
}

static int nvme_submit_io(struct nvme_ns *ns, struct nvme_user_io __user *uio)
{
	struct nvme_user_io io;
	struct nvme_command c;
	unsigned length, meta_len;
	void __user *metadata;

	if (copy_from_user(&io, uio, sizeof(io)))
		return -EFAULT;
	if (io.flags)
		return -EINVAL;

	switch (io.opcode) {
	case nvme_cmd_write:
	case nvme_cmd_read:
	case nvme_cmd_compare:
		break;
	default:
		return -EINVAL;
	}

	length = (io.nblocks + 1) << ns->head->lba_shift;

	if ((io.control & NVME_RW_PRINFO_PRACT) &&
	    (ns->head->ms == ns->head->pi_size)) {
		/*
		 * Protection information is stripped/inserted by the
		 * controller.
		 */
		if (nvme_to_user_ptr(io.metadata))
			return -EINVAL;
		meta_len = 0;
		metadata = NULL;
	} else {
		meta_len = (io.nblocks + 1) * ns->head->ms;
		metadata = nvme_to_user_ptr(io.metadata);
	}

	if (ns->head->features & NVME_NS_EXT_LBAS) {
		length += meta_len;
		meta_len = 0;
	} else if (meta_len) {
		if ((io.metadata & 3) || !io.metadata)
			return -EINVAL;
	}

	memset(&c, 0, sizeof(c));
	c.rw.opcode = io.opcode;
	c.rw.flags = io.flags;
	c.rw.nsid = cpu_to_le32(ns->head->ns_id);
	c.rw.slba = cpu_to_le64(io.slba);
	c.rw.length = cpu_to_le16(io.nblocks);
	c.rw.control = cpu_to_le16(io.control);
	c.rw.dsmgmt = cpu_to_le32(io.dsmgmt);
	c.rw.reftag = cpu_to_le32(io.reftag);
	c.rw.apptag = cpu_to_le16(io.apptag);
	c.rw.appmask = cpu_to_le16(io.appmask);

	return nvme_submit_user_cmd(ns->queue, &c, io.addr, length, metadata,
			meta_len, lower_32_bits(io.slba), NULL, 0, 0);
}

static bool nvme_validate_passthru_nsid(struct nvme_ctrl *ctrl,
					struct nvme_ns *ns, __u32 nsid)
{
	if (ns && nsid != ns->head->ns_id) {
		dev_err(ctrl->device,
			"%s: nsid (%u) in cmd does not match nsid (%u)"
			"of namespace\n",
			current->comm, nsid, ns->head->ns_id);
		return false;
	}

	return true;
}

static int nvme_user_cmd(struct nvme_ctrl *ctrl, struct nvme_ns *ns,
		struct nvme_passthru_cmd __user *ucmd, unsigned int flags,
		bool open_for_write)
{
	struct nvme_passthru_cmd cmd;
	struct nvme_command c;
	unsigned timeout = 0;
	u64 result;
	int status;

	if (copy_from_user(&cmd, ucmd, sizeof(cmd)))
		return -EFAULT;
	if (cmd.flags)
		return -EINVAL;
	if (!nvme_validate_passthru_nsid(ctrl, ns, cmd.nsid))
		return -EINVAL;

	memset(&c, 0, sizeof(c));
	c.common.opcode = cmd.opcode;
	c.common.flags = cmd.flags;
	c.common.nsid = cpu_to_le32(cmd.nsid);
	c.common.cdw2[0] = cpu_to_le32(cmd.cdw2);
	c.common.cdw2[1] = cpu_to_le32(cmd.cdw3);
	c.common.cdw10 = cpu_to_le32(cmd.cdw10);
	c.common.cdw11 = cpu_to_le32(cmd.cdw11);
	c.common.cdw12 = cpu_to_le32(cmd.cdw12);
	c.common.cdw13 = cpu_to_le32(cmd.cdw13);
	c.common.cdw14 = cpu_to_le32(cmd.cdw14);
	c.common.cdw15 = cpu_to_le32(cmd.cdw15);

#ifdef HAVE_BDEV_IS_PARTITION
	if (!nvme_cmd_allowed(ns, &c, 0, open_for_write))
#else
	if (!nvme_cmd_allowed(ns, &c, open_for_write))
#endif
		return -EACCES;

	if (cmd.timeout_ms)
		timeout = msecs_to_jiffies(cmd.timeout_ms);

	status = nvme_submit_user_cmd(ns ? ns->queue : ctrl->admin_q, &c,
			cmd.addr, cmd.data_len, nvme_to_user_ptr(cmd.metadata),
			cmd.metadata_len, 0, &result, timeout, 0);

	if (status >= 0) {
		if (put_user(result, &ucmd->result))
			return -EFAULT;
	}

	return status;
}

static int nvme_user_cmd64(struct nvme_ctrl *ctrl, struct nvme_ns *ns,
		struct nvme_passthru_cmd64 __user *ucmd, unsigned int flags,
		bool open_for_write)
{
	struct nvme_passthru_cmd64 cmd;
	struct nvme_command c;
	unsigned timeout = 0;
	int status;

	if (copy_from_user(&cmd, ucmd, sizeof(cmd)))
		return -EFAULT;
	if (cmd.flags)
		return -EINVAL;
	if (!nvme_validate_passthru_nsid(ctrl, ns, cmd.nsid))
		return -EINVAL;

	memset(&c, 0, sizeof(c));
	c.common.opcode = cmd.opcode;
	c.common.flags = cmd.flags;
	c.common.nsid = cpu_to_le32(cmd.nsid);
	c.common.cdw2[0] = cpu_to_le32(cmd.cdw2);
	c.common.cdw2[1] = cpu_to_le32(cmd.cdw3);
	c.common.cdw10 = cpu_to_le32(cmd.cdw10);
	c.common.cdw11 = cpu_to_le32(cmd.cdw11);
	c.common.cdw12 = cpu_to_le32(cmd.cdw12);
	c.common.cdw13 = cpu_to_le32(cmd.cdw13);
	c.common.cdw14 = cpu_to_le32(cmd.cdw14);
	c.common.cdw15 = cpu_to_le32(cmd.cdw15);

#ifdef HAVE_BDEV_IS_PARTITION
	if (!nvme_cmd_allowed(ns, &c, flags, open_for_write))
#else
	if (!nvme_cmd_allowed(ns, &c, open_for_write))
#endif
		return -EACCES;

	if (cmd.timeout_ms)
		timeout = msecs_to_jiffies(cmd.timeout_ms);

	status = nvme_submit_user_cmd(ns ? ns->queue : ctrl->admin_q, &c,
			cmd.addr, cmd.data_len, nvme_to_user_ptr(cmd.metadata),
			cmd.metadata_len, 0, &cmd.result, timeout, flags);

	if (status >= 0) {
		if (put_user(cmd.result, &ucmd->result))
			return -EFAULT;
	}

	return status;
}

struct nvme_uring_data {
	__u64	metadata;
	__u64	addr;
	__u32	data_len;
	__u32	metadata_len;
	__u32	timeout_ms;
};

#if defined(HAVE_IO_URING_CMD_H) && defined(HAVE_BIO_INTEGRITY_MAP_USER)
/*
 * This overlays struct io_uring_cmd pdu.
 * Expect build errors if this grows larger than that.
 */
struct nvme_uring_cmd_pdu {
	struct request *req;
	struct bio *bio;
	u64 result;
	int status;
};

static inline struct nvme_uring_cmd_pdu *nvme_uring_cmd_pdu(
		struct io_uring_cmd *ioucmd)
{
	return (struct nvme_uring_cmd_pdu *)&ioucmd->pdu;
}

static void nvme_uring_task_cb(struct io_uring_cmd *ioucmd,
			       unsigned issue_flags)
{
	struct nvme_uring_cmd_pdu *pdu = nvme_uring_cmd_pdu(ioucmd);

	if (pdu->bio)
		blk_rq_unmap_user(pdu->bio);
	io_uring_cmd_done(ioucmd, pdu->status, pdu->result, issue_flags);
}

static enum rq_end_io_ret nvme_uring_cmd_end_io(struct request *req,
						blk_status_t err)
{
	struct io_uring_cmd *ioucmd = req->end_io_data;
	struct nvme_uring_cmd_pdu *pdu = nvme_uring_cmd_pdu(ioucmd);

	if (nvme_req(req)->flags & NVME_REQ_CANCELLED)
		pdu->status = -EINTR;
	else
		pdu->status = nvme_req(req)->status;
	pdu->result = le64_to_cpu(nvme_req(req)->result.u64);

	/*
	 * For iopoll, complete it directly.
	 * Otherwise, move the completion to task work.
	 */
	if (blk_rq_is_poll(req))
		nvme_uring_task_cb(ioucmd, IO_URING_F_UNLOCKED);
	else
		io_uring_cmd_do_in_task_lazy(ioucmd, nvme_uring_task_cb);

	return RQ_END_IO_FREE;
}

static int nvme_uring_cmd_io(struct nvme_ctrl *ctrl, struct nvme_ns *ns,
		struct io_uring_cmd *ioucmd, unsigned int issue_flags, bool vec)
{
	struct nvme_uring_cmd_pdu *pdu = nvme_uring_cmd_pdu(ioucmd);
	const struct nvme_uring_cmd *cmd = io_uring_sqe_cmd(ioucmd->sqe);
	struct request_queue *q = ns ? ns->queue : ctrl->admin_q;
	struct nvme_uring_data d;
	struct nvme_command c;
	struct request *req;
	blk_opf_t rq_flags = REQ_ALLOC_CACHE;
	blk_mq_req_flags_t blk_flags = 0;
	int ret;

	c.common.opcode = READ_ONCE(cmd->opcode);
	c.common.flags = READ_ONCE(cmd->flags);
	if (c.common.flags)
		return -EINVAL;

	c.common.command_id = 0;
	c.common.nsid = cpu_to_le32(cmd->nsid);
	if (!nvme_validate_passthru_nsid(ctrl, ns, le32_to_cpu(c.common.nsid)))
		return -EINVAL;

	c.common.cdw2[0] = cpu_to_le32(READ_ONCE(cmd->cdw2));
	c.common.cdw2[1] = cpu_to_le32(READ_ONCE(cmd->cdw3));
	c.common.metadata = 0;
	c.common.dptr.prp1 = c.common.dptr.prp2 = 0;
	c.common.cdw10 = cpu_to_le32(READ_ONCE(cmd->cdw10));
	c.common.cdw11 = cpu_to_le32(READ_ONCE(cmd->cdw11));
	c.common.cdw12 = cpu_to_le32(READ_ONCE(cmd->cdw12));
	c.common.cdw13 = cpu_to_le32(READ_ONCE(cmd->cdw13));
	c.common.cdw14 = cpu_to_le32(READ_ONCE(cmd->cdw14));
	c.common.cdw15 = cpu_to_le32(READ_ONCE(cmd->cdw15));

#ifdef HAVE_BDEV_IS_PARTITION
	if (!nvme_cmd_allowed(ns, &c, 0, ioucmd->file->f_mode & FMODE_WRITE))
#else
	if (!nvme_cmd_allowed(ns, &c, ioucmd->file->f_mode & FMODE_WRITE))
#endif
		return -EACCES;

	d.metadata = READ_ONCE(cmd->metadata);
	d.addr = READ_ONCE(cmd->addr);
	d.data_len = READ_ONCE(cmd->data_len);
	d.metadata_len = READ_ONCE(cmd->metadata_len);
	d.timeout_ms = READ_ONCE(cmd->timeout_ms);

	if (issue_flags & IO_URING_F_NONBLOCK) {
		rq_flags |= REQ_NOWAIT;
		blk_flags = BLK_MQ_REQ_NOWAIT;
	}
	if (issue_flags & IO_URING_F_IOPOLL)
		rq_flags |= REQ_POLLED;

	req = nvme_alloc_user_request(q, &c, rq_flags, blk_flags);
	if (IS_ERR(req))
		return PTR_ERR(req);
	req->timeout = d.timeout_ms ? msecs_to_jiffies(d.timeout_ms) : 0;

	if (d.addr && d.data_len) {
		ret = nvme_map_user_request(req, d.addr,
			d.data_len, nvme_to_user_ptr(d.metadata),
			d.metadata_len, 0, ioucmd, vec);
		if (ret)
			return ret;
	}

	/* to free bio on completion, as req->bio will be null at that time */
	pdu->bio = req->bio;
	pdu->req = req;
	req->end_io_data = ioucmd;
	req->end_io = nvme_uring_cmd_end_io;
	blk_execute_rq_nowait(req, false);
	return -EIOCBQUEUED;
}
#endif // HAVE_IO_URING_CMD_H

static bool is_ctrl_ioctl(unsigned int cmd)
{
	if (cmd == NVME_IOCTL_ADMIN_CMD || cmd == NVME_IOCTL_ADMIN64_CMD)
		return true;
#ifdef HAVE_LINUX_SED_OPAL_H
	if (is_sed_ioctl(cmd))
		return true;
#endif
	return false;
}

static int nvme_ctrl_ioctl(struct nvme_ctrl *ctrl, unsigned int cmd,
		void __user *argp, bool open_for_write)
{
	switch (cmd) {
	case NVME_IOCTL_ADMIN_CMD:
		return nvme_user_cmd(ctrl, NULL, argp, 0, open_for_write);
	case NVME_IOCTL_ADMIN64_CMD:
		return nvme_user_cmd64(ctrl, NULL, argp, 0, open_for_write);
	default:
#ifdef HAVE_LINUX_SED_OPAL_H
		return sed_ioctl(ctrl->opal_dev, cmd, argp);
#else
		return 0;
#endif
	}
}

#ifdef COMPAT_FOR_U64_ALIGNMENT
struct nvme_user_io32 {
	__u8	opcode;
	__u8	flags;
	__u16	control;
	__u16	nblocks;
	__u16	rsvd;
	__u64	metadata;
	__u64	addr;
	__u64	slba;
	__u32	dsmgmt;
	__u32	reftag;
	__u16	apptag;
	__u16	appmask;
} __attribute__((__packed__));
#define NVME_IOCTL_SUBMIT_IO32	_IOW('N', 0x42, struct nvme_user_io32)
#endif /* COMPAT_FOR_U64_ALIGNMENT */

static int nvme_ns_ioctl(struct nvme_ns *ns, unsigned int cmd,
		void __user *argp, unsigned int flags, bool open_for_write)
{
	switch (cmd) {
	case NVME_IOCTL_ID:
		force_successful_syscall_return();
		return ns->head->ns_id;
	case NVME_IOCTL_IO_CMD:
		return nvme_user_cmd(ns->ctrl, ns, argp, flags, open_for_write);
	/*
	 * struct nvme_user_io can have different padding on some 32-bit ABIs.
	 * Just accept the compat version as all fields that are used are the
	 * same size and at the same offset.
	 */
#ifdef COMPAT_FOR_U64_ALIGNMENT
	case NVME_IOCTL_SUBMIT_IO32:
#endif
	case NVME_IOCTL_SUBMIT_IO:
		return nvme_submit_io(ns, argp);
#ifdef HAVE_NVME_IOCTL_IO64_CMD_VEC
	case NVME_IOCTL_IO64_CMD_VEC:
		flags |= NVME_IOCTL_VEC;
		fallthrough;
#endif
	case NVME_IOCTL_IO64_CMD:
		return nvme_user_cmd64(ns->ctrl, ns, argp, flags,
				       open_for_write);
	default:
		return -ENOTTY;
	}
}

#ifdef HAVE_GENDISK_OPEN_MODE
int nvme_ioctl(struct block_device *bdev, blk_mode_t mode,
		unsigned int cmd, unsigned long arg)
#else
int nvme_ioctl(struct block_device *bdev, fmode_t mode,
		unsigned int cmd, unsigned long arg)
#endif
{
	struct nvme_ns *ns = bdev->bd_disk->private_data;
#ifdef HAVE_GENDISK_OPEN_MODE
	bool open_for_write = mode & BLK_OPEN_WRITE;
#else
	bool open_for_write = mode & FMODE_WRITE;
#endif
	void __user *argp = (void __user *)arg;
	unsigned int flags = 0;

#ifdef HAVE_BDEV_IS_PARTITION
	if (bdev_is_partition(bdev))
		flags |= NVME_IOCTL_PARTITION;
#endif

	if (is_ctrl_ioctl(cmd))
		return nvme_ctrl_ioctl(ns->ctrl, cmd, argp, open_for_write);
	return nvme_ns_ioctl(ns, cmd, argp, flags, open_for_write);
}

long nvme_ns_chr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct nvme_ns *ns =
		container_of(file_inode(file)->i_cdev, struct nvme_ns, cdev);
	bool open_for_write = file->f_mode & FMODE_WRITE;
	void __user *argp = (void __user *)arg;

	if (is_ctrl_ioctl(cmd))
		return nvme_ctrl_ioctl(ns->ctrl, cmd, argp, open_for_write);
	return nvme_ns_ioctl(ns, cmd, argp, 0, open_for_write);
}

#if defined(HAVE_IO_URING_CMD_H) && defined(HAVE_BIO_INTEGRITY_MAP_USER)
static int nvme_uring_cmd_checks(unsigned int issue_flags)
{

	/* NVMe passthrough requires big SQE/CQE support */
	if ((issue_flags & (IO_URING_F_SQE128|IO_URING_F_CQE32)) !=
	    (IO_URING_F_SQE128|IO_URING_F_CQE32))
		return -EOPNOTSUPP;
	return 0;
}

static int nvme_ns_uring_cmd(struct nvme_ns *ns, struct io_uring_cmd *ioucmd,
			     unsigned int issue_flags)
{
	struct nvme_ctrl *ctrl = ns->ctrl;
	int ret;

	BUILD_BUG_ON(sizeof(struct nvme_uring_cmd_pdu) > sizeof(ioucmd->pdu));

	ret = nvme_uring_cmd_checks(issue_flags);
	if (ret)
		return ret;

	switch (ioucmd->cmd_op) {
	case NVME_URING_CMD_IO:
		ret = nvme_uring_cmd_io(ctrl, ns, ioucmd, issue_flags, false);
		break;
	case NVME_URING_CMD_IO_VEC:
		ret = nvme_uring_cmd_io(ctrl, ns, ioucmd, issue_flags, true);
		break;
	default:
		ret = -ENOTTY;
	}

	return ret;
}

int nvme_ns_chr_uring_cmd(struct io_uring_cmd *ioucmd, unsigned int issue_flags)
{
	struct nvme_ns *ns = container_of(file_inode(ioucmd->file)->i_cdev,
			struct nvme_ns, cdev);

	return nvme_ns_uring_cmd(ns, ioucmd, issue_flags);
}

int nvme_ns_chr_uring_cmd_iopoll(struct io_uring_cmd *ioucmd,
				 struct io_comp_batch *iob,
				 unsigned int poll_flags)
{
	struct nvme_uring_cmd_pdu *pdu = nvme_uring_cmd_pdu(ioucmd);
	struct request *req = pdu->req;

	if (req && blk_rq_is_poll(req))
		return blk_rq_poll(req, iob, poll_flags);
	return 0;
}
#endif
#ifdef CONFIG_NVME_MULTIPATH
static int nvme_ns_head_ctrl_ioctl(struct nvme_ns *ns, unsigned int cmd,
		void __user *argp, struct nvme_ns_head *head, int srcu_idx,
		bool open_for_write)
	__releases(&head->srcu)
{
	struct nvme_ctrl *ctrl = ns->ctrl;
	int ret;

	nvme_get_ctrl(ns->ctrl);
	srcu_read_unlock(&head->srcu, srcu_idx);
	ret = nvme_ctrl_ioctl(ns->ctrl, cmd, argp, open_for_write);

	nvme_put_ctrl(ctrl);
	return ret;
}

#ifdef HAVE_GENDISK_OPEN_MODE
int nvme_ns_head_ioctl(struct block_device *bdev, blk_mode_t mode,
		unsigned int cmd, unsigned long arg)
#else
int nvme_ns_head_ioctl(struct block_device *bdev, fmode_t mode,
		unsigned int cmd, unsigned long arg)
#endif
{
	struct nvme_ns_head *head = bdev->bd_disk->private_data;
#ifdef HAVE_GENDISK_OPEN_MODE
	bool open_for_write = mode & BLK_OPEN_WRITE;
#else
	bool open_for_write = mode & FMODE_WRITE;
#endif
	void __user *argp = (void __user *)arg;
	struct nvme_ns *ns;
	int srcu_idx, ret = -EWOULDBLOCK;
	unsigned int flags = 0;

#ifdef HAVE_BDEV_IS_PARTITION
	if (bdev_is_partition(bdev))
		flags |= NVME_IOCTL_PARTITION;
#endif

	srcu_idx = srcu_read_lock(&head->srcu);
	ns = nvme_find_path(head);
	if (!ns)
		goto out_unlock;

	/*
	 * Handle ioctls that apply to the controller instead of the namespace
	 * seperately and drop the ns SRCU reference early.  This avoids a
	 * deadlock when deleting namespaces using the passthrough interface.
	 */
	if (is_ctrl_ioctl(cmd))
		return nvme_ns_head_ctrl_ioctl(ns, cmd, argp, head, srcu_idx,
					       open_for_write);

	ret = nvme_ns_ioctl(ns, cmd, argp, flags, open_for_write);
out_unlock:
	srcu_read_unlock(&head->srcu, srcu_idx);
	return ret;
}

long nvme_ns_head_chr_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	bool open_for_write = file->f_mode & FMODE_WRITE;
	struct cdev *cdev = file_inode(file)->i_cdev;
	struct nvme_ns_head *head =
		container_of(cdev, struct nvme_ns_head, cdev);
	void __user *argp = (void __user *)arg;
	struct nvme_ns *ns;
	int srcu_idx, ret = -EWOULDBLOCK;

	srcu_idx = srcu_read_lock(&head->srcu);
	ns = nvme_find_path(head);
	if (!ns)
		goto out_unlock;

	if (is_ctrl_ioctl(cmd))
		return nvme_ns_head_ctrl_ioctl(ns, cmd, argp, head, srcu_idx,
				open_for_write);

	ret = nvme_ns_ioctl(ns, cmd, argp, 0, open_for_write);
out_unlock:
	srcu_read_unlock(&head->srcu, srcu_idx);
	return ret;
}

#if defined(HAVE_IO_URING_CMD_H) && defined(HAVE_BIO_INTEGRITY_MAP_USER)
int nvme_ns_head_chr_uring_cmd(struct io_uring_cmd *ioucmd,
		unsigned int issue_flags)
{
	struct cdev *cdev = file_inode(ioucmd->file)->i_cdev;
	struct nvme_ns_head *head = container_of(cdev, struct nvme_ns_head, cdev);
	int srcu_idx = srcu_read_lock(&head->srcu);
	struct nvme_ns *ns = nvme_find_path(head);
	int ret = -EINVAL;

	if (ns)
		ret = nvme_ns_uring_cmd(ns, ioucmd, issue_flags);
	srcu_read_unlock(&head->srcu, srcu_idx);
	return ret;
}
#endif /* HAVE_IO_URING_CMD_H */
#endif /* CONFIG_NVME_MULTIPATH */

#if defined(HAVE_IO_URING_CMD_H) && defined(HAVE_BIO_INTEGRITY_MAP_USER)
int nvme_dev_uring_cmd(struct io_uring_cmd *ioucmd, unsigned int issue_flags)
{
	struct nvme_ctrl *ctrl = ioucmd->file->private_data;
	int ret;

	/* IOPOLL not supported yet */
	if (issue_flags & IO_URING_F_IOPOLL)
		return -EOPNOTSUPP;

	ret = nvme_uring_cmd_checks(issue_flags);
	if (ret)
		return ret;

	switch (ioucmd->cmd_op) {
	case NVME_URING_CMD_ADMIN:
		ret = nvme_uring_cmd_io(ctrl, NULL, ioucmd, issue_flags, false);
		break;
	case NVME_URING_CMD_ADMIN_VEC:
		ret = nvme_uring_cmd_io(ctrl, NULL, ioucmd, issue_flags, true);
		break;
	default:
		ret = -ENOTTY;
	}

	return ret;
}
#endif /* HAVE_IO_URING_CMD_H */

static int nvme_dev_user_cmd(struct nvme_ctrl *ctrl, void __user *argp,
		bool open_for_write)
{
	struct nvme_ns *ns;
	int ret, srcu_idx;

	srcu_idx = srcu_read_lock(&ctrl->srcu);
	if (list_empty(&ctrl->namespaces)) {
		ret = -ENOTTY;
		goto out_unlock;
	}

	ns = list_first_or_null_rcu(&ctrl->namespaces, struct nvme_ns, list);
	if (ns != list_last_entry(&ctrl->namespaces, struct nvme_ns, list)) {
		dev_warn(ctrl->device,
			"NVME_IOCTL_IO_CMD not supported when multiple namespaces present!\n");
		ret = -EINVAL;
		goto out_unlock;
	}

	dev_warn(ctrl->device,
		"using deprecated NVME_IOCTL_IO_CMD ioctl on the char device!\n");
	if (!nvme_get_ns(ns)) {
		ret = -ENXIO;
		goto out_unlock;
	}
	srcu_read_unlock(&ctrl->srcu, srcu_idx);

	ret = nvme_user_cmd(ctrl, ns, argp, 0, open_for_write);
	nvme_put_ns(ns);
	return ret;

out_unlock:
	srcu_read_unlock(&ctrl->srcu, srcu_idx);
	return ret;
}

long nvme_dev_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	bool open_for_write = file->f_mode & FMODE_WRITE;
	struct nvme_ctrl *ctrl = file->private_data;
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case NVME_IOCTL_ADMIN_CMD:
		return nvme_user_cmd(ctrl, NULL, argp, 0, open_for_write);
	case NVME_IOCTL_ADMIN64_CMD:
		return nvme_user_cmd64(ctrl, NULL, argp, 0, open_for_write);
	case NVME_IOCTL_IO_CMD:
		return nvme_dev_user_cmd(ctrl, argp, open_for_write);
	case NVME_IOCTL_RESET:
		if (!capable(CAP_SYS_ADMIN))
			return -EACCES;
		dev_warn(ctrl->device, "resetting controller\n");
		return nvme_reset_ctrl_sync(ctrl);
	case NVME_IOCTL_SUBSYS_RESET:
		if (!capable(CAP_SYS_ADMIN))
			return -EACCES;
		return nvme_reset_subsystem(ctrl);
	case NVME_IOCTL_RESCAN:
		if (!capable(CAP_SYS_ADMIN))
			return -EACCES;
		nvme_queue_scan(ctrl);
		return 0;
	default:
		return -ENOTTY;
	}
}
