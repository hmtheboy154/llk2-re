#pragma once

#include "core/config.hpp"
#include "core/game_state.hpp"
#include "media/asset_manager.hpp"

#include <cstdint>

namespace llk2::media {

enum class BlitOp {
    SrcCopy,  // dst = src (Win32 SRCCOPY 0x00CC0020)
    SrcAnd,   // dst = dst & src (Win32 SRCAND 0x008800C6)
    SrcPaint  // dst = dst | src (Win32 SRCPAINT 0x00EE0086)
};

class BoardRenderer {
public:
    static constexpr int WIDTH = core::GameState::kCanvasWidth;   // 768
    static constexpr int HEIGHT = core::GameState::kCanvasHeight; // 550
    static constexpr int TILE_W = 40;
    static constexpr int TILE_H = 50;

    explicit BoardRenderer(const AssetManager& assets);

    // Render current GameState (either title screen or active game board) into the internal 768x550 RGBA buffer
    void render(const core::GameState& state);
    void render_title_screen();
    void render_board(const core::GameState& state);

    // Force-invalidate the cached static board surface (Pic_Buffer equivalent)
    void invalidate_cache() noexcept { cache_key_.valid = false; }

    const RgbaSurface& canvas() const { return canvas_; }

    // Scale internal 768x550 canvas to an arbitrary output size using Nearest or Bilinear filtering
    void scale_to(int dst_w, int dst_h, core::ScaleFilter filter, RgbaSurface& out_surf) const;

    // Helper for computing aspect-preserving viewport inside a window of size (win_w, win_h)
    static void compute_aspect_viewport(int win_w, int win_h, int& vp_x, int& vp_y, int& vp_w, int& vp_h);

private:
    struct BaseCacheKey {
        bool valid = false;
        bool board_active = false;
        bool animating_link = false;
        int level = -1;
        int palette_off = -1;
        int lives = -1;
        int rem_sec = -1;
        int iscore = -1;
        int tiles_left = -1;
        int sel1_c = -1;
        int sel1_r = -1;
        int sel2_c = -1;
        int sel2_r = -1;
        uint64_t board_hash = 0;

        bool operator==(const BaseCacheKey& o) const noexcept = default;
    };

    static uint64_t compute_board_hash(const core::Board& b) noexcept;
    void rebuild_base_cache(const core::GameState& state);

    static void blit_to(RgbaSurface& dst, const RgbaSurface& src,
                        int dst_x, int dst_y, int w, int h, int src_x, int src_y, BlitOp op);
    void set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    static void draw_hollow_rect_to(RgbaSurface& dst, int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
    void draw_hollow_rect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
    void draw_thick_line(int x1, int y1, int x2, int y2, int width, uint8_t r, uint8_t g, uint8_t b);

    const AssetManager& assets_;
    RgbaSurface base_cache_; // Equivalent to VB5 Pic_Buffer (background + HUD + 144 tiles)
    RgbaSurface canvas_;     // Equivalent to VB5 Pic_Show (base_cache_ + hover frame / lightning beam)
    BaseCacheKey cache_key_{};
};

} // namespace llk2::media
