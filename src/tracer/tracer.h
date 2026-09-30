#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "elf/elf_reader.h"

namespace stackwatch {

struct BreakpointHit {
	uint64_t address;
	std::string function_name;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
};

// M1: forks/execs the target under PTRACE_TRACEME, plants INT3 breakpoints
// on the given PLT call sites, and reports register state (per the System V
// x86-64 calling convention) whenever one is hit.
class Tracer {
public:
	explicit Tracer(std::string target_path);

	void setBreakpoints(const std::vector<DangerousCallSite>& sites);

	// Launches the target and runs until exit, invoking onHit for each
	// breakpoint encountered.
	void run();

private:
	std::string target_path_;
	std::vector<DangerousCallSite> breakpoints_;
};

} // namespace stackwatch
