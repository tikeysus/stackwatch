# stackwatch

A runtime stack-buffer-overflow detector for x86-64 Linux binaries. It
ptrace-attaches to a target process, breakpoints calls to dangerous libc
functions (`strcpy`, `gets`, `sprintf`, `strcat`, `memcpy`), disassembles the
calling function with Capstone to recover the destination buffer's
stack-frame layout, and flags a write the moment it would overflow into the
saved return address.

## Status

Scaffold only — no detection logic implemented yet. Currently on M0 (ELF
parsing / PLT resolution). See `CLAUDE.md` for the full architecture and
milestone list (M0-M6), and `TODO.md` for the current milestone's checklist.

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
