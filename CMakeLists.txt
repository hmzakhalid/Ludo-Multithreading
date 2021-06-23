cmake_minimum_required (VERSION 3.12)

project ("Ludo-Multithreading")
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Raylib vars - set RAYLIB_DIR to a directory with lib/ and include/ folders of Raylib
set(RAYLIB_DIR "/usr/local" CACHE PATH "Raylib install directory")
message("Raylib directory: " ${RAYLIB_DIR})

# Project config
include_directories(${RAYLIB_DIR}/include)
link_directories(${RAYLIB_DIR}/lib)

# Add list of source files here, or use `file` to generate it
set(PROJECT_SOURCES i192011_project.cpp)

add_executable (${PROJECT_NAME} ${PROJECT_SOURCES})

# Winmm and Imm32 is necessary under Windows. I haven't tested it under Linux, MacOS, or other platforms - feel free to make pull requests with properly modified CMakeLists for any platform you want.
target_link_libraries(${PROJECT_NAME} raylib Threads::Threads)
