#include "ui/imgui/imgui_app.hpp"
#include "ui/common/ui_helpers.hpp"

#include "imgui.h"
#if defined(LLK2_USE_SDL3)
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#else
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#endif

#if defined(IMGUI_ENABLE_FREETYPE)
#include "misc/freetype/imgui_freetype.h"
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace llk2::ui::imgui {

namespace {

std::string tr_clean(const core::I18n& i18n, std::string_view key) {
    return common::strip_mnemonic(i18n.tr(key));
}

} // namespace

ImGuiApp::ImGuiApp(media::AssetManager& assets,
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
        if (window_) {
            SDL_SetWindowTitle(window_, i18n_.tr("app.title").c_str());
        }
    });
}

ImGuiApp::~ImGuiApp() {
    shutdown();
}

bool ImGuiApp::init(bool headless_mode, std::string* err_out) {
    if (initialized_) return true;
    headless_mode_ = headless_mode;

#if defined(LLK2_USE_SDL3)
    if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)) {
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
            if (err_out) *err_out = std::string("SDL_InitSubSystem(VIDEO) failed: ") + SDL_GetError();
            return false;
        }
    }
#else
    if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)) {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
            if (err_out) *err_out = std::string("SDL_InitSubSystem(VIDEO) failed: ") + SDL_GetError();
            return false;
        }
    }
#endif

    float factor = (config_.scale_mode == core::WindowScaleMode::FreeAspect) ? 1.0f : config_.scale_factor();
    int init_w = static_cast<int>(std::lround(core::GameState::kCanvasWidth * factor));
    int init_h = static_cast<int>(std::lround(core::GameState::kCanvasHeight * factor)) + static_cast<int>(std::lround(menu_bar_h_));

#if defined(LLK2_USE_SDL3)
    SDL_WindowFlags win_flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (config_.scale_mode == core::WindowScaleMode::FreeAspect) {
        win_flags |= SDL_WINDOW_RESIZABLE;
    }
    if (headless_mode_) {
        win_flags |= SDL_WINDOW_HIDDEN;
    }
    window_ = SDL_CreateWindow(i18n_.tr("app.title").c_str(), init_w, init_h, win_flags);
    if (!window_) {
        if (err_out) *err_out = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
        return false;
    }

    if (headless_mode_) {
        sdl_renderer_ = SDL_CreateRenderer(window_, SDL_SOFTWARE_RENDERER);
    }
    if (!sdl_renderer_) {
        sdl_renderer_ = SDL_CreateRenderer(window_, nullptr);
    }
    if (!sdl_renderer_) {
        sdl_renderer_ = SDL_CreateRenderer(window_, SDL_SOFTWARE_RENDERER);
    }
    if (!sdl_renderer_) {
        if (err_out) *err_out = std::string("SDL_CreateRenderer failed: ") + SDL_GetError();
        shutdown();
        return false;
    }
    if (!headless_mode_) {
        SDL_SetRenderVSync(sdl_renderer_, 1);
    }
#else
    Uint32 win_flags = SDL_WINDOW_ALLOW_HIGHDPI;
    if (config_.scale_mode == core::WindowScaleMode::FreeAspect) {
        win_flags |= SDL_WINDOW_RESIZABLE;
    }
    if (headless_mode_) {
        win_flags |= SDL_WINDOW_HIDDEN;
    }
    window_ = SDL_CreateWindow(i18n_.tr("app.title").c_str(),
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               init_w, init_h, win_flags);
    if (!window_) {
        if (err_out) *err_out = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
        return false;
    }

    if (headless_mode_) {
        sdl_renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    } else {
        sdl_renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!sdl_renderer_) {
            sdl_renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
        }
    }
    if (!sdl_renderer_) {
        if (err_out) *err_out = std::string("SDL_CreateRenderer failed: ") + SDL_GetError();
        shutdown();
        return false;
    }
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // Do not pollute working dir with imgui.ini

    setup_style();
    setup_fonts();

#if defined(LLK2_USE_SDL3)
    if (!ImGui_ImplSDL3_InitForSDLRenderer(window_, sdl_renderer_)) {
        if (err_out) *err_out = "ImGui_ImplSDL3_InitForSDLRenderer failed";
        shutdown();
        return false;
    }
    if (!ImGui_ImplSDLRenderer3_Init(sdl_renderer_)) {
        if (err_out) *err_out = "ImGui_ImplSDLRenderer3_Init failed";
        shutdown();
        return false;
    }
#else
    if (!ImGui_ImplSDL2_InitForSDLRenderer(window_, sdl_renderer_)) {
        if (err_out) *err_out = "ImGui_ImplSDL2_InitForSDLRenderer failed";
        shutdown();
        return false;
    }
    if (!ImGui_ImplSDLRenderer2_Init(sdl_renderer_)) {
        if (err_out) *err_out = "ImGui_ImplSDLRenderer2_Init failed";
        shutdown();
        return false;
    }
#endif

    if (!create_textures(err_out)) {
        shutdown();
        return false;
    }

    renderer_.render(game_state_);
    board_dirty_ = true;
    upload_board_texture();
    update_texture_filter();
    apply_window_scale();

    initialized_ = true;
    return true;
}

void ImGuiApp::shutdown() {
    if (!window_ && !sdl_renderer_ && !ImGui::GetCurrentContext()) {
        return;
    }

    if (initialized_) {
        save_config_and_scores();
    }

    destroy_textures();

    if (ImGui::GetCurrentContext()) {
#if defined(LLK2_USE_SDL3)
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
#else
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
#endif
        ImGui::DestroyContext();
    }

    if (sdl_renderer_) {
        SDL_DestroyRenderer(sdl_renderer_);
        sdl_renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    initialized_ = false;
}

void ImGuiApp::setup_style() {
    ImGui::StyleColorsDark();
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 5.0f;
    st.FrameRounding = 3.0f;
    st.PopupRounding = 4.0f;
    st.ScrollbarRounding = 4.0f;
    st.WindowPadding = ImVec2(12.0f, 10.0f);
    st.FramePadding = ImVec2(6.0f, 4.0f);
    st.ItemSpacing = ImVec2(8.0f, 5.0f);
    st.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
    st.Colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.13f, 0.16f, 0.98f);
    st.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.45f);
}

void ImGuiApp::setup_fonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

#if defined(IMGUI_ENABLE_FREETYPE)
    io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());
    io.Fonts->FontLoaderFlags = ImGuiFreeTypeLoaderFlags_LightHinting;
#endif

    // Load a full Latin + Vietnamese base font first, then merge a CJK font via MergeMode
    // so English, Vietnamese (full diacritics), Traditional Chinese, and Simplified Chinese
    // all render simultaneously with zero missing glyphs.
    auto latin_match = common::find_system_latin_font(false, "vi", false);
    auto cjk_match = common::find_system_cjk_font(false, config_.language, false);

    ImFont* loaded = nullptr;
    if (latin_match.valid()) {
        ImFontConfig cfg;
        cfg.FontNo = static_cast<ImU32>(std::max(0, latin_match.face_index));
        cfg.OversampleH = 2;
        cfg.OversampleV = 1;
        cfg.PixelSnapH = true;
        loaded = io.Fonts->AddFontFromFileTTF(latin_match.path.c_str(), 16.0f, &cfg);
    }

    if (cjk_match.valid() && cjk_match.path != latin_match.path) {
        ImFontConfig cfg;
        cfg.FontNo = static_cast<ImU32>(std::max(0, cjk_match.face_index));
        cfg.OversampleH = 2;
        cfg.OversampleV = 1;
        cfg.PixelSnapH = true;
        cfg.MergeMode = (loaded != nullptr);
        ImFont* cjk_font = io.Fonts->AddFontFromFileTTF(cjk_match.path.c_str(), 16.0f, &cfg);
        if (!loaded && cjk_font) {
            loaded = cjk_font;
        }
    }

    if (loaded) {
        io.FontDefault = loaded;
        return;
    }

    io.Fonts->AddFontDefault();
}

bool ImGuiApp::create_textures(std::string* err_out) {
    board_tex_ = SDL_CreateTexture(
        sdl_renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
        media::BoardRenderer::WIDTH, media::BoardRenderer::HEIGHT);
    if (!board_tex_) {
        if (err_out) *err_out = std::string("SDL_CreateTexture(board) failed: ") + SDL_GetError();
        return false;
    }

    auto make_static_tex = [&](const media::RgbaSurface& surf) -> SDL_Texture* {
        if (!surf.valid()) return nullptr;
        SDL_Texture* t = SDL_CreateTexture(
            sdl_renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
            surf.width, surf.height);
        if (t) {
            SDL_UpdateTexture(t, nullptr, surf.pixels.data(), surf.width * 4);
        }
        return t;
    };

    explain_tex_lang_.clear();
    ensure_explain_texture_for_lang();
    author_tex_ = make_static_tex(assets_.form_author());
    return true;
}

void ImGuiApp::ensure_explain_texture_for_lang() {
    if (!sdl_renderer_) return;
    const std::string& lang = i18n_.language();
    if (explain_tex_ && explain_tex_lang_ == lang) return;

    if (explain_tex_) {
        SDL_DestroyTexture(explain_tex_);
        explain_tex_ = nullptr;
    }
    const auto& surf = assets_.form_explain_for_lang(lang);
    if (surf.valid()) {
        explain_tex_ = SDL_CreateTexture(
            sdl_renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
            surf.width, surf.height);
        if (explain_tex_) {
            SDL_UpdateTexture(explain_tex_, nullptr, surf.pixels.data(), surf.width * 4);
            explain_tex_w_ = surf.width;
            explain_tex_h_ = surf.height;
        }
    }
    explain_tex_lang_ = lang;
}

void ImGuiApp::destroy_textures() {
    if (author_tex_) {
        SDL_DestroyTexture(author_tex_);
        author_tex_ = nullptr;
    }
    if (explain_tex_) {
        SDL_DestroyTexture(explain_tex_);
        explain_tex_ = nullptr;
    }
    if (board_tex_) {
        SDL_DestroyTexture(board_tex_);
        board_tex_ = nullptr;
    }
}

void ImGuiApp::upload_board_texture() {
    if (!board_tex_ || !board_dirty_) return;
    const auto& surf = renderer_.canvas();
    if (!surf.valid()) return;
    SDL_UpdateTexture(board_tex_, nullptr, surf.pixels.data(), surf.width * 4);
    board_dirty_ = false;
}

void ImGuiApp::update_texture_filter() {
    if (!board_tex_) return;
#if defined(LLK2_USE_SDL3)
    SDL_ScaleMode mode = (config_.scale_filter == core::ScaleFilter::Bilinear)
                             ? SDL_SCALEMODE_LINEAR
                             : SDL_SCALEMODE_NEAREST;
#else
    SDL_ScaleMode mode = (config_.scale_filter == core::ScaleFilter::Bilinear)
                             ? SDL_ScaleModeLinear
                             : SDL_ScaleModeNearest;
#endif
    SDL_SetTextureScaleMode(board_tex_, mode);
}

void ImGuiApp::apply_window_scale() {
    if (!window_) return;
    int mh = static_cast<int>(std::lround(menu_bar_h_));
    if (config_.scale_mode == core::WindowScaleMode::FreeAspect) {
        SDL_SetWindowResizable(window_, true);
        SDL_SetWindowMinimumSize(window_,
                                 core::GameState::kCanvasWidth / 2,
                                 core::GameState::kCanvasHeight / 2 + mh);
    } else {
        float factor = config_.scale_factor();
        int target_w = static_cast<int>(std::lround(core::GameState::kCanvasWidth * factor));
        int target_h = static_cast<int>(std::lround(core::GameState::kCanvasHeight * factor)) + mh;
        SDL_SetWindowResizable(window_, false);
        SDL_SetWindowMinimumSize(window_, target_w, target_h);
        SDL_SetWindowSize(window_, target_w, target_h);
    }
    update_texture_filter();
}

void ImGuiApp::save_config_and_scores() {
    config_.save(work_dir_ / "config.json");
    scores_.save_json(work_dir_ / "scores.json");
    scores_.save_legacy_hst(work_dir_ / "d4s.hst");
}

std::string ImGuiApp::window_title() const {
    if (!window_) return i18n_.tr("app.title");
    const char* t = SDL_GetWindowTitle(window_);
    return t ? std::string(t) : std::string();
}

void ImGuiApp::trigger_new_game() {
    uint64_t now = common::now_steady_ms();
    game_state_.start_new_game(now);
    renderer_.render(game_state_);
    board_dirty_ = true;
}

void ImGuiApp::trigger_give_up() {
    game_state_.give_up_game();
    renderer_.render(game_state_);
    board_dirty_ = true;
}

void ImGuiApp::set_language(const std::string& lang_code) {
    if (i18n_.set_language(lang_code)) {
        config_.language = lang_code;
        save_config_and_scores();
    }
}

void ImGuiApp::set_scale_mode(core::WindowScaleMode mode) {
    config_.scale_mode = mode;
    apply_window_scale();
    save_config_and_scores();
}

void ImGuiApp::set_scale_filter(core::ScaleFilter filter) {
    config_.scale_filter = filter;
    update_texture_filter();
    save_config_and_scores();
}

void ImGuiApp::set_midi_backend(core::MidiBackend backend) {
    config_.midi_backend = backend;
    audio_.update(config_);
    save_config_and_scores();
}

void ImGuiApp::set_midi_track_enabled(int track_id, bool enabled) {
    config_.set_track_enabled(track_id, enabled);
    audio_.update(config_);
    save_config_and_scores();
}

void ImGuiApp::open_records_modal(int editable_rank) {
    records_editable_rank_ = editable_rank;
    std::string init_name = config_.player_name.empty() ? i18n_.tr("table.default_player") : config_.player_name;
    std::snprintf(records_name_buf_, sizeof(records_name_buf_), "%s", init_name.c_str());
    if (editable_rank >= 0 && editable_rank < core::HighScoreTable::kCount) {
        scores_.update_rank_name(editable_rank, init_name);
    }
    show_records_modal_ = true;
    records_open_pending_ = true;
}

void ImGuiApp::open_explain_modal() {
    show_explain_modal_ = true;
    explain_open_pending_ = true;
}

void ImGuiApp::open_author_modal() {
    show_author_modal_ = true;
    author_open_pending_ = true;
}

void ImGuiApp::compute_viewport_pixels(int& out_x, int& out_y, int& out_w, int& out_h,
                                       float& out_scale_x, float& out_scale_y) const {
    int win_w = core::GameState::kCanvasWidth;
    int win_h = core::GameState::kCanvasHeight + static_cast<int>(std::lround(menu_bar_h_));
    int pix_w = win_w;
    int pix_h = win_h;

    if (window_) {
        SDL_GetWindowSize(window_, &win_w, &win_h);
    }
    if (sdl_renderer_) {
#if defined(LLK2_USE_SDL3)
        SDL_GetCurrentRenderOutputSize(sdl_renderer_, &pix_w, &pix_h);
#else
        SDL_GetRendererOutputSize(sdl_renderer_, &pix_w, &pix_h);
#endif
    }

    out_scale_x = (win_w > 0) ? (static_cast<float>(pix_w) / static_cast<float>(win_w)) : 1.0f;
    out_scale_y = (win_h > 0) ? (static_cast<float>(pix_h) / static_cast<float>(win_h)) : 1.0f;

    int top_px = static_cast<int>(std::lround(menu_bar_h_ * out_scale_y));
    int avail_w = std::max(1, pix_w);
    int avail_h = std::max(1, pix_h - top_px);

    int vx = 0, vy = 0, vw = avail_w, vh = avail_h;
    media::BoardRenderer::compute_aspect_viewport(avail_w, avail_h, vx, vy, vw, vh);

    out_x = vx;
    out_y = top_px + vy;
    out_w = vw;
    out_h = vh;
}

bool ImGuiApp::map_window_to_logical(float wx, float wy, float& out_lx, float& out_ly) const {
    int vx = 0, vy = 0, vw = 0, vh = 0;
    float sx = 1.0f, sy = 1.0f;
    compute_viewport_pixels(vx, vy, vw, vh, sx, sy);
    if (vw <= 0 || vh <= 0) return false;

    float px = wx * sx - static_cast<float>(vx);
    float py = wy * sy - static_cast<float>(vy);
    if (px < 0.0f || py < 0.0f || px >= static_cast<float>(vw) || py >= static_cast<float>(vh)) {
        return false;
    }

    out_lx = px * static_cast<float>(core::GameState::kCanvasWidth) / static_cast<float>(vw);
    out_ly = py * static_cast<float>(core::GameState::kCanvasHeight) / static_cast<float>(vh);
    return true;
}

void ImGuiApp::process_game_events() {
    auto events = game_state_.consume_events();
    if (events.empty()) return;

    bool queued_modal = false;
    for (const auto& ev : events) {
        if (ev.type == core::GameEventType::PlaySfx) {
            audio_.play_sfx(ev.sfx_id);
        } else {
            pending_modals_.push_back(ev);
            queued_modal = true;
        }
    }

    // Re-render cleared board right away so uncovered current stage background
    // (Pic_Level(level_)) stays visible behind any modal popup!
    renderer_.render(game_state_);
    board_dirty_ = true;
    upload_board_texture();

    if (queued_modal && pending_modals_.size() == 1) {
        game_modal_open_pending_ = true;
    }
}

void ImGuiApp::tick_game() {
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
    }
}

void ImGuiApp::handle_sdl_event(const SDL_Event& ev, bool& want_quit) {
#if defined(LLK2_USE_SDL3)
    ImGui_ImplSDL3_ProcessEvent(&ev);
#else
    ImGui_ImplSDL2_ProcessEvent(&ev);
#endif

    const ImGuiIO& io = ImGui::GetIO();
    const bool modal_blocking = !pending_modals_.empty() || show_records_modal_ ||
                                show_explain_modal_ || show_author_modal_ || show_path_modal_;

#if defined(LLK2_USE_SDL3)
    switch (ev.type) {
        case SDL_EVENT_QUIT:
            want_quit = true;
            break;

        case SDL_EVENT_KEY_DOWN:
            if (!ev.key.repeat) {
                if (ev.key.key == SDLK_F12) {
                    show_inspector_ = !show_inspector_;
                } else if (ev.key.key == SDLK_F2 && game_state_.in_title_screen() && !modal_blocking) {
                    trigger_new_game();
                }
            }
            break;

        case SDL_EVENT_MOUSE_MOTION:
            if (!modal_blocking && !io.WantCaptureMouse) {
                float lx = -1.0f, ly = -1.0f;
                int prev_hc = game_state_.hover_c();
                int prev_hr = game_state_.hover_r();
                if (map_window_to_logical(ev.motion.x, ev.motion.y, lx, ly)) {
                    game_state_.on_mouse_move(lx, ly);
                } else {
                    game_state_.on_mouse_move(-1.0f, -1.0f);
                }
                if (game_state_.hover_c() != prev_hc || game_state_.hover_r() != prev_hr) {
                    renderer_.render(game_state_);
                    board_dirty_ = true;
                }
            }
            break;

        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            if (game_state_.hover_c() != -1 || game_state_.hover_r() != -1) {
                game_state_.on_mouse_move(-1.0f, -1.0f);
                renderer_.render(game_state_);
                board_dirty_ = true;
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (!modal_blocking && !io.WantCaptureMouse) {
                float lx = -1.0f, ly = -1.0f;
                if (map_window_to_logical(ev.button.x, ev.button.y, lx, ly)) {
                    int btn = 0;
                    if (ev.button.button == SDL_BUTTON_LEFT) btn = 1;
                    else if (ev.button.button == SDL_BUTTON_RIGHT) btn = 2;
                    if (btn != 0) {
                        uint64_t now = common::now_steady_ms();
                        game_state_.on_mouse_down(btn, lx, ly, now);
                        process_game_events();
                        renderer_.render(game_state_);
                        board_dirty_ = true;
                    }
                }
            }
            break;

        default:
            break;
    }
#else
    switch (ev.type) {
        case SDL_QUIT:
            want_quit = true;
            break;

        case SDL_KEYDOWN:
            if (!ev.key.repeat) {
                if (ev.key.keysym.sym == SDLK_F12) {
                    show_inspector_ = !show_inspector_;
                } else if (ev.key.keysym.sym == SDLK_F2 && game_state_.in_title_screen() && !modal_blocking) {
                    trigger_new_game();
                }
            }
            break;

        case SDL_MOUSEMOTION:
            if (!modal_blocking && !io.WantCaptureMouse) {
                float lx = -1.0f, ly = -1.0f;
                int prev_hc = game_state_.hover_c();
                int prev_hr = game_state_.hover_r();
                if (map_window_to_logical(static_cast<float>(ev.motion.x), static_cast<float>(ev.motion.y), lx, ly)) {
                    game_state_.on_mouse_move(lx, ly);
                } else {
                    game_state_.on_mouse_move(-1.0f, -1.0f);
                }
                if (game_state_.hover_c() != prev_hc || game_state_.hover_r() != prev_hr) {
                    renderer_.render(game_state_);
                    board_dirty_ = true;
                }
            }
            break;

        case SDL_WINDOWEVENT:
            if (ev.window.event == SDL_WINDOWEVENT_LEAVE) {
                if (game_state_.hover_c() != -1 || game_state_.hover_r() != -1) {
                    game_state_.on_mouse_move(-1.0f, -1.0f);
                    renderer_.render(game_state_);
                    board_dirty_ = true;
                }
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (!modal_blocking && !io.WantCaptureMouse) {
                float lx = -1.0f, ly = -1.0f;
                if (map_window_to_logical(static_cast<float>(ev.button.x), static_cast<float>(ev.button.y), lx, ly)) {
                    int btn = 0;
                    if (ev.button.button == SDL_BUTTON_LEFT) btn = 1;
                    else if (ev.button.button == SDL_BUTTON_RIGHT) btn = 2;
                    if (btn != 0) {
                        uint64_t now = common::now_steady_ms();
                        game_state_.on_mouse_down(btn, lx, ly, now);
                        process_game_events();
                        renderer_.render(game_state_);
                        board_dirty_ = true;
                    }
                }
            }
            break;

        default:
            break;
    }
#endif
}

#if defined(LLK2_USE_SDL3)
struct Sdl3DialogTag {
    ImGuiApp* app;
    int action;
};

void SDLCALL ImGuiApp::on_sdl3_file_dialog(void* userdata, const char* const* filelist, int /*filter*/) {
    auto* tag = static_cast<Sdl3DialogTag*>(userdata);
    if (!tag) return;
    ImGuiApp* self = tag->app;
    auto act = static_cast<FileDialogAction>(tag->action);
    delete tag;

    if (!filelist || !filelist[0] || !filelist[0][0]) {
        return; // cancelled or error
    }
    std::lock_guard<std::mutex> lock(self->file_choice_mutex_);
    self->file_choices_.push_back({act, std::string(filelist[0])});
}
#endif

void ImGuiApp::request_file_dialog(FileDialogAction action) {
#if defined(LLK2_USE_SDL3)
    if (!headless_mode_) {
        static const SDL_DialogFileFilter hst_filters[] = {
            {"Legacy High Score (d4s.hst)", "hst;HST"},
            {"All Files", "*"}
        };
        static const SDL_DialogFileFilter sf2_filters[] = {
            {"SoundFont (*.sf2, *.sf3)", "sf2;SF2;sf3;SF3"},
            {"All Files", "*"}
        };

        auto* tag = new Sdl3DialogTag{this, static_cast<int>(action)};
        std::string default_loc = (action == FileDialogAction::ExportHst)
                                      ? (work_dir_ / "d4s.hst").string()
                                      : work_dir_.string();

        if (action == FileDialogAction::ExportHst) {
            SDL_ShowSaveFileDialog(on_sdl3_file_dialog, tag, window_, hst_filters, 2, default_loc.c_str());
        } else if (action == FileDialogAction::ImportHst) {
            SDL_ShowOpenFileDialog(on_sdl3_file_dialog, tag, window_, hst_filters, 2, default_loc.c_str(), false);
        } else {
            SDL_ShowOpenFileDialog(on_sdl3_file_dialog, tag, window_, sf2_filters, 2, default_loc.c_str(), false);
        }
        return;
    }
#endif
    // Fallback in-ImGui path modal (SDL2 or headless)
    path_modal_action_ = action;
    std::string def = (action == FileDialogAction::SelectSoundFont)
                          ? config_.soundfont_path
                          : (work_dir_ / "d4s.hst").string();
    std::snprintf(path_modal_buf_, sizeof(path_modal_buf_), "%s", def.c_str());
    show_path_modal_ = true;
    path_modal_open_pending_ = true;
}

void ImGuiApp::process_pending_file_choices() {
    std::deque<PendingFileChoice> batch;
    {
        std::lock_guard<std::mutex> lock(file_choice_mutex_);
        batch.swap(file_choices_);
    }

    for (const auto& item : batch) {
        if (item.path.empty()) continue;
        switch (item.action) {
            case FileDialogAction::ImportHst:
                if (scores_.load_legacy_hst(item.path)) {
                    save_config_and_scores();
                    open_records_modal(-1);
                }
                break;

            case FileDialogAction::ExportHst:
                scores_.save_legacy_hst(item.path);
                break;

            case FileDialogAction::SelectSoundFont:
                if (audio_.load_soundfont(item.path)) {
                    config_.soundfont_path = item.path;
                    set_midi_backend(core::MidiBackend::EmbeddedSoundFont);
                }
                break;
        }
    }
}

void ImGuiApp::draw_menu_bar() {
    if (!ImGui::BeginMainMenuBar()) return;

    float measured_h = ImGui::GetWindowHeight();
    if (measured_h > 4.0f && std::abs(measured_h - menu_bar_h_) > 0.5f) {
        menu_bar_h_ = measured_h;
        if (!scale_applied_with_exact_menu_h_) {
            scale_applied_with_exact_menu_h_ = true;
            apply_window_scale();
        }
    } else if (!scale_applied_with_exact_menu_h_) {
        scale_applied_with_exact_menu_h_ = true;
    }

    const bool on_title = game_state_.in_title_screen();

    // Order: 遊戲(G) - 背景聲音(B) - 設定(S) - 關於(A)
    if (ImGui::BeginMenu((tr_clean(i18n_, "menu.game") + "###menu_game").c_str())) {
        if (ImGui::MenuItem((tr_clean(i18n_, "menu.game.new") + "###act_new").c_str(), "F2", false, on_title)) {
            trigger_new_game();
        }
        if (ImGui::MenuItem((tr_clean(i18n_, "menu.game.giveup") + "###act_giveup").c_str(), nullptr, false, !on_title)) {
            trigger_give_up();
        }
        ImGui::Separator();
        if (ImGui::MenuItem((tr_clean(i18n_, "menu.game.record") + "###act_record").c_str())) {
            open_records_modal(-1);
        }
        ImGui::Separator();
        if (ImGui::MenuItem((tr_clean(i18n_, "menu.game.exit") + "###act_exit").c_str())) {
            request_quit_ = true;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu((tr_clean(i18n_, "menu.audio") + "###menu_audio").c_str())) {
        bool music = config_.music_on;
        if (ImGui::MenuItem((tr_clean(i18n_, "menu.audio.music") + "###act_music").c_str(), nullptr, &music)) {
            config_.music_on = music;
            audio_.update(config_);
            save_config_and_scores();
        }
        bool sound = config_.sound_on;
        if (ImGui::MenuItem((tr_clean(i18n_, "menu.audio.sound") + "###act_sound").c_str(), nullptr, &sound)) {
            config_.sound_on = sound;
            audio_.update(config_);
            save_config_and_scores();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu((tr_clean(i18n_, "menu.settings") + "###menu_settings").c_str())) {
        // Language (zh-TW, zh-CN, en, vi + any custom locales/*.json)
        if (ImGui::BeginMenu((tr_clean(i18n_, "menu.settings.language") + "###sub_lang").c_str())) {
            for (const auto& code : common::ordered_languages(i18n_)) {
                std::string label = common::language_display_name(i18n_, code, false) + "###lang_" + code;
                if (ImGui::MenuItem(label.c_str(), nullptr, config_.language == code)) {
                    set_language(code);
                }
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        // Music Tracks (Artist Name - Track Name)
        if (ImGui::BeginMenu((tr_clean(i18n_, "menu.settings.midi_tracks") + "###sub_tracks").c_str())) {
            const char* keys[3] = {
                "menu.settings.track_101",
                "menu.settings.track_102",
                "menu.settings.track_103"
            };
            for (int i = 0; i < 3; ++i) {
                int tid = 101 + i;
                bool enabled = config_.is_track_enabled(tid);
                if (ImGui::MenuItem((i18n_.tr(keys[i]) + "###track_" + std::to_string(tid)).c_str(), nullptr, &enabled)) {
                    set_midi_track_enabled(tid, enabled);
                }
            }
            ImGui::EndMenu();
        }

        // Music Playback Device
        if (ImGui::BeginMenu((tr_clean(i18n_, "menu.settings.midi_device") + "###sub_device").c_str())) {
            bool has_ost = audio_.has_ost_available();
            bool is_ost = (config_.midi_backend == core::MidiBackend::OstAudio && has_ost);
            bool is_native = (config_.midi_backend == core::MidiBackend::NativeOS);
            bool is_embedded = (!is_ost && !is_native);

            if (ImGui::MenuItem((i18n_.tr("menu.settings.midi_ost") + "###dev_ost").c_str(), nullptr, is_ost, has_ost)) {
                set_midi_backend(core::MidiBackend::OstAudio);
            }
            if (ImGui::MenuItem((i18n_.tr("menu.settings.midi_native") + "###dev_native").c_str(), nullptr, is_native)) {
                set_midi_backend(core::MidiBackend::NativeOS);
            }
            if (ImGui::MenuItem((i18n_.tr("menu.settings.midi_embedded") + "###dev_embedded").c_str(), nullptr, is_embedded)) {
                set_midi_backend(core::MidiBackend::EmbeddedSoundFont);
            }
            ImGui::Separator();
            if (ImGui::MenuItem((i18n_.tr("menu.settings.select_soundfont") + "###act_sf2").c_str())) {
                request_file_dialog(FileDialogAction::SelectSoundFont);
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        // Window Scale & Filter
        if (ImGui::BeginMenu((tr_clean(i18n_, "menu.settings.scale") + "###sub_scale").c_str())) {
            if (ImGui::MenuItem(i18n_.tr("menu.settings.scale_1x").c_str(), nullptr,
                                config_.scale_mode == core::WindowScaleMode::Scale1x)) {
                set_scale_mode(core::WindowScaleMode::Scale1x);
            }
            if (ImGui::MenuItem(i18n_.tr("menu.settings.scale_1_5x").c_str(), nullptr,
                                config_.scale_mode == core::WindowScaleMode::Scale1_5x)) {
                set_scale_mode(core::WindowScaleMode::Scale1_5x);
            }
            if (ImGui::MenuItem(i18n_.tr("menu.settings.scale_2x").c_str(), nullptr,
                                config_.scale_mode == core::WindowScaleMode::Scale2x)) {
                set_scale_mode(core::WindowScaleMode::Scale2x);
            }
            if (ImGui::MenuItem(i18n_.tr("menu.settings.scale_3x").c_str(), nullptr,
                                config_.scale_mode == core::WindowScaleMode::Scale3x)) {
                set_scale_mode(core::WindowScaleMode::Scale3x);
            }
            if (ImGui::MenuItem(i18n_.tr("menu.settings.scale_free").c_str(), nullptr,
                                config_.scale_mode == core::WindowScaleMode::FreeAspect)) {
                set_scale_mode(core::WindowScaleMode::FreeAspect);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(i18n_.tr("menu.settings.filter_nearest").c_str(), nullptr,
                                config_.scale_filter == core::ScaleFilter::Nearest)) {
                set_scale_filter(core::ScaleFilter::Nearest);
            }
            if (ImGui::MenuItem(i18n_.tr("menu.settings.filter_bilinear").c_str(), nullptr,
                                config_.scale_filter == core::ScaleFilter::Bilinear)) {
                set_scale_filter(core::ScaleFilter::Bilinear);
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem((i18n_.tr("menu.settings.import_hst") + "###act_import").c_str())) {
            request_file_dialog(FileDialogAction::ImportHst);
        }
        if (ImGui::MenuItem((i18n_.tr("menu.settings.export_hst") + "###act_export").c_str())) {
            request_file_dialog(FileDialogAction::ExportHst);
        }

        ImGui::Separator();
        std::string insp_lbl = i18n_.tr("menu.settings.inspector") + "###act_inspector";
        ImGui::MenuItem(insp_lbl.c_str(), "F12", &show_inspector_);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu((tr_clean(i18n_, "menu.about") + "###menu_about").c_str())) {
        if (ImGui::MenuItem((tr_clean(i18n_, "menu.about.explain") + "###act_explain").c_str())) {
            open_explain_modal();
        }
        if (ImGui::MenuItem((tr_clean(i18n_, "menu.about.author") + "###act_author").c_str())) {
            open_author_modal();
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void ImGuiApp::draw_game_event_modal() {
    if (pending_modals_.empty()) return;

    const auto& ev = pending_modals_.front();
    std::string title;
    std::string body;
    bool check_hs = false;

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
            check_hs = true;
            break;
        }

        case core::GameEventType::NoMovesGameOverModal:
            title = i18n_.tr("msg.no_moves.title");
            body = i18n_.tr("msg.no_moves.body");
            check_hs = true;
            break;

        case core::GameEventType::TimeoutGameOverModal: {
            int total_sc = static_cast<int>(std::lround(ev.total_score));
            title = i18n_.tr("msg.timeout.title");
            body = i18n_.format("msg.timeout.body", {
                {"bonus", std::to_string(ev.bonus)},
                {"score", std::to_string(total_sc)}
            });
            check_hs = true;
            break;
        }

        case core::GameEventType::PlaySfx:
            pending_modals_.pop_front();
            return;
    }

    std::string popup_label = title + "###llk2_game_modal";
    if (game_modal_open_pending_) {
        ImGui::OpenPopup(popup_label.c_str());
        game_modal_open_pending_ = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(popup_label.c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted(body.c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float btn_w = 100.0f;
        float avail = ImGui::GetContentRegionAvail().x;
        if (avail > btn_w) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - btn_w) * 0.5f);
        }

        bool ok = ImGui::Button((i18n_.tr("table.btn.ok") + "###game_modal_ok").c_str(), ImVec2(btn_w, 0)) ||
                  ImGui::IsKeyPressed(ImGuiKey_Enter) ||
                  ImGui::IsKeyPressed(ImGuiKey_KeypadEnter) ||
                  ImGui::IsKeyPressed(ImGuiKey_Space);

        if (ok) {
            float final_score = ev.total_score;
            pending_modals_.pop_front();
            ImGui::CloseCurrentPopup();

            if (!pending_modals_.empty()) {
                game_modal_open_pending_ = true;
            } else if (check_hs && scores_.is_high_score(final_score)) {
                std::string def_name = config_.player_name.empty() ? i18n_.tr("table.default_player") : config_.player_name;
                int rank = scores_.insert_score(def_name, final_score);
                if (rank >= 0) {
                    ack_game_after_records_ = true;
                    open_records_modal(rank);
                } else {
                    game_state_.acknowledge_modal(common::now_steady_ms());
                    renderer_.render(game_state_);
                    board_dirty_ = true;
                }
            } else {
                game_state_.acknowledge_modal(common::now_steady_ms());
                renderer_.render(game_state_);
                board_dirty_ = true;
            }
        }
        ImGui::EndPopup();
    }
}

void ImGuiApp::draw_records_modal() {
    if (!show_records_modal_) return;

    std::string popup_label = i18n_.tr("dialog.table.title") + "###llk2_records_modal";
    if (records_open_pending_) {
        ImGui::OpenPopup(popup_label.c_str());
        records_open_pending_ = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(340.0f, 0.0f), ImGuiCond_Always);

    if (ImGui::BeginPopupModal(popup_label.c_str(), &show_records_modal_,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        if (ImGui::BeginTable("##hs_table", 3,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn(i18n_.tr("table.col.rank").c_str(), ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn(i18n_.tr("table.col.name").c_str(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(i18n_.tr("table.col.score").c_str(), ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            for (int i = 0; i < core::HighScoreTable::kCount; ++i) {
                const auto& e = scores_.entry(i);
                const bool highlight = (i == records_editable_rank_);
                ImGui::TableNextRow();
                if (highlight) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
                }

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", i + 1);

                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(e.name.c_str());

                ImGui::TableSetColumnIndex(2);
                int iscore = static_cast<int>(std::lround(e.score));
                ImGui::Text("%d", iscore);

                if (highlight) {
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndTable();
        }

        if (records_editable_rank_ >= 0 && records_editable_rank_ < core::HighScoreTable::kCount) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            std::string prompt = i18n_.tr("table.prompt.name") + ":";
            ImGui::TextUnformatted(prompt.c_str());
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText("##player_name_edit", records_name_buf_, sizeof(records_name_buf_))) {
                std::string nm = records_name_buf_;
                if (nm.empty()) nm = i18n_.tr("table.default_player");
                scores_.update_rank_name(records_editable_rank_, nm);
            }
        }

        ImGui::Spacing();
        float btn_w = 100.0f;
        float avail = ImGui::GetContentRegionAvail().x;
        if (avail > btn_w) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - btn_w) * 0.5f);
        }

        bool accept = ImGui::Button((i18n_.tr("table.btn.ok") + "###records_ok").c_str(), ImVec2(btn_w, 0)) ||
                      ImGui::IsKeyPressed(ImGuiKey_Escape) ||
                      ImGui::IsKeyPressed(ImGuiKey_Enter) ||
                      ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
        if (accept || !show_records_modal_) {
            if (records_editable_rank_ >= 0 && records_editable_rank_ < core::HighScoreTable::kCount) {
                std::string nm = records_name_buf_;
                if (nm.empty()) nm = i18n_.tr("table.default_player");
                scores_.update_rank_name(records_editable_rank_, nm);
                config_.player_name = nm;
                save_config_and_scores();
            }
            show_records_modal_ = false;
            records_editable_rank_ = -1;
            ImGui::CloseCurrentPopup();

            if (ack_game_after_records_) {
                ack_game_after_records_ = false;
                game_state_.acknowledge_modal(common::now_steady_ms());
                renderer_.render(game_state_);
                board_dirty_ = true;
            }
        }
        ImGui::EndPopup();
    }
}

void ImGuiApp::draw_image_modal(const char* popup_id, const std::string& title,
                                SDL_Texture* tex, int tex_w, int tex_h, bool& open_flag,
                                bool apply_explain_overlay) {
    if (!open_flag) return;

    std::string label = title + "###" + popup_id;
    bool& pending = (std::strcmp(popup_id, "llk2_explain") == 0) ? explain_open_pending_ : author_open_pending_;
    if (pending) {
        ImGui::OpenPopup(label.c_str());
        pending = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
    if (ImGui::BeginPopupModal(label.c_str(), &open_flag,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        if (tex && tex_w > 0 && tex_h > 0) {
            ImVec2 vp = ImGui::GetMainViewport()->WorkSize;
            float max_w = std::max(120.0f, vp.x - 40.0f);
            float max_h = std::max(120.0f, vp.y - 60.0f);
            float scale = std::min(1.0f, std::min(max_w / static_cast<float>(tex_w),
                                                  max_h / static_cast<float>(tex_h)));
            ImVec2 draw_sz(static_cast<float>(tex_w) * scale, static_cast<float>(tex_h) * scale);
            ImGui::Image(reinterpret_cast<ImTextureID>(tex), draw_sz);

            if (apply_explain_overlay) {
                auto ov = common::build_explain_overlay(assets_, i18n_);
                if (ov.enabled) {
                    ImVec2 p0 = ImGui::GetItemRectMin();
                    ImVec2 p1 = ImGui::GetItemRectMax();
                    float sx = (p1.x - p0.x) / static_cast<float>(common::ExplainOverlayLayout::kImageW);
                    float sy = (p1.y - p0.y) / static_cast<float>(common::ExplainOverlayLayout::kImageH);
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    ImFont* font = ImGui::GetFont();

                    const auto& ex_surf = assets_.form_explain_for_lang(i18n_.language());
                    ImU32 mask_col = IM_COL32(2, 2, 2, 255);
                    if (ex_surf.valid() && ex_surf.width > 10 && ex_surf.height > 10) {
                        size_t idx = (static_cast<size_t>(10) * ex_surf.width + 10) * 4;
                        mask_col = IM_COL32(ex_surf.pixels[idx], ex_surf.pixels[idx + 1], ex_surf.pixels[idx + 2], 255);
                    }
                    auto fill_mask = [&](const common::ExplainRect& r) {
                        dl->AddRectFilled(
                            ImVec2(p0.x + r.x * sx, p0.y + r.y * sy),
                            ImVec2(p0.x + (r.x + r.w) * sx, p0.y + (r.y + r.h) * sy),
                            mask_col);
                    };
                    fill_mask(common::ExplainOverlayLayout::kMaskTurn0);
                    fill_mask(common::ExplainOverlayLayout::kMaskTurn1);
                    fill_mask(common::ExplainOverlayLayout::kMaskTurn2Top);
                    fill_mask(common::ExplainOverlayLayout::kMaskTurn2Bot);
                    fill_mask(common::ExplainOverlayLayout::kMaskRulesBox);

                    auto draw_centered = [&](const common::ExplainRect& r, const std::string& text) {
                        auto lines = common::split_lines(text);
                        float fsz = std::max(10.0f, 15.5f * sy);
                        float line_h = fsz * 1.15f;
                        float total_h = line_h * static_cast<float>(lines.size());
                        float start_y = p0.y + r.y * sy + (r.h * sy - total_h) * 0.5f;
                        for (size_t i = 0; i < lines.size(); ++i) {
                            ImVec2 tsz = font->CalcTextSizeA(fsz, FLT_MAX, 0.0f, lines[i].c_str());
                            float tx = p0.x + r.x * sx + (r.w * sx - tsz.x) * 0.5f;
                            float ty = start_y + static_cast<float>(i) * line_h;
                            dl->AddText(font, fsz, ImVec2(tx, ty), IM_COL32(245, 245, 245, 255), lines[i].c_str());
                        }
                    };
                    draw_centered(common::ExplainOverlayLayout::kMaskTurn0, ov.turn_0);
                    draw_centered(common::ExplainOverlayLayout::kMaskTurn1, ov.turn_1);
                    draw_centered(common::ExplainOverlayLayout::kMaskTurn2Top, ov.turn_2);
                    draw_centered(common::ExplainOverlayLayout::kMaskTurn2Bot, ov.turn_2);

                    // Original VB5 bright green (2, 254, 2) rules body sized comfortably for the 146x314 box
                    const auto& rb = common::ExplainOverlayLayout::kTextRulesBody;
                    float body_fsz = std::max(10.0f, 14.5f * sy);
                    ImVec4 body_clip(p0.x + rb.x * sx, p0.y + rb.y * sy,
                                     p0.x + (rb.x + rb.w) * sx, p0.y + (rb.y + rb.h) * sy);
                    dl->AddText(font, body_fsz, ImVec2(body_clip.x, body_clip.y),
                                IM_COL32(2, 254, 2, 255), ov.body.c_str(), nullptr,
                                rb.w * sx, &body_clip);

                    // Original VB5 bright yellow (254, 253, 5) controls legend
                    const auto& rc = common::ExplainOverlayLayout::kTextControls;
                    float ctrl_fsz = std::max(10.0f, 14.0f * sy);
                    ImVec4 ctrl_clip(p0.x + rc.x * sx, p0.y + rc.y * sy,
                                     p0.x + (rc.x + rc.w) * sx, p0.y + (rc.y + rc.h) * sy);
                    dl->AddText(font, ctrl_fsz, ImVec2(ctrl_clip.x, ctrl_clip.y),
                                IM_COL32(254, 253, 5, 255), ov.controls.c_str(), nullptr,
                                rc.w * sx, &ctrl_clip);
                }
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                open_flag = false;
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_Enter) || !open_flag) {
            open_flag = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

void ImGuiApp::draw_path_fallback_modal() {
    if (!show_path_modal_) return;

    std::string title;
    if (path_modal_action_ == FileDialogAction::ImportHst) {
        title = i18n_.tr("menu.settings.import_hst");
    } else if (path_modal_action_ == FileDialogAction::ExportHst) {
        title = i18n_.tr("menu.settings.export_hst");
    } else {
        title = i18n_.tr("menu.settings.select_soundfont");
    }
    std::string label = title + "###llk2_path_modal";
    if (path_modal_open_pending_) {
        ImGui::OpenPopup(label.c_str());
        path_modal_open_pending_ = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(460.0f, 0.0f), ImGuiCond_Always);
    if (ImGui::BeginPopupModal(label.c_str(), &show_path_modal_,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::SetNextItemWidth(-1.0f);
        bool enter = ImGui::InputText("##path_input", path_modal_buf_, sizeof(path_modal_buf_),
                                      ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        if (enter || ImGui::Button(i18n_.tr("table.btn.ok").c_str(), ImVec2(90.0f, 0))) {
            std::lock_guard<std::mutex> lock(file_choice_mutex_);
            file_choices_.push_back({path_modal_action_, std::string(path_modal_buf_)});
            show_path_modal_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(90.0f, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            show_path_modal_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ImGuiApp::draw_inspector() {
    if (!show_inspector_) return;

    auto snap = common::build_inspector_snapshot(game_state_, audio_, config_, i18n_);
    std::string title = i18n_.tr("inspector.title") + "###llk2_inspector";

    ImGui::SetNextWindowSize(ImVec2(330.0f, 0.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(title.c_str(), &show_inspector_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s: %s", i18n_.tr("inspector.mode").c_str(), snap.mode.c_str());
        ImGui::Text("%s: %s", i18n_.tr("inspector.stage").c_str(), snap.stage.c_str());
        ImGui::Text("%s: %s", i18n_.tr("inspector.tiles_left").c_str(), snap.tiles_left.c_str());
        ImGui::TextUnformatted(snap.lives_score.c_str());
        ImGui::Text("%s: %s", i18n_.tr("inspector.time_left").c_str(), snap.time_left.c_str());
        ImGui::Separator();
        ImGui::Text("%s: %s", i18n_.tr("inspector.audio_backend").c_str(), snap.audio_backend.c_str());
        ImGui::Text("%s: %s", i18n_.tr("inspector.soundfont").c_str(), snap.soundfont.c_str());
        ImGui::Text("%s: %s", i18n_.tr("inspector.track").c_str(), snap.track.c_str());
        ImGui::Separator();
        ImGui::Text("%s: %s", i18n_.tr("inspector.next_hint").c_str(), snap.next_hint.c_str());
        ImGui::Checkbox((i18n_.tr("inspector.highlight_hint") + "###chk_hint").c_str(), &show_hint_overlay_);
    }
    ImGui::End();
}

void ImGuiApp::draw_hint_overlay() {
    if (!show_hint_overlay_ || !game_state_.board_active() || game_state_.in_title_screen()) {
        return;
    }
    int c1 = static_cast<int>(std::lround(game_state_.board().hint_x1()));
    int r1 = static_cast<int>(std::lround(game_state_.board().hint_y1()));
    int c2 = static_cast<int>(std::lround(game_state_.board().hint_x2()));
    int r2 = static_cast<int>(std::lround(game_state_.board().hint_y2()));
    if (!game_state_.board().cell(c1, r1).occupied || !game_state_.board().cell(c2, r2).occupied) {
        return;
    }

    int vx = 0, vy = 0, vw = 0, vh = 0;
    float sx = 1.0f, sy = 1.0f;
    compute_viewport_pixels(vx, vy, vw, vh, sx, sy);
    if (vw <= 0 || vh <= 0) return;

    // Convert viewport pixel rect back to ImGui logical display coordinates
    float lvx = static_cast<float>(vx) / sx;
    float lvy = static_cast<float>(vy) / sy;
    float lvw = static_cast<float>(vw) / sx;
    float lvh = static_cast<float>(vh) / sy;

    float scale_x = lvw / static_cast<float>(core::GameState::kCanvasWidth);
    float scale_y = lvh / static_cast<float>(core::GameState::kCanvasHeight);

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    auto draw_tile_box = [&](int c, int r) {
        float x0 = lvx + (static_cast<float>(core::GameState::kOffX + c * media::BoardRenderer::TILE_W)) * scale_x;
        float y0 = lvy + (static_cast<float>(core::GameState::kOffY + r * media::BoardRenderer::TILE_H)) * scale_y;
        float x1 = x0 + 40.0f * scale_x;
        float y1 = y0 + 50.0f * scale_y;
        dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(255, 220, 40, 240), 2.0f, 0, 3.0f);
    };
    draw_tile_box(c1, r1);
    draw_tile_box(c2, r2);
}

void ImGuiApp::draw_modals() {
    ensure_explain_texture_for_lang();
    draw_game_event_modal();
    draw_records_modal();
    draw_image_modal("llk2_explain", i18n_.tr("dialog.explain.title"),
                     explain_tex_, explain_tex_w_, explain_tex_h_,
                     show_explain_modal_, true);
    draw_image_modal("llk2_author", i18n_.tr("dialog.author.title"),
                     author_tex_, assets_.form_author().width, assets_.form_author().height,
                     show_author_modal_, false);
    draw_path_fallback_modal();
}

void ImGuiApp::render_frame() {
    if (!initialized_) return;

    process_pending_file_choices();
    tick_game();
    upload_board_texture();

#if defined(LLK2_USE_SDL3)
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
#else
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
#endif
    ImGui::NewFrame();

    draw_menu_bar();
    draw_hint_overlay();
    draw_inspector();
    draw_modals();

    ImGui::Render();

    // Render board directly with exact physical pixel viewport & Nearest/Linear scale mode
    SDL_SetRenderScale(sdl_renderer_, 1.0f, 1.0f);
    SDL_SetRenderDrawColor(sdl_renderer_, 0, 0, 0, 255);
    SDL_RenderClear(sdl_renderer_);

    int vx = 0, vy = 0, vw = 0, vh = 0;
    float sx = 1.0f, sy = 1.0f;
    compute_viewport_pixels(vx, vy, vw, vh, sx, sy);

    if (board_tex_ && vw > 0 && vh > 0) {
#if defined(LLK2_USE_SDL3)
        SDL_FRect dst{static_cast<float>(vx), static_cast<float>(vy),
                      static_cast<float>(vw), static_cast<float>(vh)};
        SDL_RenderTexture(sdl_renderer_, board_tex_, nullptr, &dst);
#else
        SDL_Rect dst{vx, vy, vw, vh};
        SDL_RenderCopy(sdl_renderer_, board_tex_, nullptr, &dst);
#endif
    }

    ImGuiIO& io = ImGui::GetIO();
    SDL_SetRenderScale(sdl_renderer_, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
#if defined(LLK2_USE_SDL3)
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), sdl_renderer_);
#else
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), sdl_renderer_);
#endif
    SDL_RenderPresent(sdl_renderer_);
}

bool ImGuiApp::save_screenshot(const std::filesystem::path& png_path) {
    if (!sdl_renderer_) return false;
#if defined(LLK2_USE_SDL3)
    SDL_Surface* snap = SDL_RenderReadPixels(sdl_renderer_, nullptr);
    if (!snap) return false;
    SDL_Surface* rgba = SDL_ConvertSurface(snap, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(snap);
    if (!rgba) return false;
    int ok = stbi_write_png(png_path.string().c_str(), rgba->w, rgba->h, 4, rgba->pixels, rgba->pitch);
    SDL_DestroySurface(rgba);
    return ok != 0;
#else
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(sdl_renderer_, &w, &h);
    if (w <= 0 || h <= 0) return false;
    std::vector<uint8_t> pixels(static_cast<size_t>(w) * h * 4);
    if (SDL_RenderReadPixels(sdl_renderer_, nullptr, SDL_PIXELFORMAT_RGBA32, pixels.data(), w * 4) != 0) {
        return false;
    }
    return stbi_write_png(png_path.string().c_str(), w, h, 4, pixels.data(), w * 4) != 0;
#endif
}

int ImGuiApp::run() {
    if (!initialized_) return 1;

    request_quit_ = false;
    bool want_quit = false;

    while (!want_quit && !request_quit_) {
        SDL_Event ev;
        // Event-driven low-CPU wait: 16 ms during active play/animation, 50 ms when idle on title screen.
        int wait_ms = (!game_state_.in_title_screen() || game_state_.animating_link() || show_inspector_) ? 16 : 50;
        if (SDL_WaitEventTimeout(&ev, wait_ms)) {
            handle_sdl_event(ev, want_quit);
            while (SDL_PollEvent(&ev)) {
                handle_sdl_event(ev, want_quit);
            }
        }
        if (want_quit || request_quit_) break;
        render_frame();
    }

    save_config_and_scores();
    return 0;
}

} // namespace llk2::ui::imgui
