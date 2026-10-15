#include "elf/elf_reader.h"

#include <filesystem>
#include <stdexcept>

#include <elfio/elfio.hpp>

namespace stackwatch {

ElfReader::ElfReader(std::string path) : path_(std::move(path)) {}

bool ElfReader::load() {
	std::error_code ec;
	if (!std::filesystem::is_regular_file(path_, ec)) {
		return false;
	}

	ELFIO::elfio reader;
	if (!reader.load(path_)) {
		return false;
	}
	if (reader.get_class() != ELFIO::ELFCLASS64) {
		return false;
	}
	if (reader.get_machine() != ELFIO::EM_X86_64) {
		return false;
	}

	loaded_ = true;
	return true;
}

std::vector<DangerousCallSite> ElfReader::findDangerousPltEntries() const {
	if (!loaded_) {
		throw std::logic_error(
			"ElfReader::findDangerousPltEntries called before a successful load()");
	}
	// TODO(M0): walk .dynsym, match against {strcpy, gets, sprintf, strcat,
	// memcpy}, resolve each symbol's PLT stub address via .rela.plt.
	throw std::logic_error("ElfReader::findDangerousPltEntries not implemented");
}

} // namespace stackwatch
