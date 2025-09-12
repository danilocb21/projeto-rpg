cmake_minimum_required(VERSION 3.16)
project(c_tale)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)

file(GLOB SOURCES "src/*.c")
add_executable(c_tale ${SOURCES})

# add_executable(c_tale
#     main.c
# )

target_link_libraries(c_tale
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    m
)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(c_tale PRIVATE -Wall -Wextra -Wpedantic)
endif()