/* Hardware-free mocks around the unmodified production setup functions. */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint32_t u32;
typedef uint32_t __u32;
typedef uint64_t __u64;
#define PAGE_SIZE 4096
#define GFP_KERNEL 0
#define __iomem
#define MLX5_UARS_IN_PAGE 1
#define MLX5_IB_MMAP_OFFSET_START 1
#define MLX5_IB_MMAP_OFFSET_END 255
#define MLX5_IB_MMAP_TYPE_QP_FARM_UAR 10
#define MLX5_IB_MMAP_TYPE_QP_FARM_DB 11
#define MLX5_SRM_DIRECT_DB_MAX_BATCH 32
#define MLX5_CAP_GEN(dev, field) ((dev)->uar_4k)
#define offsetofend(type, field) (offsetof(type, field) + sizeof(((type *)0)->field))
#define min_t(type, a, b) ((type)(a) < (type)(b) ? (type)(a) : (type)(b))

struct rdma_user_mmap_entry { uint64_t offset; };
struct mlx5_user_mmap_entry {
    struct rdma_user_mmap_entry rdma_entry;
    int mmap_flag;
    uint64_t address;
};
struct mlx5_qp_farm_db_mmap_entry {
    struct mlx5_user_mmap_entry mentry;
    void *cpu_addr;
    uint64_t dma_addr;
};
struct mlx5_ib_ucontext { int ibucontext; };
struct mock_mdev { uint64_t bar_addr; bool uar_4k; };
struct mlx5_ib_dev { struct mock_mdev *mdev; };
struct mock_uar { unsigned int index; void *map; };
struct mock_bfreg { struct mock_uar *up; void *map; };
struct mlx5_ib_srmc;
struct mlx5_ib_qp {
    struct { struct mock_bfreg *bfreg; u32 buf_size; } bf;
    struct { unsigned int index; void *db; uint64_t dma; } db;
    struct mlx5_ib_srmc *srmc_owner, *large_srmc_owner;
    struct mlx5_user_mmap_entry *farm_uar_mmap_entry, *large_farm_uar_mmap_entry;
    struct mlx5_qp_farm_db_mmap_entry *farm_db_mmap_entry, *large_farm_db_mmap_entry;
};
struct mlx5_ib_srmc {
    struct { struct mlx5_ib_qp *qp; } ini_cb;
    unsigned int owner_worker;
};
struct mlx5_ib_sched_worker { void *credit_ctrl; u32 kqp_begin; };
struct mock_sched { struct mlx5_ib_sched_worker *workers; unsigned int worker_count; };
static struct { struct mock_sched *scheds; unsigned int num_sched; } sched_group;
static unsigned int inserts, allocs, fail_insert;

static void *kzalloc(size_t bytes, int flags)
{
    (void)flags;
    allocs++;
    return calloc(1, bytes);
}
static void kfree(void *p) { free(p); }
static size_t cache_line_size(void) { return 64; }
static int rdma_user_mmap_entry_insert_range(int *ctx,
    struct rdma_user_mmap_entry *entry, size_t len,
    unsigned long start, unsigned long end)
{
    (void)ctx; (void)start; (void)end;
    assert(len == PAGE_SIZE);
    inserts++;
    if (inserts == fail_insert)
        return -ENOMEM;
    entry->offset = (uint64_t)inserts * PAGE_SIZE;
    return 0;
}
static uint64_t mlx5_qp_entry_to_mmap_offset(struct mlx5_user_mmap_entry *entry)
{
    return entry->rdma_entry.offset;
}

#include "actual_setup.h"

static void cleanup(struct mlx5_ib_qp *qp)
{
    free(qp->farm_uar_mmap_entry);
    free(qp->farm_db_mmap_entry);
    free(qp->large_farm_uar_mmap_entry);
    free(qp->large_farm_db_mmap_entry);
    qp->farm_uar_mmap_entry = qp->large_farm_uar_mmap_entry = NULL;
    qp->farm_db_mmap_entry = qp->large_farm_db_mmap_entry = NULL;
}

int main(void)
{
    const size_t old_size = offsetof(struct mlx5_ib_modify_qp_resp,
                                     large_farm_uar_mmap_offset);
    const size_t new_size = sizeof(struct mlx5_ib_modify_qp_resp);
    const size_t lengths[] = {0, old_size, new_size - 1, new_size, new_size + 64};
    char small_page[PAGE_SIZE], large_page[PAGE_SIZE];
    char small_uar_page[PAGE_SIZE], large_uar_page[PAGE_SIZE];
    struct mock_mdev mdev = {.bar_addr = 0x100000, .uar_4k = true};
    struct mlx5_ib_dev dev = {&mdev};
    struct mlx5_ib_ucontext context = {0};
    struct mock_uar up[2] = {{3, small_uar_page}, {7, large_uar_page}};
    struct mock_bfreg bf[2] = {{&up[0], small_uar_page + 128},
                              {&up[1], large_uar_page + 256}};
    struct mlx5_ib_qp small = {.bf = {&bf[0], 256},
        .db = {2, small_page + 128, 0x200080}};
    struct mlx5_ib_qp large = {.bf = {&bf[1], 256},
        .db = {5, large_page + 320, 0x400140}};
    struct mlx5_ib_srmc owners[2] = {{{&small}, 1}, {{&large}, 1}};
    struct mlx5_ib_sched_worker workers[2] = {{&small, 0}, {&large, 32}};
    struct mock_sched sched = {workers, 2};
    struct mlx5_ib_qp qp = {.srmc_owner = &owners[0], .large_srmc_owner = &owners[1]};
    struct mlx5_ib_modify_qp_resp resp = {0};
    unsigned int before;

    assert(new_size == old_size + 48);
    sched_group.scheds = &sched;
    sched_group.num_sched = 1;
    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        assert(mlx5_ib_modify_qp_resp_length(false, lengths[i]) == old_size);
        assert(mlx5_ib_modify_qp_resp_length(true, lengths[i]) ==
               min_t(size_t, lengths[i], new_size));
    }
    assert(!mlx5_ib_prepare_farm_db_mmaps(&dev, &context, &qp, &resp));
    assert(resp.comp_mask == MLX5_IB_MODIFY_QP_RESP_MASK_FARM_DB);
    assert(qp.farm_db_mmap_entry->cpu_addr == small_page);
    assert(resp.farm_db_offset == 128 && resp.farm_credit_slot_idx == 32);
    for (size_t i = 0; i < 3; i++) {
        resp.response_length = lengths[i];
        before = allocs;
        assert(!mlx5_ib_prepare_large_farm_db_mmaps(&dev, &context, &qp, &resp));
        assert(allocs == before && !qp.large_farm_db_mmap_entry);
        assert(!(resp.comp_mask & MLX5_IB_MODIFY_QP_RESP_MASK_LARGE_FARM_DB));
    }
    resp.response_length = new_size;
    workers[1].credit_ctrl = NULL;
    assert(mlx5_ib_prepare_large_farm_db_mmaps(&dev, &context, &qp, &resp) == -EAGAIN);
    workers[1].credit_ctrl = &large;
    fail_insert = inserts + 2; /* UAR succeeds, DB entry insert fails. */
    assert(mlx5_ib_prepare_large_farm_db_mmaps(&dev, &context, &qp, &resp) == -ENOMEM);
    assert(qp.large_farm_uar_mmap_entry && !qp.large_farm_db_mmap_entry);
    assert(!(resp.comp_mask & MLX5_IB_MODIFY_QP_RESP_MASK_LARGE_FARM_DB));
    fail_insert = 0;
    assert(!mlx5_ib_prepare_large_farm_db_mmaps(&dev, &context, &qp, &resp));
    assert(resp.comp_mask == (MLX5_IB_MODIFY_QP_RESP_MASK_FARM_DB |
                             MLX5_IB_MODIFY_QP_RESP_MASK_LARGE_FARM_DB));
    assert(qp.large_farm_db_mmap_entry->cpu_addr == large_page);
    assert(qp.large_farm_db_mmap_entry->dma_addr == 0x400000);
    assert(qp.large_farm_uar_mmap_entry->address == mdev.bar_addr + 7 * PAGE_SIZE);
    assert(resp.large_farm_uar_reg_offset == 256 && resp.large_farm_db_offset == 320);
    assert(resp.large_farm_db_mmap_offset != resp.farm_db_mmap_offset);
    assert(resp.large_farm_credit_slot_idx == resp.farm_credit_slot_idx);
    assert(resp.large_farm_direct_db_batch == MLX5_SRM_DIRECT_DB_MAX_BATCH);
    before = allocs;
    assert(!mlx5_ib_prepare_large_farm_db_mmaps(&dev, &context, &qp, &resp));
    assert(allocs == before); /* Repeat RTR does not leak duplicate entries. */
    cleanup(&qp);
    puts("PASS: ordinary/Hollow response lengths, large capability gating, distinct DBR/UAR, shared worker credit, retry/reuse");
    return 0;
}
