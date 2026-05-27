cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

#TAPPA01
add_executable(tappa01 tappa01.cpp)
target_include_directories(tappa01 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(tappa01 PRIVATE cxx_std_20)
# Tappa 01 ha bisogno anche di Graphics per disegnare il cerchio
target_link_libraries(tappa01 PRIVATE SFML::Graphics SFML::Window SFML::System)

#TAPPA02
add_executable(tappa02 tappa02.cpp)
target_include_directories(tappa02 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(tappa02 PRIVATE cxx_std_20)
target_link_libraries(tappa02 PRIVATE SFML::Window SFML::System)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tappa02)
