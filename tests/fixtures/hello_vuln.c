// Fixture for ElfReader::findDangerousPltEntries().
//
// Imports several dangerous libc functions interleaved with harmless ones
// (puts, printf), so a correct implementation must index .rela.plt by the
// relocation's own index rather than by a running count of matches.
//
// `gets` is deliberately absent: glibc dropped its declaration in C11 and
// keeps only a compat symbol, so a new program can't portably link against
// it. The other four of the dangerous set are covered.
//
// Build via tools/fixtures/build-vuln-fixtures.sh -- it passes -fno-builtin
// so these stay real PLT calls instead of being expanded inline.

#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
	char buf[64];
	char scratch[64];

	puts("stackwatch fixture");
	strcpy(buf, argc > 1 ? argv[1] : "default");
	printf("%s\n", buf);
	sprintf(scratch, "%s-%d", buf, argc);
	strcat(buf, "!");
	memcpy(scratch, buf, sizeof(scratch) - 1);
	scratch[sizeof(scratch) - 1] = '\0';
	puts(scratch);

	return 0;
}
