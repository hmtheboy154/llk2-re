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

set(_LLK2_IMGUI_DEFAULT OFF)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/imgui/imgui.h")
    set(_LLK2_IMGUI_DEFAULT ON)
endif()
set(_LLK2_BUI_DEFAULT OFF)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/BUI/CMakeLists.txt")
    set(_LLK2_BUI_DEFAULT ON)
endif()

option(LLK2_UI_QT          "Build Qt 6 native desktop executable (llk2-qt)"      ON)
option(LLK2_UI_IMGUI       "Build Dear ImGui executable (llk2-imgui)"            ${_LLK2_IMGUI_DEFAULT})
option(LLK2_UI_BUI         "Build BUI native Blender-UI executable (llk2-bui)"   ${_LLK2_BUI_DEFAULT})
option(LLK2_BUILD_TESTS    "Build CTest unit test suite"                         ON)

# Optional system Fontconfig (used on Linux by llk2-imgui / llk2-bui for CJK font lookup)
set(LLK2_HAVE_FONTCONFIG FALSE)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FONTCONFIG QUIET IMPORTED_TARGET fontconfig)
        if(FONTCONFIG_FOUND)
            set(LLK2_HAVE_FONTCONFIG TRUE)
            message(STATUS "LLK2: Found fontconfig for Linux CJK system font lookup")
        endif()
    endif()
endif()

# ── Phase 1: libllk2_core (Pure C++20 game logic, i18n, config, high scores) ─
# Single-source-of-truth localization: embed locales/*.json into generated builtin_locales.hpp
file(GLOB LLK2_LOCALE_JSON_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/locales/*.json")
set(LLK2_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(LLK2_BUILTIN_LOCALES_HEADER "${LLK2_GENERATED_DIR}/builtin_locales.hpp")

add_custom_command(
    OUTPUT "${LLK2_BUILTIN_LOCALES_HEADER}"
    COMMAND ${CMAKE_COMMAND}
        -DLOCALES_DIR=${CMAKE_CURRENT_SOURCE_DIR}/locales
        -DOUTPUT_HEADER=${LLK2_BUILTIN_LOCALES_HEADER}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_builtin_locales.cmake
    DEPENDS ${LLK2_LOCALE_JSON_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_builtin_locales.cmake
    COMMENT "Embedding locales/*.json into generated/builtin_locales.hpp"
)

add_library(llk2_core STATIC
    cpp/core/app_paths.cpp
    cpp/core/board.cpp
    cpp/core/hst_score.cpp
    cpp/core/config.cpp
    cpp/core/i18n.cpp
    cpp/core/game_state.cpp
    "${LLK2_BUILTIN_LOCALES_HEADER}"
)

target_include_directories(llk2_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
)
target_include_directories(llk2_core PRIVATE
    "${LLK2_GENERATED_DIR}"
)
target_compile_definitions(llk2_core PRIVATE
    LLK2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
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

function(llk2_copy_bundled_soundfont target_name)
    if(EXISTS "${LLK2_BUNDLED_SF2}")
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LLK2_BUNDLED_SF2}"
                "$<TARGET_FILE_DIR:${target_name}>/soundfont.sf2"
            COMMENT "Copying bundled GeneralUser-GS.sf2 next to ${target_name}"
        )
    endif()
endfunction()

# ── Phase 3: Qt 6 Native Desktop Frontend (llk2-qt) ──────────────────────────
if(LLK2_UI_QT)
    find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

    add_executable(llk2-qt
        cpp/ui/qt/main_qt.cpp
        cpp/ui/qt/main_window.cpp
        cpp/ui/qt/table_dialog.cpp
        cpp/ui/qt/image_dialogs.cpp
        cpp/ui/qt/inspector_dialog.cpp
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

    llk2_copy_bundled_soundfont(llk2-qt)
endif()

# ── Phase 4a: Dear ImGui + SDL3/SDL2 Frontend (llk2-imgui) ───────────────────
if(LLK2_UI_IMGUI)
    set(LLK2_IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/imgui")
    if(NOT EXISTS "${LLK2_IMGUI_DIR}/imgui.h")
        message(FATAL_ERROR "LLK2_UI_IMGUI=ON requires subprojects/imgui submodule (run: git submodule update --init)")
    endif()

    set(LLK2_IMGUI_SRCS
        ${LLK2_IMGUI_DIR}/imgui.cpp
        ${LLK2_IMGUI_DIR}/imgui_draw.cpp
        ${LLK2_IMGUI_DIR}/imgui_tables.cpp
        ${LLK2_IMGUI_DIR}/imgui_widgets.cpp
    )
    if(LLK2_FOUND_SDL3)
        list(APPEND LLK2_IMGUI_SRCS
            ${LLK2_IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
            ${LLK2_IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
        )
    else()
        list(APPEND LLK2_IMGUI_SRCS
            ${LLK2_IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
            ${LLK2_IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
        )
    endif()

    find_package(Freetype QUIET)
    if(Freetype_FOUND)
        message(STATUS "LLK2: Enabling FreeType font rasterizer for llk2-imgui (crisp CJK OTF/TTC)")
        list(APPEND LLK2_IMGUI_SRCS
            ${LLK2_IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
        )
    endif()

    add_library(llk2_imgui_lib STATIC ${LLK2_IMGUI_SRCS})
    target_include_directories(llk2_imgui_lib PUBLIC
        ${LLK2_IMGUI_DIR}
        ${LLK2_IMGUI_DIR}/backends
    )
    target_link_libraries(llk2_imgui_lib PUBLIC llk2_media)
    if(Freetype_FOUND)
        target_compile_definitions(llk2_imgui_lib PUBLIC IMGUI_ENABLE_FREETYPE)
        target_link_libraries(llk2_imgui_lib PUBLIC Freetype::Freetype)
    endif()
    if(MSVC)
        target_compile_options(llk2_imgui_lib PRIVATE /W1)
    else()
        target_compile_options(llk2_imgui_lib PRIVATE -w)
    endif()

    add_executable(llk2-imgui
        cpp/ui/imgui/main_imgui.cpp
        cpp/ui/imgui/imgui_app.cpp
    )
    target_include_directories(llk2-imgui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp
    )
    target_link_libraries(llk2-imgui PRIVATE
        llk2_media
        llk2_imgui_lib
    )
    if(LLK2_HAVE_FONTCONFIG)
        target_compile_definitions(llk2-imgui PRIVATE LLK2_HAVE_FONTCONFIG=1)
        target_link_libraries(llk2-imgui PRIVATE PkgConfig::FONTCONFIG)
    endif()

    if(MSVC)
        target_compile_options(llk2-imgui PRIVATE /W4 /utf-8)
    else()
        target_compile_options(llk2-imgui PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    llk2_copy_bundled_soundfont(llk2-imgui)
endif()

# ── Phase 4b: BUI Native Blender-UI Frontend (llk2-bui) ──────────────────────
# Note: BUI is GPL-3.0-or-later and intentionally isolated to this separate target.
if(LLK2_UI_BUI)
    set(LLK2_BUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/BUI")
    if(NOT EXISTS "${LLK2_BUI_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "LLK2_UI_BUI=ON requires subprojects/BUI submodule (run: git submodule update --init)")
    endif()

    set(BUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUI_BUILD_PYTHON_BINDINGS OFF CACHE BOOL "" FORCE)
    set(BUI_BUILD_LUA_BINDINGS OFF CACHE BOOL "" FORCE)
    set(BUI_BUILD_QT_BACKEND OFF CACHE BOOL "" FORCE)
    set(BUI_BUILD_IMGUI OFF CACHE BOOL "" FORCE)
    set(BUI_VIDEO ON CACHE BOOL "" FORCE)
    set(BUI_GPU ON CACHE BOOL "" FORCE)
    set(BUI_FILEBROWSER ON CACHE BOOL "" FORCE)

    add_subdirectory(${LLK2_BUI_DIR} EXCLUDE_FROM_ALL)

    add_executable(llk2-bui
        cpp/ui/bui/main_bui.cpp
        cpp/ui/bui/bui_app.cpp
    )
    target_include_directories(llk2-bui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp
    )
    target_link_libraries(llk2-bui PRIVATE
        llk2_media
        bui::kit
    )
    if(LLK2_HAVE_FONTCONFIG)
        target_compile_definitions(llk2-bui PRIVATE LLK2_HAVE_FONTCONFIG=1)
        target_link_libraries(llk2-bui PRIVATE PkgConfig::FONTCONFIG)
    endif()

    if(MSVC)
        target_compile_options(llk2-bui PRIVATE /W4 /utf-8)
    else()
        target_compile_options(llk2-bui PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    llk2_copy_bundled_soundfont(llk2-bui)
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

    if(LLK2_UI_IMGUI)
        add_executable(test_imgui_ui tests/test_imgui_ui.cpp)
        target_compile_definitions(test_imgui_ui PRIVATE
            LLK2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            LLK2_IMGUI_BINARY="$<TARGET_FILE:llk2-imgui>"
        )
        add_test(NAME test_imgui_ui COMMAND test_imgui_ui)
    endif()

    if(LLK2_UI_BUI)
        add_executable(test_bui_ui tests/test_bui_ui.cpp)
        target_compile_definitions(test_bui_ui PRIVATE
            LLK2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            LLK2_BUI_BINARY="$<TARGET_FILE:llk2-bui>"
        )
        add_test(NAME test_bui_ui COMMAND test_bui_ui)
    endif()
endif()
