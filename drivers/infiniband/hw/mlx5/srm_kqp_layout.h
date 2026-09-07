/* SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB */
#ifndef MLX5_SRM_KQP_LAYOUT_H
#define MLX5_SRM_KQP_LAYOUT_H

/* Each peer owns [small lanes][optional large lanes] in a disjoint block.
 * These helpers are also compiled by the hardware-independent layout tests.
 * Callers validate lanes > 0, levels in {1,2}, workers <= lanes and capacity.
 */
static inline unsigned int mlx5_srm_layout_large(unsigned int slot,
                                                unsigned int lanes,
                                                unsigned int levels)
{
    return levels == 2 && slot % (lanes * levels) >= lanes;
}

static inline unsigned int mlx5_srm_layout_owner(unsigned int slot,
                                                unsigned int lanes,
                                                unsigned int workers)
{
    /* Inverse of worker boundaries floor(lanes * worker / workers),
     * including lane counts not divisible by the number of workers.
     */
    return ((slot % lanes + 1) * workers - 1) / lanes;
}

static inline unsigned int mlx5_srm_layout_owns(unsigned int slot,
                                               unsigned int lanes,
                                               unsigned int begin,
                                               unsigned int end)
{
    unsigned int lane = slot % lanes;

    return lane >= begin && lane < end;
}

/* Rebuild only when a complete peer block is published, never per WQE.
 * Both rings span ALL peers.  A small pass still precedes a large pass.
 */
static inline void mlx5_srm_layout_build_rings(
    unsigned int *next, unsigned int *prev, unsigned int heads[2],
    unsigned int counts[2], unsigned int slots, unsigned int lanes,
    unsigned int levels, unsigned int begin, unsigned int end)
{
    unsigned int tails[2] = {0, 0};
    unsigned int slot, cls;

    heads[0] = heads[1] = 0;
    counts[0] = counts[1] = 0;
    for (slot = 0; slot < slots; slot++) {
        if (!mlx5_srm_layout_owns(slot, lanes, begin, end))
            continue;
        cls = mlx5_srm_layout_large(slot, lanes, levels);
        if (!counts[cls]) {
            heads[cls] = slot;
            next[slot] = prev[slot] = slot;
        } else {
            next[tails[cls]] = slot;
            prev[slot] = tails[cls];
            next[slot] = heads[cls];
            prev[heads[cls]] = slot;
        }
        tails[cls] = slot;
        counts[cls]++;
    }
}

#endif
