cmake_minimum_required(VERSION 3.25)

# Project name and a few useful settings. Other commands can pick up the results
project(
  bentoclient
  VERSION 0.1
  DESCRIPTION "trade data from databento"
  LANGUAGES CXX)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

  # Optionally set things like CMAKE_CXX_STANDARD, CMAKE_POSITION_INDEPENDENT_CODE here

  # Let's ensure -std=c++xx instead of -std=g++xx
  set(CMAKE_CXX_EXTENSIONS OFF)
  # Helps vscode seeing header files
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  
  # Let's nicely support folders in IDEs
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # Testing only available if this is the main app
  # Note this needs to be done in the main CMakeLists
  # since it calls enable_testing, which must be in the
  # main CMakeLists.
  include(CTest)

  # Docs only available if this is the main app
  find_package(Doxygen)
  if(Doxygen_FOUND)
    add_subdirectory(docs)
  else()
    message(STATUS "Doxygen not found, not building docs")
  endif()
endif()

# FetchContent added in CMake 3.11, downloads during the configure step
# FetchContent_MakeAvailable was added in CMake 3.14; simpler usage
include(FetchContent)

# Accumulator library
# This is header only, so could be replaced with git submodules or FetchContent
find_package(Boost REQUIRED)
# Adds Boost::boost

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(DATABENTO_BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)
# Formatting library
FetchContent_Declare(
  fmtlib
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG HEAD)
FetchContent_MakeAvailable(fmtlib)
# Adds fmt::fmt
FetchContent_Declare(
  databento
  GIT_REPOSITORY https://github.com/databento/databento-cpp
  GIT_TAG HEAD)
FetchContent_MakeAvailable(databento)

# The compiled library code is here
add_subdirectory(src)

# The executable code is here
add_subdirectory(apps)

#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/getkey.sh  ${CMAKE_CURRENT_BINARY_DIR}/apps/getkey.sh)

# Testing only available if this is the main app
# Emergency override BENTOCLIENT_CMAKE_BUILD_TESTING provided as well
if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR BENTOCLIENT_CMAKE_BUILD_TESTING)
   AND BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(TARGETS bentohistchains RUNTIME DESTINATION bin)
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/scripts/getkey.sh DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts/ DESTINATION . 
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/ DESTINATION . 
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE
  DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
)

