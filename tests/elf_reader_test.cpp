#include "elf/elf_reader.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

#include <elfio/elfio.hpp>

namespace {

int g_failures = 0;

void check(bool condition, const std::string& case_name) {
	if (condition) {
		std::printf("[PASS] %s\n", case_name.c_str());
	} else {
		std::printf("[FAIL] %s\n", case_name.c_str());
		++g_failures;
	}
}

std::string fixture(const std::string& name) {
	return std::string(FIXTURES_DIR) + "/" + name;
}

bool loadOf(const std::string& fixture_name) {
	stackwatch::ElfReader reader(fixture(fixture_name));
	return reader.load();
}

int g_skipped = 0;

void skip(const std::string& case_name, const std::string& reason) {
	std::printf("[SKIP] %s (%s)\n", case_name.c_str(), reason.c_str());
	++g_skipped;
}

bool fixtureExists(const std::string& name) {
	std::error_code ec;
	return std::filesystem::is_regular_file(fixture(name), ec);
}

// Expected {symbol, PLT stub address} pairs, generated from objdump by
// tools/fixtures/build-vuln-fixtures.sh. objdump derives stub addresses
// independently of ElfReader, so comparing against it is a real cross-check
// rather than asserting our own arithmetic against itself.
std::map<std::string, uint64_t> readOracle(const std::string& name) {
	std::map<std::string, uint64_t> expected;
	std::ifstream in(fixture(name));
	std::string symbol;
	std::string address;
	while (in >> symbol >> address) {
		expected[symbol] = std::stoull(address, nullptr, 16);
	}
	return expected;
}

std::map<std::string, uint64_t> resolvedSites(const std::string& fixture_name) {
	std::map<std::string, uint64_t> sites;
	stackwatch::ElfReader reader(fixture(fixture_name));
	if (!reader.load()) {
		return sites;
	}
	for (const stackwatch::DangerousCallSite& site : reader.findDangerousPltEntries()) {
		sites[site.function_name] = site.plt_address;
	}
	return sites;
}

// The hello_vuln family needs an x86-64 Linux toolchain to produce, so every
// case using it is skipped rather than failed when the binary or its oracle
// is missing.
bool vulnFixtureReady(const std::string& binary) {
	return fixtureExists(binary) && fixtureExists(binary + ".plt.expected");
}

} // namespace

int main() {
	// 1. Valid ELF64 x86-64 dynamic executable.
	check(loadOf("hello_dyn") == true, "dynamic executable loads");

	// 2. Valid ELF64 x86-64 shared object.
	check(loadOf("libhello.so") == true, "shared object loads");

	// 3. Valid ELF32 is rejected.
	check(loadOf("hello_32") == false, "ELF32 binary rejected");

	// 4. Valid ELF64 but EM_AARCH64 is rejected.
	check(loadOf("hello_aarch64") == false, "AArch64 binary rejected");

	// 5. Valid ELF64 but EM_386 is rejected.
	check(loadOf("hello_em386") == false, "EM_386 binary rejected");

	// 6. Nonexistent path.
	check(loadOf("does_not_exist") == false, "nonexistent path rejected");

	// 7. Empty file.
	check(loadOf("hello_empty") == false, "empty file rejected");

	// 8. Non-ELF file.
	check(loadOf("hello_nonelf_text.txt") == false, "non-ELF text file rejected");

	// 9. Truncated ELF header.
	check(loadOf("hello_truncated") == false, "truncated ELF header rejected");

	// 10. Directory path. Created here rather than checked into git, since
	// git doesn't track empty directories.
	std::filesystem::create_directories(fixture("hello_directory"));
	check(loadOf("hello_directory") == false, "directory path rejected");

	// 11. Statically-linked ELF64 x86-64 is rejected (no PT_DYNAMIC segment).
	// See AGENTS.md: "Decision: reject statically-linked binaries".
	check(loadOf("hello_static") == false, "static binary rejected");

	// 12. load() is idempotent across repeated calls.
	{
		stackwatch::ElfReader reader(fixture("hello_dyn"));
		bool first = reader.load();
		bool second = reader.load();
		check(first == true && second == true, "load() twice on valid file both succeed");
	}

	// 13. findDangerousPltEntries() before a successful load() throws loudly.
	{
		stackwatch::ElfReader reader(fixture("hello_dyn"));
		bool threw = false;
		try {
			reader.findDangerousPltEntries();
		} catch (const std::logic_error&) {
			threw = true;
		}
		check(threw, "findDangerousPltEntries() before load() throws");
	}

	// 14. Dynamic binary with no dangerous calls resolves to an empty list.
	{
		stackwatch::ElfReader reader(fixture("hello_dyn"));
		reader.load();
		check(reader.findDangerousPltEntries().empty(),
			"findDangerousPltEntries() on hello_dyn returns no entries");
	}

	// 15. Shared object with no dangerous calls resolves to an empty list.
	{
		stackwatch::ElfReader reader(fixture("libhello.so"));
		reader.load();
		check(reader.findDangerousPltEntries().empty(),
			"findDangerousPltEntries() on libhello.so returns no entries");
	}

	// 16. findDangerousPltEntries() throws on a rejected static binary
	// (load() returned false, so loaded_ stays false).
	{
		stackwatch::ElfReader reader(fixture("hello_static"));
		reader.load();
		bool threw = false;
		try {
			reader.findDangerousPltEntries();
		} catch (const std::logic_error&) {
			threw = true;
		}
		check(threw, "findDangerousPltEntries() throws after rejected static binary");
	}

	// 17. findDangerousPltEntries() is stable across repeated calls.
	{
		stackwatch::ElfReader reader(fixture("hello_dyn"));
		reader.load();
		auto first = reader.findDangerousPltEntries();
		auto second = reader.findDangerousPltEntries();
		check(first.size() == second.size(),
			"findDangerousPltEntries() returns consistent results across calls");
	}

	// --- .plt.sec / positive-resolution cases, against the hello_vuln family
	// built by tools/fixtures/build-vuln-fixtures.sh. See THEORY.md for why
	// CET binaries need a different stub index.

	// 18. CET binary: every dangerous import resolves to the address objdump
	// reports for its .plt.sec stub. This is the case the current
	// PLT0 + 16*i arithmetic is expected to get wrong.
	if (!vulnFixtureReady("hello_vuln")) {
		skip("CET fixture matches objdump .plt.sec addresses", "fixture not built");
	} else {
		const std::map<std::string, uint64_t> expected = readOracle("hello_vuln.plt.expected");
		check(!expected.empty() && resolvedSites("hello_vuln") == expected,
			"CET fixture matches objdump .plt.sec addresses");
	}

	// 19. Non-CET binary: the classic single-.plt layout still resolves, so
	// adding .plt.sec support doesn't regress the PLT0 + 16*i path.
	if (!vulnFixtureReady("hello_vuln_nocet")) {
		skip("non-CET fixture matches objdump .plt addresses", "fixture not built");
	} else {
		const std::map<std::string, uint64_t> expected =
			readOracle("hello_vuln_nocet.plt.expected");
		check(!expected.empty() && resolvedSites("hello_vuln_nocet") == expected,
			"non-CET fixture matches objdump .plt addresses");
	}

	// 20. Several dangerous imports in one binary each land on their own
	// stub -- no two symbols sharing a computed address.
	if (!vulnFixtureReady("hello_vuln")) {
		skip("multiple dangerous imports resolve to distinct stubs", "fixture not built");
	} else {
		const std::map<std::string, uint64_t> sites = resolvedSites("hello_vuln");
		std::set<uint64_t> addresses;
		for (const auto& entry : sites) {
			addresses.insert(entry.second);
		}
		check(sites.size() >= 3 && addresses.size() == sites.size(),
			"multiple dangerous imports resolve to distinct stubs");
	}

	// 21. Harmless imports interleaved with dangerous ones are filtered out.
	// The fixture calls puts/printf between its strcpy/sprintf/strcat/memcpy;
	// an implementation that indexed stubs by match count instead of by
	// .rela.plt index would still pass here but fail case 18's addresses.
	if (!vulnFixtureReady("hello_vuln")) {
		skip("harmless imports excluded from results", "fixture not built");
	} else {
		const std::map<std::string, uint64_t> sites = resolvedSites("hello_vuln");
		check(sites.count("puts") == 0 && sites.count("printf") == 0,
			"harmless imports excluded from results");
	}

	// 22. Bounds invariant: each resolved address lies within the section it
	// was computed from. Catches an off-by-one stub even without an oracle.
	if (!fixtureExists("hello_vuln")) {
		skip("resolved addresses fall inside their PLT section", "fixture not built");
	} else {
		ELFIO::elfio elf;
		bool in_bounds = elf.load(fixture("hello_vuln"));
		if (in_bounds) {
			const ELFIO::section* plt = elf.sections[".plt.sec"];
			if (plt == nullptr) {
				plt = elf.sections[".plt"];
			}
			in_bounds = plt != nullptr;
			if (in_bounds) {
				const uint64_t low = plt->get_address();
				const uint64_t high = low + plt->get_size();
				for (const auto& entry : resolvedSites("hello_vuln")) {
					if (entry.second < low || entry.second >= high) {
						in_bounds = false;
					}
				}
			}
		}
		check(in_bounds, "resolved addresses fall inside their PLT section");
	}

	// 23. .plt present but .rela.plt stripped: an empty list, not a crash.
	if (!fixtureExists("hello_vuln_norelaplt")) {
		skip("binary with no .rela.plt resolves to empty", "fixture not built");
	} else {
		stackwatch::ElfReader reader(fixture("hello_vuln_norelaplt"));
		const bool loaded = reader.load();
		check(loaded && reader.findDangerousPltEntries().empty(),
			"binary with no .rela.plt resolves to empty");
	}

	if (g_skipped > 0) {
		std::printf(
			"\n%d case(s) skipped: run tools/fixtures/build-vuln-fixtures.sh\n"
			"on an x86-64 Linux host to build the hello_vuln fixtures.\n",
			g_skipped);
	}

	if (g_failures == 0) {
		std::printf("\nAll executed test cases passed.\n");
		return 0;
	}
	std::printf("\n%d test case(s) failed.\n", g_failures);
	return 1;
}
