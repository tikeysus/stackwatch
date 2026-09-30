# Test fixtures

Vulnerable C test programs go here, compiled with `-O0 -fno-omit-frame-pointer`
to keep stack-frame recovery reliable for M0-M4 (see the "known limitation" in
`CLAUDE.md`). Once the core detection pipeline works against these, M5 moves
on to real CVE binaries for validation.

No test runner is wired up yet — these are just source files until M1
produces something to assert against.
