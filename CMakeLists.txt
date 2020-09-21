cmake_minimum_required(VERSION 3.13.4)
project(extend VERSION 1.0 LANGUAGES CXX C)

find_package(LLVM REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_DEBUG_POSTFIX -dbg)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

execute_process(
  COMMAND llvm-config --link-static --libs support
  OUTPUT_VARIABLE LLVM_LINKER_EXE_FLAGS
  COMMAND_ERROR_IS_FATAL ANY)
string(STRIP "${LLVM_LINKER_EXE_FLAGS}" LLVM_LINKER_EXE_FLAGS)
message(STATUS "LLVM_LINKER_EXE_FLAGS: ${LLVM_LINKER_EXE_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -Wl,--as-needed -pthread -static ${LLVM_LINKER_EXE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-O3")

# LLVM
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Libs
set(INCLUDE_DIRS
  $<BUILD_INTERFACE:${extend_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${extend_SOURCE_DIR}/libs/flat_hash_map>
)
set(LIBS)
set(SYSTEM_LIBS)

function(add_subdirectory_library NAME PATH)
  add_subdirectory(${PATH})
  set(SYSTEM_LIBS ${NAME} ${SYSTEM_LIBS} PARENT_SCOPE)
endfunction()

add_subdirectory_library(g3logger libs/g3log)
add_subdirectory_library(EASTL libs/EASTL)
add_subdirectory_library(lyra libs/Lyra)
add_subdirectory_library(Catch2 libs/Catch2)

# Enable warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

option(EXTEND_CLANG_TIDY_ENABLED ON) # Make build slower
if (${EXTEND_CLANG_TIDY_ENABLED} AND ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-header-filter=.*")
else()
  unset(CMAKE_CXX_CLANG_TIDY)
endif()

# Tests
enable_testing()
include(CTest)
aux_source_directory(src/tests TEST_SOURCES)
add_executable(tests ${TEST_SOURCES})
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/libs/Catch2/contrib")
include(Catch)
catch_discover_tests(tests)


function(target_link_libraries_system NAME)
  foreach(LIB ${SYSTEM_LIBS})
    get_target_property(lib_include_dirs ${LIB} INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(${NAME} SYSTEM PRIVATE ${lib_include_dirs})
    target_link_libraries(${NAME} ${LIB})
  endforeach(LIB)
  target_link_libraries(${NAME} ${LIBS})
endfunction(target_link_libraries_system)

# Libraries and executables
function(library NAME)
  aux_source_directory(src/${NAME} SOURCES)
  set(TEST_SOURCES ${SOURCES})
  list(FILTER TEST_SOURCES INCLUDE REGEX ".*\.test\.cpp")
  target_sources(tests PUBLIC ${TEST_SOURCES})
  list(FILTER SOURCES EXCLUDE REGEX ".*\.test\.cpp")
  if (SOURCES)
    add_library(${NAME} STATIC ${SOURCES})
    target_link_libraries_system(${NAME})
    target_include_directories(${NAME} PUBLIC ${INCLUDE_DIRS})
    set(LIBS ${NAME} ${LIBS} PARENT_SCOPE)
  endif (SOURCES)
endfunction()

function(executable NAME)
  aux_source_directory(src/${NAME} SOURCES)
  set(TEST_SOURCES ${SOURCES})
  list(FILTER TEST_SOURCES INCLUDE REGEX ".*\.test\.cpp")
  target_sources(tests PUBLIC ${TEST_SOURCES})
  list(FILTER SOURCES EXCLUDE REGEX ".*\.test\.cpp")
  add_executable(${NAME} ${SOURCES})
  target_link_libraries_system(${NAME})
  target_include_directories(${NAME} PUBLIC ${INCLUDE_DIRS})
  set_target_properties(${NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
  install(TARGETS ${NAME} DESTINATION bin)
endfunction()

library(io)
library(text)
library(textdata)
library(textcode)
executable(pretty)

# Tests
target_link_libraries_system(tests)
target_include_directories(tests PUBLIC ${INCLUDE_DIRS})
 
option(EXTEND_BUILD_DOC "Build documentation" ON)
if(${EXTEND_BUILD_DOC})
  find_package(Doxygen REQUIRED doxygen)
  configure_file(Doxyfile.in Doxyfile @ONLY)
  add_custom_target(docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile >/dev/null
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen")
endif()
