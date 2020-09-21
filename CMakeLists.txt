cmake_minimum_required(VERSION 3.13.4)
project(extend VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_DEBUG_POSTFIX -dbg)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(COMMON_FLAGS "-march=native")
set(CMAKE_C_FLAGS "${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "-fno-exceptions -fno-rtti -stdlib=libc++ ${COMMON_FLAGS}")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -flto")
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -lpthread") #  --rtlib=compiler-rt --unwindlib=libunwind

# Libs
set(INCLUDE_DIRS
  $<BUILD_INTERFACE:${extend_SOURCE_DIR}/src>
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
set(EXTEND_CLANG_TIDY_ENABLED ON CACHE BOOL "Enable clang-tidy? make the build slower")
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
  add_library(${NAME} STATIC ${SOURCES})
  target_link_libraries_system(${NAME})
  target_include_directories(${NAME} PUBLIC ${INCLUDE_DIRS})
  set(LIBS ${NAME} ${LIBS} PARENT_SCOPE)
endfunction()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nodefaultlibs -static -lc++ -lc++abi -lm -lgcc -lgcc_eh -lc")
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
endfunction()

library(textdata)
library(io)
executable(pretty)

# Tests
target_link_libraries_system(tests)
target_include_directories(tests PUBLIC ${INCLUDE_DIRS})
 
option(BUILD_DOC "Build documentation" ON)
if(${BUILD_DOC})
  find_package(Doxygen REQUIRED doxygen)
  configure_file(Doxyfile.in Doxyfile @ONLY)
  add_custom_target(docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen")
endif()
