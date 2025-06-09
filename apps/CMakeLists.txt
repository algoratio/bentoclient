find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(bentohistchains bentohistchains.cpp)
target_compile_features(bentohistchains PRIVATE cxx_std_17)
target_link_libraries(bentohistchains PRIVATE bentoclient_library fmt::fmt Boost::program_options)

