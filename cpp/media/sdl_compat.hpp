#pragma once

// Unified SDL3 / SDL2 header & helper layer
#if defined(LLK2_USE_SDL3)
#include <SDL3/SDL.h>
#elif defined(LLK2_USE_SDL2)
#include <SDL2/SDL.h>
#else
#error "Define either LLK2_USE_SDL3 or LLK2_USE_SDL2"
#endif

#include <cstdint>

namespace llk2::media {

inline bool sdl_init_audio() {
#if defined(LLK2_USE_SDL3)
    if (SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) return true;
    return SDL_InitSubSystem(SDL_INIT_AUDIO);
#else
    if (SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) return true;
    return SDL_InitSubSystem(SDL_INIT_AUDIO) == 0;
#endif
}

inline bool sdl_init_gamepad() {
#if defined(LLK2_USE_SDL3)
    if (SDL_WasInit(SDL_INIT_GAMEPAD) & SDL_INIT_GAMEPAD) return true;
    return SDL_InitSubSystem(SDL_INIT_GAMEPAD);
#else
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) & SDL_INIT_GAMECONTROLLER) return true;
    return SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0;
#endif
}

inline uint64_t sdl_ticks_ms() {
    return static_cast<uint64_t>(SDL_GetTicks());
}

inline bool sdl_open_url(const char* url) {
    if (!url || !*url) return false;
#if defined(LLK2_USE_SDL3)
    return SDL_OpenURL(url);
#else
    return SDL_OpenURL(url) == 0;
#endif
}

} // namespace llk2::media
