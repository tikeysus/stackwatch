#!/bin/sh
# Builds the hello_vuln fixture family and their PLT address oracles.
#
# Must run in an x86-64 Linux environment with gcc and binutils: the
# debian:trixie container used by CI, or the VM under tools/vm/. See
# THEORY.md for why the CET/non-CET split matters.
#
# Produces, in tests/fixtures/:
#   hello_vuln             default toolchain flags (CET -> split .plt/.plt.sec)
#   hello_vuln_nocet       -fcf-protection=none   (classic single .plt)
#   hello_vuln_norelaplt   hello_vuln with .rela.plt stripped
#   *.plt.expected         {name, address} oracle per binary, from objdump
set -eu

cd "$(dirname "$0")/../.."
fixtures=tests/fixtures
src=$fixtures/hello_vuln.c

# -O0 -fno-omit-frame-pointer per AGENTS.md (keeps frame recovery reliable
# for M2). -fno-builtin stops gcc expanding strcpy/memcpy inline, which
# would leave no PLT entry to find.
CFLAGS="-O0 -fno-omit-frame-pointer -fno-builtin"

echo "toolchain: $(gcc -dumpmachine), $(gcc -dumpversion)"

gcc $CFLAGS -fcf-protection=full -o "$fixtures/hello_vuln" "$src"
gcc $CFLAGS -fcf-protection=none -o "$fixtures/hello_vuln_nocet" "$src"

# A binary with a .plt but no .rela.plt, to prove the resolver returns an
# empty list rather than crashing on a malformed/stripped relocation table.
objcopy --remove-section=.rela.plt "$fixtures/hello_vuln" \
	"$fixtures/hello_vuln_norelaplt"

# Oracle: objdump labels each PLT stub <name@plt> at its real address. It
# derives those independently of ElfReader, so comparing against it is a
# genuine cross-check rather than asserting our arithmetic against itself.
write_oracle() {
	binary=$1
	out=$2
	section=.plt
	if objdump -h "$binary" | grep -q '[.]plt[.]sec'; then
		section=.plt.sec
	fi
	echo "  $(basename "$binary"): reading stubs from $section"
	objdump -d --section="$section" "$binary" \
		| sed -n 's/^0*\([0-9a-f][0-9a-f]*\) <\([a-z_][a-z_0-9]*\)@plt>:$/\2 0x\1/p' \
		| grep -E '^(strcpy|gets|sprintf|strcat|memcpy) ' \
		| sort > "$out"
}

write_oracle "$fixtures/hello_vuln" "$fixtures/hello_vuln.plt.expected"
write_oracle "$fixtures/hello_vuln_nocet" "$fixtures/hello_vuln_nocet.plt.expected"

echo
echo "fixtures built:"
for f in hello_vuln hello_vuln_nocet hello_vuln_norelaplt; do
	printf '  %-22s %s\n' "$f" "$(file -b "$fixtures/$f" | cut -c1-60)"
done
echo
echo "oracles:"
for f in hello_vuln.plt.expected hello_vuln_nocet.plt.expected; do
	echo "  $f:"
	sed 's/^/    /' "$fixtures/$f"
done
