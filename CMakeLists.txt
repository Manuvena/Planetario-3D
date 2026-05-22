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

# --- TAPPA 01: Setup Finestra ---
add_executable(Planetario3D main.cpp)
target_compile_features(Planetario3D PRIVATE cxx_std_20)
target_link_libraries(Planetario3D PRIVATE SFML::Graphics SFML::Window SFML::System)

# --- TAPPA 02: Inizializzazione OpenGL e GLAD ---
add_executable(Tappa02 Cartella-Tappa02/main.cpp)
target_include_directories(Tappa02 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(Tappa02 PRIVATE cxx_std_20)
target_link_libraries(Tappa02 PRIVATE SFML::Window SFML::System)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)
