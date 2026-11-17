cmake_minimum_required(VERSION 3.21)
project(TowerDefense CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Algorithm core (Kruskal + multiple knapsack DP): pure C++, no SFML.
# Compiled into both executables below.
set(CORE_SOURCES
    core/dataset_loader.cpp
    core/disjoint_set.cpp
    core/mst_planner.cpp
    core/knapsack_allocator.cpp
    core/pipeline.cpp
    core/supply_network.cpp
    core/algorithm_trace.cpp
)

# --- Target 1: console test runner (core only, no SFML) ---
add_executable(test_runner tests/test_runner.cpp ${CORE_SOURCES})
target_include_directories(test_runner PRIVATE core)
target_compile_options(test_runner PRIVATE -Wall -Wextra)

# Regenerates docs/test_output.txt:  cmake --build build --target test_output
add_custom_target(test_output
    COMMAND ${CMAKE_COMMAND}
            -DRUNNER=$<TARGET_FILE:test_runner>
            -DDATA_DIR=${CMAKE_SOURCE_DIR}/data
            -DOUTPUT=${CMAKE_SOURCE_DIR}/docs/test_output.txt
            -P ${CMAKE_SOURCE_DIR}/tests/save_test_output.cmake
    DEPENDS test_runner
    COMMENT "Running test_runner and saving docs/test_output.txt"
)

# --- Target 2: the game (SFML 3) ---
# Change this path (or pass -DSFML_DIR=...) if SFML is installed somewhere else.
set(SFML_DIR "C:/SFML-3.0.2/SFML-3.0.2/lib/cmake/SFML" CACHE PATH "Folder containing SFMLConfig.cmake")
find_package(SFML 3 COMPONENTS Graphics Window System Audio REQUIRED)

add_executable(tower_defense
    App/main.cpp
    App/game_manager.cpp
    App/asset_manager.cpp
    App/grid.cpp
    App/node.cpp
    App/min_heap.cpp
    App/a_star_path_finder.cpp
    App/enemy_manager.cpp
    App/Enemy.cpp
    App/tower_manager.cpp
    App/tower.cpp
    App/barrier_tower.cpp
    App/gatling_tower.cpp
    App/frost_tower.cpp
    App/artillery_tower.cpp
    App/projectile.cpp
    App/projectile_manager.cpp
    App/supply_overlay.cpp
    App/ui_draw.cpp
    App/effects.cpp
    App/sound_manager.cpp
    App/hud.cpp
    App/menus.cpp
    App/algorithm_views.cpp
    ${CORE_SOURCES}
)
target_include_directories(tower_defense PRIVATE App core)
target_link_libraries(tower_defense PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)

# Copy the SFML DLLs next to tower_defense.exe so it runs from the build folder
add_custom_command(TARGET tower_defense POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:tower_defense> $<TARGET_FILE_DIR:tower_defense>
    COMMAND_EXPAND_LISTS
)

# --- Optional target 3: A* vs Dijkstra check (uses the game's Grid, so it needs SFML) ---
#   cmake -S . -B build -DBUILD_ASTAR_CHECK=ON
#   cmake --build build --target astar_check
#   cd build; .\astar_check.exe
option(BUILD_ASTAR_CHECK "Build the A* vs Dijkstra evidence program" OFF)
if(BUILD_ASTAR_CHECK)
    add_executable(astar_check
        tests/astar_check.cpp
        App/a_star_path_finder.cpp
        App/min_heap.cpp
        App/node.cpp
        App/grid.cpp
    )
    target_include_directories(astar_check PRIVATE App)
    target_link_libraries(astar_check PRIVATE SFML::Graphics SFML::Window SFML::System)
    add_custom_command(TARGET astar_check POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:astar_check> $<TARGET_FILE_DIR:astar_check>
        COMMAND_EXPAND_LISTS
    )
endif()
