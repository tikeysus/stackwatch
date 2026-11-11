#!/bin/bash
# Boot the x86-64 development VM, headless and detached, and wait for sshd.
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/config.sh"

for f in "$VM_DISK" "$VM_SEED"; do
	if [[ ! -f "$f" ]]; then
		echo "Missing $f -- run fetch-image.sh and make-seed.sh first." >&2
		exit 1
	fi
done

if [[ -f "$VM_PID" ]] && kill -0 "$(cat "$VM_PID")" 2>/dev/null; then
	echo "Already running (pid $(cat "$VM_PID"))."
	echo "  ssh -p $VM_SSH_PORT $VM_USER@localhost"
	exit 0
fi

# No -accel: an arm64 host cannot hardware-accelerate x86-64, so this is TCG
# emulation, deliberately. A real x86-64 kernel runs here, which is the whole
# point -- ptrace register semantics are faithful, unlike the Rosetta path
# where PTRACE_GETREGS returns EIO.
#
# -cpu max rather than -cpu host: "host" is meaningless without hardware
# virtualisation and qemu rejects it under TCG.
qemu-system-x86_64 \
	-M q35 -cpu max -smp "$VM_CPUS" -m "$VM_MEM" \
	-drive file="$VM_DISK",format=qcow2,if=virtio \
	-drive file="$VM_SEED",format=raw,media=cdrom \
	-device virtio-net-pci,netdev=n0 \
	-netdev user,id=n0,hostfwd=tcp::"$VM_SSH_PORT"-:22 \
	-fsdev local,id=src,path="$SHARE_PATH",security_model=mapped-xattr \
	-device virtio-9p-pci,fsdev=src,mount_tag="$SHARE_TAG" \
	-display none \
	-serial "file:$VM_LOG" \
	-monitor "unix:$VM_MONITOR,server,nowait" \
	-pidfile "$VM_PID" \
	-daemonize

echo "qemu started (pid $(cat "$VM_PID")). Console log: $VM_LOG"
printf 'Waiting for sshd on port %s' "$VM_SSH_PORT"

# First boot is much slower than later ones: cloud-init grows the filesystem
# and installs the toolchain, all under emulation.
deadline=$(( $(date +%s) + 1200 ))
until ssh_vm true 2>/dev/null; do
	if ! kill -0 "$(cat "$VM_PID")" 2>/dev/null; then
		printf '\nqemu died. Last console output:\n'
		tail -30 "$VM_LOG"
		exit 1
	fi
	if (( $(date +%s) > deadline )); then
		printf '\nTimed out waiting for ssh. Last console output:\n'
		tail -30 "$VM_LOG"
		exit 1
	fi
	printf '.'
	sleep 5
done

printf '\nReady.\n\n'
# The -i and IdentitiesOnly are not optional: the VM trusts only its own
# dedicated key, and without IdentitiesOnly ssh offers every key in ~/.ssh
# and can exhaust MaxAuthTries before reaching it.
echo "  ssh -p $VM_SSH_PORT -i $VM_SSH_KEY -o IdentitiesOnly=yes $VM_USER@localhost"
echo "  repo shared at $SHARE_MOUNT (build elsewhere, e.g. cmake -S ~/stackwatch -B ~/build-vm)"
