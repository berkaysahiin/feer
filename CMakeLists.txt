cmake_minimum_required(VERSION 4.0)

project(feer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FEER_BUILD_TESTS "Build feer tests" OFF)

add_library(feer INTERFACE)
add_library(feer::feer ALIAS feer)

target_compile_features(feer INTERFACE cxx_std_20)

target_include_directories(
    feer
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(FEER_BUILD_TESTS)
    include(CTest)

    include(FetchContent)
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.12
    )
    FetchContent_MakeAvailable(doctest)

    file(
        GLOB_RECURSE FEER_TEST_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/src/*.cpp"
    )

    if(FEER_TEST_SOURCES STREQUAL "")
        message(FATAL_ERROR "No test sources found under tests/src")
    endif()

    add_executable(feer_tests ${FEER_TEST_SOURCES})
    target_link_libraries(feer_tests PRIVATE feer::feer doctest::doctest)

    include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
    doctest_discover_tests(feer_tests)

    if(BUILD_TESTING)
        function(add_compile_fail_test source_file)
            file(RELATIVE_PATH test_rel "${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail" "${source_file}")
            string(REPLACE "/" "_" test_name "${test_rel}")
            string(REPLACE "\\" "_" test_name "${test_name}")
            string(REPLACE ".cpp" "" test_name "${test_name}")
            set(test_name "compile_fail_${test_name}")

            add_test(
                NAME ${test_name}
                COMMAND
                    ${CMAKE_CXX_COMPILER}
                    -std=c++20
                    -I${CMAKE_CURRENT_SOURCE_DIR}/include
                    -c
                    ${source_file}
                    -o
                    ${CMAKE_CURRENT_BINARY_DIR}/${test_name}.o
            )
            set_tests_properties(${test_name} PROPERTIES WILL_FAIL TRUE)
        endfunction()

        file(
            GLOB_RECURSE FEER_COMPILE_FAIL_SOURCES
            CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/*.cpp"
        )

        foreach(compile_fail_source IN LISTS FEER_COMPILE_FAIL_SOURCES)
            add_compile_fail_test("${compile_fail_source}")
        endforeach()
    endif()
endif()
