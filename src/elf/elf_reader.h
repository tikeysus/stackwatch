#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace stackwatch {

struct DangerousCallSite {
	std::string function_name;
	uint64_t plt_address;
};

// M0: parses the target's ELF64 header, section headers, and .dynsym via
// ELFIO to resolve PLT stub addresses for a fixed set of dangerous libc
// functions (strcpy, gets, sprintf, strcat, memcpy).
class ElfReader {
public:
	explicit ElfReader(std::string path);

	// Loads and parses the ELF file. Returns false on parse failure.
	bool load();

	// Returns PLT call sites for any dangerous function present in the
	// binary's dynamic symbol table.
	std::vector<DangerousCallSite> findDangerousPltEntries() const;

private:
	std::string path_;
};

} // namespace stackwatch
