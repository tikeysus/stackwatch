cmake_minimum_required(VERSION 3.16)
project(stackwatch CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAPSTONE REQUIRED capstone)

add_executable(stackwatch
	src/main.cpp
	src/elf/elf_reader.cpp
	src/tracer/tracer.cpp
	src/analysis/frame_analyzer.cpp
	src/report/overflow_report.cpp
)

target_include_directories(stackwatch PRIVATE
	src
	third_party/ELFIO
	${CAPSTONE_INCLUDE_DIRS}
)

target_link_libraries(stackwatch PRIVATE ${CAPSTONE_LIBRARIES})
target_compile_options(stackwatch PRIVATE -Wall -Wextra ${CAPSTONE_CFLAGS_OTHER})

enable_testing()

add_executable(elf_reader_tests
	tests/elf_reader_test.cpp
	src/elf/elf_reader.cpp
)
target_include_directories(elf_reader_tests PRIVATE
	src
	third_party/ELFIO
)
target_compile_definitions(elf_reader_tests PRIVATE
	FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
)
target_compile_options(elf_reader_tests PRIVATE -Wall -Wextra)

add_test(NAME elf_reader_tests COMMAND elf_reader_tests)
