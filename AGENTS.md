# CLAUDE.md

## What this is

`stackwatch` is a runtime stack-buffer-overflow detector for x86-64 Linux binaries. It attaches to a target process via `ptrace`, intercepts calls to dangerous libc functions (`strcpy`, `gets`, `sprintf`, `strcat`, `memcpy`), and flags overflows at the moment they'd occur by combining:

- **Static analysis** (via Capstone): disassemble the caller function around a flagged call site to recover the stack frame size and destination buffer offset (from `sub rsp, N` / `lea`/`mov [rbp-X]` patterns).
- **Dynamic instrumentation** (via `ptrace`): breakpoint on PLT stubs for dangerous functions, read arguments from registers per the System V x86-64 calling convention, and compute the actual write length at runtime.

## Planned architecture

Four layers, expected to end up as roughly one module each:

1. **ELF/symbol layer** — parses the target's ELF headers, PLT/GOT, and dynamic symbol table to locate call sites for the dangerous-function set.
2. **Tracer (ptrace)** — forks/execs the target under `PTRACE_TRACEME`, plants `INT3` on PLT stub addresses, catches `SIGTRAP`, reads registers for arguments.
3. **Static frame analysis (Capstone)** — disassembles the caller around a hit call site to recover frame size and buffer offset.
4. **Overflow decision + reporting** — compares actual vs. static buffer size, classifies overflow vs. safe, and produces a structured report (aggregatable across a batch of binaries).

## Tech choices and constraints

- Language: C++.
- Disassembly: Capstone (not a hand-rolled decoder) — analysis logic is the point, not instruction decoding.
- Platform: x86-64 Linux only. Dynamically-linked, `-O0 -fno-omit-frame-pointer` target binaries only for v1 — static buffer-size recovery is unreliable against optimized/stripped binaries (frame pointer omission, inlining), and this is a known, accepted limitation rather than something to solve in v1.
- Out of scope for v1: heap bugs (UAF/double-free), format-string `%n` writes, non-x86-64 targets, statically-linked targets (no PLT), multi-threaded targets, exploit generation (detection only).

### Open issue: statically-linked binaries in `ElfReader::load()`

`ElfReader::load()` currently only checks `ELFCLASS64` + `EM_X86_64`, so a
statically-linked ELF64 x86-64 binary passes `load()` even though static
linking is out of scope for v1 (no PLT to enumerate). For now this is
intentional — `load()` returns `true`, and `findDangerousPltEntries()` will
just come back empty for such a binary once implemented. Revisit whether
`load()` itself should reject static binaries outright when PLT enumeration
(M0 remainder) is implemented.

## Milestones (implementation order)

- M0: ELF parsing — enumerate PLT entries for dangerous functions, no execution yet.
- M1: ptrace harness — launch target, breakpoint on `strcpy`, print args on hit.
- M2: Capstone integration — recover stack frame size + buffer offset for the flagged call.
- M3: Overflow decision logic — classify overflow vs. safe, report distance to saved RIP.
- M4: Extend to the full dangerous-function set.
- M5: Validate against 2-3 real CVE binaries, capture detection metrics.
- M6 (stretch): extend the same harness to a second bug class (use-after-free).

Sourcing real CVE binaries that still build on a modern toolchain is nontrivial (likely needs an old-glibc Docker image or VM) — pick concrete CVEs before starting M5.