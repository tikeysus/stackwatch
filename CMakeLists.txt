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
