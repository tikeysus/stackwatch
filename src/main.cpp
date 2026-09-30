#include <cstdio>

#include "elf/elf_reader.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		std::fprintf(stderr, "usage: %s <target-binary>\n", argv[0]);
		return 1;
	}

	// TODO: wire up ElfReader -> Tracer -> FrameAnalyzer -> OverflowReport
	// once M0 lands.
	stackwatch::ElfReader reader(argv[1]);
	reader.load();

	return 0;
}
