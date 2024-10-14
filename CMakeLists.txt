cmake_minimum_required(VERSION 3.19)

project(
    Josa.Visitor
    VERSION 1.0.0
    DESCRIPTION "A C++17 non-intrusive visitor for class hierarchies"
    LANGUAGES CXX)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

add_library(Visitor INTERFACE)
add_library(Josa::Visitor ALIAS Visitor)
target_compile_features(Visitor INTERFACE cxx_std_17)
target_include_directories(Visitor INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)

    option(JOSA_VISITOR_BUILD_TESTS "whether or not tests should be built" ON)

    if(JOSA_VISITOR_BUILD_TESTS)
        add_subdirectory(test)
    endif()
    
endif()
