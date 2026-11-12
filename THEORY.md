# Theory notes

Background reading for decisions made in this repo. Not API docs — the
"why this is shaped like that" that isn't obvious from the code.

## The PLT, lazy binding, and why `.plt.sec` exists

### The classic layout

When a dynamically-linked program calls `strcpy`, the compiler can't emit a
direct call: `strcpy` lives in libc, mapped at an address not known until
the loader picks one (ASLR). So the call goes through two tables:

- **PLT** (Procedure Linkage Table, section `.plt`) — executable stubs. Each
  imported function gets one 16-byte stub. `call strcpy@plt` targets this.
- **GOT** (Global Offset Table, section `.got.plt`) — writable function
  pointers, one slot per stub.

The stub is roughly `jmp *GOT[n]`, plus a fallback path. Classic layout:

    .plt + 0    PLT0        push GOT[1]; jmp *GOT[2]   (resolver trampoline)
    .plt + 16   stub for .rela.plt[0]
    .plt + 32   stub for .rela.plt[1]
    ...

One header stub, then one 16-byte stub per `.rela.plt` relocation, in the
same order. That is exactly the assumption encoded in
`resolveDangerousPltEntries()`: `plt_address = .plt base + 16 + i * 16`.

**Lazy binding** is why the indirection exists at all. At startup GOT[n]
points back into its own stub's second instruction, which pushes the
relocation index and jumps to the resolver. The resolver finds the real
`strcpy`, overwrites GOT[n] with it, and jumps there. Every later call goes
straight through the patched pointer. Cost is paid once, per function
actually called, instead of resolving every import at startup.

This matters to stackwatch beyond address arithmetic: the PLT stub is a
single fixed address inside the target that every call to `strcpy` funnels
through, which is what makes "one `INT3` per dangerous function" a viable
hooking strategy. No PLT (static binaries) means no chokepoint — see the
rejection decision in AGENTS.md.

### What CET changed

Intel CET (Control-flow Enforcement Technology) adds **IBT**, Indirect
Branch Tracking: an indirect jump may only land on an `endbr64` instruction,
or the CPU faults. This kills ROP/JOP gadget chains that jump into the
middle of existing code.

PLT stubs are indirect-jump targets, so they need `endbr64` — but that's 4
more bytes in an already-full 16-byte stub. The linker's answer was to split
the table in two:

- `.plt.sec` — one 16-byte stub per import, starting with `endbr64`, doing
  the actual `jmp *GOT[n]`. **This is what call sites now target.**
- `.plt` — kept for the lazy-resolution path only.

Crucially `.plt.sec` has **no PLT0 header**, so its indexing is
`.plt.sec base + i * 16` — no `+ 16` offset. Applying the classic formula to
a CET binary yields an address one stub too high, and breakpointing it would
trap on the wrong function or mid-instruction.

Hence the rule stackwatch follows: **if `.plt.sec` is present, resolve
against it (`base + i * 16`); otherwise fall back to `.plt`
(`base + 16 + i * 16`).**

### What's actually out there (measured on Debian 13, gcc 14)

`-fcf-protection=full` alone does **not** produce a `.plt.sec`. It marks the
object as CET-capable, but GNU ld only splits the table when asked:

    -fcf-protection=full                        -> .plt .plt.got
    -fcf-protection=full -Wl,-z,lazy            -> .plt .plt.got
    -fcf-protection=full -Wl,-z,ibtplt          -> .plt .plt.got .plt.sec
    -fcf-protection=full -Wl,-z,lazy,-z,ibtplt  -> .plt .plt.got .plt.sec
    -fcf-protection=full -Wl,-z,lazy,-z,ibt     -> .plt .plt.got .plt.sec

And no Debian 13 system binary carries one: `ls`, `cat`, `cp`, `objdump`,
`find`, `sed`, `grep`, `dash`, `apt-get` and libc itself are all `.plt` +
`.plt.got`, 0 of 9. Distros that pass `-z ibtplt` (Fedora/RHEL) do ship
`.plt.sec` binaries, so the support is worth having — but on Debian the
classic path is the one that runs.

This is why `tests/fixtures/hello_vuln` is built with an explicit
`-Wl,-z,ibtplt`: without it the fixture silently exercises the classic
layout and the `.plt.sec` branch gets no coverage at all.

### Further reading

- System V ABI, x86-64 supplement — the normative PLT/GOT description
  <https://gitlab.com/x86-psABIs/x86-64-ABI>
- Ulrich Drepper, "How To Write Shared Libraries" — the canonical deep dive
  on lazy binding (§1.5.5)
  <https://www.akkadia.org/drepper/dsohowto.pdf>
- Intel CET specification, IBT chapter
  <https://www.intel.com/content/www/us/en/developer/articles/technical/technical-look-control-flow-enforcement-technology.html>
- Inspect either layout yourself:
  `objdump -d --section=.plt --section=.plt.sec <binary>` and
  `readelf -r <binary>` to see `.rela.plt` ordering line up with the stubs.
