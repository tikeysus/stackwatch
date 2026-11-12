# stackwatch

A runtime stack-buffer-overflow detector for x86-64 Linux binaries. It
ptrace-attaches to a target process, breakpoints calls to dangerous libc
functions (`strcpy`, `gets`, `sprintf`, `strcat`, `memcpy`), disassembles the
calling function with Capstone to recover the destination buffer's
stack-frame layout, and flags a write the moment it would overflow into the
saved return address.

## Status

M0 (ELF parsing / PLT resolution) is complete: `ElfReader` validates a
target, resolves PLT stub addresses for the dangerous-function set from
`.rela.plt` — handling both the classic `.plt` layout and the `.plt.sec`
split that Intel CET produces — and `stackwatch <binary>` prints them. No
tracing, disassembly, or overflow detection yet; those are M1-M3. See `AGENTS.md` for the full
architecture and milestone list (M0-M6), `TODO.md` for the current
milestone's checklist, and `THEORY.md` for PLT/GOT background.

## Building

Requires an x86-64 Linux environment with Capstone installed
(`libcapstone-dev` or equivalent) and a C++20 compiler. On an Apple Silicon
Mac, this means building and running inside a Linux VM (UTM/Lima + QEMU),
since `ptrace`, ELF, and x86-64 are all Linux/architecture-specific.

```sh
cmake -S . -B build
cmake --build build
```

ELFIO (ELF parsing) is vendored under `third_party/ELFIO` — no separate
install needed for it.
