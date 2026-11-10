# Native PC build of core/ for verification. Separate from the firmware build:
#   cmake -S host -B build-host -G Ninja && cmake --build build-host
cmake_minimum_required(VERSION 3.20)
project(tia_host CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../core)

# Adapted core
add_executable(tia_dump tia_dump.cpp ${CORE_DIR}/tiasound.cpp)
target_include_directories(tia_dump PRIVATE ${CORE_DIR})
target_compile_options(tia_dump PRIVATE -Wall -Wextra -O2)

# Unmodified MAME reference
add_executable(tia_dump_ref tia_dump.cpp reference/tiasound.cpp)
target_include_directories(tia_dump_ref PRIVATE reference)
target_compile_definitions(tia_dump_ref PRIVATE TIA_REFERENCE)
target_compile_options(tia_dump_ref PRIVATE -O2)

# Unmodified Stella audio channels (uses std::unreachable, so C++23)
add_executable(tia_dump_stella tia_dump.cpp stella/AudioChannel.cxx)
target_include_directories(tia_dump_stella PRIVATE stella)
target_compile_definitions(tia_dump_stella PRIVATE TIA_STELLA)
target_compile_options(tia_dump_stella PRIVATE -O2)
set_target_properties(tia_dump_stella PROPERTIES CXX_STANDARD 23)
