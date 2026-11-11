#!/bin/bash
# One-time: download the Debian 13 x86-64 cloud image, verify it, and turn it
# into this VM's 20G disk. Safe to re-run; it refuses to clobber an existing
# disk so rebuilding the VM is always an explicit choice.
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/config.sh"

mkdir -p "$VM_DIR"

if [[ -f "$VM_DISK" ]]; then
	echo "Disk already exists: $VM_DISK"
	echo "Delete it by hand if you really want to rebuild the VM from scratch."
	exit 0
fi

if [[ ! -f "$IMAGE_CACHE" ]]; then
	echo "Downloading $(basename "$IMAGE_URL") (~419 MiB)..."
	curl -fL --progress-bar -o "$IMAGE_CACHE.part" "$IMAGE_URL"
	mv "$IMAGE_CACHE.part" "$IMAGE_CACHE"
fi

echo "Verifying SHA512..."
# Checked rather than trusted: this image is the VM's entire userland, and a
# truncated download would fail much later and far less legibly.
printf '%s  %s\n' "$IMAGE_SHA512" "$IMAGE_CACHE" | shasum -a 512 -c -

echo "Creating $VM_DISK and growing to $VM_DISK_SIZE..."
cp "$IMAGE_CACHE" "$VM_DISK"
qemu-img resize "$VM_DISK" "$VM_DISK_SIZE"

qemu-img info "$VM_DISK" | sed -n '1,6p'
echo "Done. Next: tools/vm/make-seed.sh"
