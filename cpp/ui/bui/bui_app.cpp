#include "ui/bui/bui_app.hpp"
#include "ui/common/ui_helpers.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace llk2::ui::bui {

namespace {

constexpr const char* kUiFontFamily = "llk2_ui";
constexpr float kUiFontPoints = 12.0f;

std::string tr_clean(const core::I18n& i18n, std::string_view key) {
    return common::strip_mnemonic(i18n.tr(key));
}

BKMenuSpec make_item(const std::string& label, uintptr_t action_tag,
                     int checked = -1, bool enabled = true,
                     const char* shortcut = nullptr) {
    BKMenuSpec s{};
    s.text = label.c_str();
    s.shortcut = shortcut;
    s.checked = checked;
    s.enabled = enabled ? 1 : 0;
    s.ref = reinterpret_cast<void*>(action_tag);
    return s;
}

BKMenuSpec make_sep() {
    BKMenuSpec s{};
    s.text = "-";
    s.checked = -1;
    s.enabled = 1;
    return s;
}

BKMenuSpec make_submenu(const std::string& label, const BKMenuSpec* sub, int nsub) {
    BKMenuSpec s{};
    s.text = label.c_str();
    s.checked = -1;
    s.enabled = 1;
    s.sub = sub;
    s.nsub = nsub;
    return s;
}

bool write_bui_raster_png(const BuiRaster* r, const std::filesystem::path& png_path) {
    if (!r) return false;
    int w = BUI_raster_width(r);
    int h = BUI_raster_height(r);
    const uint32_t* px = BUI_raster_pixels(r);
    if (!px || w <= 0 || h <= 0) return false;

    // BuiRaster stores 0x00RRGGBB; convert to packed RGBA8888 for PNG export
    std::vector<uint8_t> rgba(static_cast<size_t>(w) * h * 4);
    for (int i = 0; i < w * h; ++i) {
        uint32_t c = px[i];
        rgba[static_cast<size_t>(i) * 4 + 0] = static_cast<uint8_t>((c >> 16) & 0xFFu);
        rgba[static_cast<size_t>(i) * 4 + 1] = static_cast<uint8_t>((c >> 8) & 0xFFu);
        rgba[static_cast<size_t>(i) * 4 + 2] = static_cast<uint8_t>(c & 0xFFu);
        rgba[static_cast<size_t>(i) * 4 + 3] = 255;
    }
    return stbi_write_png(png_path.string().c_str(), w, h, 4, rgba.data(), w * 4) != 0;
}

} // namespace

BuiApp::BuiApp(media::AssetManager& assets,
               media::AudioEngine& audio,
               core::Config& config,
               core::I18n& i18n,
               core::HighScoreTable& scores,
               const std::filesystem::path& work_dir)
    : assets_(assets),
      audio_(audio),
      config_(config),
      i18n_(i18n),
      scores_(scores),
      work_dir_(work_dir),
      renderer_(assets) {
    i18n_.add_change_listener([this](const std::string& /*lang*/) {
        update_window_title();
        rebuild_menus();
    });
}

BuiApp::~BuiApp() {
    shutdown();
}

void BuiApp::setup_cjk_font() {
    auto reg = common::find_system_cjk_font(false, config_.language, /*require_index_zero=*/true);
    if (reg.valid()) {
        auto bytes = common::read_file_bytes(reg.path);
        if (!bytes.empty() &&
            BUI_text_add_font(kUiFontFamily, 0, bytes.data(), static_cast<int>(bytes.size()))) {
            cjk_font_loaded_ = true;
        }
    }

    auto bold = common::find_system_cjk_font(true, config_.language, /*require_index_zero=*/true);
    if (bold.valid()) {
        auto bytes = common::read_file_bytes(bold.path);
        if (!bytes.empty()) {
            BUI_text_add_font(kUiFontFamily, BUI_FONT_BOLD, bytes.data(), static_cast<int>(bytes.size()));
        }
    }

    apply_cjk_font();
}

void BuiApp::apply_cjk_font() {
    if (cjk_font_loaded_) {
        BK_set_font(kUiFontFamily);
    }
    BK_set_font_size(kUiFontPoints);
}

void BuiApp::build_ui_tree() {
    // Top Blender MenuBar header: 遊戲(G) - 背景聲音(B) - 設定(S) - 關於(A)
    menubar_ = BK_node_new("menubar");
    mb_game_ = BK_node_new("menubutton");
    mb_audio_ = BK_node_new("menubutton");
    mb_settings_ = BK_node_new("menubutton");
    mb_about_ = BK_node_new("menubutton");

    BK_node_add(menubar_, mb_game_, -1);
    BK_node_add(menubar_, mb_audio_, -1);
    BK_node_add(menubar_, mb_settings_, -1);
    BK_node_add(menubar_, mb_about_, -1);

    // Root container using BK_GEOM_PLACE so modal_backdrop_ overlays board_node_ cleanly
    root_ = BK_node_new("frame");
    BK_set_num(root_, "padding", 0);

    board_node_ = BK_node_new("custom");
    BKCustomClass cls{};
    cls.event = &BuiApp::on_board_event;
    cls.draw = &BuiApp::on_board_draw;
    cls.natural_w = media::BoardRenderer::WIDTH;
    cls.natural_h = media::BoardRenderer::HEIGHT;
    BK_custom_set(board_node_, &cls, this);
    BK_node_add(root_, board_node_, -1);
    BK_geom_set_manager(board_node_, BK_GEOM_PLACE);
    BK_geom_set(board_node_, "relx", 0.0);
    BK_geom_set(board_node_, "rely", 0.0);
    BK_geom_set(board_node_, "relwidth", 1.0);
    BK_geom_set(board_node_, "relheight", 1.0);

    // Semi-transparent modal overlay scrim (keeps cleared stage background visible behind modals!)
    modal_backdrop_ = BK_node_new("frame");
    float scrim[4] = {0.0f, 0.0f, 0.0f, 0.45f};
    BK_set_color(modal_backdrop_, "bg", scrim);
    BK_set_num(modal_backdrop_, "events", 1);
    BK_set_num(modal_backdrop_, "visible", 0);
    BK_node_add(root_, modal_backdrop_, -1);
    BK_geom_set_manager(modal_backdrop_, BK_GEOM_PLACE);
    BK_geom_set(modal_backdrop_, "relx", 0.0);
    BK_geom_set(modal_backdrop_, "rely", 0.0);
    BK_geom_set(modal_backdrop_, "relwidth", 1.0);
    BK_geom_set(modal_backdrop_, "relheight", 1.0);

    ctx_ = BK_context_new(root_);
    BK_context_set_header(ctx_, menubar_);

    BKHost host{};
    host.data = this;
    host.emit = &BuiApp::on_host_emit;
    BK_context_set_host(ctx_, &host);

    // Keyboard shortcuts
    BK_bind(ctx_, "F2", reinterpret_cast<void*>(static_cast<uintptr_t>(MenuAction::NewGame)));
}

void BuiApp::rebuild_menus() {
    if (!mb_game_ || !mb_audio_ || !mb_settings_ || !mb_about_) return;

    const bool on_title = game_state_.in_title_screen();
    const bool is_cn = (config_.language == "zh-CN");

    // 1. 遊戲(G)
    std::string t_game = tr_clean(i18n_, "menu.game");
    std::string s_new = tr_clean(i18n_, "menu.game.new");
    std::string s_giveup = tr_clean(i18n_, "menu.game.giveup");
    std::string s_record = tr_clean(i18n_, "menu.game.record");
    std::string s_exit = tr_clean(i18n_, "menu.game.exit");

    BK_set_str(mb_game_, "text", t_game.c_str());
    BKMenuSpec game_items[] = {
        make_item(s_new, static_cast<uintptr_t>(MenuAction::NewGame), -1, on_title, "F2"),
        make_item(s_giveup, static_cast<uintptr_t>(MenuAction::GiveUp), -1, !on_title),
        make_sep(),
        make_item(s_record, static_cast<uintptr_t>(MenuAction::Records)),
        make_sep(),
        make_item(s_exit, static_cast<uintptr_t>(MenuAction::Exit)),
    };
    BK_menubutton_set_items(mb_game_, game_items, static_cast<int>(sizeof(game_items) / sizeof(game_items[0])));

    // 2. 背景聲音(B)
    std::string t_audio = tr_clean(i18n_, "menu.audio");
    std::string s_music = tr_clean(i18n_, "menu.audio.music");
    std::string s_sound = tr_clean(i18n_, "menu.audio.sound");

    BK_set_str(mb_audio_, "text", t_audio.c_str());
    BKMenuSpec audio_items[] = {
        make_item(s_music, static_cast<uintptr_t>(MenuAction::ToggleMusic), config_.music_on ? 1 : 0),
        make_item(s_sound, static_cast<uintptr_t>(MenuAction::ToggleSound), config_.sound_on ? 1 : 0),
    };
    BK_menubutton_set_items(mb_audio_, audio_items, static_cast<int>(sizeof(audio_items) / sizeof(audio_items[0])));

    // 3. 設定(S)
    std::string t_settings = tr_clean(i18n_, "menu.settings");
    std::string s_lang = tr_clean(i18n_, "menu.settings.language");
    std::string s_tw = i18n_.tr("lang.zh_TW");
    std::string s_cn = i18n_.tr("lang.zh_CN");
    BKMenuSpec lang_sub[] = {
        make_item(s_tw, static_cast<uintptr_t>(MenuAction::LangZhTW), !is_cn ? 1 : 0),
        make_item(s_cn, static_cast<uintptr_t>(MenuAction::LangZhCN), is_cn ? 1 : 0),
    };

    std::string s_tracks = tr_clean(i18n_, "menu.settings.midi_tracks");
    std::string s_t101 = i18n_.tr("menu.settings.track_101");
    std::string s_t102 = i18n_.tr("menu.settings.track_102");
    std::string s_t103 = i18n_.tr("menu.settings.track_103");
    BKMenuSpec tracks_sub[] = {
        make_item(s_t101, static_cast<uintptr_t>(MenuAction::Track101), config_.is_track_enabled(101) ? 1 : 0),
        make_item(s_t102, static_cast<uintptr_t>(MenuAction::Track102), config_.is_track_enabled(102) ? 1 : 0),
        make_item(s_t103, static_cast<uintptr_t>(MenuAction::Track103), config_.is_track_enabled(103) ? 1 : 0),
    };

    bool has_ost = audio_.has_ost_available();
    bool is_ost = (config_.midi_backend == core::MidiBackend::OstAudio && has_ost);
    bool is_native = (config_.midi_backend == core::MidiBackend::NativeOS);
    bool is_embedded = (!is_ost && !is_native);

    std::string s_device = tr_clean(i18n_, "menu.settings.midi_device");
    std::string s_ost = i18n_.tr("menu.settings.midi_ost");
    std::string s_native = i18n_.tr("menu.settings.midi_native");
    std::string s_embedded = i18n_.tr("menu.settings.midi_embedded");
    std::string s_sf2 = i18n_.tr("menu.settings.select_soundfont");
    BKMenuSpec device_sub[] = {
        make_item(s_ost, static_cast<uintptr_t>(MenuAction::BackendOst), is_ost ? 1 : 0, has_ost),
        make_item(s_native, static_cast<uintptr_t>(MenuAction::BackendNative), is_native ? 1 : 0),
        make_item(s_embedded, static_cast<uintptr_t>(MenuAction::BackendEmbedded), is_embedded ? 1 : 0),
        make_sep(),
        make_item(s_sf2, static_cast<uintptr_t>(MenuAction::SelectSoundFont)),
    };

    std::string s_scale = tr_clean(i18n_, "menu.settings.scale");
    std::string s_1x = i18n_.tr("menu.settings.scale_1x");
    std::string s_15x = i18n_.tr("menu.settings.scale_1_5x");
    std::string s_2x = i18n_.tr("menu.settings.scale_2x");
    std::string s_3x = i18n_.tr("menu.settings.scale_3x");
    std::string s_free = i18n_.tr("menu.settings.scale_free");
    std::string s_near = i18n_.tr("menu.settings.filter_nearest");
    std::string s_lin = i18n_.tr("menu.settings.filter_bilinear");
    BKMenuSpec scale_sub[] = {
        make_item(s_1x, static_cast<uintptr_t>(MenuAction::Scale1x), config_.scale_mode == core::WindowScaleMode::Scale1x ? 1 : 0),
        make_item(s_15x, static_cast<uintptr_t>(MenuAction::Scale1_5x), config_.scale_mode == core::WindowScaleMode::Scale1_5x ? 1 : 0),
        make_item(s_2x, static_cast<uintptr_t>(MenuAction::Scale2x), config_.scale_mode == core::WindowScaleMode::Scale2x ? 1 : 0),
        make_item(s_3x, static_cast<uintptr_t>(MenuAction::Scale3x), config_.scale_mode == core::WindowScaleMode::Scale3x ? 1 : 0),
        make_item(s_free, static_cast<uintptr_t>(MenuAction::ScaleFree), config_.scale_mode == core::WindowScaleMode::FreeAspect ? 1 : 0),
        make_sep(),
        make_item(s_near, static_cast<uintptr_t>(MenuAction::FilterNearest), config_.scale_filter == core::ScaleFilter::Nearest ? 1 : 0),
        make_item(s_lin, static_cast<uintptr_t>(MenuAction::FilterBilinear), config_.scale_filter == core::ScaleFilter::Bilinear ? 1 : 0),
    };

    std::string s_theme = is_cn ? "界面主题 (Blender)" : "介面主題 (Blender)";
    std::string s_dark = is_cn ? "Blender 深色 (Dark)" : "Blender 深色 (Dark)";
    std::string s_light = is_cn ? "Blender 浅色 (Light)" : "Blender 淺色 (Light)";
    BKMenuSpec theme_sub[] = {
        make_item(s_dark, static_cast<uintptr_t>(MenuAction::ThemeDark), current_theme_ != "light" ? 1 : 0),
        make_item(s_light, static_cast<uintptr_t>(MenuAction::ThemeLight), current_theme_ == "light" ? 1 : 0),
    };

    std::string s_imp = i18n_.tr("menu.settings.import_hst");
    std::string s_exp = i18n_.tr("menu.settings.export_hst");

    BK_set_str(mb_settings_, "text", t_settings.c_str());
    BKMenuSpec settings_items[] = {
        make_submenu(s_lang, lang_sub, 2),
        make_sep(),
        make_submenu(s_tracks, tracks_sub, 3),
        make_submenu(s_device, device_sub, 5),
        make_sep(),
        make_submenu(s_scale, scale_sub, 8),
        make_submenu(s_theme, theme_sub, 2),
        make_sep(),
        make_item(s_imp, static_cast<uintptr_t>(MenuAction::ImportHst)),
        make_item(s_exp, static_cast<uintptr_t>(MenuAction::ExportHst)),
    };
    BK_menubutton_set_items(mb_settings_, settings_items, static_cast<int>(sizeof(settings_items) / sizeof(settings_items[0])));

    // 4. 關於(A)
    std::string t_about = tr_clean(i18n_, "menu.about");
    std::string s_explain = tr_clean(i18n_, "menu.about.explain");
    std::string s_author = tr_clean(i18n_, "menu.about.author");

    BK_set_str(mb_about_, "text", t_about.c_str());
    BKMenuSpec about_items[] = {
        make_item(s_explain, static_cast<uintptr_t>(MenuAction::ShowExplain)),
        make_item(s_author, static_cast<uintptr_t>(MenuAction::ShowAuthor)),
    };
    BK_menubutton_set_items(mb_about_, about_items, static_cast<int>(sizeof(about_items) / sizeof(about_items[0])));

    BK_node_relayout(menubar_);
}

void BuiApp::update_window_title() {
    current_title_ = i18n_.tr("app.title");
    if (window_) {
        BK_window_set_title(window_, current_title_.c_str());
    }
}

void BuiApp::apply_window_scale() {
    int mb_sz[2] = {0, 28};
    if (menubar_) {
        BK_node_natural(menubar_, mb_sz);
        if (mb_sz[1] <= 0) mb_sz[1] = 28;
    }
    int mb_h = mb_sz[1];

    if (window_) {
        if (config_.scale_mode == core::WindowScaleMode::FreeAspect) {
            BK_window_wm(window_, "resizable", 1, 1);
            BK_window_wm(window_, "minsize",
                         core::GameState::kCanvasWidth / 2,
                         core::GameState::kCanvasHeight / 2 + mb_h);
        } else {
            float factor = config_.scale_factor();
            int target_w = static_cast<int>(std::lround(core::GameState::kCanvasWidth * factor));
            int target_h = static_cast<int>(std::lround(core::GameState::kCanvasHeight * factor)) + mb_h;
            BK_window_wm(window_, "resizable", 0, 0);
            BK_window_wm(window_, "minsize", target_w, target_h);
            BK_window_set_size(window_, target_w, target_h);
        }
    }
    if (board_node_) {
        BK_node_invalidate(board_node_);
    }
}

void BuiApp::upload_board_texture() {
    if (!board_dirty_) return;
    const auto& surf = renderer_.canvas();
    if (!surf.valid()) return;

    if (board_tex_ == 0) {
        board_tex_ = BUI_texture_create(surf.width, surf.height, surf.pixels.data(), surf.width * 4);
    } else {
        BUI_texture_update(board_tex_, 0, 0, surf.width, surf.height, surf.pixels.data(), surf.width * 4);
    }
    board_dirty_ = false;
    if (board_node_) {
        BK_node_invalidate(board_node_);
    }
}

bool BuiApp::init(bool headless_mode, std::string* err_out) {
    if (initialized_) return true;
    headless_mode_ = headless_mode;

    BK_set_theme(current_theme_.c_str());
    setup_cjk_font();
    build_ui_tree();
    update_window_title();
    rebuild_menus();

    renderer_.render(game_state_);
    board_dirty_ = true;
    upload_board_texture();

    int mb_sz[2] = {0, 28};
    BK_node_natural(menubar_, mb_sz);
    int mb_h = (mb_sz[1] > 0) ? mb_sz[1] : 28;

    float factor = (config_.scale_mode == core::WindowScaleMode::FreeAspect) ? 1.0f : config_.scale_factor();
    int init_w = static_cast<int>(std::lround(core::GameState::kCanvasWidth * factor));
    int init_h = static_cast<int>(std::lround(core::GameState::kCanvasHeight * factor)) + mb_h;

    if (!headless_mode_ && BK_window_supported()) {
        window_ = BK_window_new(ctx_, current_title_.c_str(), init_w, init_h);
        if (!window_) {
            if (err_out) *err_out = "BK_window_new failed";
            shutdown();
            return false;
        }
        BKWindowCallbacks cb{};
        cb.data = this;
        cb.on_frame = &BuiApp::on_window_frame;
        cb.on_timer = &BuiApp::on_window_timer;
        cb.on_close = &BuiApp::on_window_close;
        BK_window_set_callbacks(window_, &cb);
        apply_window_scale();
    } else {
        headless_raster_ = BUI_raster_create(init_w, init_h);
        if (!headless_raster_) {
            if (err_out) *err_out = "BUI_raster_create failed";
            shutdown();
            return false;
        }
    }

    initialized_ = true;
    return true;
}

void BuiApp::shutdown() {
    if (initialized_) {
        save_config_and_scores();
    }

    if (window_) {
        if (tick_timer_id_ != 0) {
            BK_window_cancel_timer(window_, tick_timer_id_);
            tick_timer_id_ = 0;
        }
        BK_window_free(window_);
        window_ = nullptr;
    }
    if (headless_raster_) {
        BUI_raster_free(headless_raster_);
        headless_raster_ = nullptr;
    }
    if (board_tex_ != 0) {
        BUI_texture_free(board_tex_);
        board_tex_ = 0;
    }
    if (ctx_) {
        BK_context_free(ctx_);
        ctx_ = nullptr;
    }
    if (menubar_) {
        BK_node_free(menubar_);
        menubar_ = nullptr;
        mb_game_ = mb_audio_ = mb_settings_ = mb_about_ = nullptr;
    }
    if (root_) {
        BK_node_free(root_);
        root_ = nullptr;
        board_node_ = modal_backdrop_ = modal_dialog_ = nullptr;
        modal_ok_btn_ = modal_name_edit_ = modal_rank_name_lbl_ = modal_image_node_ = nullptr;
    }
    initialized_ = false;
}

void BuiApp::save_config_and_scores() {
    config_.save(work_dir_ / "config.json");
    scores_.save_json(work_dir_ / "scores.json");
    scores_.save_legacy_hst(work_dir_ / "d4s.hst");
}

void BuiApp::queue_deferred(DeferredAction action) {
    deferred_actions_.push_back(action);
    if (window_) {
        BK_window_request_frame(window_, 0);
    }
}

void BuiApp::flush_deferred_actions() {
    if (deferred_actions_.empty()) return;

    std::vector<DeferredAction> batch;
    batch.swap(deferred_actions_);

    for (DeferredAction act : batch) {
        switch (act) {
            case DeferredAction::CloseModal:
                close_active_modal();
                break;
            case DeferredAction::AcceptGameModal:
                accept_game_event_modal();
                break;
            case DeferredAction::AcceptRecordsModal:
                accept_records_modal();
                break;
            case DeferredAction::OpenExplain:
                open_explain_modal();
                break;
            case DeferredAction::OpenAuthor:
                open_author_modal();
                break;
            case DeferredAction::OpenRecords:
                open_records_modal(-1);
                break;
        }
    }
}

void BuiApp::trigger_new_game() {
    close_active_modal();
    game_state_.start_new_game(common::now_steady_ms());
    rebuild_menus();
    renderer_.render(game_state_);
    board_dirty_ = true;
    upload_board_texture();
}

void BuiApp::trigger_give_up() {
    close_active_modal();
    game_state_.give_up_game();
    rebuild_menus();
    renderer_.render(game_state_);
    board_dirty_ = true;
    upload_board_texture();
}

void BuiApp::set_language(const std::string& lang_code) {
    if (i18n_.set_language(lang_code)) {
        config_.language = lang_code;
        rebuild_menus();
        save_config_and_scores();
    }
}

void BuiApp::set_scale_mode(core::WindowScaleMode mode) {
    config_.scale_mode = mode;
    rebuild_menus();
    apply_window_scale();
    save_config_and_scores();
}

void BuiApp::set_scale_filter(core::ScaleFilter filter) {
    config_.scale_filter = filter;
    rebuild_menus();
    if (board_node_) {
        BK_node_invalidate(board_node_);
    }
    save_config_and_scores();
}

void BuiApp::set_midi_backend(core::MidiBackend backend) {
    config_.midi_backend = backend;
    rebuild_menus();
    audio_.update(config_);
    save_config_and_scores();
}

void BuiApp::set_midi_track_enabled(int track_id, bool enabled) {
    config_.set_track_enabled(track_id, enabled);
    rebuild_menus();
    audio_.update(config_);
    save_config_and_scores();
}

void BuiApp::set_blender_theme(const std::string& theme_name) {
    if (BK_set_theme(theme_name.c_str())) {
        current_theme_ = BK_theme_name();
        // BK_set_theme resets the font family to Inter; restore our CJK UI font immediately.
        apply_cjk_font();
        rebuild_menus();
        if (root_) {
            BK_node_relayout(root_);
            BK_node_invalidate(root_);
        }
    }
}

void BuiApp::clear_modal_content() {
    if (modal_dialog_) {
        if (modal_backdrop_) {
            BK_node_remove(modal_backdrop_, modal_dialog_);
        }
        BK_node_free(modal_dialog_);
        modal_dialog_ = nullptr;
    }
    modal_ok_btn_ = nullptr;
    modal_name_edit_ = nullptr;
    modal_rank_name_lbl_ = nullptr;
    modal_image_node_ = nullptr;
}

void BuiApp::close_active_modal() {
    clear_modal_content();
    active_modal_ = ModalKind::None;
    records_editable_rank_ = -1;
    if (modal_backdrop_) {
        BK_set_num(modal_backdrop_, "visible", 0);
        BK_node_invalidate(modal_backdrop_);
    }
    if (board_node_) {
        BK_node_invalidate(board_node_);
    }
}

void BuiApp::show_game_event_modal(const core::GameEvent& ev) {
    clear_modal_content();

    std::string title;
    std::string body;
    switch (ev.type) {
        case core::GameEventType::StageClearModal:
            title = i18n_.tr("msg.stage_clear.title");
            body = i18n_.format("msg.stage_clear.body", {
                {"level", std::to_string(ev.next_level_display)},
                {"bonus", std::to_string(ev.bonus)}
            });
            break;

        case core::GameEventType::AllStagesClearModal: {
            int total_bonus = ev.bonus + ev.life_bonus;
            int total_sc = static_cast<int>(std::lround(ev.total_score));
            title = i18n_.tr("msg.all_clear.title");
            body = i18n_.format("msg.all_clear.body", {
                {"bonus", std::to_string(total_bonus)},
                {"score", std::to_string(total_sc)}
            });
            break;
        }

        case core::GameEventType::NoMovesGameOverModal:
            title = i18n_.tr("msg.no_moves.title");
            body = i18n_.tr("msg.no_moves.body");
            break;

        case core::GameEventType::TimeoutGameOverModal: {
            int total_sc = static_cast<int>(std::lround(ev.total_score));
            title = i18n_.tr("msg.timeout.title");
            body = i18n_.format("msg.timeout.body", {
                {"bonus", std::to_string(ev.bonus)},
                {"score", std::to_string(total_sc)}
            });
            break;
        }

        case core::GameEventType::PlaySfx:
            return;
    }

    // Authentic Blender panel dialog centered over the cleared board
    modal_dialog_ = BK_node_new("frame");
    BK_set_str(modal_dialog_, "title", title.c_str());
    BK_set_num(modal_dialog_, "padding", 16);
    BK_set_num(modal_dialog_, "events", 1);

    auto lines = common::split_lines(body);
    for (const auto& line : lines) {
        BKNode* lbl = BK_node_new("label");
        BK_set_str(lbl, "text", line.c_str());
        BK_node_add(modal_dialog_, lbl, -1);
        BK_geom_set_manager(lbl, BK_GEOM_PACK);
        BK_geom_set(lbl, "side", 0);
        BK_geom_set(lbl, "fill", 1);
        BK_geom_set(lbl, "pady0", 3);
        BK_geom_set(lbl, "pady1", 3);
    }

    BKNode* sep = BK_node_new("separator");
    BK_set_num(sep, "line", 1);
    BK_set_num(sep, "factor", 0.4);
    BK_node_add(modal_dialog_, sep, -1);
    BK_geom_set_manager(sep, BK_GEOM_PACK);
    BK_geom_set(sep, "side", 0);
    BK_geom_set(sep, "fill", 1);
    BK_geom_set(sep, "pady0", 8);
    BK_geom_set(sep, "pady1", 8);

    modal_ok_btn_ = BK_node_new("button");
    BK_set_str(modal_ok_btn_, "text", i18n_.tr("table.btn.ok").c_str());
    BK_node_add(modal_dialog_, modal_ok_btn_, -1);
    BK_geom_set_manager(modal_ok_btn_, BK_GEOM_PACK);
    BK_geom_set(modal_ok_btn_, "side", 0);
    BK_geom_set(modal_ok_btn_, "reqwidth", 110);
    BK_geom_set(modal_ok_btn_, "pady0", 4);

    BK_node_add(modal_backdrop_, modal_dialog_, -1);
    BK_geom_set_manager(modal_dialog_, BK_GEOM_PLACE);
    BK_geom_set(modal_dialog_, "relx", 0.5);
    BK_geom_set(modal_dialog_, "rely", 0.5);
    BK_geom_set(modal_dialog_, "anchor", BK_ANCHOR_N | BK_ANCHOR_S | BK_ANCHOR_W | BK_ANCHOR_E);

    active_modal_ = ModalKind::GameEvent;
    BK_set_num(modal_backdrop_, "visible", 1);
    BK_node_relayout(modal_backdrop_);
    BK_node_invalidate(modal_backdrop_);
}

void BuiApp::show_next_game_modal() {
    if (pending_game_modals_.empty()) return;
    show_game_event_modal(pending_game_modals_.front());
}

void BuiApp::accept_game_event_modal() {
    if (pending_game_modals_.empty()) {
        close_active_modal();
        return;
    }

    core::GameEvent ev = pending_game_modals_.front();
    pending_game_modals_.pop_front();

    bool check_hs = (ev.type == core::GameEventType::AllStagesClearModal ||
                     ev.type == core::GameEventType::NoMovesGameOverModal ||
                     ev.type == core::GameEventType::TimeoutGameOverModal);

    if (!pending_game_modals_.empty()) {
        show_next_game_modal();
        return;
    }

    if (check_hs && scores_.is_high_score(ev.total_score)) {
        std::string def_name = config_.player_name.empty() ? i18n_.tr("table.default_player") : config_.player_name;
        int rank = scores_.insert_score(def_name, ev.total_score);
        if (rank >= 0) {
            ack_game_after_records_ = true;
            open_records_modal(rank);
            return;
        }
    }

    close_active_modal();
    game_state_.acknowledge_modal(common::now_steady_ms());
    rebuild_menus();
    renderer_.render(game_state_);
    board_dirty_ = true;
    upload_board_texture();
}

void BuiApp::open_records_modal(int editable_rank) {
    clear_modal_content();
    records_editable_rank_ = editable_rank;

    std::string init_name = config_.player_name.empty() ? i18n_.tr("table.default_player") : config_.player_name;
    if (editable_rank >= 0 && editable_rank < core::HighScoreTable::kCount) {
        scores_.update_rank_name(editable_rank, init_name);
    }

    modal_dialog_ = BK_node_new("frame");
    BK_set_str(modal_dialog_, "title", i18n_.tr("dialog.table.title").c_str());
    BK_set_num(modal_dialog_, "padding", 14);
    BK_set_num(modal_dialog_, "events", 1);

    // 3-column grid table (Rank / Name / Score)
    BKNode* grid = BK_node_new("frame");
    BK_set_num(grid, "padding", 4);
    BK_frame_grid_weight(grid, 1, 0, 0.0, 56);
    BK_frame_grid_weight(grid, 1, 1, 1.0, 160);
    BK_frame_grid_weight(grid, 1, 2, 0.0, 76);

    auto add_cell = [&](int row, int col, const std::string& text,
                        const char* align, bool bold, bool highlight) -> BKNode* {
        BKNode* lbl = BK_node_new("label");
        BK_set_str(lbl, "text", text.c_str());
        BK_set_str(lbl, "text_align", align);
        if (bold) BK_set_num(lbl, "bold", 1);
        if (highlight) {
            float red[4] = {1.0f, 0.35f, 0.35f, 1.0f};
            BK_set_color(lbl, "text_color", red);
        }
        BK_node_add(grid, lbl, -1);
        BK_geom_set_manager(lbl, BK_GEOM_GRID);
        BK_geom_set(lbl, "row", row);
        BK_geom_set(lbl, "column", col);
        BK_geom_set(lbl, "sticky", BK_ANCHOR_W | BK_ANCHOR_E);
        BK_geom_set(lbl, "padx0", 8);
        BK_geom_set(lbl, "padx1", 8);
        BK_geom_set(lbl, "pady0", 2);
        BK_geom_set(lbl, "pady1", 2);
        return lbl;
    };

    add_cell(0, 0, i18n_.tr("table.col.rank"), "center", true, false);
    add_cell(0, 1, i18n_.tr("table.col.name"), "left", true, false);
    add_cell(0, 2, i18n_.tr("table.col.score"), "right", true, false);

    BKNode* hdr_sep = BK_node_new("separator");
    BK_set_num(hdr_sep, "line", 1);
    BK_set_num(hdr_sep, "factor", 0.3);
    BK_node_add(grid, hdr_sep, -1);
    BK_geom_set_manager(hdr_sep, BK_GEOM_GRID);
    BK_geom_set(hdr_sep, "row", 1);
    BK_geom_set(hdr_sep, "column", 0);
    BK_geom_set(hdr_sep, "columnspan", 3);
    BK_geom_set(hdr_sep, "sticky", BK_ANCHOR_W | BK_ANCHOR_E);
    BK_geom_set(hdr_sep, "pady0", 3);
    BK_geom_set(hdr_sep, "pady1", 3);

    for (int i = 0; i < core::HighScoreTable::kCount; ++i) {
        const auto& e = scores_.entry(i);
        bool hl = (i == editable_rank);
        int iscore = static_cast<int>(std::lround(e.score));
        add_cell(i + 2, 0, std::to_string(i + 1), "center", hl, hl);
        BKNode* nm_lbl = add_cell(i + 2, 1, e.name, "left", hl, hl);
        add_cell(i + 2, 2, std::to_string(iscore), "right", hl, hl);
        if (hl) {
            modal_rank_name_lbl_ = nm_lbl;
        }
    }

    BK_node_add(modal_dialog_, grid, -1);
    BK_geom_set_manager(grid, BK_GEOM_PACK);
    BK_geom_set(grid, "side", 0);
    BK_geom_set(grid, "fill", 1);

    if (editable_rank >= 0 && editable_rank < core::HighScoreTable::kCount) {
        BKNode* row = BK_node_new("frame");
        BK_set_num(row, "padding", 4);

        BKNode* prompt = BK_node_new("label");
        std::string ptext = i18n_.tr("table.prompt.name") + ":";
        BK_set_str(prompt, "text", ptext.c_str());
        BK_node_add(row, prompt, -1);
        BK_geom_set_manager(prompt, BK_GEOM_PACK);
        BK_geom_set(prompt, "side", 2); // left
        BK_geom_set(prompt, "padx1", 8);

        modal_name_edit_ = BK_node_new("text");
        BK_set_str(modal_name_edit_, "value", init_name.c_str());
        BK_node_add(row, modal_name_edit_, -1);
        BK_geom_set_manager(modal_name_edit_, BK_GEOM_PACK);
        BK_geom_set(modal_name_edit_, "side", 2);
        BK_geom_set(modal_name_edit_, "fill", 1);
        BK_geom_set(modal_name_edit_, "expand", 1);
        BK_geom_set(modal_name_edit_, "reqwidth", 160);

        BK_node_add(modal_dialog_, row, -1);
        BK_geom_set_manager(row, BK_GEOM_PACK);
        BK_geom_set(row, "side", 0);
        BK_geom_set(row, "fill", 1);
        BK_geom_set(row, "pady0", 8);
    }

    modal_ok_btn_ = BK_node_new("button");
    BK_set_str(modal_ok_btn_, "text", i18n_.tr("table.btn.ok").c_str());
    BK_node_add(modal_dialog_, modal_ok_btn_, -1);
    BK_geom_set_manager(modal_ok_btn_, BK_GEOM_PACK);
    BK_geom_set(modal_ok_btn_, "side", 0);
    BK_geom_set(modal_ok_btn_, "reqwidth", 110);
    BK_geom_set(modal_ok_btn_, "pady0", 10);

    BK_node_add(modal_backdrop_, modal_dialog_, -1);
    BK_geom_set_manager(modal_dialog_, BK_GEOM_PLACE);
    BK_geom_set(modal_dialog_, "relx", 0.5);
    BK_geom_set(modal_dialog_, "rely", 0.5);
    BK_geom_set(modal_dialog_, "reqwidth", 350);
    BK_geom_set(modal_dialog_, "anchor", BK_ANCHOR_N | BK_ANCHOR_S | BK_ANCHOR_W | BK_ANCHOR_E);

    active_modal_ = ModalKind::Records;
    BK_set_num(modal_backdrop_, "visible", 1);
    BK_node_relayout(modal_backdrop_);
    BK_node_invalidate(modal_backdrop_);

    if (modal_name_edit_ && ctx_) {
        BK_focus_node(ctx_, modal_name_edit_);
    }
}

void BuiApp::accept_records_modal() {
    if (records_editable_rank_ >= 0 && records_editable_rank_ < core::HighScoreTable::kCount && modal_name_edit_) {
        const char* raw = BK_get_str(modal_name_edit_, "value");
        std::string nm = raw ? std::string(raw) : std::string();
        if (nm.empty()) nm = i18n_.tr("table.default_player");
        scores_.update_rank_name(records_editable_rank_, nm);
        config_.player_name = nm;
        save_config_and_scores();
    }

    close_active_modal();

    if (ack_game_after_records_) {
        ack_game_after_records_ = false;
        game_state_.acknowledge_modal(common::now_steady_ms());
        rebuild_menus();
        renderer_.render(game_state_);
        board_dirty_ = true;
        upload_board_texture();
    }
}

void BuiApp::open_explain_modal() {
    clear_modal_content();
    const auto& surf = assets_.form_explain();
    if (!surf.valid()) return;

    modal_dialog_ = BK_node_new("frame");
    BK_set_str(modal_dialog_, "title", i18n_.tr("dialog.explain.title").c_str());
    BK_set_num(modal_dialog_, "padding", 8);
    BK_set_num(modal_dialog_, "events", 1);

    modal_image_node_ = BK_node_new("label");
    BK_node_set_image(modal_image_node_, surf.pixels.data(), surf.width, surf.height);
    BK_set_num(modal_image_node_, "events", 1);
    BK_node_add(modal_dialog_, modal_image_node_, -1);
    BK_geom_set_manager(modal_image_node_, BK_GEOM_PACK);
    BK_geom_set(modal_image_node_, "side", 0);
    BK_geom_set(modal_image_node_, "reqwidth", surf.width);
    BK_geom_set(modal_image_node_, "reqheight", surf.height);

    BKNode* sep = BK_node_new("separator");
    BK_set_num(sep, "line", 1);
    BK_set_num(sep, "factor", 0.3);
    BK_node_add(modal_dialog_, sep, -1);
    BK_geom_set_manager(sep, BK_GEOM_PACK);
    BK_geom_set(sep, "side", 0);
    BK_geom_set(sep, "fill", 1);
    BK_geom_set(sep, "pady0", 6);
    BK_geom_set(sep, "pady1", 6);

    modal_ok_btn_ = BK_node_new("button");
    BK_set_str(modal_ok_btn_, "text", i18n_.tr("dialog.btn.close").c_str());
    BK_node_add(modal_dialog_, modal_ok_btn_, -1);
    BK_geom_set_manager(modal_ok_btn_, BK_GEOM_PACK);
    BK_geom_set(modal_ok_btn_, "side", 0);
    BK_geom_set(modal_ok_btn_, "reqwidth", 110);

    BK_node_add(modal_backdrop_, modal_dialog_, -1);
    BK_geom_set_manager(modal_dialog_, BK_GEOM_PLACE);
    BK_geom_set(modal_dialog_, "relx", 0.5);
    BK_geom_set(modal_dialog_, "rely", 0.5);
    BK_geom_set(modal_dialog_, "anchor", BK_ANCHOR_N | BK_ANCHOR_S | BK_ANCHOR_W | BK_ANCHOR_E);

    active_modal_ = ModalKind::ExplainImage;
    BK_set_num(modal_backdrop_, "visible", 1);
    BK_node_relayout(modal_backdrop_);
    BK_node_invalidate(modal_backdrop_);
}

void BuiApp::open_author_modal() {
    clear_modal_content();
    const auto& surf = assets_.form_author();
    if (!surf.valid()) return;

    modal_dialog_ = BK_node_new("frame");
    BK_set_str(modal_dialog_, "title", i18n_.tr("dialog.author.title").c_str());
    BK_set_num(modal_dialog_, "padding", 8);
    BK_set_num(modal_dialog_, "events", 1);

    modal_image_node_ = BK_node_new("label");
    BK_node_set_image(modal_image_node_, surf.pixels.data(), surf.width, surf.height);
    BK_set_num(modal_image_node_, "events", 1);
    BK_node_add(modal_dialog_, modal_image_node_, -1);
    BK_geom_set_manager(modal_image_node_, BK_GEOM_PACK);
    BK_geom_set(modal_image_node_, "side", 0);
    BK_geom_set(modal_image_node_, "reqwidth", surf.width);
    BK_geom_set(modal_image_node_, "reqheight", surf.height);

    BKNode* sep = BK_node_new("separator");
    BK_set_num(sep, "line", 1);
    BK_set_num(sep, "factor", 0.3);
    BK_node_add(modal_dialog_, sep, -1);
    BK_geom_set_manager(sep, BK_GEOM_PACK);
    BK_geom_set(sep, "side", 0);
    BK_geom_set(sep, "fill", 1);
    BK_geom_set(sep, "pady0", 6);
    BK_geom_set(sep, "pady1", 6);

    modal_ok_btn_ = BK_node_new("button");
    BK_set_str(modal_ok_btn_, "text", i18n_.tr("dialog.btn.close").c_str());
    BK_node_add(modal_dialog_, modal_ok_btn_, -1);
    BK_geom_set_manager(modal_ok_btn_, BK_GEOM_PACK);
    BK_geom_set(modal_ok_btn_, "side", 0);
    BK_geom_set(modal_ok_btn_, "reqwidth", 110);

    BK_node_add(modal_backdrop_, modal_dialog_, -1);
    BK_geom_set_manager(modal_dialog_, BK_GEOM_PLACE);
    BK_geom_set(modal_dialog_, "relx", 0.5);
    BK_geom_set(modal_dialog_, "rely", 0.5);
    BK_geom_set(modal_dialog_, "anchor", BK_ANCHOR_N | BK_ANCHOR_S | BK_ANCHOR_W | BK_ANCHOR_E);

    active_modal_ = ModalKind::AuthorImage;
    BK_set_num(modal_backdrop_, "visible", 1);
    BK_node_relayout(modal_backdrop_);
    BK_node_invalidate(modal_backdrop_);
}

void BuiApp::on_import_hst() {
    if (headless_mode_ || !BK_window_supported()) return;
    const char* filters[] = {
        "Legacy High Score (d4s.hst)\t*.hst *.HST",
        "All Files\t*"
    };
    char** paths = nullptr;
    int count = BK_file_dialog(i18n_.tr("menu.settings.import_hst").c_str(),
                               0, work_dir_.string().c_str(), "d4s.hst",
                               filters, 2, 0, &paths);
    if (count > 0 && paths && paths[0]) {
        if (scores_.load_legacy_hst(paths[0])) {
            save_config_and_scores();
            queue_deferred(DeferredAction::OpenRecords);
        }
    }
    BK_file_dialog_free(paths);
}

void BuiApp::on_export_hst() {
    if (headless_mode_ || !BK_window_supported()) return;
    const char* filters[] = {
        "Legacy High Score (d4s.hst)\t*.hst *.HST",
        "All Files\t*"
    };
    char** paths = nullptr;
    int count = BK_file_dialog(i18n_.tr("menu.settings.export_hst").c_str(),
                               1, work_dir_.string().c_str(), "d4s.hst",
                               filters, 2, 0, &paths);
    if (count > 0 && paths && paths[0]) {
        scores_.save_legacy_hst(paths[0]);
    }
    BK_file_dialog_free(paths);
}

void BuiApp::on_select_soundfont() {
    if (headless_mode_ || !BK_window_supported()) return;
    const char* filters[] = {
        "SoundFont (*.sf2)\t*.sf2 *.SF2 *.sf3 *.SF3",
        "All Files\t*"
    };
    char** paths = nullptr;
    int count = BK_file_dialog(i18n_.tr("menu.settings.select_soundfont").c_str(),
                               0, work_dir_.string().c_str(), "",
                               filters, 2, 0, &paths);
    if (count > 0 && paths && paths[0]) {
        std::string sf_path = paths[0];
        if (audio_.load_soundfont(sf_path)) {
            config_.soundfont_path = sf_path;
            set_midi_backend(core::MidiBackend::EmbeddedSoundFont);
        }
    }
    BK_file_dialog_free(paths);
}

void BuiApp::process_game_events() {
    auto events = game_state_.consume_events();
    if (events.empty()) return;

    bool had_modal = false;
    for (const auto& ev : events) {
        if (ev.type == core::GameEventType::PlaySfx) {
            audio_.play_sfx(ev.sfx_id);
        } else {
            pending_game_modals_.push_back(ev);
            had_modal = true;
        }
    }

    // Flush the cleared board immediately BEFORE showing any stage-clear / game-over modal
    // so the uncovered stage background (Pic_Level(level_)) stays visible behind the dialog!
    renderer_.render(game_state_);
    board_dirty_ = true;
    upload_board_texture();

    if (had_modal && active_modal_ == ModalKind::None) {
        show_next_game_modal();
    }
}

void BuiApp::tick_game() {
    uint64_t now = common::now_steady_ms();
    int prev_rem = game_state_.remaining_seconds();
    bool prev_anim = game_state_.animating_link();
    int prev_tiles = game_state_.board().tiles_left();

    game_state_.tick(now);
    audio_.update(config_);
    process_game_events();

    if (game_state_.remaining_seconds() != prev_rem ||
        game_state_.animating_link() != prev_anim ||
        game_state_.board().tiles_left() != prev_tiles) {
        renderer_.render(game_state_);
        board_dirty_ = true;
        upload_board_texture();
    }
}

bool BuiApp::map_board_local_to_logical(float bx, float by, float& out_lx, float& out_ly) const {
    if (!board_node_) return false;
    int rect[4] = {0, 0, 0, 0};
    BK_node_rect(board_node_, rect);
    if (rect[2] <= 0 || rect[3] <= 0) return false;

    int vx = 0, vy = 0, vw = 0, vh = 0;
    media::BoardRenderer::compute_aspect_viewport(rect[2], rect[3], vx, vy, vw, vh);
    if (vw <= 0 || vh <= 0) return false;

    float rx = bx - static_cast<float>(vx);
    float ry = by - static_cast<float>(vy);
    if (rx < 0.0f || ry < 0.0f || rx >= static_cast<float>(vw) || ry >= static_cast<float>(vh)) {
        return false;
    }

    out_lx = rx * static_cast<float>(core::GameState::kCanvasWidth) / static_cast<float>(vw);
    out_ly = ry * static_cast<float>(core::GameState::kCanvasHeight) / static_cast<float>(vh);
    return true;
}

void BuiApp::on_host_emit(void* data, BKNode* node, int kind, void* ref, double /*a*/, double /*b*/) {
    auto* self = static_cast<BuiApp*>(data);
    if (!self) return;

    if (kind == BK_EV_CALL) {
        auto act = static_cast<MenuAction>(reinterpret_cast<uintptr_t>(ref));
        switch (act) {
            case MenuAction::NewGame:
                if (self->game_state_.in_title_screen() && self->active_modal_ == ModalKind::None) {
                    self->trigger_new_game();
                }
                break;
            case MenuAction::GiveUp:
                self->trigger_give_up();
                break;
            case MenuAction::Records:
                self->queue_deferred(DeferredAction::OpenRecords);
                break;
            case MenuAction::Exit:
                self->save_config_and_scores();
                if (self->window_) {
                    BK_window_close(self->window_);
                }
                break;
            case MenuAction::ToggleMusic:
                self->config_.music_on = !self->config_.music_on;
                self->audio_.update(self->config_);
                self->rebuild_menus();
                self->save_config_and_scores();
                break;
            case MenuAction::ToggleSound:
                self->config_.sound_on = !self->config_.sound_on;
                self->audio_.update(self->config_);
                self->rebuild_menus();
                self->save_config_and_scores();
                break;
            case MenuAction::LangZhTW:
                self->set_language("zh-TW");
                break;
            case MenuAction::LangZhCN:
                self->set_language("zh-CN");
                break;
            case MenuAction::Track101:
                self->set_midi_track_enabled(101, !self->config_.is_track_enabled(101));
                break;
            case MenuAction::Track102:
                self->set_midi_track_enabled(102, !self->config_.is_track_enabled(102));
                break;
            case MenuAction::Track103:
                self->set_midi_track_enabled(103, !self->config_.is_track_enabled(103));
                break;
            case MenuAction::BackendOst:
                self->set_midi_backend(core::MidiBackend::OstAudio);
                break;
            case MenuAction::BackendNative:
                self->set_midi_backend(core::MidiBackend::NativeOS);
                break;
            case MenuAction::BackendEmbedded:
                self->set_midi_backend(core::MidiBackend::EmbeddedSoundFont);
                break;
            case MenuAction::SelectSoundFont:
                self->on_select_soundfont();
                break;
            case MenuAction::Scale1x:
                self->set_scale_mode(core::WindowScaleMode::Scale1x);
                break;
            case MenuAction::Scale1_5x:
                self->set_scale_mode(core::WindowScaleMode::Scale1_5x);
                break;
            case MenuAction::Scale2x:
                self->set_scale_mode(core::WindowScaleMode::Scale2x);
                break;
            case MenuAction::Scale3x:
                self->set_scale_mode(core::WindowScaleMode::Scale3x);
                break;
            case MenuAction::ScaleFree:
                self->set_scale_mode(core::WindowScaleMode::FreeAspect);
                break;
            case MenuAction::FilterNearest:
                self->set_scale_filter(core::ScaleFilter::Nearest);
                break;
            case MenuAction::FilterBilinear:
                self->set_scale_filter(core::ScaleFilter::Bilinear);
                break;
            case MenuAction::ThemeDark:
                self->set_blender_theme("dark");
                break;
            case MenuAction::ThemeLight:
                self->set_blender_theme("light");
                break;
            case MenuAction::ImportHst:
                self->on_import_hst();
                break;
            case MenuAction::ExportHst:
                self->on_export_hst();
                break;
            case MenuAction::ShowExplain:
                self->queue_deferred(DeferredAction::OpenExplain);
                break;
            case MenuAction::ShowAuthor:
                self->queue_deferred(DeferredAction::OpenAuthor);
                break;
            case MenuAction::None:
                break;
        }
        return;
    }

    // IMPORTANT: Never call BK_node_free() synchronously inside BUI's input dispatch callbacks
    // (BK_EV_PRESS / BK_EV_CLICK / BK_EV_SUBMIT / BK_EV_KEY) because BUI still touches `target`
    // after emitting the event. Always defer modal destruction to on_window_frame / flush!
    if (kind == BK_EV_CLICK && node == self->modal_ok_btn_) {
        if (self->active_modal_ == ModalKind::GameEvent) {
            self->queue_deferred(DeferredAction::AcceptGameModal);
        } else if (self->active_modal_ == ModalKind::Records) {
            self->queue_deferred(DeferredAction::AcceptRecordsModal);
        } else {
            self->queue_deferred(DeferredAction::CloseModal);
        }
        return;
    }

    if (kind == BK_EV_CHANGE && node == self->modal_name_edit_) {
        if (self->records_editable_rank_ >= 0 && self->records_editable_rank_ < core::HighScoreTable::kCount) {
            const char* raw = BK_get_str(node, "value");
            std::string nm = raw ? std::string(raw) : std::string();
            if (nm.empty()) nm = self->i18n_.tr("table.default_player");
            self->scores_.update_rank_name(self->records_editable_rank_, nm);
            if (self->modal_rank_name_lbl_) {
                BK_set_str(self->modal_rank_name_lbl_, "text",
                           self->scores_.entry(self->records_editable_rank_).name.c_str());
            }
        }
        return;
    }

    if (kind == BK_EV_SUBMIT && node == self->modal_name_edit_) {
        if (self->active_modal_ == ModalKind::Records) {
            self->queue_deferred(DeferredAction::AcceptRecordsModal);
        }
        return;
    }

    // VB5 Form_Click behavior for Explain/Author image splashes (click image or backdrop closes safely)
    if (kind == BK_EV_PRESS &&
        (self->active_modal_ == ModalKind::ExplainImage || self->active_modal_ == ModalKind::AuthorImage) &&
        (node == self->modal_image_node_ || node == self->modal_backdrop_)) {
        self->queue_deferred(DeferredAction::CloseModal);
        return;
    }

    if (kind == BK_EV_KEY && self->active_modal_ != ModalKind::None && ref) {
        const char* s = static_cast<const char*>(ref);
        const char* tab = std::strchr(s, '\t');
        const char* keysym = tab ? tab + 1 : s;
        if (std::strcmp(keysym, "Escape") == 0) {
            if (self->active_modal_ == ModalKind::GameEvent) {
                self->queue_deferred(DeferredAction::AcceptGameModal);
            } else if (self->active_modal_ == ModalKind::Records) {
                self->queue_deferred(DeferredAction::AcceptRecordsModal);
            } else {
                self->queue_deferred(DeferredAction::CloseModal);
            }
        } else if (std::strcmp(keysym, "Return") == 0 || std::strcmp(keysym, "space") == 0) {
            if (self->active_modal_ == ModalKind::GameEvent) {
                self->queue_deferred(DeferredAction::AcceptGameModal);
            } else if (self->active_modal_ == ModalKind::Records && node != self->modal_name_edit_) {
                self->queue_deferred(DeferredAction::AcceptRecordsModal);
            } else if (self->active_modal_ == ModalKind::ExplainImage ||
                       self->active_modal_ == ModalKind::AuthorImage) {
                self->queue_deferred(DeferredAction::CloseModal);
            }
        }
    }
}

int BuiApp::on_board_event(void* data, BKNode* /*node*/, const BKCustomEvent* e) {
    auto* self = static_cast<BuiApp*>(data);
    if (!self || !e) return 0;
    if (self->active_modal_ != ModalKind::None) return 1;

    switch (e->type) {
        case BK_CUSTOM_MOTION: {
            float lx = -1.0f, ly = -1.0f;
            int prev_hc = self->game_state_.hover_c();
            int prev_hr = self->game_state_.hover_r();
            if (self->map_board_local_to_logical(e->x, e->y, lx, ly)) {
                self->game_state_.on_mouse_move(lx, ly);
            } else {
                self->game_state_.on_mouse_move(-1.0f, -1.0f);
            }
            if (self->game_state_.hover_c() != prev_hc || self->game_state_.hover_r() != prev_hr) {
                self->renderer_.render(self->game_state_);
                self->board_dirty_ = true;
                self->upload_board_texture();
            }
            return 1;
        }

        case BK_CUSTOM_LEAVE:
            if (self->game_state_.hover_c() != -1 || self->game_state_.hover_r() != -1) {
                self->game_state_.on_mouse_move(-1.0f, -1.0f);
                self->renderer_.render(self->game_state_);
                self->board_dirty_ = true;
                self->upload_board_texture();
            }
            return 1;

        case BK_CUSTOM_PRESS: {
            float lx = -1.0f, ly = -1.0f;
            if (!self->map_board_local_to_logical(e->x, e->y, lx, ly)) return 1;

            // BUI custom button mapping: 1 = left, 2 = middle, 3 = right
            int btn = 0;
            if (e->button == 1) btn = 1;
            else if (e->button == 3) btn = 2;

            if (btn != 0) {
                uint64_t now = common::now_steady_ms();
                self->game_state_.on_mouse_down(btn, lx, ly, now);
                self->process_game_events();
                self->renderer_.render(self->game_state_);
                self->board_dirty_ = true;
                self->upload_board_texture();
            }
            return 1;
        }

        default:
            return 0;
    }
}

void BuiApp::on_board_draw(void* data, BKNode* /*node*/, BuiRaster* r, const int rect[4]) {
    auto* self = static_cast<BuiApp*>(data);
    if (!self || !r || rect[2] <= 0 || rect[3] <= 0) return;

    const float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    BUI_raster_fill_rect(r,
                         static_cast<float>(rect[0]),
                         static_cast<float>(rect[1]),
                         static_cast<float>(rect[2]),
                         static_cast<float>(rect[3]),
                         black);

    if (self->board_tex_ == 0) return;

    int vx = 0, vy = 0, vw = 0, vh = 0;
    media::BoardRenderer::compute_aspect_viewport(rect[2], rect[3], vx, vy, vw, vh);
    if (vw <= 0 || vh <= 0) return;

    float x0 = static_cast<float>(rect[0] + vx);
    float y0 = static_cast<float>(rect[1] + vy);
    float x1 = x0 + static_cast<float>(vw);
    float y1 = y0 + static_cast<float>(vh);

    const BuiMeshVertex vtx[4] = {
        {x0, y0, 0.0f, 0.0f, 0xFFFFFFFFu},
        {x1, y0, 1.0f, 0.0f, 0xFFFFFFFFu},
        {x1, y1, 1.0f, 1.0f, 0xFFFFFFFFu},
        {x0, y1, 0.0f, 1.0f, 0xFFFFFFFFu},
    };
    const uint16_t idx[6] = {0, 1, 2, 0, 2, 3};
    int flags = (self->config_.scale_filter == core::ScaleFilter::Nearest) ? BUI_MESH_NEAREST : 0;

    BUI_raster_mesh(r, vtx, 4, idx, 6, flags, self->board_tex_, nullptr, 0.0f, 0.0f);
}

void BuiApp::on_window_frame(void* data, BKWindow* /*win*/) {
    auto* self = static_cast<BuiApp*>(data);
    if (!self) return;
    self->flush_deferred_actions();
}

void BuiApp::on_window_timer(void* data, BKWindow* win, int id) {
    auto* self = static_cast<BuiApp*>(data);
    if (!self || id != self->tick_timer_id_) return;

    self->flush_deferred_actions();
    self->tick_game();

    if (win && BK_window_is_open(win)) {
        int next_ms = (!self->game_state_.in_title_screen() || self->game_state_.animating_link()) ? 16 : 50;
        self->tick_timer_id_ = BK_window_timer(win, next_ms);
    }
}

int BuiApp::on_window_close(void* data, BKWindow* /*win*/) {
    auto* self = static_cast<BuiApp*>(data);
    if (self) {
        self->save_config_and_scores();
    }
    return 1; // allow window close
}

void BuiApp::simulate_click(int x, int y) {
    if (!ctx_) return;
    BK_motion(ctx_, x, y, 0);
    BK_press(ctx_, x, y, 0);
    BK_release(ctx_, x, y, 0);
    flush_deferred_actions();
}

bool BuiApp::render_headless_frame(const std::filesystem::path& screenshot_png) {
    if (!initialized_ || !ctx_) return false;

    flush_deferred_actions();
    tick_game();
    upload_board_texture();

    int mb_sz[2] = {0, 28};
    BK_node_natural(menubar_, mb_sz);
    int mb_h = (mb_sz[1] > 0) ? mb_sz[1] : 28;

    float factor = (config_.scale_mode == core::WindowScaleMode::FreeAspect) ? 1.0f : config_.scale_factor();
    int w = static_cast<int>(std::lround(core::GameState::kCanvasWidth * factor));
    int h = static_cast<int>(std::lround(core::GameState::kCanvasHeight * factor)) + mb_h;

    if (!headless_raster_) {
        headless_raster_ = BUI_raster_create(w, h);
    }
    if (!headless_raster_) return false;

    BK_render(ctx_, headless_raster_, w, h, 1);

    if (!screenshot_png.empty()) {
        return write_bui_raster_png(headless_raster_, screenshot_png);
    }
    return true;
}

int BuiApp::run() {
    if (!initialized_) return 1;
    if (!window_) {
        render_headless_frame();
        return 0;
    }

    BK_window_show(window_);
    tick_timer_id_ = BK_window_timer(window_, 16);
    BK_window_run();

    save_config_and_scores();
    return 0;
}

} // namespace llk2::ui::bui
