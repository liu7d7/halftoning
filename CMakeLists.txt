cmake_minimum_required(VERSION 3.22)
project(wip C)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(CMAKE_C_STANDARD 23)

add_subdirectory(src/lib/glfw)

add_executable(wip main.c src/lib/glad/glad.c src/lib/glad/glad.h src/lib/glad/khrplatform.h
        src/app.h
        src/gl.h
        src/err.h
        src/app.c
        src/gl.c
        src/arr.h
        src/arr.c
        src/lib/simplex/FastNoiseLite.h
        src/lib/simplex/FastNoiseLite.c
        src/map.h
        src/map.c
        src/world.h
        src/world.c
        src/reg.c
        src/reg.h
        src/obj.c
        src/obj.h
        src/obj.h
        src/gui.c
        src/gui.h
        src/font.c
        src/font.h
        src/stbtt_impl.c
        src/box.c
        src/box.h
        src/stb_truetype.h
)

find_package(assimp CONFIG REQUIRED)
target_link_libraries(wip PRIVATE glfw assimp::assimp)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
set_property(TARGET wip PROPERTY CXX_STANDARD 11)
set_target_properties(wip PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(wip PROPERTIES LINK_SEARCH_END_STATIC 1)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
find_package(Threads REQUIRED)

target_link_libraries(wip PRIVATE Threads::Threads)