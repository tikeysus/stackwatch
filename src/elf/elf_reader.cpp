#include "elf/elf_reader.h"

#include <stdexcept>

namespace stackwatch {

ElfReader::ElfReader(std::string path) : path_(std::move(path)) {}

bool ElfReader::load() {
	// TODO(M0): open path_ with ELFIO::elfio, validate ELFCLASS64 + EM_X86_64.
	throw std::logic_error("ElfReader::load not implemented");
}

std::vector<DangerousCallSite> ElfReader::findDangerousPltEntries() const {
	// TODO(M0): walk .dynsym, match against {strcpy, gets, sprintf, strcat,
	// memcpy}, resolve each symbol's PLT stub address via .rela.plt.
	throw std::logic_error("ElfReader::findDangerousPltEntries not implemented");
}

} // namespace stackwatch
