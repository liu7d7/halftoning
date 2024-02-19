cmake_minimum_required(VERSION 3.22)
project(game C)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(CMAKE_C_STANDARD 23)

add_subdirectory(src/lib/glfw)

add_executable(game main.c src/lib/glad/glad.c src/lib/glad/glad.h src/lib/glad/khrplatform.h
        src/game.h
        src/gl.h
        src/err.h
        src/game.c
        src/gl.c
        src/dyn_arr.h
        src/dyn_arr.c
        src/lib/simplex/FastNoiseLite.h
        src/lib/simplex/FastNoiseLite.c
        src/map.h
        src/map.c
        src/world.h
        src/world.c)

find_package(cglm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
target_link_libraries(game PRIVATE cglm::cglm glfw assimp::assimp)
