cmake_minimum_required(VERSION 3.26)
project(HelloRaylib VERSION 0.0.1 LANGUAGES CXX C)

include(cmake/get_cpm.cmake)
include(cmake/compiler_check.cmake)

CPMAddPackage("gh:raysan5/raylib#5.0")
CPMAddPackage("gh:raysan5/raygui#4.0")

# Download and build GLPK from source
include(FetchContent)
FetchContent_Declare(
    glpk
    URL https://ftp.gnu.org/gnu/glpk/glpk-5.0.tar.gz
    URL_HASH SHA256=4a1013eebb50f728fc601bdd833b0b2870333c3b3e5a816eeba921d95bec6f15
)
FetchContent_MakeAvailable(glpk)

# Build GLPK library from all source files
file(GLOB_RECURSE GLPK_SOURCES "${glpk_SOURCE_DIR}/src/*.c")
list(FILTER GLPK_SOURCES EXCLUDE REGEX ".*/main\\.c$")
list(FILTER GLPK_SOURCES EXCLUDE REGEX ".*/glpsol\\.c$")

add_library(glpk STATIC ${GLPK_SOURCES})

# Add all necessary include directories for GLPK
target_include_directories(glpk PUBLIC 
    "${glpk_SOURCE_DIR}/src"
)
target_include_directories(glpk PRIVATE
    "${glpk_SOURCE_DIR}/src/amd"
    "${glpk_SOURCE_DIR}/src/api"
    "${glpk_SOURCE_DIR}/src/bflib"
    "${glpk_SOURCE_DIR}/src/colamd"
    "${glpk_SOURCE_DIR}/src/draft"
    "${glpk_SOURCE_DIR}/src/env"
    "${glpk_SOURCE_DIR}/src/intopt"
    "${glpk_SOURCE_DIR}/src/minisat"
    "${glpk_SOURCE_DIR}/src/misc"
    "${glpk_SOURCE_DIR}/src/mpl"
    "${glpk_SOURCE_DIR}/src/npp"
    "${glpk_SOURCE_DIR}/src/proxy"
    "${glpk_SOURCE_DIR}/src/simplex"
    "${glpk_SOURCE_DIR}/src/zlib"
)

# Suppress warnings from GLPK code
if(MSVC)
    target_compile_options(glpk PRIVATE /W0)
    # Define necessary macros for Windows
    target_compile_definitions(glpk PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_executable(HelloRaylib main.cpp "deck.h" "deck.cpp" "test.cpp")
target_link_libraries(HelloRaylib PUBLIC raylib glpk)