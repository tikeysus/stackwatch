#include <cinttypes>
#include <cstdio>

#include "elf/elf_reader.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		std::fprintf(stderr, "usage: %s <target-binary>\n", argv[0]);
		return 1;
	}

	// TODO: wire up Tracer -> FrameAnalyzer -> OverflowReport once M1-M3
	// land. M0 stops at reporting the call sites ElfReader resolved.
	stackwatch::ElfReader reader(argv[1]);
	if (!reader.load()) {
		std::fprintf(stderr,
			"%s: not a supported target (expects a dynamically-linked ELF64 x86-64 binary)\n",
			argv[1]);
		return 1;
	}

	const std::vector<stackwatch::DangerousCallSite> sites = reader.findDangerousPltEntries();
	if (sites.empty()) {
		std::printf("%s: no dangerous libc calls imported\n", argv[1]);
		return 0;
	}

	std::printf("%s: %zu dangerous call site(s)\n", argv[1], sites.size());
	for (const stackwatch::DangerousCallSite& site : sites) {
		std::printf("  0x%016" PRIx64 "  %s\n", site.plt_address, site.function_name.c_str());
	}

	return 0;
}
