# Hollow RC: multiple remote nodes

## Resource layout

`num_kqps` remains the number of base lanes **per remote GID**, not the
total number of outgoing QPs on a node.  Each peer receives a disjoint block
of `num_kqps * MLX5_SRM_KERNEL_QP_LEVELS` global control/KQP slots.

For 64 lanes with size splitting off, two remote nodes use slots 0-63 and
64-127.  With splitting on, the blocks are 0-127 and 128-255; each block
contains 64 small then 64 large lanes.  Small and large partners retain the
same worker.  Worker boundaries repeat within each peer's block.

* Complete peer groups are release-published to lookup and scheduler rings.
  Concurrent logical QP and legacy AH setup is serialized on a setup-only
  mutex.  A CM callback does not take that mutex.
* Each worker expands its private hot/round-robin rings when a group becomes
  ready.  Both size classes span all published peers; small still comes first.
* Existing per-worker CQs and credit limits are **shared across peers**.  A new
  peer does not get a new full credit allowance and does not reset cumulative
  issued/completed counts or replace the credit/mailbox pointer.  Its mailbox
  hints carry global KQP slot IDs.
* SQ/control/publish mappings and CQ completion/error decoding use the unique
  global slot.  The existing rdma-core response fields already carry these
  IDs, so no userspace ABI change, provider rebuild or MPI rebuild is needed.
* Incoming target SRMC objects have separate lifetimes from outgoing SRMCs.
  Both setup paths find an empty hash bucket for every insertion, including
  when remote nodes connect back concurrently.
* Partial group failures are not published and are sticky until module
  reload.  Previously created objects stay tracked for module teardown.
  Exhausting either control slots or SRMC hash capacity returns an error;
  it never wraps onto another peer's slots.

The unchanged NUM_SRMC limit covers initiator AND target objects in the mixed
hash table; it is not an unlimited-node implementation.  The logical-QP route
limit is also unchanged.  Increasing node/rank counts may require a separate
capacity audit.  This patch does not implement independent per-job teardown:
continue reloading the module between independent cross-node Hollow jobs.

## Verification without hardware changes

From the kernel-driver repository:

```bash
python3 tests/test_srm_multipeer.py
python3 tests/check_srm_kernel_compile.py
git diff --check
```

The first test extracts the actual outgoing setup, balanced selection and
hot-ring functions, compiles them against mocked allocations/CM calls, and
tests peer reuse, concurrent setup, partial failure, capacity and target hash
collisions.  The actual layout helper is exhaustively exercised for 1-65
lanes, every valid worker count, 1/2 size classes, and 1-3 peers.

The compile check requires a previous local Kbuild and its `.o.cmd` records.
It reuses those compiler flags and kernel headers and compiles scheduler,
QP, CQ, AH and main units with size split both off and on.  All output and
dependencies are isolated from the existing build.  It does NOT link a module,
install libraries/modules, use sudo, or test hardware behavior.

## Deployment and hardware acceptance

Preserve each node's existing local IP, HCA, NUMA, worker, lane and batch
settings.  Commit/pull this code through Git, retaining these local settings.
After **all MPI/General/RDMA test processes on all participating nodes exit**,
run separately on each host with the user's normal privileges:

```bash
cd ~/zxm/rdma-kerndriver
sudo bash kernel_make.sh
```

This script builds, installs, unloads and reloads mlx5_ib.  Do not force-unload
an in-use module.  All three nodes must load the fix before a three-node
Hollow test; merely updating source does not update a running kernel.

On lingbo11, start with two ranks per node (not 128) and validate:

```bash
cd ~/zxm/mvapich2-2.3.7
HOSTS=192.168.1.5,192.168.1.1,192.168.5.67 \
HCA_MAP=192.168.1.5=mlx5_1,192.168.1.1=mlx5_3,192.168.5.67=mlx5_1 \
USER_MAP=192.168.1.5=lingbo11,192.168.1.1=lingbo12,192.168.5.67=lingbo10 \
NP=6 PPN=2 \
contrib/hollow-rc/run_osu_collective.sh hollow alltoall \
-m 1024:32768 -i 20 -x 5 -c
```

192.168.5.67 is lingbo10's SSH address.  Its mlx5_1/port 1/GID index 3 still
uses RDMA address 192.168.1.2.  HOSTS/HCA_MAP/USER_MAP keys identify hosts, not
the RDMA source address.  All participating RDMA addresses must be reachable.

Each node should report two lines like (64 lanes, split off):

```
hollow RC peer KQP group ready: gid=... base=0 count=64 total=64 workers=1
hollow RC peer KQP group ready: gid=... base=64 count=64 total=128 workers=1
```

Peer GID order may differ between hosts.  Require OSU Validation Pass, no RTR
or WC errors, and both peers' progress before increasing PPN to 4/8/128.
Repeat with size split on and with two workers if those modes will be used.
Recheck two-node Hollow and ordinary RC/XRC.  Memory/performance measurements
must use successful jobs; a failed MPI_Init's RSS peak is not an Alltoall
memory result.  Offline tests and object compilation are not proof of a
successful hardware run.
