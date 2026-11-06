cmake_minimum_required(VERSION 3.16)
project(TowerDefenseEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(TD_BUILD_EXAMPLES "Build the example games" ON)
option(TD_BUILD_TESTS "Build the unit tests" ON)
option(TD_WITH_SDL2 "Build the SDL2 graphical frontend if SDL2 is found" ON)

# --- Bibliothèque moteur ------------------------------------------------------
add_library(td_engine
    engine/src/Grid.cpp
    engine/src/Pathfinding.cpp
    engine/src/Definitions.cpp
    engine/src/World.cpp
    engine/src/Game.cpp
    engine/src/Terminal.cpp
)
add_library(td::engine ALIAS td_engine)
target_include_directories(td_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(td_engine PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(td_engine PRIVATE /W4)
else()
    target_compile_options(td_engine PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
find_package(Threads REQUIRED)
target_link_libraries(td_engine PUBLIC Threads::Threads)

# --- Frontend SDL2 (optionnel) -------------------------------------------------
if(TD_WITH_SDL2)
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        add_library(td_sdl2 backends/sdl2/SdlFrontend.cpp)
        add_library(td::sdl2 ALIAS td_sdl2)
        target_include_directories(td_sdl2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/backends/sdl2)
        if(TARGET SDL2::SDL2)
            target_link_libraries(td_sdl2 PUBLIC td_engine SDL2::SDL2)
        else()
            target_include_directories(td_sdl2 PUBLIC ${SDL2_INCLUDE_DIRS})
            target_link_libraries(td_sdl2 PUBLIC td_engine ${SDL2_LIBRARIES})
        endif()
        target_compile_definitions(td_sdl2 PUBLIC TD_HAS_SDL2=1)
        message(STATUS "SDL2 ${SDL2_VERSION} found: graphical frontend enabled")
    else()
        message(STATUS "SDL2 not found: only the terminal frontend will be built")
    endif()
endif()

# --- Exemples ------------------------------------------------------------------
if(TD_BUILD_EXAMPLES)
    add_executable(td_demo examples/demo/main.cpp)
    target_link_libraries(td_demo PRIVATE td_engine)
    if(TARGET td_sdl2)
        target_link_libraries(td_demo PRIVATE td_sdl2)
    endif()
endif()

# --- Tests ---------------------------------------------------------------------
if(TD_BUILD_TESTS)
    enable_testing()
    add_executable(td_tests tests/test_engine.cpp)
    target_link_libraries(td_tests PRIVATE td_engine)
    add_test(NAME engine COMMAND td_tests)
endif()
