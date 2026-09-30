#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace stackwatch {

struct OverflowFinding {
	std::string function_name;
	uint64_t call_site;
	uint64_t buffer_size;
	uint64_t write_size;
	bool reaches_saved_return_address;
};

// M3/M4: aggregates findings across a run (or a batch of target binaries)
// and renders them for the validation writeup in M5.
class OverflowReport {
public:
	void addFinding(OverflowFinding finding);

	const std::vector<OverflowFinding>& findings() const;

	// Human-readable summary, one line per finding.
	std::string render() const;

private:
	std::vector<OverflowFinding> findings_;
};

} // namespace stackwatch
