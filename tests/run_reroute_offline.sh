#!/usr/bin/env bash
set -euo pipefail
test_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
test_dir=$(mktemp -d -t srm-reroute-test.XXXXXXXX)
trap 'rm -f -- "$test_dir/state"; rmdir -- "$test_dir"' EXIT
for simplify in 0 1; do
    for publish_batch in 1 64; do
        "${CC:-cc}" -std=gnu11 -O1 -g -fsanitize=address,undefined \
            -DMLX5_SRM_ENABLE_REROUTE=1 \
            -DMLX5_SRM_ENABLE_CQE_SIMPLIFY="$simplify" \
            -DMLX5_SRM_CQE_PUBLISH_BATCH="$publish_batch" \
            "$test_root/tests/reroute_state_test.c" -o "$test_dir/state"
        "$test_dir/state"
    done
done
if [[ -f "$test_root/../rdma-core/kernel-headers/rdma/mlx5-srm-reroute.h" ]]; then
    cmp "$test_root/include/uapi/rdma/mlx5-srm-reroute.h" \
        "$test_root/../rdma-core/kernel-headers/rdma/mlx5-srm-reroute.h"
    cmp "$test_root/include/uapi/rdma/mlx5-abi.h" \
        "$test_root/../rdma-core/kernel-headers/rdma/mlx5-abi.h"
    printf '%s\n' 'PASS: provider/kernel ABI headers identical'
fi
