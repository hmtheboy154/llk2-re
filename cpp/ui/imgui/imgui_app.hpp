#pragma once

#include "core/config.hpp"
#include "core/game_state.hpp"
#include "core/hst_score.hpp"
#include "core/i18n.hpp"
#include "media/asset_manager.hpp"
#include "media/audio_engine.hpp"
#include "media/board_renderer.hpp"
#include "media/sdl_compat.hpp"

#include <cstdint>
#include <deque>
#include <filesystem>
#include <mutex>
#include <string>

namespace llk2::ui::imgui {

class ImGuiApp {
public:
    ImGuiApp(media::AssetManager& assets,
             media::AudioEngine& audio,
             core::Config& config,
             core::I18n& i18n,
             core::HighScoreTable& scores,
             const std::filesystem::path& work_dir);
    ~ImGuiApp();

    ImGuiApp(const ImGuiApp&) = delete;
    ImGuiApp& operator=(const ImGuiApp&) = delete;

    bool init(bool headless_mode, std::string* err_out = nullptr);
    void shutdown();

    // Runs the interactive event loop until the user closes the window.
    int run();

    // Steps a single tick + UI frame + render (used by run() and --smoke-test).
    void render_frame();

    // Optional PNG screenshot dump of the current framebuffer (for automated visual checks).
    bool save_screenshot(const std::filesystem::path& png_path);

    // Programmatic actions (shared by menus and smoke test)
    void trigger_new_game();
    void trigger_endless_game(const core::EndlessConfig& cfg);
    void trigger_give_up();
    void set_language(const std::string& lang_code);
    void set_scale_mode(core::WindowScaleMode mode);
    void set_scale_filter(core::ScaleFilter filter);
    void set_borderless_fullscreen(bool enabled);
    void set_midi_backend(core::MidiBackend backend);
    void set_midi_track_enabled(int track_id, bool enabled);

    void open_endless_modal();
    void open_records_modal(int editable_rank = -1);
    void open_explain_modal();
    void open_author_modal();
    void set_inspector_visible(bool visible) { show_inspector_ = visible; }

    core::GameState& game_state() noexcept { return game_state_; }
    const core::GameState& game_state() const noexcept { return game_state_; }
    std::string window_title() const;

private:
    enum class FileDialogAction {
        ImportHst,
        ExportHst,
        SelectSoundFont
    };

    struct PendingFileChoice {
        FileDialogAction action;
        std::string path;
    };

    void setup_fonts();
    void setup_style();
    bool create_textures(std::string* err_out);
    void destroy_textures();
    void ensure_explain_texture_for_lang();
    void upload_board_texture();
    void update_texture_filter();
    void apply_window_scale();
    void save_config_and_scores();

    void handle_sdl_event(const SDL_Event& ev, bool& want_quit);
    void process_pending_file_choices();
    void tick_game();
    void process_game_events();

    bool map_window_to_logical(float wx, float wy, float& out_lx, float& out_ly) const;
    void compute_viewport_pixels(int& out_x, int& out_y, int& out_w, int& out_h,
                                 float& out_scale_x, float& out_scale_y) const;

    void draw_menu_bar();
    void draw_modals();
    void draw_endless_modal();
    void draw_records_modal();
    void draw_image_modal(const char* popup_id, const std::string& title,
                          SDL_Texture* tex, int tex_w, int tex_h, bool& open_flag,
                          bool apply_explain_overlay = false);
    void draw_about_modal();
    void draw_game_event_modal();
    void draw_path_fallback_modal();
    void draw_inspector();
    void draw_hint_overlay();

    void request_file_dialog(FileDialogAction action);

#if defined(LLK2_USE_SDL3)
    static void SDLCALL on_sdl3_file_dialog(void* userdata, const char* const* filelist, int filter);
#endif

    media::AssetManager& assets_;
    media::AudioEngine& audio_;
    core::Config& config_;
    core::I18n& i18n_;
    core::HighScoreTable& scores_;
    std::filesystem::path work_dir_;

    core::GameState game_state_;
    media::BoardRenderer renderer_;

    SDL_Window* window_ = nullptr;
    SDL_Renderer* sdl_renderer_ = nullptr;
    SDL_Texture* board_tex_ = nullptr;
    SDL_Texture* explain_tex_ = nullptr;
    int explain_tex_w_ = 0;
    int explain_tex_h_ = 0;
    std::string explain_tex_lang_;
    SDL_Texture* author_tex_ = nullptr;
    bool initialized_ = false;
    bool headless_mode_ = false;
    bool board_dirty_ = true;
    bool request_quit_ = false;

    float menu_bar_h_ = 24.0f;
    bool scale_applied_with_exact_menu_h_ = false;

    // Modal state
    std::deque<core::GameEvent> pending_modals_;
    bool game_modal_open_pending_ = false;
    bool ack_game_after_records_ = false;

    bool show_records_modal_ = false;
    bool records_open_pending_ = false;
    int records_editable_rank_ = -1;
    char records_name_buf_[96] = {};

    bool show_explain_modal_ = false;
    bool explain_open_pending_ = false;

    bool show_author_modal_ = false;
    bool author_open_pending_ = false;

    bool show_endless_modal_ = false;
    bool endless_open_pending_ = false;
    core::EndlessConfig endless_draft_{};

    // Optional SDL2/fallback path modal
    bool show_path_modal_ = false;
    bool path_modal_open_pending_ = false;
    FileDialogAction path_modal_action_ = FileDialogAction::ImportHst;
    char path_modal_buf_[512] = {};

    // Optional F12 Inspector / Hint assistant
    bool show_inspector_ = false;
    bool show_hint_overlay_ = false;

    // Async SDL3 file dialog handoff queue
    std::mutex file_choice_mutex_;
    std::deque<PendingFileChoice> file_choices_;
};

} // namespace llk2::ui::imgui
