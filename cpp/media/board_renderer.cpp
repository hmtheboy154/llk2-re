#include "media/board_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace llk2::media {

BoardRenderer::BoardRenderer(const AssetManager& assets)
    : assets_(assets) {
    base_cache_.resize(WIDTH, HEIGHT);
    base_cache_.fill_black();
    canvas_.resize(WIDTH, HEIGHT);
    canvas_.fill_black();
}

uint64_t BoardRenderer::compute_board_hash(const core::Board& b) noexcept {
    // Fast FNV-1a 64-bit hash over the 18x11 grid occupancy and tile_id
    uint64_t h = 14695981039346656037ULL;
    for (int r = 0; r < core::Board::kRows; ++r) {
        for (int c = 0; c < core::Board::kCols; ++c) {
            const auto& cell = b.cell(c, r);
            uint16_t word = cell.occupied ? static_cast<uint16_t>(0x8000u | (cell.tile_id & 0x7FFFu)) : 0u;
            h ^= word;
            h *= 1099511628211ULL;
        }
    }
    return h;
}

void BoardRenderer::render(const core::GameState& state) {
    if (state.in_title_screen()) {
        render_title_screen();
    } else {
        render_board(state);
    }
}

void BoardRenderer::render_title_screen() {
    cache_key_.valid = false;
    canvas_.fill_black();
    if (!assets_.is_loaded()) return;

    // Blit Pic_Title (500x375) at ((768 - 500) \ 2, (550 - 375) \ 2 - 20) = (134, 67)
    blit_to(canvas_, assets_.pic_title(), (WIDTH - 500) / 2, (HEIGHT - 375) / 2 - 20, 500, 375, 0, 0, BlitOp::SrcCopy);

    // Blit studio banner (290x25 from Pic_Data at src (220, 0)) at ((768 - 290) \ 2, 550 - 30) = (239, 520)
    blit_to(canvas_, assets_.pic_data(), (WIDTH - 290) / 2, HEIGHT - 30, 290, 25, 220, 0, BlitOp::SrcCopy);
}

void BoardRenderer::rebuild_base_cache(const core::GameState& state) {
    base_cache_.fill_black();
    if (!assets_.is_loaded()) return;

    int lvl = std::clamp(state.level(), 0, 6);
    // 1. Background image Pic_Level(Level) (600x450) centered at (84, 50)
    blit_to(base_cache_, assets_.pic_level(lvl), (WIDTH - 600) / 2, (HEIGHT - 450) / 2, 600, 450, 0, 0, BlitOp::SrcCopy);

    const auto& data_surf = assets_.pic_data();
    const auto& mj_surf = assets_.pic_mjdata();

    // 2. HUD
    int anchor = (WIDTH - 300) / 2; // 234

    // Lives icon + digits
    if (state.lives() > 0) {
        blit_to(base_cache_, data_surf, 15, 10, 20, 30, 0, 25, BlitOp::SrcCopy);
        std::string s = std::to_string(state.lives());
        for (size_t i = 0; i < s.size(); ++i) {
            int digit = s[i] - '0';
            if (digit >= 0 && digit <= 9) {
                int dst_x = static_cast<int>(i + 1) * 15 + 25;
                blit_to(base_cache_, data_surf, dst_x, 10, 15, 25, digit * 15, 0, BlitOp::SrcCopy);
            }
        }
    }

    // Timer border box: Line (anchor - 2, 6)-(anchor + 301, 24), QBColor(14) (#FFFF00), B
    draw_hollow_rect_to(base_cache_, anchor - 2, 6, anchor + 301, 24, 255, 255, 0);

    // Timer bar fill
    int rem_sec = state.remaining_seconds();
    if (rem_sec > 0 && state.time_left() > 0.0f) {
        int bar_w = static_cast<int>(std::lround((static_cast<double>(rem_sec) / static_cast<double>(state.time_left())) * 300.0));
        bar_w = std::clamp(bar_w, 0, 300);
        if (bar_w > 0) {
            blit_to(base_cache_, data_surf, anchor, 8, bar_w, 15, 20, 25, BlitOp::SrcCopy);
        }
    }

    // Score digits
    {
        int iscore = std::max(0, static_cast<int>(std::lround(state.score())));
        std::string s = std::to_string(iscore);
        for (size_t i = 0; i < s.size(); ++i) {
            int digit = s[i] - '0';
            if (digit >= 0 && digit <= 9) {
                int dst_x = WIDTH - 100 + static_cast<int>(i + 1) * 15;
                blit_to(base_cache_, data_surf, dst_x, 10, 15, 25, digit * 15, 0, BlitOp::SrcCopy);
            }
        }
    }

    // 3. Board tiles (11 rows x 18 cols)
    const auto& board = state.board();
    int pal_off = std::clamp(board.palette_off(), 0, 5);
    for (int r = 0; r < core::Board::kRows; ++r) {
        for (int c = 0; c < core::Board::kCols; ++c) {
            const auto& cell = board.cell(c, r);
            if (!cell.occupied) continue;

            int px = core::GameState::kOffX + c * TILE_W;
            int py = core::GameState::kOffY + r * TILE_H;
            int t1 = cell.tile_id % 9;
            int t2 = cell.tile_id / 9;

            // 3D shell mask (46x56 from (276, 500) with SRCAND)
            blit_to(base_cache_, mj_surf, px, py, 46, 56, 276, 500, BlitOp::SrcAnd);

            // 3D shell art (selected -> y=556, unselected -> y=500 with SRCPAINT)
            bool is_sel = (c == state.sel1_c() && r == state.sel1_r()) ||
                          (state.animating_link() && c == state.sel2_c() && r == state.sel2_r());
            int shell_src_y = is_sel ? 556 : 500;
            blit_to(base_cache_, mj_surf, px, py, 46, 56, 46 * pal_off, shell_src_y, BlitOp::SrcPaint);

            // Tile face (only when board_active is true)
            if (state.board_active()) {
                blit_to(base_cache_, mj_surf, px, py, 40, 50, t1 * 40, (t2 + 5) * 50, BlitOp::SrcAnd);
                blit_to(base_cache_, mj_surf, px, py, 40, 50, t1 * 40, t2 * 50, BlitOp::SrcPaint);
            }
        }
    }
}

void BoardRenderer::render_board(const core::GameState& state) {
    if (!assets_.is_loaded()) {
        canvas_.fill_black();
        return;
    }

    const auto& board = state.board();
    BaseCacheKey key{};
    key.valid = true;
    key.board_active = state.board_active();
    key.animating_link = state.animating_link();
    key.level = std::clamp(state.level(), 0, 6);
    key.palette_off = std::clamp(board.palette_off(), 0, 5);
    key.lives = state.lives();
    key.rem_sec = state.remaining_seconds();
    key.iscore = std::max(0, static_cast<int>(std::lround(state.score())));
    key.tiles_left = board.tiles_left();
    key.sel1_c = state.sel1_c();
    key.sel1_r = state.sel1_r();
    key.sel2_c = state.animating_link() ? state.sel2_c() : -1;
    key.sel2_r = state.animating_link() ? state.sel2_r() : -1;
    key.board_hash = compute_board_hash(board);

    if (!(key == cache_key_)) {
        rebuild_base_cache(state);
        cache_key_ = key;
    }

    // Fast copy of cached Pic_Buffer (1.68 MB memcpy, < 0.03 ms)
    std::memcpy(canvas_.pixels.data(), base_cache_.pixels.data(), base_cache_.pixels.size());

    // Overlay 3-ring hover/cursor highlight box on the hovered tile (when not in link flash)
    if (!state.animating_link()) {
        int hc = state.hover_c();
        int hr = state.hover_r();
        if (hc >= 0 && hc < core::Board::kCols && hr >= 0 && hr < core::Board::kRows && board.cell(hc, hr).occupied) {
            int px = core::GameState::kOffX + hc * TILE_W;
            int py = core::GameState::kOffY + hr * TILE_H;
            if (state.cursor_active()) {
                // High-contrast Cyan/White/Cyan frame for keyboard & gamepad cursor
                draw_hollow_rect(px + 1, py + 1, px + 39, py + 49, 0, 220, 255);
                draw_hollow_rect(px + 2, py + 2, px + 38, py + 48, 255, 255, 255);
                draw_hollow_rect(px + 3, py + 3, px + 37, py + 47, 0, 170, 255);
            } else {
                draw_hollow_rect(px + 1, py + 1, px + 39, py + 49, 255, 0, 0);   // QBColor(12) Red
                draw_hollow_rect(px + 2, py + 2, px + 38, py + 48, 255, 255, 0); // QBColor(14) Yellow
                draw_hollow_rect(px + 3, py + 3, px + 37, py + 47, 255, 0, 0);   // QBColor(12) Red
            }
        }
    } else {
        // Overlay 3-pass link lightning beam
        const auto& path = state.active_link_path();
        int segs = std::clamp(static_cast<int>(path.link_n), 0, 3);
        const float raw_x[4] = {path.x1, path.x2, path.x3, path.x4};
        const float raw_y[4] = {path.y1, path.y2, path.y3, path.y4};
        int pts_x[4] = {0};
        int pts_y[4] = {0};
        for (int k = 0; k <= segs; ++k) {
            pts_x[k] = static_cast<int>(std::lround(raw_x[k])) * TILE_W + core::GameState::kOffX + 20;
            pts_y[k] = static_cast<int>(std::lround(raw_y[k])) * TILE_H + core::GameState::kOffY + 25;
        }

        struct BeamPass {
            int width;
            uint8_t r, g, b;
        };
        static constexpr BeamPass kPasses[3] = {
            {6, 255, 255, 255}, // QBColor(15) White
            {4, 255, 0,   0  }, // QBColor(12) Red
            {2, 255, 255, 0  }, // QBColor(14) Yellow
        };

        for (const auto& pass : kPasses) {
            for (int s = 0; s < segs; ++s) {
                draw_thick_line(pts_x[s], pts_y[s], pts_x[s + 1], pts_y[s + 1],
                                pass.width, pass.r, pass.g, pass.b);
            }
        }
    }
}

void BoardRenderer::blit_to(RgbaSurface& dst, const RgbaSurface& src,
                            int dst_x, int dst_y, int w, int h, int src_x, int src_y, BlitOp op) {
    if (!src.valid() || !dst.valid() || w <= 0 || h <= 0) return;

    // Clip rectangle upfront so the inner scanline loops have zero bounds-check branches
    int rx0 = 0;
    int ry0 = 0;
    if (dst_x < 0) rx0 = std::max(rx0, -dst_x);
    if (src_x < 0) rx0 = std::max(rx0, -src_x);
    if (dst_y < 0) ry0 = std::max(ry0, -dst_y);
    if (src_y < 0) ry0 = std::max(ry0, -src_y);

    int rx1 = w;
    int ry1 = h;
    rx1 = std::min(rx1, dst.width - dst_x);
    rx1 = std::min(rx1, src.width - src_x);
    ry1 = std::min(ry1, dst.height - dst_y);
    ry1 = std::min(ry1, src.height - src_y);

    if (rx0 >= rx1 || ry0 >= ry1) return;

    const int span_px = rx1 - rx0;
    const size_t span_bytes = static_cast<size_t>(span_px) * 4;

    if (op == BlitOp::SrcCopy) {
        for (int ry = ry0; ry < ry1; ++ry) {
            uint8_t* d_row = dst.pixels.data() + (static_cast<size_t>(dst_y + ry) * dst.width + (dst_x + rx0)) * 4;
            const uint8_t* s_row = src.pixels.data() + (static_cast<size_t>(src_y + ry) * src.width + (src_x + rx0)) * 4;
            std::memcpy(d_row, s_row, span_bytes);
            for (int x = 0; x < span_px; ++x) {
                d_row[x * 4 + 3] = 255;
            }
        }
    } else if (op == BlitOp::SrcAnd) {
        for (int ry = ry0; ry < ry1; ++ry) {
            uint8_t* d_row = dst.pixels.data() + (static_cast<size_t>(dst_y + ry) * dst.width + (dst_x + rx0)) * 4;
            const uint8_t* s_row = src.pixels.data() + (static_cast<size_t>(src_y + ry) * src.width + (src_x + rx0)) * 4;
            for (int x = 0; x < span_px; ++x) {
                d_row[x * 4 + 0] &= s_row[x * 4 + 0];
                d_row[x * 4 + 1] &= s_row[x * 4 + 1];
                d_row[x * 4 + 2] &= s_row[x * 4 + 2];
                d_row[x * 4 + 3] = 255;
            }
        }
    } else {
        for (int ry = ry0; ry < ry1; ++ry) {
            uint8_t* d_row = dst.pixels.data() + (static_cast<size_t>(dst_y + ry) * dst.width + (dst_x + rx0)) * 4;
            const uint8_t* s_row = src.pixels.data() + (static_cast<size_t>(src_y + ry) * src.width + (src_x + rx0)) * 4;
            for (int x = 0; x < span_px; ++x) {
                d_row[x * 4 + 0] |= s_row[x * 4 + 0];
                d_row[x * 4 + 1] |= s_row[x * 4 + 1];
                d_row[x * 4 + 2] |= s_row[x * 4 + 2];
                d_row[x * 4 + 3] = 255;
            }
        }
    }
}

void BoardRenderer::set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    size_t idx = (static_cast<size_t>(y) * WIDTH + x) * 4;
    canvas_.pixels[idx + 0] = r;
    canvas_.pixels[idx + 1] = g;
    canvas_.pixels[idx + 2] = b;
    canvas_.pixels[idx + 3] = 255;
}

void BoardRenderer::draw_hollow_rect_to(RgbaSurface& dst, int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b) {
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    auto put = [&](int x, int y) {
        if (x < 0 || x >= dst.width || y < 0 || y >= dst.height) return;
        size_t idx = (static_cast<size_t>(y) * dst.width + x) * 4;
        dst.pixels[idx + 0] = r;
        dst.pixels[idx + 1] = g;
        dst.pixels[idx + 2] = b;
        dst.pixels[idx + 3] = 255;
    };
    for (int x = x1; x <= x2; ++x) {
        put(x, y1);
        put(x, y2);
    }
    for (int y = y1; y <= y2; ++y) {
        put(x1, y);
        put(x2, y);
    }
}

void BoardRenderer::draw_hollow_rect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b) {
    draw_hollow_rect_to(canvas_, x1, y1, x2, y2, r, g, b);
}

void BoardRenderer::draw_thick_line(int x1, int y1, int x2, int y2, int width, uint8_t r, uint8_t g, uint8_t b) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int steps = std::max(dx, dy);
    int half_lo = -(width / 2);
    int half_hi = (width - 1) / 2;

    if (steps == 0) {
        for (int oy = half_lo; oy <= half_hi; ++oy) {
            for (int ox = half_lo; ox <= half_hi; ++ox) {
                set_pixel(x1 + ox, y1 + oy, r, g, b);
            }
        }
        return;
    }

    for (int s = 0; s <= steps; ++s) {
        int cx = x1 + (x2 - x1) * s / steps;
        int cy = y1 + (y2 - y1) * s / steps;
        for (int oy = half_lo; oy <= half_hi; ++oy) {
            for (int ox = half_lo; ox <= half_hi; ++ox) {
                set_pixel(cx + ox, cy + oy, r, g, b);
            }
        }
    }
}

void BoardRenderer::scale_to(int dst_w, int dst_h, core::ScaleFilter filter, RgbaSurface& out_surf) const {
    if (dst_w <= 0 || dst_h <= 0) return;
    if (dst_w == WIDTH && dst_h == HEIGHT) {
        out_surf = canvas_;
        return;
    }
    out_surf.resize(dst_w, dst_h);

    if (filter == core::ScaleFilter::Nearest) {
        for (int dy = 0; dy < dst_h; ++dy) {
            int sy = std::clamp(dy * HEIGHT / dst_h, 0, HEIGHT - 1);
            for (int dx = 0; dx < dst_w; ++dx) {
                int sx = std::clamp(dx * WIDTH / dst_w, 0, WIDTH - 1);
                size_t d_idx = (static_cast<size_t>(dy) * dst_w + dx) * 4;
                size_t s_idx = (static_cast<size_t>(sy) * WIDTH + sx) * 4;
                out_surf.pixels[d_idx + 0] = canvas_.pixels[s_idx + 0];
                out_surf.pixels[d_idx + 1] = canvas_.pixels[s_idx + 1];
                out_surf.pixels[d_idx + 2] = canvas_.pixels[s_idx + 2];
                out_surf.pixels[d_idx + 3] = 255;
            }
        }
    } else {
        // Bilinear filtering
        for (int dy = 0; dy < dst_h; ++dy) {
            float fy = (static_cast<float>(dy) + 0.5f) * static_cast<float>(HEIGHT) / static_cast<float>(dst_h) - 0.5f;
            int y0 = std::clamp(static_cast<int>(std::floor(fy)), 0, HEIGHT - 1);
            int y1 = std::clamp(y0 + 1, 0, HEIGHT - 1);
            float wy = std::clamp(fy - std::floor(fy), 0.0f, 1.0f);

            for (int dx = 0; dx < dst_w; ++dx) {
                float fx = (static_cast<float>(dx) + 0.5f) * static_cast<float>(WIDTH) / static_cast<float>(dst_w) - 0.5f;
                int x0 = std::clamp(static_cast<int>(std::floor(fx)), 0, WIDTH - 1);
                int x1 = std::clamp(x0 + 1, 0, WIDTH - 1);
                float wx = std::clamp(fx - std::floor(fx), 0.0f, 1.0f);

                size_t i00 = (static_cast<size_t>(y0) * WIDTH + x0) * 4;
                size_t i10 = (static_cast<size_t>(y0) * WIDTH + x1) * 4;
                size_t i01 = (static_cast<size_t>(y1) * WIDTH + x0) * 4;
                size_t i11 = (static_cast<size_t>(y1) * WIDTH + x1) * 4;

                size_t d_idx = (static_cast<size_t>(dy) * dst_w + dx) * 4;
                for (int c = 0; c < 3; ++c) {
                    float v0 = canvas_.pixels[i00 + c] * (1.0f - wx) + canvas_.pixels[i10 + c] * wx;
                    float v1 = canvas_.pixels[i01 + c] * (1.0f - wx) + canvas_.pixels[i11 + c] * wx;
                    float v = v0 * (1.0f - wy) + v1 * wy;
                    out_surf.pixels[d_idx + c] = static_cast<uint8_t>(std::clamp(std::lround(v), 0L, 255L));
                }
                out_surf.pixels[d_idx + 3] = 255;
            }
        }
    }
}

void BoardRenderer::compute_aspect_viewport(int win_w, int win_h, int& vp_x, int& vp_y, int& vp_w, int& vp_h) {
    if (win_w <= 0 || win_h <= 0) {
        vp_x = vp_y = vp_w = vp_h = 0;
        return;
    }
    if (win_w * HEIGHT <= win_h * WIDTH) {
        vp_w = win_w;
        vp_h = std::max(1, win_w * HEIGHT / WIDTH);
    } else {
        vp_h = win_h;
        vp_w = std::max(1, win_h * WIDTH / HEIGHT);
    }
    vp_x = (win_w - vp_w) / 2;
    vp_y = (win_h - vp_h) / 2;
}

} // namespace llk2::media
