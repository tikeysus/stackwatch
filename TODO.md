# TODO

## M0: ELF parsing

- [ ] Parse ELF64 header via ELFIO, validate ELFCLASS64 + EM_X86_64
- [ ] Enumerate `.dynsym` entries
- [ ] Resolve PLT stub addresses for `strcpy`, `gets`, `sprintf`, `strcat`, `memcpy` via `.rela.plt`
- [ ] Print found call sites for a sample dynamically-linked binary

See `CLAUDE.md` for the full milestone list (M0-M6).
