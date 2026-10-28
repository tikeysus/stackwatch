#include "elf/elf_reader.h"

#include <filesystem>
#include <set>
#include <stdexcept>

#include <elfio/elfio.hpp>

namespace stackwatch {

namespace {

const std::set<std::string>& dangerousFunctionNames() {
	static const std::set<std::string> names = {
		"strcpy", "gets", "sprintf", "strcat", "memcpy",
	};
	return names;
}

bool hasDynamicSegment(const ELFIO::elfio& reader) {
	for (unsigned int i = 0; i < reader.segments.size(); ++i) {
		if (reader.segments[i]->get_type() == ELFIO::PT_DYNAMIC) {
			return true;
		}
	}
	return false;
}

// Standard x86-64 lazy-binding PLT layout: a 16-byte PLT0 resolver stub,
// followed by one 16-byte stub per .rela.plt entry, in matching order.
// Breaks under split .plt.sec/Intel CET binaries; accepted v1 limitation
// (same category as the project's optimized-binary caveat in AGENTS.md).
std::vector<DangerousCallSite> resolveDangerousPltEntries(const ELFIO::elfio& reader) {
	std::vector<DangerousCallSite> sites;

	const ELFIO::section* plt_section = reader.sections[".plt"];
	const ELFIO::section* relplt_section = reader.sections[".rela.plt"];
	if (plt_section == nullptr || relplt_section == nullptr) {
		return sites;
	}

	const auto& dangerous = dangerousFunctionNames();
	ELFIO::relocation_section_accessor rela(reader, const_cast<ELFIO::section*>(relplt_section));
	for (ELFIO::Elf_Xword i = 0; i < rela.get_entries_num(); ++i) {
		ELFIO::Elf64_Addr offset = 0;
		ELFIO::Elf64_Addr symbol_value = 0;
		std::string symbol_name;
		unsigned type = 0;
		ELFIO::Elf_Sxword addend = 0;
		ELFIO::Elf_Sxword calc_value = 0;
		if (!rela.get_entry(i, offset, symbol_value, symbol_name, type, addend, calc_value)) {
			continue;
		}
		if (type != ELFIO::R_X86_64_JUMP_SLOT) {
			continue;
		}
		if (dangerous.find(symbol_name) == dangerous.end()) {
			continue;
		}
		uint64_t plt_address = plt_section->get_address() + 16 + static_cast<uint64_t>(i) * 16;
		sites.push_back({symbol_name, plt_address});
	}

	return sites;
}

} // namespace

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
	if (!hasDynamicSegment(reader)) {
		return false;
	}

	dangerous_sites_ = resolveDangerousPltEntries(reader);
	loaded_ = true;
	return true;
}

std::vector<DangerousCallSite> ElfReader::findDangerousPltEntries() const {
	if (!loaded_) {
		throw std::logic_error(
			"ElfReader::findDangerousPltEntries called before a successful load()");
	}
	return dangerous_sites_;
}

} // namespace stackwatch
