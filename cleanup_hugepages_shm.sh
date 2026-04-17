#!/usr/bin/env bash
set -euo pipefail

# Remove SysV SHM segments, defaulting to 2MB hugepage segments.
# Usage:
#   sudo ./cleanup_hugepages_shm.sh            # remove 2MB segments
#   sudo ./cleanup_hugepages_shm.sh 2097152    # size filter (bytes)
#   sudo ./cleanup_hugepages_shm.sh 2097152 root

size_filter="${1:-2097152}"
owner_filter="${2:-}"

echo "Scanning SysV SHM segments..."

mapfile -t lines < <(ipcs -m | awk 'NR>3 && $1 ~ /^0x/ {print $1, $2, $3, $5, $6}')

if [[ ${#lines[@]} -eq 0 ]]; then
  echo "No SysV SHM segments found."
  exit 0
fi

targets=()
for line in "${lines[@]}"; do
  read -r key shmid owner bytes nattch <<< "$line"

  if [[ "$bytes" != "$size_filter" ]]; then
    continue
  fi
  if [[ -n "$owner_filter" && "$owner" != "$owner_filter" ]]; then
    continue
  fi

  targets+=("$shmid")
  printf '  key=%s shmid=%s owner=%s bytes=%s nattch=%s\n' "$key" "$shmid" "$owner" "$bytes" "$nattch"
done

if [[ ${#targets[@]} -eq 0 ]]; then
  echo "No matching SHM segments (size=$size_filter owner=${owner_filter:-any})."
  exit 0
fi

echo
read -r -p "Remove ${#targets[@]} SHM segments listed above? [y/N] " reply
case "$reply" in
  y|Y)
    for shmid in "${targets[@]}"; do
      ipcrm -m "$shmid"
    done
    echo "Done."
    ;;
  *)
    echo "Aborted."
    exit 1
    ;;
esac
