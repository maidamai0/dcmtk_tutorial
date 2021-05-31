cmake_minimum_required(VERSION 3.16)

get_filename_component(ROOT ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${ROOT} CXX)

set(CMAKE_GENERATOR Ninja)

# cpp compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/Zc:__cplusplus /EHsc) # see https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-160 for detail
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_program(GIT_CHGLOG git-chglog)
if(GIT_CHGLOG)
  message(STATUS "Found git-chglog:${GIT_CHGLOG}")
  execute_process(
    COMMAND ${GIT_CHGLOG}
    OUTPUT_FILE CHANGELOG.md
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
else()
  message(STATUS "git-chglog not found, changelog will not be updated")
endif()

enable_testing()

# dcmtk
find_package(DCMTK REQUIRED)
include_directories(${DCMTK_INCLUDE_DIRS})
link_libraries(${DCMTK_LIBRARIES})

# fmt
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/fmt)

# spdlog
set(SPDLOG_FMT_EXTERNAL_HO CACHE "use header only fmtlib" STRING FORCE ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/spdlog)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/spdlog/include)

# tutorials
add_subdirectory(src)