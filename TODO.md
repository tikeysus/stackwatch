# TODO

## M0: ELF parsing

- [x] Parse ELF64 header via ELFIO, validate ELFCLASS64 + EM_X86_64
- [x] Reject statically-linked binaries (no `PT_DYNAMIC`) — see AGENTS.md
- [x] Enumerate `.dynsym` entries (resolved via ELFIO's relocation accessor)
- [x] Resolve PLT stub addresses for `strcpy`, `gets`, `sprintf`, `strcat`, `memcpy` via `.rela.plt`
- [x] Print found call sites for a sample dynamically-linked binary
- [ ] Prefer `.plt.sec` over `.plt` on CET binaries — see THEORY.md
- [ ] Cross-compile a `hello_vuln` fixture (calls e.g. `strcpy`) to add a positive findDangerousPltEntries() test asserting the resolved PLT address is correct

See `AGENTS.md` for the full milestone list (M0-M6) and `THEORY.md` for
background on the PLT/GOT layout.
