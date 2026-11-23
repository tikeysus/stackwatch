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

### Decision: reject statically-linked binaries in `ElfReader::load()`

Static binaries are rejected outright in `load()`, not left to silently
produce empty results from `findDangerousPltEntries()`. Detection: no
`PT_DYNAMIC` program header present → `load()` returns `false`.

Reasoning: static linking has no PLT/GOT, so there's no single call-site
address to breakpoint per dangerous function. Supporting it would mean
locating inlined/statically-linked copies of each dangerous function via
instruction-byte signature matching (glibc build/version/SIMD-variant
dependent) instead of one known PLT stub address — a fundamentally
different and much less reliable detection strategy. Out of scope for v1.

### Note: test-suite state at M0 close (2026-11-23)

`tests/elf_reader_test.cpp` case 11 is the normative assertion for the
decision above: `loadOf("hello_static") == false`. Issue #2 tracked
aligning it and was closed with M0.

Verified green on 2026-11-23 in the x86-64 VM (`tools/vm/stackwatch-vm
test`), after the merge of PR #3: 23/23 cases, 0 failures.

Two things to know when reading that output:

- `ctest` reports "1 test" because the suite is one registered target. The
  23 cases live inside that binary; `check()` accumulates into
  `g_failures` rather than aborting, so all 23 run and the binary exits
  nonzero if any fail. Issue #2's "13/13" acceptance criterion is stale.
- `skip()` cases (those needing the `hello_vuln` fixtures, which require an
  x86-64 toolchain to build) print `[SKIP]` and do not count as failures,
  so the ctest summary cannot distinguish "passed" from "skipped". Run the
  test binary directly to see the per-case list.

## Milestones (implementation order)

- M0: ELF parsing — enumerate PLT entries for dangerous functions, no execution yet.
- M1: ptrace harness — launch target, breakpoint on `strcpy`, print args on hit.
- M2: Capstone integration — recover stack frame size + buffer offset for the flagged call.
- M3: Overflow decision logic — classify overflow vs. safe, report distance to saved RIP.
- M4: Extend to the full dangerous-function set.
- M5: Validate against 2-3 real CVE binaries, capture detection metrics.
- M6 (stretch): extend the same harness to a second bug class (use-after-free).

Sourcing real CVE binaries that still build on a modern toolchain is nontrivial (likely needs an old-glibc Docker image or VM) — pick concrete CVEs before starting M5.