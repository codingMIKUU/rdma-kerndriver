#!/usr/bin/env python3
"""Exercise actual large-lane mmap setup and ABI length gates without hardware."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MLX5 = ROOT / "drivers/infiniband/hw/mlx5"


def block(text, declaration):
    start = text.index(declaration)
    opening = text.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def main():
    source = (MLX5 / "qp.c").read_text()
    uapi = (ROOT / "include/uapi/rdma/mlx5-abi.h").read_text()
    declarations = [
        block(uapi, "struct mlx5_ib_modify_qp_resp {") + ";",
        block(uapi, "enum mlx5_ib_modify_qp_resp_mask {") + ";",
    ]
    functions = [
        "static u32 mlx5_ib_modify_qp_resp_length(",
        "static int mlx5_ib_prepare_srmc_farm_db_mmaps(",
        "static int mlx5_ib_prepare_farm_db_mmaps(",
        "static int mlx5_ib_prepare_large_farm_db_mmaps(",
    ]
    # Setup must remain inside the existing size-split gate, and cleanup
    # must release both additional entries before owner unregistration.
    attach = block(source, "static int mlx5_ib_attach_hollow_rc_srmc(")
    gated = attach[attach.rindex("if (MLX5_SRM_ENABLE_LARGE_KERNEL_QP)"):]
    assert "mlx5_ib_prepare_large_farm_db_mmaps(" in gated
    destroy = block(source, "static void destroy_qp(")
    for field in ("large_farm_uar_mmap_entry", "large_farm_db_mmap_entry"):
        assert re.search(r"if \(qp->" + field + r"\).*?"
                         r"rdma_user_mmap_entry_remove\(.*?qp->" + field +
                         r" = NULL;", destroy, re.S)
    assert "resp->response_length = sizeof(*resp)" not in source
    assert "udata->outlen < legacy_resp_len" in source
    with tempfile.TemporaryDirectory(prefix="srm-large-direct-db-") as tmp:
        out = Path(tmp)
        generated = "\n".join(declarations + [block(source, f) for f in functions])
        (out / "actual_setup.h").write_text(generated)
        binary = out / "test"
        subprocess.run(["cc", "-std=gnu11", "-Wall", "-Wextra", "-Werror",
                        "-I", str(out), str(ROOT / "tests/srm_large_direct_db_test.c"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
