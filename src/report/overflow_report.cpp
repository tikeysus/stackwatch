#include "report/overflow_report.h"

#include <stdexcept>
#include <utility>

namespace stackwatch {

void OverflowReport::addFinding(OverflowFinding finding) {
	findings_.push_back(std::move(finding));
}

const std::vector<OverflowFinding>& OverflowReport::findings() const {
	return findings_;
}

std::string OverflowReport::render() const {
	// TODO(M3/M4): format each OverflowFinding as a report line (function,
	// call site, buffer_size vs write_size, whether it reaches the saved
	// return address).
	throw std::logic_error("OverflowReport::render not implemented");
}

} // namespace stackwatch
