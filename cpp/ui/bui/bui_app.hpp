#pragma once

#include "core/config.hpp"
#include "core/game_state.hpp"
#include "core/hst_score.hpp"
#include "core/i18n.hpp"
#include "media/asset_manager.hpp"
#include "media/audio_engine.hpp"
#include "media/board_renderer.hpp"
#include "media/gamepad_input.hpp"
#include "ui/common/ui_helpers.hpp"

#include "BUI_kit.h"

#include <array>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <string>
#include <vector>

namespace llk2::ui::bui {

class BuiApp {
public:
    BuiApp(media::AssetManager& assets,
           media::AudioEngine& audio,
           core::Config& config,
           core::I18n& i18n,
           core::HighScoreTable& scores,
           const std::filesystem::path& work_dir);
    ~BuiApp();

    BuiApp(const BuiApp&) = delete;
    BuiApp& operator=(const BuiApp&) = delete;

    bool init(bool headless_mode, std::string* err_out = nullptr);
    void shutdown();

    // Runs BUI's native event loop until the window closes.
    int run();

    // Headless render step into an offscreen BuiRaster (works with zero display server).
    bool render_headless_frame(const std::filesystem::path& screenshot_png = {});
    bool render_inspector_headless_frame(const std::filesystem::path& screenshot_png = {});

    // Simulates a left/right click, key press, or menu action inside the main context (for smoke-test coverage).
    void simulate_click(int x, int y);
    void simulate_right_click(int x, int y);
    void simulate_key(const char* keysym, int mods = 0);
    void simulate_language_menu_pick(size_t lang_index);

    // Programmatic actions (shared by menus and smoke test)
    void trigger_new_game();
    void trigger_endless_game(const core::EndlessConfig& cfg);
    void trigger_give_up();
    void set_language(const std::string& lang_code);
    void set_scale_mode(core::WindowScaleMode mode);
    void set_scale_filter(core::ScaleFilter filter);
    void set_borderless_fullscreen(bool enabled);
    void set_controller_rumble(bool enabled);
    void set_midi_backend(core::MidiBackend backend);
    void set_midi_track_enabled(int track_id, bool enabled);
    void set_blender_theme(const std::string& theme_name);
    void set_inspector_visible(bool visible);
    void set_hint_highlight(bool enabled);

    void open_endless_modal();
    void open_records_modal(int editable_rank = -1);
    void open_explain_modal();
    void open_author_modal();
    void close_active_modal();
    void flush_deferred_actions();

    core::GameState& game_state() noexcept { return game_state_; }
    const core::GameState& game_state() const noexcept { return game_state_; }
    std::string current_title() const { return current_title_; }
    std::string current_theme() const { return current_theme_; }
    bool is_modal_open() const noexcept { return active_modal_ != ModalKind::None; }
    bool is_inspector_visible() const noexcept { return show_inspector_; }

private:
    enum class MenuAction : uintptr_t {
        None = 0,
        NewGame,
        EndlessGame,
        GiveUp,
        Records,
        Exit,
        ToggleMusic,
        ToggleSound,
        Track101,
        Track102,
        Track103,
        BackendOst,
        BackendNative,
        BackendEmbedded,
        SelectSoundFont,
        Scale1x,
        Scale1_5x,
        Scale2x,
        Scale3x,
        ScaleFree,
        FilterNearest,
        FilterBilinear,
        ToggleFullscreen,
        ToggleRumble,
        ThemeDark,
        ThemeLight,
        ImportHst,
        ExportHst,
        ToggleInspector,
        ShowExplain,
        ShowAuthor,
        CursorLeft,
        CursorRight,
        CursorUp,
        CursorDown,
        CursorNextTile,
        CursorPrevTile,
        CursorConfirm,
        CursorCancel,
        LangBase = 1000
    };

    enum class ModalKind {
        None,
        GameEvent,
        EndlessSetup,
        Records,
        ExplainImage,
        AuthorImage
    };

    enum class DeferredAction {
        CloseModal,
        AcceptGameModal,
        AcceptEndlessModal,
        AcceptRecordsModal,
        OpenEndless,
        OpenExplain,
        OpenAuthor,
        OpenRecords,
        ToggleInspector,
        CloseInspector
    };

    void setup_ui_fonts();
    void apply_ui_font();
    const char* active_font_family() const noexcept;
    void ensure_explain_texture_for_lang();
    void ensure_author_texture();
    void rebuild_author_layout(int card_w, int card_h);
    void build_ui_tree();
    void build_inspector_tree();
    void destroy_inspector_window();
    void refresh_inspector_ui(bool force = false);
    void rebuild_menus();
    void update_window_title();
    void apply_window_scale();
    void upload_board_texture();
    void save_config_and_scores();

    void queue_deferred(DeferredAction action);

    void tick_game();
    void process_game_events();
    void show_next_game_modal();

    void clear_modal_content();
    void show_game_event_modal(const core::GameEvent& ev);
    void accept_game_event_modal();
    void accept_endless_modal();
    void sync_endless_modal_controls();
    void accept_records_modal();

    void on_import_hst();
    void on_export_hst();
    void on_select_soundfont();

    bool map_board_local_to_logical(float bx, float by, float& out_lx, float& out_ly) const;

    // Static C callbacks bridged into BUI
    static void on_host_emit(void* data, BKNode* node, int kind, void* ref, double a, double b);
    static void on_inspector_emit(void* data, BKNode* node, int kind, void* ref, double a, double b);
    static int on_board_event(void* data, BKNode* node, const BKCustomEvent* e);
    static void on_board_draw(void* data, BKNode* node, BuiRaster* r, const int rect[4]);
    static int on_explain_event(void* data, BKNode* node, const BKCustomEvent* e);
    static void on_explain_draw(void* data, BKNode* node, BuiRaster* r, const int rect[4]);
    static int on_author_event(void* data, BKNode* node, const BKCustomEvent* e);
    static void on_author_draw(void* data, BKNode* node, BuiRaster* r, const int rect[4]);
    static void on_window_frame(void* data, BKWindow* win);
    static void on_window_timer(void* data, BKWindow* win, int id);
    static void on_window_focus(void* data, BKWindow* win, int active);
    static int on_window_close(void* data, BKWindow* win);
    static void on_inspector_focus(void* data, BKWindow* win, int active);
    static int on_inspector_close(void* data, BKWindow* win);

    media::AssetManager& assets_;
    media::AudioEngine& audio_;
    core::Config& config_;
    core::I18n& i18n_;
    core::HighScoreTable& scores_;
    std::filesystem::path work_dir_;

    core::GameState game_state_;
    media::BoardRenderer renderer_;
    media::GamepadInput gamepad_;

    bool initialized_ = false;
    bool headless_mode_ = false;
    bool window_focused_ = true;
    bool insp_focused_ = false;
    bool cjk_font_loaded_ = false;
    bool latin_font_loaded_ = false;
    std::string current_theme_ = "dark";
    std::string current_title_;
    std::vector<std::string> menu_langs_;

    // Main window UI tree
    BKNode* root_ = nullptr;
    BKNode* menubar_ = nullptr;
    BKNode* mb_game_ = nullptr;
    BKNode* mb_audio_ = nullptr;
    BKNode* mb_settings_ = nullptr;
    BKNode* mb_about_ = nullptr;
    BKNode* board_node_ = nullptr;
    BKNode* modal_backdrop_ = nullptr;
    BKNode* modal_dialog_ = nullptr;
    BKNode* modal_ok_btn_ = nullptr;
    BKNode* modal_cancel_btn_ = nullptr;
    BKNode* modal_name_edit_ = nullptr;
    BKNode* modal_rank_name_lbl_ = nullptr;
    BKNode* modal_image_node_ = nullptr;

    // Endless Mode modal widgets
    BKNode* endless_rb_all_ = nullptr;
    BKNode* endless_rb_specific_ = nullptr;
    BKNode* endless_chk_shuffle_ = nullptr;
    BKNode* endless_chk_no_time_ = nullptr;
    BKNode* endless_chk_unlim_shuffle_ = nullptr;
    std::array<BKNode*, 7> endless_chk_levels_{};
    core::EndlessConfig endless_draft_{};

    BKContext* ctx_ = nullptr;
    BKWindow* window_ = nullptr;
    BuiRaster* headless_raster_ = nullptr;

    // Non-modal F12 Inspector window & tree
    bool show_inspector_ = false;
    bool show_hint_overlay_ = false;
    BKNode* insp_root_ = nullptr;
    BKNode* insp_lbl_mode_ = nullptr;
    BKNode* insp_lbl_stage_ = nullptr;
    BKNode* insp_lbl_tiles_ = nullptr;
    BKNode* insp_lbl_lives_score_ = nullptr;
    BKNode* insp_lbl_time_ = nullptr;
    BKNode* insp_lbl_backend_ = nullptr;
    BKNode* insp_lbl_sf2_ = nullptr;
    BKNode* insp_lbl_track_ = nullptr;
    BKNode* insp_lbl_controller_ = nullptr;
    BKNode* insp_lbl_hint_ = nullptr;
    BKNode* insp_chk_hint_ = nullptr;
    BKNode* insp_btn_close_ = nullptr;
    BKContext* insp_ctx_ = nullptr;
    BKWindow* insp_win_ = nullptr;
    BuiRaster* insp_headless_raster_ = nullptr;
    std::string last_insp_lines_[10];

    int board_tex_ = 0;
    int explain_tex_ = 0;
    std::string explain_tex_lang_;
    int author_tex_ = 0;
    common::AboutTextLayout author_layout_;
    float author_font_sz_ = 13.5f;
    float author_text_ox_ = 0.0f;
    float author_text_oy_ = 0.0f;
    std::string author_hovered_url_;
    bool board_dirty_ = true;
    int tick_timer_id_ = 0;

    ModalKind active_modal_ = ModalKind::None;
    std::deque<core::GameEvent> pending_game_modals_;
    std::vector<DeferredAction> deferred_actions_;
    bool ack_game_after_records_ = false;
    int records_editable_rank_ = -1;
};

} // namespace llk2::ui::bui
