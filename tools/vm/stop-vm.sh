#!/bin/bash
# Shut the VM down cleanly. Poweroff over ssh rather than killing qemu, so the
# guest filesystem unmounts properly -- same reasoning as stop-kali.sh.
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/config.sh"

if [[ ! -f "$VM_PID" ]] || ! kill -0 "$(cat "$VM_PID")" 2>/dev/null; then
	echo "Not running."
	rm -f "$VM_PID"
	exit 0
fi

pid="$(cat "$VM_PID")"
echo "Asking the guest to power off..."
if ! ssh_vm 'sudo systemctl poweroff' 2>/dev/null; then
	# ssh may be broken (or never have worked, on a bad provision). An ACPI
	# powerdown over the monitor still lets the guest unmount cleanly, which
	# killing qemu outright would not.
	echo "ssh unavailable; sending ACPI powerdown over the qemu monitor."
	if [[ -S "$VM_MONITOR" ]]; then
		echo system_powerdown | nc -U "$VM_MONITOR" >/dev/null 2>&1 || true
	else
		echo "No monitor socket; will fall back to SIGTERM."
	fi
fi

deadline=$(( $(date +%s) + 60 ))
while kill -0 "$pid" 2>/dev/null; do
	if (( $(date +%s) > deadline )); then
		echo "Still up after 60s; sending SIGTERM to qemu."
		kill "$pid" 2>/dev/null || true
		break
	fi
	sleep 2
done

rm -f "$VM_PID"
echo "Stopped."
