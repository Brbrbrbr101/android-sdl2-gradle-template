cmake_minimum_required(VERSION 3.22.1)

# Name your project (this matches the 'moduleName' from your old script)
project("main")

# 1. HEADER PATHS
# This tells the compiler where to find SDL.h
# Adjust the number of "../" based on your actual folder depth
include_directories(../../../../SDL2/include)

# 2. IMPORT PREBUILT SDL2
# This defines "SDL2" as a library that already exists on your disk
add_library(SDL2 SHARED IMPORTED)
set_target_properties(SDL2 PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../distribution/SDL2/lib/${ANDROID_ABI}/libSDL2.so)

# 3. COMPILE YOUR GAME
# Add your source files here. 
# IMPORTANT: SDL2 on Android requires "SDL_android_main.c" to boot properly.
add_library(main SHARED
    src/main.cpp 
    ../../../../SDL2/src/main/android/SDL_android_main.c)

# 4. LINK LIBRARIES
# This connects your 'main' library to SDL2 and the Android system logs/graphics
target_link_libraries(main
    SDL2
    android
    log
    GLESv2)
