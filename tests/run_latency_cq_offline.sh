#!/usr/bin/env bash
set -euo pipefail
test_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
test_dir=$(mktemp -d -t srm-latency-cq-test.XXXXXXXX)
trap 'rm -f -- "$test_dir/poll"; rmdir -- "$test_dir"' EXIT
for enabled in 0 1; do
    {
        sed -n '1,$p' "$test_root/tests/latency_cq_priority_test.c"
        sed -n '/\/\* LATENCY_CQ_PRIORITY_TEST_BEGIN/,/\/\* LATENCY_CQ_PRIORITY_TEST_END/p' \
            "$test_root/drivers/infiniband/hw/mlx5/scheduler.c"
        sed -n '/^static __always_inline int poll_srmc_inline(/,/^}/p' \
            "$test_root/drivers/infiniband/hw/mlx5/scheduler.c"
    } | "${CC:-cc}" -x c -std=gnu11 -O1 -g -Wall -Wextra -Werror \
        -Wno-unused-parameter -fsanitize=address,undefined -fno-pie -no-pie \
        -DMLX5_SRM_ENABLE_PRIVATE_CQ=1 \
        -DMLX5_SRM_ENABLE_LATENCY_CQ_PRIORITY="$enabled" \
        -o "$test_dir/poll" -
    "$test_dir/poll"
done
