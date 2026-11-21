#pragma once

#include "rng.hpp"
#include <array>
#include <cstdint>

namespace llk2::core {

struct BoardCell {
    bool occupied = false;  // VB5: -1 = occupied, 0 = empty
    int16_t tile_id = 0;    // Visual face ID 0..41 (34..41 = flower sprites)
    int16_t match_id = 0;   // Match equivalence class 0..35 (34..37 -> 34, 38..41 -> 35)
};

struct LinkPath {
    int16_t link_n = 0;     // Number of segments: 1 (0 corners), 2 (1 corner), 3 (2 corners)
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;
    float x3 = 0.0f;
    float y3 = 0.0f;
    float x4 = 0.0f;
    float y4 = 0.0f;
};

class Board {
public:
    static constexpr int kCols = 18;         // 0..17 (playable: 1..16)
    static constexpr int kRows = 11;         // 0..10 (playable: 1..9)
    static constexpr int kPlayCols = 16;
    static constexpr int kPlayRows = 9;
    static constexpr int kTotalTiles = 144;  // 16 * 9

    Board();

    // VB5 slot 01: InitBoard (fills 144 tiles, advances PaletteOff, resets TilesLeft=144)
    void init_board(Vb5Rng& rng);

    // VB5 slot 03: ShuffleRemaining (80-pass shuffle of all remaining occupied cells)
    void shuffle_remaining(Vb5Rng& rng);

    // VB5 slot 13: HasValidMove (scans all cell pairs; caches HintX1..Y2 when found)
    bool has_valid_move(LinkPath* out_path = nullptr);

    // VB5 slots 14..18: CheckLink & CheckLink0..3
    bool check_link(int x1, int y1, int x2, int y2, LinkPath* out_path = nullptr) const;
    bool check_link0(int x1, int y1, int x2, int y2, LinkPath* out_path = nullptr) const;
    bool check_link1(int x1, int y1, int x2, int y2, LinkPath* out_path = nullptr) const;
    bool check_link2(int x1, int y1, int x2, int y2, LinkPath* out_path = nullptr) const;
    bool check_link3(int x1, int y1, int x2, int y2, LinkPath* out_path = nullptr) const;

    // VB5 slots 04..09: Gravity shifts for Levels 1..6
    void shift_up();        // Level 1
    void shift_down();      // Level 2
    void shift_left();      // Level 3
    void shift_right();     // Level 4
    void shift_outward_h(); // Level 5
    void shift_inward_h();  // Level 6
    void apply_level_shift(int level);

    void clear_all();

    const BoardCell& cell(int c, int r) const;
    BoardCell& cell(int c, int r);

    int palette_off() const noexcept { return palette_off_; }
    void set_palette_off(int p) noexcept { palette_off_ = ((p % 6) + 6) % 6; }

    int tiles_left() const noexcept { return tiles_left_; }
    void set_tiles_left(int t) noexcept { tiles_left_ = t; }

    float hint_x1() const noexcept { return hint_x1_; }
    float hint_y1() const noexcept { return hint_y1_; }
    float hint_x2() const noexcept { return hint_x2_; }
    float hint_y2() const noexcept { return hint_y2_; }

    const LinkPath& last_link() const noexcept { return last_link_; }

private:
    std::array<std::array<BoardCell, kRows>, kCols> grid_{};
    int palette_off_ = 0;
    int tiles_left_ = 0;
    float hint_x1_ = -1.0f;
    float hint_y1_ = -1.0f;
    float hint_x2_ = -1.0f;
    float hint_y2_ = -1.0f;
    mutable LinkPath last_link_{};
};

} // namespace llk2::core
