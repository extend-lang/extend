cmake_minimum_required(VERSION 3.13.4)
execute_process(COMMAND git describe --tags
  OUTPUT_VARIABLE GIT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "[0-9]+\.[0-9]+\.[0-9]+(-[0-9]+)?" RAW_VERSION "${GIT_VERSION}")
string(REPLACE "-" "." RAW_VERSION "${RAW_VERSION}")
project(extend VERSION ${RAW_VERSION} LANGUAGES CXX C)

find_package(LLVM REQUIRED CONFIG)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Release|Debug")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS_INIT "-march=native -fPIC")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -fno-exceptions -fno-rtti")

set(CMAKE_C_FLAGS_DEBUG_INIT "-O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG_INIT ${CMAKE_C_FLAGS_DEBUG_INIT})

set(CMAKE_C_FLAGS_RELEASE_INIT "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT ${CMAKE_C_FLAGS_RELEASE_INIT})

execute_process(
  COMMAND llvm-config --link-static --libs support
  OUTPUT_VARIABLE LLVM_LINKER_EXE_FLAGS
  COMMAND_ERROR_IS_FATAL ANY)
string(STRIP "${LLVM_LINKER_EXE_FLAGS}" LLVM_LINKER_EXE_FLAGS)
message(STATUS "LLVM_LINKER_EXE_FLAGS: ${LLVM_LINKER_EXE_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pthread -static ${LLVM_LINKER_EXE_FLAGS} ${CMAKE_EXE_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-O3")

# # Libs
set(INCLUDE_DIRS
  $<BUILD_INTERFACE:${extend_SOURCE_DIR}/src/libs>
)
set(THIRD_PARTY_LIBS EAAssert EABase EAStdC EAThread EASTL)
set(THIRD_PARTY_INCLUDE_DIRS)
set(LIBS)

# Tests
enable_testing()
include(CTest)
set(CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/third_party/Catch2/extras;${CMAKE_CURRENT_SOURCE_DIR}/third_party/EASTL/scripts/CMake")
include(Catch)

add_subdirectory(third_party/Catch2)
add_subdirectory(third_party/EASTL/test/packages/EAAssert)
add_subdirectory(third_party/EASTL/test/packages/EABase)
add_subdirectory(third_party/EASTL/test/packages/EAStdC)
add_subdirectory(third_party/EASTL/test/packages/EAThread)
add_subdirectory(third_party/EASTL)

foreach(LIB ${THIRD_PARTY_LIBS})
  get_target_property(LIB_INCLUDE_DIRS ${LIB} INTERFACE_INCLUDE_DIRECTORIES)
  set(THIRD_PARTY_INCLUDE_DIRS ${THIRD_PARTY_INCLUDE_DIRS} ${LIB_INCLUDE_DIRS})
endforeach(LIB)

# Enable warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
option(EXTEND_CLANG_TIDY_ENABLED ON) # Make build slower
if ("${EXTEND_CLANG_TIDY_ENABLED}" AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-header-filter=.*")
else()
  unset(CMAKE_CXX_CLANG_TIDY)
endif()

function(target_link_libraries_custom NAME)
  target_include_directories(${NAME} SYSTEM PRIVATE
    ${THIRD_PARTY_INCLUDE_DIRS})
  target_include_directories(${NAME} PUBLIC ${INCLUDE_DIRS})
  target_link_libraries(${NAME} PRIVATE ${THIRD_PARTY_LIBS})
endfunction(target_link_libraries_custom)

function(subdirlist RESULT CURDIR)
  file(GLOB CHILDREN RELATIVE ${CURDIR} "${CURDIR}/*")
  set(DIRLIST "")
  foreach(CHILD ${CHILDREN})
    if(IS_DIRECTORY ${CURDIR}/${CHILD})
      list(APPEND DIRLIST ${CHILD})
    endif()
  endforeach(CHILD)
  set(${RESULT} ${DIRLIST} PARENT_SCOPE)
endfunction(subdirlist)

# Libs
set(LIB_TESTS)
function(declare_lib NAME)
  aux_source_directory(src/libs/${NAME} SOURCES)
  set(TEST_SOURCES ${SOURCES})

  # Lib
  list(FILTER SOURCES EXCLUDE REGEX ".*\.test\.cpp")
  if (SOURCES)
    add_library(${NAME} STATIC ${SOURCES})
    target_link_libraries_custom(${NAME})
    set(LIBS ${NAME} ${LIBS} PARENT_SCOPE)
  endif(SOURCES)

  # tests
  list(FILTER TEST_SOURCES INCLUDE REGEX ".*\.test\.cpp")
  if (TEST_SOURCES)
    list(FILTER SOURCES EXCLUDE REGEX "main\.cpp")
    add_executable("test.lib.${NAME}" ${TEST_SOURCES} ${SOURCES})
    set(LIB_TESTS "test.lib.${NAME}" ${LIB_TESTS} PARENT_SCOPE)
    target_link_libraries_custom("test.lib.${NAME}")
    target_link_libraries("test.lib.${NAME}" PRIVATE Catch2::Catch2WithMain)
    catch_discover_tests("test.lib.${NAME}")
  endif(TEST_SOURCES)
endfunction(declare_lib)

subdirlist(LIB_SOURCES "${extend_SOURCE_DIR}/src/libs")
foreach(NAME ${LIB_SOURCES})
  declare_lib(${NAME})
endforeach(NAME)

foreach(TEST ${LIB_TESTS})
  target_link_libraries(${TEST} PRIVATE ${LIBS})
endforeach(TEST)

# Executables
function(declare_exe NAME PATH)
  aux_source_directory(${PATH} SOURCES)
  set(TEST_SOURCES ${SOURCES})

  # executable
  list(FILTER SOURCES EXCLUDE REGEX ".*\.test\.cpp")
  add_executable(${NAME} ${SOURCES})
  target_link_libraries_custom(${NAME})
  target_link_libraries(${NAME} PRIVATE ${LIBS})

  # tests
  list(FILTER TEST_SOURCES INCLUDE REGEX ".*\.test\.cpp")
  if (TEST_SOURCES)
    list(FILTER SOURCES EXCLUDE REGEX "main\.cpp")
    add_executable("test.exe.${NAME}" ${TEST_SOURCES} ${SOURCES})
    target_link_libraries_custom("test.exe.${NAME}")
    target_link_libraries("test.exe.${NAME}" PRIVATE ${LIBS})
    target_link_libraries("test.exe.${NAME}" PRIVATE Catch2::Catch2WithMain)
    catch_discover_tests("test.exe.${NAME}")
  endif(TEST_SOURCES)
endfunction(declare_exe)

subdirlist(EXES "${extend_SOURCE_DIR}/src/bin")
foreach(NAME ${EXES})
  declare_exe(${NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/${NAME})
  install(TARGETS ${NAME} RUNTIME DESTINATION bin)
endforeach(NAME)

# Samples
subdirlist(SAMPLES "${extend_SOURCE_DIR}/src/samples")
foreach(NAME ${SAMPLES})
  declare_exe(${NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/samples/${NAME})
endforeach(NAME)

set(CPACK_PACKAGE_VENDOR "Vladimir Liutov (vs@lutov.net)")
set(CPACK_PACKAGE_CONTACT "Vladimir Liutov (vs@lutov.net)")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
include(CPack)

option(EXTEND_BUILD_DOC "Build documentation" ON)
if(${EXTEND_BUILD_DOC})
  find_package(Doxygen REQUIRED doxygen)
  configure_file(Doxyfile.in Doxyfile @ONLY)
  add_custom_target(docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile >/dev/null
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen")
endif()
