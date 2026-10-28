# TODO

## M0: ELF parsing

- [ ] Parse ELF64 header via ELFIO, validate ELFCLASS64 + EM_X86_64
- [ ] Enumerate `.dynsym` entries
- [ ] Resolve PLT stub addresses for `strcpy`, `gets`, `sprintf`, `strcat`, `memcpy` via `.rela.plt`
- [ ] Print found call sites for a sample dynamically-linked binary
- [ ] Cross-compile a `hello_vuln` fixture (calls e.g. `strcpy`) to add a positive findDangerousPltEntries() test asserting the resolved PLT address is correct

See `CLAUDE.md` for the full milestone list (M0-M6).
