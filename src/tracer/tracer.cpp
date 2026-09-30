#include "tracer/tracer.h"

#include <stdexcept>
#include <utility>

namespace stackwatch {

Tracer::Tracer(std::string target_path) : target_path_(std::move(target_path)) {}

void Tracer::setBreakpoints(const std::vector<DangerousCallSite>& sites) {
	breakpoints_ = sites;
}

void Tracer::run() {
	// TODO(M1): fork(); child does PTRACE_TRACEME + execv(target_path_);
	// parent waits, plants INT3 (0xCC) at each breakpoints_ address, and on
	// SIGTRAP reads rdi/rsi/rdx via PTRACE_GETREGS to build a BreakpointHit.
	throw std::logic_error("Tracer::run not implemented");
}

} // namespace stackwatch
