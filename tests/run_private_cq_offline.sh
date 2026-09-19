#!/usr/bin/env bash
set -euo pipefail
test_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
test_dir=$(mktemp -d -t srm-private-cq-test.XXXXXXXX)
trap 'rm -f -- "$test_dir/poll"; rmdir -- "$test_dir"' EXIT
for reroute in 0 1; do
  for budget in 1 3 256; do
    for timing in 0 1; do
        {
            sed -n '1,$p' "$test_root/tests/private_cq_test.c"
            sed -n '/\/\* PRIVATE_CQ_COMPLETE_TEST_BEGIN/,/\/\* PRIVATE_CQ_COMPLETE_TEST_END/p' \
                "$test_root/drivers/infiniband/hw/mlx5/cq.c"
            sed -n '/\/\* PRIVATE_CQ_TEST_BEGIN/,/\/\* PRIVATE_CQ_TEST_END/p' \
                "$test_root/drivers/infiniband/hw/mlx5/cq.c"
            sed -n '/\/\* PRIVATE_CQ_BUDGET_TEST_BEGIN/,/\/\* PRIVATE_CQ_BUDGET_TEST_END/p' \
                "$test_root/drivers/infiniband/hw/mlx5/scheduler.c"
        } | "${CC:-cc}" -x c -std=gnu11 -O1 -g -Wall -Wextra -Werror \
            -fsanitize=address,undefined -fno-pie -no-pie \
            -DMLX5_SRM_PRIVATE_CQ_POLL_BUDGET="$budget" \
            -DMLX5_SRM_ENABLE_PRIVATE_CQ=1 \
            -DMLX5_SRM_ENABLE_REROUTE="$reroute" \
            -DMLX5_SRM_ENABLE_WQE_TIMING="$timing" \
            -o "$test_dir/poll" -
        "$test_dir/poll"
    done
  done
done
