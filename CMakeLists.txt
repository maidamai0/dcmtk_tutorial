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
# add -DCMAKE_CXX_FLAGS=/Zc:__cplusplus to cmake command line to enable c++17
# add_compile_options not work, dont know why
set(CMAKE_DEBUG_POSTFIX d)
set(DCMTK_ENABLE_STL ON)
set(DCMTK_OVERWRITE_WIN32_COMPILER_FLAGS OFF)
set(DCMTK_WIDE_CHAR_FILE_IO_FUNCTIONS ON)
set(DCMTK_WIDE_CHAR_MAIN_FUNCTION ON)
set(DCMTK_WITH_PNG ON)
set(DCMTK_WITH_TIFF ON)
set(DCMTK_WITH_OPENSSL OFF)
set(DCMTK_WITH_ZLIB ON)
set(DCMTK_WITH_ICONV ON)
set(DCMTK_WITH_XML ON)
set(DCMTK_WITH_OPENJPEG ON)
set(DCMTK_SUPPORT_LIBRARIES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/dcmtk-3.6.6-win64-support-MD-iconv-msvc-15.8)
set(WITH_LIBXMLINC ${CMAKE_CURRENT_SOURCE_DIR}/deps/dcmtk-3.6.6-win64-support-MD-iconv-msvc-15.8/libxml2-iconv-2.9.10) # bug for xml
add_subdirectory(deps/dcmtk)
include_directories(${CMAKE_INSTALL_PREFIX}/include) # install path of dcmtk

# fmt
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/fmt)

# spdlog
set(SPDLOG_FMT_EXTERNAL_HO CACHE "use header only fmtlib" STRING FORCE ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/spdlog)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/spdlog/include)

# tutorials
add_subdirectory(src)

# test
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/doctest)
add_subdirectory(test)
