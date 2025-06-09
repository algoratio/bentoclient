# Testing library
FetchContent_Declare(
  catch
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4)

# Ensure Catch2 is built as a shared library
set(CATCH_BUILD_SHARED_LIBS ON CACHE BOOL "Build Catch2 as a shared library" FORCE)

FetchContent_MakeAvailable(catch)
# Adds Catch2::Catch2

file(GLOB MODULE_LIST CONFIGURE_DEPENDS "*.cpp")
# Tests need to be added as executables first
add_executable(testlib ${MODULE_LIST})

# I'm using C++17 in the test
target_compile_features(testlib PRIVATE cxx_std_17)

set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost REQUIRED COMPONENTS serialization filesystem)

# Should be linked to the main library, as well as the Catch2 testing library
target_link_libraries(testlib PRIVATE bentoclient_library 
    Boost::boost 
    Boost::serialization 
    Boost::filesystem
    databento::databento 
    fmt::fmt
    Catch2::Catch2WithMain)
# This is the directory where the test executable will be created
# If you register a test, then ctest and make test will run it.
# You can also run examples and check the output, as well.
add_test(NAME testlibtest COMMAND testlib) # Command can be a target
