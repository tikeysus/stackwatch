#pragma once

#include <cstdint>
#include <optional>

namespace stackwatch {

struct StackFrameInfo {
	uint64_t frame_size;
	int64_t buffer_offset_from_rbp;
};

// M2: disassembles the caller function around a breakpoint hit (via
// Capstone) to recover the stack frame size (sub rsp, N) and the destination
// buffer's offset from rbp (lea/mov [rbp-X]).
class FrameAnalyzer {
public:
	FrameAnalyzer() = default;

	// code: raw bytes of the caller function; func_start: its virtual address;
	// call_site: address of the dangerous call within it.
	std::optional<StackFrameInfo> analyze(const uint8_t* code, size_t code_len,
	                                       uint64_t func_start, uint64_t call_site) const;
};

} // namespace stackwatch
