cmake_minimum_required(VERSION 3.13.4)
execute_process(COMMAND git describe --tags
  OUTPUT_VARIABLE GIT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "[0-9]+\.[0-9]+\.[0-9]+" RAW_VERSION "${GIT_VERSION}")
project(extend VERSION ${RAW_VERSION} LANGUAGES CXX C)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Release|Debug")

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

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pthread -static")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-O3")

# # Libs
set(INCLUDE_DIRS
  $<BUILD_INTERFACE:${extend_SOURCE_DIR}/src/libs>
)
set(LIBS)

# Enable warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

option(EXTEND_CLANG_TIDY_ENABLED ON) # Make build slower
if ("${EXTEND_CLANG_TIDY_ENABLED}" AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-header-filter=.*")
else()
  unset(CMAKE_CXX_CLANG_TIDY)
endif()

# Tests
enable_testing()
include(CTest)
set(CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/third_party/Catch2/extras")
include(Catch)

add_subdirectory(third_party/Catch2)


# function(target_link_libraries_system NAME)
#   foreach(LIB ${SYSTEM_LIBS})
#     get_target_property(lib_include_dirs ${LIB} INTERFACE_INCLUDE_DIRECTORIES)
#     target_include_directories(${NAME} SYSTEM PRIVATE ${lib_include_dirs})
#     target_link_libraries(${NAME} ${LIB})
#   endforeach(LIB)
#   target_link_libraries(${NAME} ${LIBS})
# endfunction(target_link_libraries_system)
#
# # Libraries and executables
# function(library NAME)
#   aux_source_directory(src/${NAME} SOURCES)
#   set(TEST_SOURCES ${SOURCES})
#   list(FILTER TEST_SOURCES INCLUDE REGEX ".*\.test\.cpp")
#   target_sources(tests PUBLIC ${TEST_SOURCES})
#   list(FILTER SOURCES EXCLUDE REGEX ".*\.test\.cpp")
#   if (SOURCES)
#     add_library(${NAME} STATIC ${SOURCES})
#     target_link_libraries_system(${NAME})
#     target_include_directories(${NAME} PUBLIC ${INCLUDE_DIRS})
#     set(LIBS ${NAME} ${LIBS} PARENT_SCOPE)
#   endif (SOURCES)
# endfunction()
#
# function(executable NAME)
#   aux_source_directory(src/${NAME} SOURCES)
#   set(TEST_SOURCES ${SOURCES})
#   list(FILTER TEST_SOURCES INCLUDE REGEX ".*\.test\.cpp")
#   target_sources(tests PUBLIC ${TEST_SOURCES})
#   list(FILTER SOURCES EXCLUDE REGEX ".*\.test\.cpp")
#   add_executable(${NAME} ${SOURCES})
#   target_link_libraries_system(${NAME})
#   target_include_directories(${NAME} PUBLIC ${INCLUDE_DIRS})
#   set_target_properties(${NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
#   install(TARGETS ${NAME} DESTINATION bin)
# endfunction()

function(subdirlist result curdir)
  file(GLOB children RELATIVE ${curdir} "${curdir}/*")
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist} PARENT_SCOPE)
endfunction()

# Libs
function(declare_lib NAME)
  aux_source_directory(src/libs/${NAME} SOURCES)
  set(TEST_SOURCES ${SOURCES})

  # Lib
  list(FILTER SOURCES EXCLUDE REGEX ".*\.test\.cpp")
  if (SOURCES)
    add_library(${NAME} STATIC ${SOURCES})
    target_include_directories(${NAME} PUBLIC ${INCLUDE_DIRS})
    set(LIBS ${NAME} ${LIBS} PARENT_SCOPE)
  endif()

  # tests
  list(FILTER TEST_SOURCES INCLUDE REGEX ".*\.test\.cpp")
  if (TEST_SOURCES)
    list(FILTER SOURCES EXCLUDE REGEX "main\.cpp")
    add_executable("test.lib.${NAME}" ${TEST_SOURCES} ${SOURCES})
    target_include_directories("test.lib.${NAME}" PUBLIC ${INCLUDE_DIRS})
    target_link_libraries("test.lib.${NAME}" PRIVATE Catch2::Catch2WithMain)
    catch_discover_tests("test.lib.${NAME}")
  endif()
endfunction()

subdirlist(LIB_SOURCES "${extend_SOURCE_DIR}/src/libs")
foreach(NAME ${LIB_SOURCES})
  declare_lib(${NAME})
endforeach()

# Executables
function(declare_exe NAME PATH)
  aux_source_directory(${PATH} SOURCES)
  set(TEST_SOURCES ${SOURCES})

  # executable
  list(FILTER SOURCES EXCLUDE REGEX ".*\.test\.cpp")
  add_executable(${NAME} ${SOURCES})
  target_link_libraries(${NAME} PRIVATE ${LIBS})
  target_include_directories(${NAME} PUBLIC ${INCLUDE_DIRS})
  install(TARGETS ${NAME} DESTINATION bin)

  # tests
  list(FILTER TEST_SOURCES INCLUDE REGEX ".*\.test\.cpp")
  if (TEST_SOURCES)
    list(FILTER SOURCES EXCLUDE REGEX "main\.cpp")
    add_executable("test.exe.${NAME}" ${TEST_SOURCES} ${SOURCES})
    target_include_directories("test.exe.${NAME}" PUBLIC ${INCLUDE_DIRS})
    target_link_libraries("test.exe.${NAME}" PRIVATE Catch2::Catch2WithMain ${LIBS})
    catch_discover_tests("test.exe.${NAME}")
  endif()
endfunction()

subdirlist(EXES "${extend_SOURCE_DIR}/src/exe")
foreach(NAME ${EXES})
  declare_exe(${NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/exe/${NAME})
endforeach()

# Samples
subdirlist(SAMPLES "${extend_SOURCE_DIR}/src/samples")
foreach(NAME ${SAMPLES})
  declare_exe(${NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/samples/${NAME})
endforeach()




# # Tests
# target_link_libraries_system(tests)
# target_include_directories(tests PUBLIC ${INCLUDE_DIRS})
#
# option(EXTEND_BUILD_DOC "Build documentation" ON)
# if(${EXTEND_BUILD_DOC})
#   find_package(Doxygen REQUIRED doxygen)
#   configure_file(Doxyfile.in Doxyfile @ONLY)
#   add_custom_target(docs ALL
#     COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile >/dev/null
#     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#     COMMENT "Generating API documentation with Doxygen")
# endif()
