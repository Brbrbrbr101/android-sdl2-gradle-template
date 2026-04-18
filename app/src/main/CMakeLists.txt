cmake_minimum_required(VERSION 3.22.1)

project("main")

# 1. Path to SDL2 Headers
# Adjust the ../ count if your SDL2 folder is elsewhere
include_directories(../../../../SDL2/include)

# 2. Link the prebuilt SDL2 shared library
add_library(SDL2 SHARED IMPORTED)
set_target_properties(SDL2 PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../distribution/SDL2/lib/${ANDROID_ABI}/libSDL2.so)

# 3. Build your game library
# Includes your code + the required SDL Android glue
add_library(main SHARED
    main.cpp 
    ../../../../SDL2/src/main/android/SDL_android_main.c)

# 4. Standard Android links
target_link_libraries(main
    SDL2
    android
    log
    GLESv2)
