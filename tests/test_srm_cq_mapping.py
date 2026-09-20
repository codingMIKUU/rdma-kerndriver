#!/usr/bin/env python3
"""Fault-inject the production CQ mapper offline; no device/module is touched.

Compile extracted production functions against mock umem/SG/page operations,
using ASan/UBSan and real pthread mutexes. Also check ordinary RC stays outside
the extra mapping path. This does not replace hardware create/destroy testing.
"""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MLX5 = ROOT / "drivers/infiniband/hw/mlx5"


def function(source, name):
    pos = re.search(r"\b" + re.escape(name) + r"\(", source).start()
    start = source.rfind("\n", 0, pos) + 1
    # The lookup function's pointer return type occupies the preceding line.
    if name == "mlx5_ib_find_cqb_by_cqn_locked":
        start = source.rfind("\n", 0, start - 1) + 1
    body = source.index("{", pos)
    depth = 1
    end = body + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


HARNESS = r'''
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
typedef uint32_t u32;
#define PAGE_SIZE ((size_t)4096)
#define DIV_ROUND_UP(n,d) (((n)+(d)-1)/(d))
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define check_add_overflow(a,b,out) __builtin_add_overflow(a,b,out)
#define IS_ERR_OR_NULL(p) (!(p) || (uintptr_t)(p) >= (uintptr_t)-4095)
#define READ_ONCE(x) (x)
#define GFP_KERNEL 0
#define VM_MAP 0
#define PAGE_KERNEL 0
#define pr_info(...) ((void)0)
#define pr_err(...) ((void)0)
#define pr_warn_ratelimited(...) ((void)0)
#define mutex_lock(p) assert(!pthread_mutex_lock(p))
#define mutex_unlock(p) assert(!pthread_mutex_unlock(p))
static int srm_numa_node;
struct page { int refs; };
struct scatterlist { struct page **pages; unsigned int count; };
struct sg_page_iter { struct scatterlist *sg; unsigned int index; };
#define for_each_sg_page(sgl,it,nents,start) \
    for ((it)->sg=(sgl), (it)->index=(start); \
         (nents) && (it)->index<(it)->sg->count; ++(it)->index)
static struct page *sg_page_iter_page(struct sg_page_iter *it)
{ return it->sg->pages[it->index]; }
struct ib_umem {
    size_t length, address;
    bool is_odp, is_dmabuf, is_peer;
    struct { struct scatterlist *sgl; } sg_head;
    unsigned int sg_nents;
    struct { struct { struct scatterlist *sgl; unsigned int orig_nents; } sgt; } sgt_append;
};
static size_t ib_umem_offset(struct ib_umem *u) { return u->address % PAGE_SIZE; }
struct mlx5_ib_cq {
    pthread_mutex_t resize_mutex;
    bool srm_cq_mapped;
    struct { struct ib_umem *umem; } buf;
    struct { int cqn; } mcq;
    int cqe_size;
};
struct mlx5_ib_cqbuf {
    void *buf, *vmap_base;
    struct page **pages;
    size_t npages, cq_size;
    int cqn, cqe_sz;
    unsigned int owner_worker;
};
struct mlx5_ib_sched { unsigned int worker_count; };
struct mlx5_ib_sched_group {
    pthread_mutex_t cq_lock;
    bool owner_stopping;
    int cqb_cnt, num_sched;
    struct mlx5_ib_sched *scheds;
    struct mlx5_ib_cqbuf *cqb_arr[4];
};
static int fail_alloc, allocations, live_allocs, maps, fail_map;
static void *mapping_base;
static void *kzalloc_node(size_t bytes, int flags, int node)
{
    (void)flags; (void)node;
    if (++allocations == fail_alloc) return NULL;
    void *p = calloc(1, bytes);
    assert(p); ++live_allocs; return p;
}
static void kfree(void *p) { if (p) { assert(live_allocs > 0); --live_allocs; free(p); } }
static void get_page(struct page *p) { assert(p && p->refs > 0); ++p->refs; }
static void put_page(struct page *p) { assert(p && p->refs > 0); --p->refs; }
static void *vmap(struct page **pages, size_t n, int flags, int prot)
{
    (void)flags; (void)prot;
    for (size_t i=0; i<n; ++i) assert(pages[i] && pages[i]->refs == 2);
    if (fail_map) return NULL;
    assert(!mapping_base); mapping_base=malloc(n*PAGE_SIZE);
    assert(mapping_base); ++maps; return mapping_base;
}
static void vunmap(void *p)
{ assert(p && p == mapping_base); free(p); mapping_base=NULL; --maps; }
'''

TESTS = r'''
static struct page pg[3];
static struct page *pages[3];
static struct scatterlist sg;
static struct ib_umem umem;
static struct mlx5_ib_cq cq;
static struct mlx5_ib_sched sched;
static struct mlx5_ib_sched_group group;
static void init(void)
{
    assert(!live_allocs && !maps && !mapping_base);
    fail_alloc=allocations=fail_map=0;
    for (int i=0; i<3; ++i) { pg[i].refs=1; pages[i]=&pg[i]; }
    sg=(struct scatterlist){pages,3};
    umem=(struct ib_umem){ .length=4096, .address=0x100040,
        .sg_head={&sg}, .sg_nents=1, .sgt_append={{&sg,1}} };
    cq=(struct mlx5_ib_cq){ .buf={&umem}, .mcq={17}, .cqe_size=64 };
    sched=(struct mlx5_ib_sched){ .worker_count=2 };
    group=(struct mlx5_ib_sched_group){ .num_sched=1, .scheds=&sched };
    assert(!pthread_mutex_init(&cq.resize_mutex,NULL));
    assert(!pthread_mutex_init(&group.cq_lock,NULL));
}
static void end(void)
{
    assert(!live_allocs && !maps && !mapping_base);
    for (int i=0; i<3; ++i) assert(pg[i].refs==1);
    assert(!pthread_mutex_destroy(&cq.resize_mutex));
    assert(!pthread_mutex_destroy(&group.cq_lock));
}
static void expect_error(int err)
{
    assert(mlx5_ib_map_cq_ubuf(&group,&cq)==err);
    assert(!cq.srm_cq_mapped && !group.cqb_cnt && !group.cqb_arr[0]);
    end();
}
static void *attach(void *unused)
{ (void)unused; assert(!mlx5_ib_map_cq_ubuf(&group,&cq)); return NULL; }
int main(void)
{
    /* Offset spans two physical pages. Concurrent QPs must map it only once. */
    init();
    pthread_t threads[16];
    for (int i=0; i<16; ++i) assert(!pthread_create(&threads[i],NULL,attach,NULL));
    for (int i=0; i<16; ++i) assert(!pthread_join(threads[i],NULL));
    assert(group.cqb_cnt==1 && allocations==2 && maps==1);
    struct mlx5_ib_cqbuf *cqb=group.cqb_arr[0];
    assert(cqb->npages==2 && cqb->buf==(char*)cqb->vmap_base+64);
    assert(cqb->owner_worker==1 && pg[0].refs==2 && pg[1].refs==2 && pg[2].refs==1);
    /* Original umem may be released before deferred CQ-map reclamation. */
    put_page(&pg[0]); put_page(&pg[1]);
    mlx5_ib_cqb_release(cqb);
    assert(pg[0].refs==0 && pg[1].refs==0);
    pg[0].refs=pg[1].refs=1; end();
    for (int n=1; n<=2; ++n) { init(); fail_alloc=n; expect_error(-ENOMEM); }
    init(); fail_map=1; expect_error(-ENOMEM);
    init(); sg.count=1; expect_error(-EFAULT);
    init(); pages[1]=NULL; expect_error(-EFAULT);
    init(); cq.buf.umem=NULL; expect_error(-EINVAL);
    init(); cq.buf.umem=(void*)(intptr_t)-ENOMEM; expect_error(-EINVAL);
    init(); umem.length=0; expect_error(-EINVAL);
    init(); umem.length=SIZE_MAX; expect_error(-EOVERFLOW);
    init(); umem.length=SIZE_MAX-64; expect_error(-EOVERFLOW);
    init(); umem.length=((size_t)UINT_MAX+1)*PAGE_SIZE; expect_error(-EOVERFLOW);
    init(); cq.cqe_size=128; expect_error(-EOPNOTSUPP);
    init(); umem.is_odp=true; expect_error(-EOPNOTSUPP);
    init(); umem.is_dmabuf=true; expect_error(-EOPNOTSUPP);
    init(); umem.is_peer=true; expect_error(-EOPNOTSUPP);
    init(); umem.sg_nents=umem.sgt_append.sgt.orig_nents=0; expect_error(-EINVAL);
    init(); umem.sg_head.sgl=umem.sgt_append.sgt.sgl=NULL; expect_error(-EINVAL);
    init(); group.owner_stopping=true; expect_error(-ESHUTDOWN);
    init(); assert(mlx5_ib_map_cq_ubuf(NULL,&cq)==-EINVAL);
    assert(mlx5_ib_map_cq_ubuf(&group,NULL)==-EINVAL); end();
    /* Full and duplicate registries do not consume a slot or acquire refs. */
    init(); struct mlx5_ib_cqbuf dummy[4]={0};
    for (int i=0; i<4; ++i) { dummy[i].cqn=i; group.cqb_arr[i]=&dummy[i]; }
    group.cqb_cnt=4;
    assert(mlx5_ib_map_cq_ubuf(&group,&cq)==-ENOSPC && group.cqb_cnt==4);
    dummy[0].cqn=17;
    assert(mlx5_ib_map_cq_ubuf(&group,&cq)==-EEXIST && group.cqb_cnt==4);
    /* Failure while filling a hole leaves it reusable. */
    group.cqb_arr[0]=NULL; fail_map=1;
    assert(mlx5_ib_map_cq_ubuf(&group,&cq)==-ENOMEM && !group.cqb_arr[0]);
    fail_map=0; umem.address=0x100000;
    assert(!mlx5_ib_map_cq_ubuf(&group,&cq) && group.cqb_cnt==4);
    assert(group.cqb_arr[0]->npages==1);
    mlx5_ib_cqb_release(group.cqb_arr[0]); end();
    puts("PASS CQ mapping: failure cleanup, page lifetime, shared-CQ race, offsets, registry");
    return 0;
}
'''


def main():
    scheduler = (MLX5 / "scheduler.c").read_text()
    cq = (MLX5 / "cq.c").read_text()
    qp = (MLX5 / "qp.c").read_text()
    mapper = function(scheduler, "mlx5_ib_map_cq_ubuf")
    assert "get_user_pages(" not in mapper
    assert "mlx5_ib_map_cq_ubuf(" not in function(cq, "mlx5_ib_create_cq")
    destroy = function(cq, "mlx5_ib_destroy_cq")
    assert destroy.index("if (mcq->srm_cq_mapped)") < destroy.index("mlx5_ib_unmap_cq_ubuf(")
    resize = function(cq, "mlx5_ib_resize_cq")
    assert resize.index("if (cq->srm_cq_mapped)") < resize.index("resize_user(")
    create = function(qp, "create_user_qp")
    hollow = create[create.index("if (mlx5_ib_is_hollow_rc_qp(qp))"):
                    create.index("if (mlx5_ib_is_skip_kern_qp(qp))")]
    assert hollow.index("mlx5_ib_map_cq_ubuf(") < hollow.index("mlx5_ib_bind_usr_rc_cq(")
    source = HARNESS + function(scheduler, "mlx5_ib_cqb_release")
    source += function(scheduler, "mlx5_ib_find_cqb_by_cqn_locked") + mapper + TESTS
    with tempfile.TemporaryDirectory(prefix="srm-cq-map-test-") as tmp:
        for append in (False, True):
            exe = str(Path(tmp) / "test")
            command = shlex.split(os.environ.get("CC", "cc")) + [
                "-x", "c", "-std=gnu11", "-O1", "-g", "-Wall", "-Wextra",
                "-Werror", "-Wno-sign-compare", "-pthread", "-fno-pie", "-no-pie",
                "-fsanitize=address,undefined", "-o", exe, "-"]
            if append:
                command.append("-DHAVE_SG_APPEND_TABLE=1")
            subprocess.run(command, input=source, text=True, check=True)
            subprocess.run([exe], check=True)
    print("PASS: both umem SG layouts; ordinary RC bypasses scheduler CQ mapping")


if __name__ == "__main__":
    main()
