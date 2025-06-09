# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/bentoclient/*.hpp")
#set(HEADER_LIST "${PROJECT_SOURCE_DIR}/include/bentoclient/bentoclientlib.hpp")
file(GLOB MODULE_LIST CONFIGURE_DEPENDS "*.cpp")
# Make an automatic library - will be static or dynamic based on user setting
add_library(bentoclient_library SHARED ${MODULE_LIST} ${HEADER_LIST})

# We need this directory, and users of our library will need it too
target_include_directories(bentoclient_library PUBLIC ../include)

set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost REQUIRED COMPONENTS serialization filesystem log log_setup thread)

# This depends on (header only) boost plus serialization and filesystem
target_link_libraries(bentoclient_library 
      PRIVATE 
        Boost::boost 
        Boost::serialization 
        Boost::filesystem
        Boost::log
        Boost::log_setup
        Boost::thread
        databento::databento 
        fmt::fmt)

# All users of this library will need at least C++11
target_compile_features(bentoclient_library PUBLIC cxx_std_11)
# Position-independent code for shared libraries
target_compile_options(bentoclient_library PRIVATE -fPIC) 
# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/include"
  PREFIX "Header Files"
  FILES ${HEADER_LIST})

# Install rules (optional)
install(TARGETS bentoclient_library
LIBRARY DESTINATION lib
PUBLIC_HEADER DESTINATION include
)
