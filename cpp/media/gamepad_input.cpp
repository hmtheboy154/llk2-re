#include "media/gamepad_input.hpp"

#include <algorithm>
#include <cmath>

namespace llk2::media {

namespace {
constexpr int kStickDeadzone = 14745;    // ~45% of 32767
constexpr int kTriggerThreshold = 16384; // ~50% of 32767
constexpr uint64_t kRepeatDelayMs = 220;
constexpr uint64_t kRepeatIntervalMs = 80;
constexpr uint64_t kHotplugScanIntervalMs = 500;
} // namespace

GamepadInput::~GamepadInput() {
    shutdown();
}

bool GamepadInput::init() {
    if (initialized_) return true;
    if (!sdl_init_gamepad()) {
        return false;
    }
    initialized_ = true;
    refresh_connection(0, true);
    return true;
}

void GamepadInput::shutdown() {
    close_active_pad();
    reset_input_state();
    initialized_ = false;
}

void GamepadInput::close_active_pad() {
    if (pad_) {
#if defined(LLK2_USE_SDL3)
        SDL_CloseGamepad(pad_);
        pad_id_ = 0;
#else
        SDL_GameControllerClose(pad_);
#endif
        pad_ = nullptr;
    }
    active_name_.clear();
}

void GamepadInput::reset_input_state() {
    held_dc_ = 0;
    held_dr_ = 0;
    next_repeat_ms_ = 0;
    prev_confirm_ = false;
    prev_cancel_ = false;
    prev_hint_ = false;
    prev_inspector_ = false;
    prev_start_ = false;
    prev_lb_ = false;
    prev_rb_ = false;
}

void GamepadInput::refresh_connection(uint64_t now_ms, bool force) {
    if (!initialized_) return;

#if defined(LLK2_USE_SDL3)
    if (pad_ && !SDL_GamepadConnected(pad_)) {
        close_active_pad();
        reset_input_state();
        force = true;
    }
    if (pad_) return;
    if (!force && last_scan_ms_ != 0 && now_ms < last_scan_ms_ + kHotplugScanIntervalMs) {
        return;
    }
    last_scan_ms_ = now_ms;

    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (ids) {
        for (int i = 0; i < count; ++i) {
            SDL_Gamepad* opened = SDL_OpenGamepad(ids[i]);
            if (opened) {
                pad_ = opened;
                pad_id_ = ids[i];
                const char* nm = SDL_GetGamepadName(pad_);
                active_name_ = (nm && *nm) ? std::string(nm) : std::string("Gamepad");
                reset_input_state();
                break;
            }
        }
        SDL_free(ids);
    }
#else
    if (pad_ && !SDL_GameControllerGetAttached(pad_)) {
        close_active_pad();
        reset_input_state();
        force = true;
    }
    if (pad_) return;
    if (!force && last_scan_ms_ != 0 && now_ms < last_scan_ms_ + kHotplugScanIntervalMs) {
        return;
    }
    last_scan_ms_ = now_ms;

    int num = SDL_NumJoysticks();
    for (int i = 0; i < num; ++i) {
        if (!SDL_IsGameController(i)) continue;
        SDL_GameController* opened = SDL_GameControllerOpen(i);
        if (opened) {
            pad_ = opened;
            const char* nm = SDL_GameControllerName(pad_);
            active_name_ = (nm && *nm) ? std::string(nm) : std::string("Gamepad");
            reset_input_state();
            break;
        }
    }
#endif
}

GamepadActions GamepadInput::poll(uint64_t now_ms, bool window_focused) {
    GamepadActions out{};
    if (!initialized_) return out;

#if defined(LLK2_USE_SDL3)
    SDL_UpdateGamepads();
#else
    SDL_GameControllerUpdate();
#endif

    refresh_connection(now_ms, false);
    if (!pad_) return out;

    if (!window_focused) {
        reset_input_state();
        return out;
    }

#if defined(LLK2_USE_SDL3)
    const bool d_up = SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_DPAD_UP);
    const bool d_down = SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    const bool d_left = SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    const bool d_right = SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);

    const int ax_x = static_cast<int>(SDL_GetGamepadAxis(pad_, SDL_GAMEPAD_AXIS_LEFTX));
    const int ax_y = static_cast<int>(SDL_GetGamepadAxis(pad_, SDL_GAMEPAD_AXIS_LEFTY));
    const int lt = static_cast<int>(SDL_GetGamepadAxis(pad_, SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
    const int rt = static_cast<int>(SDL_GetGamepadAxis(pad_, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));

    const bool btn_confirm = SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_SOUTH) || (rt >= kTriggerThreshold);
    const bool btn_cancel = SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_EAST) || (lt >= kTriggerThreshold);
    const bool btn_hint = SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_NORTH) ||
                          SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_WEST);
    const bool btn_insp = SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_BACK);
    const bool btn_start = SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_START);
    const bool btn_lb = SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    const bool btn_rb = SDL_GetGamepadButton(pad_, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
#else
    const bool d_up = SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_DPAD_UP) != 0;
    const bool d_down = SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_DPAD_DOWN) != 0;
    const bool d_left = SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_DPAD_LEFT) != 0;
    const bool d_right = SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) != 0;

    const int ax_x = static_cast<int>(SDL_GameControllerGetAxis(pad_, SDL_CONTROLLER_AXIS_LEFTX));
    const int ax_y = static_cast<int>(SDL_GameControllerGetAxis(pad_, SDL_CONTROLLER_AXIS_LEFTY));
    const int lt = static_cast<int>(SDL_GameControllerGetAxis(pad_, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    const int rt = static_cast<int>(SDL_GameControllerGetAxis(pad_, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

    const bool btn_confirm = (SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_A) != 0) || (rt >= kTriggerThreshold);
    const bool btn_cancel = (SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_B) != 0) || (lt >= kTriggerThreshold);
    const bool btn_hint = (SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_Y) != 0) ||
                          (SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_X) != 0);
    const bool btn_insp = SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_BACK) != 0;
    const bool btn_start = SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_START) != 0;
    const bool btn_lb = SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) != 0;
    const bool btn_rb = SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) != 0;
#endif

    // Resolve single dominant cardinal direction (D-Pad takes priority over Left Stick)
    int dc = 0;
    int dr = 0;
    if (d_left != d_right) {
        dc = d_right ? 1 : -1;
    } else if (d_up != d_down) {
        dr = d_down ? 1 : -1;
    } else {
        int abs_x = std::abs(ax_x);
        int abs_y = std::abs(ax_y);
        if (abs_x >= kStickDeadzone || abs_y >= kStickDeadzone) {
            if (abs_x >= abs_y) {
                dc = (ax_x > 0) ? 1 : -1;
            } else {
                dr = (ax_y > 0) ? 1 : -1;
            }
        }
    }

    if (dc == 0 && dr == 0) {
        held_dc_ = 0;
        held_dr_ = 0;
        next_repeat_ms_ = 0;
    } else if (dc != held_dc_ || dr != held_dr_) {
        held_dc_ = dc;
        held_dr_ = dr;
        out.move_dc = dc;
        out.move_dr = dr;
        next_repeat_ms_ = now_ms + kRepeatDelayMs;
    } else if (now_ms >= next_repeat_ms_) {
        out.move_dc = dc;
        out.move_dr = dr;
        next_repeat_ms_ = now_ms + kRepeatIntervalMs;
    }

    // Edge-triggered button actions
    if (btn_confirm && !prev_confirm_) out.confirm = true;
    if (btn_cancel && !prev_cancel_) out.cancel = true;
    if (btn_hint && !prev_hint_) out.toggle_hint = true;
    if (btn_insp && !prev_inspector_) out.toggle_inspector = true;
    if (btn_start && !prev_start_) out.start_pressed = true;
    if (btn_lb && !prev_lb_) out.jump_dir = -1;
    else if (btn_rb && !prev_rb_) out.jump_dir = 1;

    prev_confirm_ = btn_confirm;
    prev_cancel_ = btn_cancel;
    prev_hint_ = btn_hint;
    prev_inspector_ = btn_insp;
    prev_start_ = btn_start;
    prev_lb_ = btn_lb;
    prev_rb_ = btn_rb;

    return out;
}

void GamepadInput::rumble(float low_freq, float high_freq, uint32_t duration_ms, bool enabled) {
    if (!enabled || !pad_ || duration_ms == 0) return;
    uint16_t lo = static_cast<uint16_t>(std::clamp(low_freq, 0.0f, 1.0f) * 65535.0f);
    uint16_t hi = static_cast<uint16_t>(std::clamp(high_freq, 0.0f, 1.0f) * 65535.0f);
#if defined(LLK2_USE_SDL3)
    SDL_RumbleGamepad(pad_, lo, hi, duration_ms);
#else
    SDL_GameControllerRumble(pad_, lo, hi, duration_ms);
#endif
}

void GamepadInput::rumble_for_sfx(int sfx_id, bool enabled) {
    if (!enabled || !pad_) return;
    switch (sfx_id) {
        case 202: // Select tile
            rumble(0.08f, 0.18f, 28, true);
            break;
        case 203: // Match success
            rumble(0.22f, 0.45f, 70, true);
            break;
        case 204: // Invalid match / timeout
            rumble(0.40f, 0.15f, 110, true);
            break;
        case 205: // Stage clear
            rumble(0.35f, 0.65f, 240, true);
            break;
        case 206: // Deadlock auto-shuffle
            rumble(0.45f, 0.35f, 160, true);
            break;
        default:
            break;
    }
}

} // namespace llk2::media
