#pragma once

#include "media/sdl_compat.hpp"

#include <cstdint>
#include <string>

namespace llk2::media {

struct GamepadActions {
    int move_dc = 0;               // -1, 0, +1 (edge + hold-repeat from D-Pad or Left Stick)
    int move_dr = 0;               // -1, 0, +1
    int jump_dir = 0;              // -1 (LB), +1 (RB), 0
    bool confirm = false;          // South (A/Cross) or RT edge
    bool cancel = false;           // East (B/Circle) or LT edge
    bool toggle_hint = false;      // North (Y/Triangle) or West (X/Square) edge
    bool toggle_inspector = false; // Back/Select edge
    bool start_pressed = false;    // Start/Menu edge

    bool any() const noexcept {
        return move_dc != 0 || move_dr != 0 || jump_dir != 0 ||
               confirm || cancel || toggle_hint || toggle_inspector || start_pressed;
    }
};

// Cross-platform SDL3 / SDL2 Gamepad manager (works cleanly inside Qt, ImGui, and BUI
// without requiring SDL_INIT_VIDEO or stealing the frontend's native event loop).
class GamepadInput {
public:
    GamepadInput() = default;
    ~GamepadInput();

    GamepadInput(const GamepadInput&) = delete;
    GamepadInput& operator=(const GamepadInput&) = delete;

    // Initializes SDL gamepad subsystem and scans for connected controllers.
    // Returns false gracefully if unavailable (e.g., headless CI).
    bool init();
    void shutdown();

    // Polls connected controller state (with hot-plug refresh and hold-repeat).
    // Pass window_focused = false when app window is unfocused to ignore background input.
    GamepadActions poll(uint64_t now_ms, bool window_focused = true);

    // Optional haptic feedback (no-op when disabled or unsupported by controller).
    void rumble(float low_freq, float high_freq, uint32_t duration_ms, bool enabled);
    void rumble_for_sfx(int sfx_id, bool enabled);

    bool initialized() const noexcept { return initialized_; }
    bool connected() const noexcept { return pad_ != nullptr; }
    const std::string& active_name() const noexcept { return active_name_; }

private:
    void refresh_connection(uint64_t now_ms, bool force);
    void close_active_pad();
    void reset_input_state();

    bool initialized_ = false;
#if defined(LLK2_USE_SDL3)
    SDL_Gamepad* pad_ = nullptr;
    SDL_JoystickID pad_id_ = 0;
#else
    SDL_GameController* pad_ = nullptr;
#endif
    std::string active_name_;
    uint64_t last_scan_ms_ = 0;

    // Directional hold-repeat state
    int held_dc_ = 0;
    int held_dr_ = 0;
    uint64_t next_repeat_ms_ = 0;

    // Previous-frame edge detection
    bool prev_confirm_ = false;
    bool prev_cancel_ = false;
    bool prev_hint_ = false;
    bool prev_inspector_ = false;
    bool prev_start_ = false;
    bool prev_lb_ = false;
    bool prev_rb_ = false;
};

} // namespace llk2::media
