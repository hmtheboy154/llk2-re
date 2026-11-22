cmake_minimum_required(VERSION 3.21)
project(llk2_remastered VERSION 2.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "LLK2: Setting default build type to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Build Options ────────────────────────────────────────────────────────────
set(LLK2_SDL_VERSION "AUTO" CACHE STRING "SDL version to use: AUTO, 3, or 2")
set_property(CACHE LLK2_SDL_VERSION PROPERTY STRINGS "AUTO" "3" "2")

option(LLK2_UI_QT          "Build Qt 6 native desktop executable (llk2-qt)"      ON)
option(LLK2_UI_IMGUI       "Build Dear ImGui executable (llk2-imgui)"            OFF)
option(LLK2_UI_BUI         "Build BUI zero-dependency executable (llk2-bui)"     OFF)
option(LLK2_BUILD_TESTS    "Build CTest unit test suite"                         ON)

# ── Phase 1: libllk2_core (Pure C++20 game logic, i18n, config, high scores) ─
add_library(llk2_core STATIC
    cpp/core/board.cpp
    cpp/core/hst_score.cpp
    cpp/core/config.cpp
    cpp/core/i18n.cpp
    cpp/core/game_state.cpp
)

target_include_directories(llk2_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
)

if(MSVC)
    target_compile_options(llk2_core PRIVATE /W4 /utf-8)
else()
    target_compile_options(llk2_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Phase 2: libllk2_media (SDL2/3 board compositor, asset loader, dual MIDI) ─
set(LLK2_FOUND_SDL3 FALSE)
set(LLK2_FOUND_SDL2 FALSE)

if(LLK2_SDL_VERSION STREQUAL "3")
    find_package(SDL3 REQUIRED)
    set(LLK2_FOUND_SDL3 TRUE)
elseif(LLK2_SDL_VERSION STREQUAL "2")
    find_package(SDL2 REQUIRED)
    set(LLK2_FOUND_SDL2 TRUE)
else()
    find_package(SDL3 QUIET)
    if(SDL3_FOUND)
        set(LLK2_FOUND_SDL3 TRUE)
    else()
        find_package(SDL2 REQUIRED)
        set(LLK2_FOUND_SDL2 TRUE)
    endif()
endif()

add_library(llk2_media STATIC
    cpp/media/asset_manager.cpp
    cpp/media/board_renderer.cpp
    cpp/media/midi_native.cpp
    cpp/media/audio_engine.cpp
)

target_include_directories(llk2_media PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/TinySoundFont
    ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/dr_libs
)

set(LLK2_BUNDLED_SF2 "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/GeneralUser-GS/GeneralUser-GS.sf2")
if(EXISTS "${LLK2_BUNDLED_SF2}")
    target_compile_definitions(llk2_media PRIVATE
        LLK2_BUNDLED_SOUNDFONT="${LLK2_BUNDLED_SF2}"
    )
endif()

target_link_libraries(llk2_media PUBLIC llk2_core)

if(LLK2_FOUND_SDL3)
    message(STATUS "LLK2: Using SDL3 backend")
    target_compile_definitions(llk2_media PUBLIC LLK2_USE_SDL3=1)
    target_link_libraries(llk2_media PUBLIC SDL3::SDL3)
elseif(LLK2_FOUND_SDL2)
    message(STATUS "LLK2: Using SDL2 backend")
    target_compile_definitions(llk2_media PUBLIC LLK2_USE_SDL2=1)
    if(TARGET SDL2::SDL2)
        target_link_libraries(llk2_media PUBLIC SDL2::SDL2)
    else()
        target_include_directories(llk2_media PUBLIC ${SDL2_INCLUDE_DIRS})
        target_link_libraries(llk2_media PUBLIC ${SDL2_LIBRARIES})
    endif()
endif()

find_package(Threads REQUIRED)
target_link_libraries(llk2_media PUBLIC Threads::Threads)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(ALSA QUIET)
    if(ALSA_FOUND)
        message(STATUS "LLK2: Found ALSA for Native OS MIDI Sequencer output")
        target_compile_definitions(llk2_media PUBLIC LLK2_HAVE_ALSA=1)
        target_include_directories(llk2_media PRIVATE ${ALSA_INCLUDE_DIRS})
        target_link_libraries(llk2_media PUBLIC ${ALSA_LIBRARIES})
    endif()
elseif(WIN32)
    target_link_libraries(llk2_media PUBLIC winmm)
endif()

# ── Phase 3: Qt 6 Native Desktop Frontend (llk2-qt) ──────────────────────────
if(LLK2_UI_QT)
    find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

    add_executable(llk2-qt
        cpp/ui/qt/main_qt.cpp
        cpp/ui/qt/main_window.cpp
        cpp/ui/qt/table_dialog.cpp
        cpp/ui/qt/image_dialogs.cpp
    )

    target_include_directories(llk2-qt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp
    )

    target_link_libraries(llk2-qt PRIVATE
        llk2_media
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
    )

    if(MSVC)
        target_compile_options(llk2-qt PRIVATE /W4 /utf-8)
    else()
        target_compile_options(llk2-qt PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(EXISTS "${LLK2_BUNDLED_SF2}")
        add_custom_command(TARGET llk2-qt POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LLK2_BUNDLED_SF2}"
                "$<TARGET_FILE_DIR:llk2-qt>/soundfont.sf2"
            COMMENT "Copying bundled GeneralUser-GS.sf2 next to llk2-qt"
        )
    endif()
endif()

# ── Unit Tests (CTest) ───────────────────────────────────────────────────────
if(LLK2_BUILD_TESTS)
    enable_testing()

    add_executable(test_checklink tests/test_checklink.cpp)
    target_link_libraries(test_checklink PRIVATE llk2_core)
    add_test(NAME test_checklink COMMAND test_checklink)

    add_executable(test_shifts tests/test_shifts.cpp)
    target_link_libraries(test_shifts PRIVATE llk2_core)
    add_test(NAME test_shifts COMMAND test_shifts)

    add_executable(test_scoring_hst tests/test_scoring_hst.cpp)
    target_link_libraries(test_scoring_hst PRIVATE llk2_core)
    add_test(NAME test_scoring_hst COMMAND test_scoring_hst)

    add_executable(test_media tests/test_media.cpp)
    target_link_libraries(test_media PRIVATE llk2_media)
    target_compile_definitions(test_media PRIVATE
        LLK2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    add_test(NAME test_media COMMAND test_media)

    if(LLK2_UI_QT)
        add_executable(test_qt_ui tests/test_qt_ui.cpp)
        target_compile_definitions(test_qt_ui PRIVATE
            LLK2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            LLK2_QT_BINARY="$<TARGET_FILE:llk2-qt>"
        )
        add_test(NAME test_qt_ui COMMAND test_qt_ui)
    endif()
endif()
