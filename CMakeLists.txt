#cmake_minimum_required(VERSION 3.26)

#project(HelloRaylib VERSION 0.0.1 LANGUAGES CXX)

#include(cmake/get_cpm.cmake)
#include(cmake/compiler_check.cmake)

#CPMAddPackage("gh:raysan5/raylib#5.0")
#CPMAddPackage("gh:raysan5/raygui#4.0")
#CPMAddPackage("gh:raysan5/rpng#1.0")

#add_executable(HelloRaylib main.cpp "deck.h" "deck.cpp" "test.cpp")
#target_link_libraries(HelloRaylib PUBLIC raylib)

cmake_minimum_required(VERSION 3.26)
project(HelloRaylib VERSION 0.0.1 LANGUAGES CXX)
include(cmake/get_cpm.cmake)
include(cmake/compiler_check.cmake)

CPMAddPackage("gh:raysan5/raylib#5.0")
CPMAddPackage("gh:raysan5/raygui#4.0")

# Add GLPK - Use relative path from project root
set(GLPK_ROOT "${CMAKE_SOURCE_DIR}/glpk-4.65")
include_directories(${GLPK_ROOT}/src)

# Choose the right library based on architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit
    link_directories(${GLPK_ROOT}/w64)
    set(GLPK_LIB "glpk_4_65")
    set(GLPK_DLL "${GLPK_ROOT}/w64/glpk_4_65.dll")
else()
    # 32-bit
    link_directories(${GLPK_ROOT}/w32)
    set(GLPK_LIB "glpk_4_65")
    set(GLPK_DLL "${GLPK_ROOT}/w32/glpk_4_65.dll")
endif()

add_executable(HelloRaylib main.cpp "deck.h" "deck.cpp" "test.cpp")
target_link_libraries(HelloRaylib PUBLIC raylib ${GLPK_LIB})

# Automatically copy GLPK DLL to output directory
add_custom_command(TARGET HelloRaylib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${GLPK_DLL}"
    $<TARGET_FILE_DIR:HelloRaylib>
)