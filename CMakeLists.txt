cmake_minimum_required(VERSION 3.28)

project(CMakeSFMLProject LANGUAGES C CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)

FetchContent_MakeAvailable(SFML)

find_package(OpenGL REQUIRED)

add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Risorse"
            "${CMAKE_BINARY_DIR}/Risorse"
)

set(GLAD_SOURCES "")

if(EXISTS "${CMAKE_SOURCE_DIR}/glad/src/gl.c")
    list(APPEND GLAD_SOURCES "${CMAKE_SOURCE_DIR}/glad/src/gl.c")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/glad/src/glad.c")
    list(APPEND GLAD_SOURCES "${CMAKE_SOURCE_DIR}/glad/src/glad.c")
endif()

function(configura_tappa TARGET_NAME TAPPA_DIR)
    target_include_directories(${TARGET_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/${TAPPA_DIR}"
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/glad"
        "${CMAKE_SOURCE_DIR}/glad/include"
        "${CMAKE_SOURCE_DIR}/glm"
    )

    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

    target_link_libraries(${TARGET_NAME} PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        OpenGL::GL
    )

    add_dependencies(${TARGET_NAME} copy_resources)
endfunction()

#TAPPA 01
add_executable(Tappa01 Tappa01/tappa01.cpp ${GLAD_SOURCES})
configura_tappa(Tappa01 Tappa01)

#TAPPA 02
add_executable(Tappa02 Tappa02/tappa02.cpp ${GLAD_SOURCES})
configura_tappa(Tappa02 Tappa02)

#TAPPA 03
add_executable(Tappa03 Tappa03/tappa03.cpp ${GLAD_SOURCES})
configura_tappa(Tappa03 Tappa03)

#TAPPA 04
add_executable(Tappa04 Tappa04/tappa04.cpp ${GLAD_SOURCES})
configura_tappa(Tappa04 Tappa04)

#TAPPA 05
add_executable(Tappa05 Tappa05/tappa05.cpp ${GLAD_SOURCES})
configura_tappa(Tappa05 Tappa05)

#TAPPA 06
add_executable(Tappa06 Tappa06/tappa06.cpp ${GLAD_SOURCES})
configura_tappa(Tappa06 Tappa06)

#TAPPA 07
add_executable(Tappa07 Tappa07/tappa07.cpp ${GLAD_SOURCES})
configura_tappa(Tappa07 Tappa07)

#TAPPA 08
add_executable(Tappa08 Tappa08/tappa08.cpp ${GLAD_SOURCES})
configura_tappa(Tappa08 Tappa08)

#TAPPA 09
add_executable(Tappa09 Tappa09/tappa09.cpp ${GLAD_SOURCES})
configura_tappa(Tappa09 Tappa09)

#TAPPA 10
add_executable(Tappa10 Tappa10/tappa10.cpp ${GLAD_SOURCES})
configura_tappa(Tappa10 Tappa10)

#TAPPA 11
add_executable(Tappa11 Tappa11/tappa11.cpp ${GLAD_SOURCES})
configura_tappa(Tappa11 Tappa11)

#TAPPA 12
add_executable(Tappa12 Tappa12/tappa12.cpp ${GLAD_SOURCES})
configura_tappa(Tappa12 Tappa12)