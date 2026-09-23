#!/usr/bin/env python3
"""Test the production non-simplified private-CQ poller without RDMA devices."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MLX5 = ROOT / "drivers/infiniband/hw/mlx5"


def region(source, name):
    start = source.index("/* " + name + "_BEGIN")
    end = source.index("/* " + name + "_END", start)
    return source[start:end]


def main():
    cq = (MLX5 / "cq.c").read_text()
    scheduler = (MLX5 / "scheduler.c").read_text()
    header = (MLX5 / "scheduler.h").read_text()
    mock = (ROOT / "tests/private_cq_test.c").read_text().split("static void finish(")[0]
    mock = mock.replace("struct ib_wc { u32 status, vendor_err; };",
                        "struct ib_wc { void *qp; u64 wr_id; u32 status, vendor_err; };")
    mock = mock.replace("static u32 head, ci_stores", "static int smash_on_release;\nstatic u32 head, ci_stores")
    mock = mock.replace("    last_ci = c->cons_index;", "    last_ci = c->cons_index;\n"
                        "    if (smash_on_release) memset(entries, 0xcc, sizeof(entries));")
    mock += "\n#define mb() ((void)0)\n#define SQ_DEPTH 35000\n"
    mock += "#define MLX5_SRM_WRID_KQP_SHIFT 48\n#define MLX5_SRM_WRID_POST_MASK ((1ULL<<48)-1)\n"
    mock += re.search(r"static inline u64 mlx5_srm_make_wrid\(.*?^}", header, re.M | re.S)[0]
    mock += region(scheduler, "BYTE_WINDOW_TEST")
    mock += region(cq, "PRIVATE_CQ_COMPLETE_TEST")
    mock += region(cq, "PRIVATE_CQ_NATIVE_TEST")
    mock += region(scheduler, "PRIVATE_CQ_BUDGET_TEST")
    body = r'''
static struct ib_wc wcs[1024];
static void *raw[1024];
static struct mlx5_cqe64 user_cq[1024];
static u64 next(u64 n) {
#if MLX5_SRM_ENABLE_REROUTE
    return mlx5_srm_rr_seq(n);
#else
    return n;
#endif
}
static void deliver(int n) {
    int i;
    /* Model the existing distributor: finish reading every raw entry first. */
    for (i=0; i<n; ++i) memcpy(&user_cq[i], raw[i], sizeof(user_cq[i]));
    smash_on_release=1;
    mlx5_ib_release_srm_private_cq(&s);
    smash_on_release=0;
    for (i=0; i<n; ++i) assert(ntohl(user_cq[i].sop_drop_qpn)==77);
}
int main(void) {
    struct mlx5_ib_sched_worker worker;
    struct mlx5_ib_sched sched={&worker};
    u32 done, size;
    int n, i, left;
    for (size=64; size<=128; size*=2) {
        const u64 starts[]={0,65534,(1ULL<<32)-2,(1ULL<<48)-2,
#if MLX5_SRM_ENABLE_REROUTE
                            (1ULL<<63)-2};
#else
                            UINT64_MAX-1};
#endif
        reset(0,size);
        assert(mlx5_ib_poll_srm_private_cq(&s,10,wcs,raw,&done)==0 && done==0);
        assert(!ci_stores && !publishes);
        reset(10,size); add(10,0,77);
        assert(mlx5_ib_poll_srm_private_cq(&s,0,wcs,raw,&done)==0);
        assert(!ci_stores && !cq.mcq.cons_index);
        for (unsigned k=0; k<sizeof(starts)/sizeof(starts[0]); ++k) {
            u64 st=starts[k];
            reset(st,size); s.srmc_idx=7;
            for (i=0; i<10; ++i) add(st+i,0,77);
            ctrl.db_tail=next(st+10); left=10;
            assert(mlx5_srm_refresh_poll_budget(&sched,&s)==left); /* user DB */
            while (left) {
                u32 ci_before=ci_stores;
                n=mlx5_ib_poll_srm_private_cq(&s,left,wcs,raw,&done);
                assert(n==min_t(int,left,MLX5_SRM_PRIVATE_CQ_POLL_BUDGET));
                assert(done==(u32)n && ci_stores==ci_before && !publishes);
                for (i=0; i<n; ++i) {
                    assert(wcs[i].wr_id==mlx5_srm_make_wrid(7,next(st+i)));
                    assert(!wcs[i].status && !wcs[i].vendor_err && wcs[i].qp==&qp.ibqp);
                    assert(((struct mlx5_cqe64 *)raw[i])->wqe_counter==htons((u16)(st+i)));
                }
                assert(ctrl.cons_idx==starts[k]); /* poll must NOT complete logical users */
                mlx5_ib_release_srm_private_cq(&s);
                assert(ci_stores==ci_before+1 && last_ci==cq.mcq.cons_index);
                left-=n; st=next(st+n);
                assert(s.cq_complete_idx==st && qp.sq.tail==(u32)st);
                assert(mlx5_srm_refresh_poll_budget(&sched,&s)==left);
            }
        }
        reset(0,size); add(0,0,77); add(1,0,77);
        n=mlx5_ib_poll_srm_private_cq(&s,1,wcs,raw,&done);
        assert(n==1 && done==1 && cq.mcq.cons_index==1); /* DB snapshot bounds */
        deliver(n); assert(!user_cq[0].opcode && user_cq[0].wqe_counter==0);
        reset(0,size); add(0,MLX5_CQE_REQ_ERR,77); add(1,MLX5_CQE_REQ_ERR,77);
        n=mlx5_ib_poll_srm_private_cq(&s,2,wcs,raw,&done);
        assert(n==1 && done==1 && wcs[0].status==13 && wcs[0].vendor_err==0x87);
        assert(cq.mcq.cons_index==1 && !ci_stores);
        deliver(n); assert(user_cq[0].opcode==MLX5_CQE_REQ_ERR);
        reset(8,size); add(8,0,88);
        assert(mlx5_ib_poll_srm_private_cq(&s,1,wcs,raw,&done)==-EINVAL && !done);
        assert(s.cq_complete_idx==8 && !cq.mcq.cons_index);
        reset(8,size); add(8,11,77);
        assert(mlx5_ib_poll_srm_private_cq(&s,1,wcs,raw,&done)==-EINVAL && !done);
        reset(8,size); add(7,0,77); /* stale CQE must not create a credit */
        assert(mlx5_ib_poll_srm_private_cq(&s,1,wcs,raw,&done)<0 && !done);
        assert(s.cq_complete_idx==8);
        reset(65534,size); add(65536,0,77);
#if MLX5_SRM_ENABLE_REROUTE
        assert(mlx5_ib_poll_srm_private_cq(&s,3,wcs,raw,&done)<0 && !done);
        reset(20,size); rr_maintenance=1; rr_maintenance_end=25;
        add(24,0,77);
        assert(mlx5_ib_poll_srm_private_cq(&s,5,wcs,raw,&done)==1 && done==1);
        assert(s.cq_complete_idx==25 && ctrl.cons_idx==20 && !ci_stores);
        /* The existing dispatcher resolves/skips the NOP, not this poller. */
        struct mlx5_ib_srmc *origin=&s;
        u64 post=wcs[0].wr_id & MLX5_SRM_WRID_POST_MASK;
        assert(mlx5_srm_rr_complete(&origin,&post,0,0,0)==2);
        mlx5_ib_release_srm_private_cq(&s);
#else
        assert(mlx5_ib_poll_srm_private_cq(&s,3,wcs,raw,&done)==1 && done==3);
        assert(s.cq_complete_idx==65537 && ctrl.cons_idx==65534 && !ci_stores);
#endif
        /* Administrative-only poll; no WC and no fabricated WQE credits. */
        reset(0,size); cq.resize_buf=calloc(1,sizeof(*cq.resize_buf));
        add(0,MLX5_CQE_RESIZE_CQ,77);
        assert(mlx5_ib_poll_srm_private_cq(&s,1,wcs,raw,&done)==0 && !done);
        assert(ci_stores==1 && !cq.resize_buf && !s.cq_complete_idx);
        /* Resize cannot free CQ storage still borrowed by earlier CQEs. */
        reset(0,size); cq.resize_buf=calloc(1,sizeof(*cq.resize_buf));
        add(0,0,77); add(0,MLX5_CQE_RESIZE_CQ,77); add(1,0,77);
        assert(mlx5_ib_poll_srm_private_cq(&s,2,wcs,raw,&done)==1 && done==1);
        assert(cq.resize_buf && !ci_stores);
        mlx5_ib_release_srm_private_cq(&s);
        assert(mlx5_ib_poll_srm_private_cq(&s,1,wcs,raw,&done)==1 && done==1);
        assert(!cq.resize_buf && ci_stores==1);
        mlx5_ib_release_srm_private_cq(&s);
        reset(0,size); core.state=MLX5_DEVICE_STATE_INTERNAL_ERROR;
        assert(mlx5_ib_poll_srm_private_cq(&s,1,wcs,raw,&done)==-EIO && !done);
    }
    printf("PASS native private CQ: reroute=%d bytes=%u budget=%d\n",
           MLX5_SRM_ENABLE_REROUTE,MLX5_SRM_MAX_INFLIGHT_BYTES,MLX5_SRM_PRIVATE_CQ_POLL_BUDGET);
}
'''
    # Scheduler must release borrowed CQEs only AFTER completing routing/copy.
    native = scheduler.split("#else /* Historical CQE copy/routing")[1].split(
        "#endif /* MLX5_SRM_ENABLE_CQE_SIMPLIFY */")[0]
    assert native.index("mlx5_srm_ctrl_complete_flush(") < native.index("mlx5_ib_release_srm_private_cq(")
    assert native.index("mlx5_ib_release_srm_private_cq(") < native.index("atomic64_add(")
    with tempfile.TemporaryDirectory(prefix="srm-native-private-cq-") as temp:
        binary = str(Path(temp) / "poll")
        for reroute in (0, 1):
            for byte_limit in (0, 32768):
                for budget in (1, 3, 256):
                    subprocess.run(["cc", "-std=gnu11", "-O1", "-g",
                                    "-Werror=implicit-function-declaration",
                                    "-fsanitize=address,undefined", "-fno-pie", "-no-pie",
                                    "-I" + str(ROOT / "include/uapi"),
                                    "-DMLX5_SRM_ENABLE_PRIVATE_CQ=1",
                                    "-DMLX5_SRM_ENABLE_CQE_SIMPLIFY=0",
                                    "-DMLX5_SRM_ENABLE_WQE_TIMING=0",
                                    f"-DMLX5_SRM_ENABLE_REROUTE={reroute}",
                                    f"-DMLX5_SRM_MAX_INFLIGHT_BYTES={byte_limit}",
                                    f"-DMLX5_SRM_PRIVATE_CQ_POLL_BUDGET={budget}",
                                    "-x", "c", "-", "-o", binary],
                                   input=mock + body, text=True, check=True)
                    subprocess.run([binary], check=True)


if __name__ == "__main__":
    main()
