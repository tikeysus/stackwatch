#include "elf/elf_reader.h"

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

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

	// 11. Statically-linked ELF64 x86-64 currently accepted (decision pending,
	// see AGENTS.md note under M0).
	check(loadOf("hello_static") == true, "static binary currently accepted");

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

	if (g_failures == 0) {
		std::printf("\nAll test cases passed.\n");
		return 0;
	}
	std::printf("\n%d test case(s) failed.\n", g_failures);
	return 1;
}
