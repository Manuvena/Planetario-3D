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
target_link_libraries(tappa01 PRIVATE SFML::Graphics SFML::Window SFML::System)

#TAPPA02
add_executable(tappa02 tappa02.cpp)
target_include_directories(tappa02 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(tappa02 PRIVATE cxx_std_20)
target_link_libraries(tappa02 PRIVATE SFML::Window SFML::System)

#TAPPA 03
add_executable(tappa03 tappa03.cpp)
target_include_directories(tappa03 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(tappa03 PRIVATE cxx_std_20)
target_link_libraries(tappa03 PRIVATE SFML::Window SFML::System)

#TAPPA 04
add_executable(tappa04 tappa04.cpp)
target_include_directories(tappa04 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(tappa04 PRIVATE cxx_std_20)
target_link_libraries(tappa04 PRIVATE SFML::Window SFML::System)

#TAPPA 05
add_executable(tappa05 tappa05.cpp)
target_include_directories(tappa05 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tappa05 PRIVATE cxx_std_20)
target_link_libraries(tappa05 PRIVATE SFML::Window SFML::System)

#TAPPA 06
add_executable(tappa06 tappa06.cpp)
target_include_directories(tappa06 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tappa06 PRIVATE cxx_std_20)
target_link_libraries(tappa06 PRIVATE SFML::Window SFML::System)

#TAPPA 07
add_executable(tappa07 tappa07.cpp)
target_include_directories(tappa07 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tappa07 PRIVATE cxx_std_20)
target_link_libraries(tappa07 PRIVATE SFML::Window SFML::System)

#TAPPA 08
add_executable(tappa08 tappa08.cpp)
target_include_directories(tappa08 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tappa08 PRIVATE cxx_std_20)
target_link_libraries(tappa08 PRIVATE SFML::Window SFML::System SFML::Graphics)

#TAPPA 09
add_executable(tappa09 tappa09.cpp)
target_include_directories(tappa09 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tappa09 PRIVATE cxx_std_20)
target_link_libraries(tappa09 PRIVATE SFML::Window SFML::System SFML::Graphics)

#TAPPA 10
add_executable(tappa10 tappa10.cpp)
target_include_directories(tappa10 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tappa10 PRIVATE cxx_std_20)
target_link_libraries(tappa10 PRIVATE SFML::Window SFML::System SFML::Graphics)

#TAPPA 11
add_executable(tappa11 tappa11.cpp)
target_include_directories(tappa11 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tappa11 PRIVATE cxx_std_20)
target_link_libraries(tappa11 PRIVATE SFML::Window SFML::System SFML::Graphics)

#TAPPA 12
add_executable(tappa12 tappa12.cpp)
target_include_directories(tappa12 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tappa12 PRIVATE cxx_std_20)
target_link_libraries(tappa12 PRIVATE SFML::Window SFML::System SFML::Graphics)

