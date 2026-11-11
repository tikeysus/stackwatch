#!/bin/bash
# Build the cloud-init NoCloud seed ISO that configures the VM on first boot.
# Re-run after editing cloud-init/user-data.template, but note that cloud-init
# applies most of it only on an instance's FIRST boot -- for a changed
# user-data to take effect you generally need a fresh disk.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$here/config.sh"

mkdir -p "$VM_DIR"

# Generate this VM's dedicated keypair on first use. No passphrase, because
# the lifecycle scripts have to log in unattended; see the note in config.sh.
if [[ ! -f "$SSH_PUBKEY" ]]; then
	if [[ "$SSH_PUBKEY" == "$VM_SSH_KEY.pub" ]]; then
		echo "Generating a dedicated keypair for this VM at $VM_SSH_KEY..."
		ssh-keygen -t ed25519 -N '' -C "stackwatch-vm" -f "$VM_SSH_KEY" >/dev/null
		chmod 600 "$VM_SSH_KEY"
	else
		echo "No SSH public key at $SSH_PUBKEY" >&2
		exit 1
	fi
fi
staging="$(mktemp -d)"
# The staging dir holds a rendered user-data containing the public key;
# removed on exit so it does not linger in /tmp.
trap 'rm -rf "$staging"' EXIT

# The key is piped through sed's pattern file rather than held in a shell
# variable, so it never lands in the process table or shell history.
sed -e "/__SSH_PUBKEY__/r $SSH_PUBKEY" \
    -e "/__SSH_PUBKEY__/d" \
    -e "s|__VM_USER__|$VM_USER|g" \
    -e "s|__SHARE_MOUNT__|$SHARE_MOUNT|g" \
    "$here/cloud-init/user-data.template" > "$staging/user-data.raw"

# The `r` command above inserts the key line at column 0, but it sits inside a
# YAML sequence that needs its "      - " prefix preserved. Re-indent the
# inserted line in place of the removed placeholder line.
awk '
	/^ssh-/ { printf "      - %s\n", $0; next }
	{ print }
' "$staging/user-data.raw" > "$staging/user-data"
rm -f "$staging/user-data.raw"

cp "$here/cloud-init/meta-data" "$staging/meta-data"

rm -f "$VM_SEED"
# The volume label MUST be CIDATA: that is how cloud-init's NoCloud datasource
# finds this ISO. hdiutil is used instead of xorriso/mkisofs purely to avoid
# adding a Homebrew dependency -- it ships with macOS.
hdiutil makehybrid -quiet -iso -joliet \
	-default-volume-name CIDATA \
	-o "$VM_SEED" "$staging"

echo "Seed ISO: $VM_SEED"
echo "Done. Next: tools/vm/start-vm.sh"
