# Test fixtures

Vulnerable C test programs go here, compiled with `-O0 -fno-omit-frame-pointer`
to keep stack-frame recovery reliable for M0-M4 (see the "known limitation" in
`AGENTS.md`). Once the core detection pipeline works against these, M5 moves
on to real CVE binaries for validation.

`tests/elf_reader_test.cpp` asserts against these via `ctest`. Most are
deliberately malformed or wrong-architecture inputs for `ElfReader::load()`'s
rejection paths: `hello_32` (ELFCLASS32), `hello_aarch64` / `hello_em386`
(wrong `e_machine`), `hello_static` (no `PT_DYNAMIC`), `hello_empty`,
`hello_truncated`, `hello_nonelf_text.txt`. `hello_dyn` and `libhello.so` are
the valid-input cases. `hello_directory` is created by the test at runtime,
since git doesn't track empty directories.

Note that no fixture currently imports a dangerous libc function, so
`findDangerousPltEntries()`'s resolution path is only covered on its
empty-result branch. A `hello_vuln` fixture to close that gap is tracked in
`TODO.md`.
