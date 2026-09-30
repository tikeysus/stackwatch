#include "analysis/frame_analyzer.h"

#include <stdexcept>

namespace stackwatch {

std::optional<StackFrameInfo> FrameAnalyzer::analyze(const uint8_t* code, size_t code_len,
                                                       uint64_t func_start, uint64_t call_site) const {
	(void)code;
	(void)code_len;
	(void)func_start;
	(void)call_site;
	// TODO(M2): cs_disasm() over [func_start, call_site), scan for a
	// `sub rsp, N` prologue and the nearest `lea reg, [rbp-X]` / `mov [rbp-X],
	// reg` feeding the flagged call's destination argument.
	throw std::logic_error("FrameAnalyzer::analyze not implemented");
}

} // namespace stackwatch
