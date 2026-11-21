#include "board.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace llk2::core {

Board::Board() {
    clear_all();
}

void Board::clear_all() {
    for (int c = 0; c < kCols; ++c) {
        for (int r = 0; r < kRows; ++r) {
            grid_[c][r] = BoardCell{};
        }
    }
    tiles_left_ = 0;
}

const BoardCell& Board::cell(int c, int r) const {
    if (c < 0 || c >= kCols || r < 0 || r >= kRows) {
        throw std::out_of_range("Board::cell index out of range");
    }
    return grid_[c][r];
}

BoardCell& Board::cell(int c, int r) {
    if (c < 0 || c >= kCols || r < 0 || r >= kRows) {
        throw std::out_of_range("Board::cell index out of range");
    }
    return grid_[c][r];
}

// === VB5 slot 01: InitBoard (0x4A8720 / 0x4A8730) ===
void Board::init_board(Vb5Rng& rng) {
    std::array<BoardCell, kTotalTiles> tmp{};
    const BoardCell empty_cell{};

    // PaletteOff = (Int(Rnd() * 5) + PaletteOff + 1) Mod 6
    palette_off_ = (rng.next_int(5) + palette_off_ + 1) % 6;
    tiles_left_ = kTotalTiles;

    for (int t = 0; t < kTotalTiles; ++t) {
        tmp[t].occupied = true;
        if (t < 136) {
            tmp[t].tile_id = static_cast<int16_t>(t / 4);
        } else {
            tmp[t].tile_id = static_cast<int16_t>(t - 102); // 34..41
        }
        tmp[t].match_id = static_cast<int16_t>(t / 4);      // 0..35 (34 & 35 = flower groups)
    }

    int k = 0;
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            if (r >= 1 && r <= 9 && c >= 1 && c <= 16) {
                grid_[c][r] = tmp[k++];
            } else {
                grid_[c][r] = empty_cell;
            }
        }
    }
}

// === VB5 slot 03: ShuffleRemaining (0x4A90B0 / 0x4A90C0) ===
void Board::shuffle_remaining(Vb5Rng& rng) {
    std::array<BoardCell, kTotalTiles> tmp{};
    int n = 0;
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            if (grid_[c][r].occupied) {
                if (n < kTotalTiles) {
                    tmp[n] = grid_[c][r];
                }
                ++n;
            }
        }
    }
    if (n <= 0) {
        return;
    }

    // VB5:
    // n = n - 1
    // For i = 0 To 79
    //     For j = 0 To n
    //         k = Int(Rnd() * n)
    //         swap tmp(k), tmp(j)
    int n_minus_1 = n - 1;
    for (int i = 0; i < 80; ++i) {
        for (int j = 0; j <= n_minus_1; ++j) {
            int k = rng.next_int(n_minus_1);
            std::swap(tmp[k], tmp[j]);
        }
    }

    int idx = 0;
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            if (grid_[c][r].occupied) {
                grid_[c][r] = tmp[idx++];
            }
        }
    }
}

// === VB5 slot 04: ShiftUp (0x4A94B0) ===
void Board::shift_up() {
    std::array<BoardCell, 10> cells{};
    const BoardCell empty_cell{};
    for (int c = 1; c <= 16; ++c) {
        int n = 0;
        for (int r = 1; r <= 9; ++r) {
            if (grid_[c][r].occupied) {
                ++n;
                cells[n] = grid_[c][r];
            }
        }
        if (n < 9 && n > 0) {
            int m = 1;
            for (int r = 1; r <= 9; ++r) {
                if (r > n) {
                    grid_[c][r] = empty_cell;
                } else {
                    grid_[c][r] = cells[m++];
                }
            }
        }
    }
}

// === VB5 slot 05: ShiftDown (0x4A9710) ===
void Board::shift_down() {
    std::array<BoardCell, 10> cells{};
    const BoardCell empty_cell{};
    for (int c = 1; c <= 16; ++c) {
        int n = 0;
        for (int r = 1; r <= 9; ++r) {
            if (grid_[c][r].occupied) {
                ++n;
                cells[n] = grid_[c][r];
            }
        }
        if (n < 9 && n > 0) {
            int m = 1;
            for (int r = 1; r <= 9; ++r) {
                if (r <= 9 - n) {
                    grid_[c][r] = empty_cell;
                } else {
                    grid_[c][r] = cells[m++];
                }
            }
        }
    }
}

// === VB5 slot 06: ShiftLeft (0x4A9980) ===
void Board::shift_left() {
    std::array<BoardCell, 17> cells{};
    const BoardCell empty_cell{};
    for (int r = 1; r <= 9; ++r) {
        int n = 0;
        for (int c = 1; c <= 16; ++c) {
            if (grid_[c][r].occupied) {
                ++n;
                cells[n] = grid_[c][r];
            }
        }
        if (n < 16 && n > 0) {
            int m = 1;
            for (int c = 1; c <= 16; ++c) {
                if (c > n) {
                    grid_[c][r] = empty_cell;
                } else {
                    grid_[c][r] = cells[m++];
                }
            }
        }
    }
}

// === VB5 slot 07: ShiftRight (0x4A9BE0) ===
void Board::shift_right() {
    std::array<BoardCell, 17> cells{};
    const BoardCell empty_cell{};
    for (int r = 1; r <= 9; ++r) {
        int n = 0;
        for (int c = 1; c <= 16; ++c) {
            if (grid_[c][r].occupied) {
                ++n;
                cells[n] = grid_[c][r];
            }
        }
        if (n < 16 && n > 0) {
            int m = 1;
            for (int c = 1; c <= 16; ++c) {
                if (c <= 16 - n) {
                    grid_[c][r] = empty_cell;
                } else {
                    grid_[c][r] = cells[m++];
                }
            }
        }
    }
}

// === VB5 slot 08: ShiftOutwardH (0x4A9E50) ===
void Board::shift_outward_h() {
    std::array<BoardCell, 17> cells{};
    const BoardCell empty_cell{};
    for (int r = 1; r <= 9; ++r) {
        int n = 0;
        for (int c = 1; c <= 8; ++c) {
            if (grid_[c][r].occupied) {
                ++n;
                cells[n] = grid_[c][r];
            }
        }
        if (n < 8 && n > 0) {
            int m = 1;
            for (int c = 1; c <= 8; ++c) {
                if (c <= n) {
                    grid_[c][r] = cells[m++];
                } else {
                    grid_[c][r] = empty_cell;
                }
            }
        }
    }
    for (int r = 1; r <= 9; ++r) {
        int n = 0;
        for (int c = 9; c <= 16; ++c) {
            if (grid_[c][r].occupied) {
                ++n;
                cells[n] = grid_[c][r];
            }
        }
        if (n < 8 && n > 0) {
            int m = 1;
            for (int c = 1; c <= 8; ++c) {
                if (c > 8 - n) {
                    grid_[8 + c][r] = cells[m++];
                } else {
                    grid_[8 + c][r] = empty_cell;
                }
            }
        }
    }
}

// === VB5 slot 09: ShiftInwardH (0x4AA4F0) ===
void Board::shift_inward_h() {
    std::array<BoardCell, 17> cells{};
    const BoardCell empty_cell{};
    for (int r = 1; r <= 9; ++r) {
        int n = 0;
        for (int c = 1; c <= 8; ++c) {
            if (grid_[c][r].occupied) {
                ++n;
                cells[n] = grid_[c][r];
            }
        }
        if (n < 8 && n > 0) {
            int m = 1;
            for (int c = 1; c <= 8; ++c) {
                if (c > 8 - n) {
                    grid_[c][r] = cells[m++];
                } else {
                    grid_[c][r] = empty_cell;
                }
            }
        }
    }
    for (int r = 1; r <= 9; ++r) {
        int n = 0;
        for (int c = 9; c <= 16; ++c) {
            if (grid_[c][r].occupied) {
                ++n;
                cells[n] = grid_[c][r];
            }
        }
        if (n < 8 && n > 0) {
            int m = 1;
            for (int c = 1; c <= 8; ++c) {
                if (c <= n) {
                    grid_[8 + c][r] = cells[m++];
                } else {
                    grid_[8 + c][r] = empty_cell;
                }
            }
        }
    }
}

void Board::apply_level_shift(int level) {
    switch (level) {
        case 1: shift_up(); break;
        case 2: shift_down(); break;
        case 3: shift_left(); break;
        case 4: shift_right(); break;
        case 5: shift_outward_h(); break;
        case 6: shift_inward_h(); break;
        default: break;
    }
}

// === VB5 slot 13: HasValidMove (0x4AE4D0 / 0x4AE590) ===
bool Board::has_valid_move(LinkPath* out_path) {
    int idx = -1;
    for (int r = 0; r <= 10; ++r) {
        for (int c = 0; c <= 17; ++c) {
            ++idx;
            if (grid_[c][r].occupied) {
                if (idx < 197) {
                    for (int k = idx + 1; k <= 197; ++k) {
                        int c2 = k % 18;
                        int r2 = k / 18;
                        if (grid_[c2][r2].occupied) {
                            if (grid_[c][r].match_id == grid_[c2][r2].match_id) {
                                if (check_link(c, r, c2, r2, out_path)) {
                                    hint_x1_ = static_cast<float>(c);
                                    hint_y1_ = static_cast<float>(r);
                                    hint_x2_ = static_cast<float>(c2);
                                    hint_y2_ = static_cast<float>(r2);
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

// === VB5 slot 14: CheckLink (0x4AE690 / 0x4AE7C0) ===
bool Board::check_link(int x1, int y1, int x2, int y2, LinkPath* out_path) const {
    if (x1 > x2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    bool ok = false;
    if (x1 == x2) {
        if (y1 > y2) {
            std::swap(y1, y2);
        }
        ok = check_link0(x1, y1, x2, y2, out_path);
    } else if (x1 < x2) {
        if (y1 == y2) {
            ok = check_link1(x1, y1, x2, y2, out_path);
        } else if (y1 < y2) {
            ok = check_link2(x1, y1, x2, y2, out_path);
        } else {
            ok = check_link3(x1, y1, x2, y2, out_path);
        }
    }
    return ok;
}

// === VB5 slot 15: CheckLink0 — same column (x1 == x2, y1 <= y2) ===
bool Board::check_link0(int x1, int y1, int x2, int y2, LinkPath* out_path) const {
    auto set_out = [&](const LinkPath& p) {
        last_link_ = p;
        if (out_path) {
            *out_path = p;
        }
    };

    int cnt = 0;
    for (int i = y1; i <= y2; ++i) {
        if (grid_[x1][i].occupied) ++cnt;
    }
    if (cnt == 2) {
        set_out(LinkPath{1, float(x1), float(y1), float(x2), float(y2), 0, 0, 0, 0});
        return true;
    }

    for (int m = x1 - 1; m >= 0; --m) {
        cnt = 0;
        for (int i = m; i <= x1; ++i) {
            if (grid_[i][y1].occupied) ++cnt;
            if (grid_[i][y2].occupied) ++cnt;
        }
        for (int i = y1; i <= y2; ++i) {
            if (grid_[m][i].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(m), float(y1), float(m), float(y2), float(x2), float(y2)});
            return true;
        }
    }

    for (int m = x1 + 1; m <= 17; ++m) {
        cnt = 0;
        for (int i = x1; i <= m; ++i) {
            if (grid_[i][y1].occupied) ++cnt;
            if (grid_[i][y2].occupied) ++cnt;
        }
        for (int i = y1; i <= y2; ++i) {
            if (grid_[m][i].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(m), float(y1), float(m), float(y2), float(x2), float(y2)});
            return true;
        }
    }
    return false;
}

// === VB5 slot 16: CheckLink1 — same row (y1 == y2, x1 < x2) ===
bool Board::check_link1(int x1, int y1, int x2, int y2, LinkPath* out_path) const {
    auto set_out = [&](const LinkPath& p) {
        last_link_ = p;
        if (out_path) {
            *out_path = p;
        }
    };

    int cnt = 0;
    for (int i = x1; i <= x2; ++i) {
        if (grid_[i][y1].occupied) ++cnt;
    }
    if (cnt == 2) {
        set_out(LinkPath{1, float(x1), float(y1), float(x2), float(y2), 0, 0, 0, 0});
        return true;
    }

    for (int m = y1 - 1; m >= 0; --m) {
        cnt = 0;
        for (int i = m; i <= y1; ++i) {
            if (grid_[x1][i].occupied) ++cnt;
            if (grid_[x2][i].occupied) ++cnt;
        }
        for (int i = x1; i <= x2; ++i) {
            if (grid_[i][m].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(x1), float(m), float(x2), float(m), float(x2), float(y2)});
            return true;
        }
    }

    for (int m = y1 + 1; m <= 10; ++m) {
        cnt = 0;
        for (int i = y1; i <= m; ++i) {
            if (grid_[x1][i].occupied) ++cnt;
            if (grid_[x2][i].occupied) ++cnt;
        }
        for (int i = x1; i <= x2; ++i) {
            if (grid_[i][m].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(x1), float(m), float(x2), float(m), float(x2), float(y2)});
            return true;
        }
    }
    return false;
}

// === VB5 slot 17: CheckLink2 — x1 < x2, y1 < y2 ===
bool Board::check_link2(int x1, int y1, int x2, int y2, LinkPath* out_path) const {
    auto set_out = [&](const LinkPath& p) {
        last_link_ = p;
        if (out_path) {
            *out_path = p;
        }
    };

    // 1-corner path through (x2, y1)
    int cnt = 0;
    for (int i = x1; i <= x2; ++i) {
        if (grid_[i][y1].occupied) ++cnt;
    }
    for (int i = y1; i <= y2; ++i) {
        if (grid_[x2][i].occupied) ++cnt;
    }
    if (cnt == 2) {
        set_out(LinkPath{2, float(x1), float(y1), float(x2), float(y1), float(x2), float(y2), 0, 0});
        return true;
    }

    // 1-corner path through (x1, y2)
    cnt = 0;
    for (int i = y1; i <= y2; ++i) {
        if (grid_[x1][i].occupied) ++cnt;
    }
    for (int i = x1; i <= x2; ++i) {
        if (grid_[i][y2].occupied) ++cnt;
    }
    if (cnt == 2) {
        set_out(LinkPath{2, float(x1), float(y1), float(x1), float(y2), float(x2), float(y2), 0, 0});
        return true;
    }

    // Stage 1: interior columns x1 + 1 .. x2 - 1
    if (x2 - x1 > 1) {
        for (int m = x1 + 1; m <= x2 - 1; ++m) {
            cnt = 0;
            for (int i = x1; i <= m; ++i) {
                if (grid_[i][y1].occupied) ++cnt;
            }
            for (int i = m; i <= x2; ++i) {
                if (grid_[i][y2].occupied) ++cnt;
            }
            for (int i = y1; i <= y2; ++i) {
                if (grid_[m][i].occupied) ++cnt;
            }
            if (cnt == 2) {
                set_out(LinkPath{3, float(x1), float(y1), float(m), float(y1), float(m), float(y2), float(x2), float(y2)});
                return true;
            }
        }
    }

    // Stage 2: interior rows y1 + 1 .. y2 - 1
    if (y2 - y1 > 1) {
        for (int m = y1 + 1; m <= y2 - 1; ++m) {
            cnt = 0;
            for (int i = y1; i <= m; ++i) {
                if (grid_[x1][i].occupied) ++cnt;
            }
            for (int i = m; i <= y2; ++i) {
                if (grid_[x2][i].occupied) ++cnt;
            }
            for (int i = x1; i <= x2; ++i) {
                if (grid_[i][m].occupied) ++cnt;
            }
            if (cnt == 2) {
                set_out(LinkPath{3, float(x1), float(y1), float(x1), float(m), float(x2), float(m), float(x2), float(y2)});
                return true;
            }
        }
    }

    // Stage 3: rows above y1 (y1 - 1 down to 0)
    for (int m = y1 - 1; m >= 0; --m) {
        cnt = 0;
        for (int i = m; i <= y1; ++i) {
            if (grid_[x1][i].occupied) ++cnt;
        }
        for (int i = m; i <= y2; ++i) {
            if (grid_[x2][i].occupied) ++cnt;
        }
        for (int i = x1; i <= x2; ++i) {
            if (grid_[i][m].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(x1), float(m), float(x2), float(m), float(x2), float(y2)});
            return true;
        }
    }

    // Stage 4: rows below y2 (y2 + 1 up to 10)
    for (int m = y2 + 1; m <= 10; ++m) {
        cnt = 0;
        for (int i = y1; i <= m; ++i) {
            if (grid_[x1][i].occupied) ++cnt;
        }
        for (int i = y2; i <= m; ++i) {
            if (grid_[x2][i].occupied) ++cnt;
        }
        for (int i = x1; i <= x2; ++i) {
            if (grid_[i][m].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(x1), float(m), float(x2), float(m), float(x2), float(y2)});
            return true;
        }
    }

    // Stage 5: columns left of x1 (x1 - 1 down to 0)
    for (int m = x1 - 1; m >= 0; --m) {
        cnt = 0;
        for (int i = m; i <= x1; ++i) {
            if (grid_[i][y1].occupied) ++cnt;
        }
        for (int i = m; i <= x2; ++i) {
            if (grid_[i][y2].occupied) ++cnt;
        }
        for (int i = y1; i <= y2; ++i) {
            if (grid_[m][i].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(m), float(y1), float(m), float(y2), float(x2), float(y2)});
            return true;
        }
    }

    // Stage 6: columns right of x2 (x2 + 1 up to 17)
    for (int m = x2 + 1; m <= 17; ++m) {
        cnt = 0;
        for (int i = x1; i <= m; ++i) {
            if (grid_[i][y1].occupied) ++cnt;
        }
        for (int i = x2; i <= m; ++i) {
            if (grid_[i][y2].occupied) ++cnt;
        }
        for (int i = y1; i <= y2; ++i) {
            if (grid_[m][i].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(m), float(y1), float(m), float(y2), float(x2), float(y2)});
            return true;
        }
    }
    return false;
}

// === VB5 slot 18: CheckLink3 — x1 < x2, y1 > y2 ===
bool Board::check_link3(int x1, int y1, int x2, int y2, LinkPath* out_path) const {
    auto set_out = [&](const LinkPath& p) {
        last_link_ = p;
        if (out_path) {
            *out_path = p;
        }
    };

    // 1-corner path through (x2, y1)
    int cnt = 0;
    for (int i = x1; i <= x2; ++i) {
        if (grid_[i][y1].occupied) ++cnt;
    }
    for (int i = y2; i <= y1; ++i) {
        if (grid_[x2][i].occupied) ++cnt;
    }
    if (cnt == 2) {
        set_out(LinkPath{2, float(x1), float(y1), float(x2), float(y1), float(x2), float(y2), 0, 0});
        return true;
    }

    // 1-corner path through (x1, y2)
    cnt = 0;
    for (int i = y2; i <= y1; ++i) {
        if (grid_[x1][i].occupied) ++cnt;
    }
    for (int i = x1; i <= x2; ++i) {
        if (grid_[i][y2].occupied) ++cnt;
    }
    if (cnt == 2) {
        set_out(LinkPath{2, float(x1), float(y1), float(x1), float(y2), float(x2), float(y2), 0, 0});
        return true;
    }

    // Stage 1: interior columns x1 + 1 .. x2 - 1
    if (x2 - x1 > 1) {
        for (int m = x1 + 1; m <= x2 - 1; ++m) {
            cnt = 0;
            for (int i = x1; i <= m; ++i) {
                if (grid_[i][y1].occupied) ++cnt;
            }
            for (int i = m; i <= x2; ++i) {
                if (grid_[i][y2].occupied) ++cnt;
            }
            for (int i = y2; i <= y1; ++i) {
                if (grid_[m][i].occupied) ++cnt;
            }
            if (cnt == 2) {
                set_out(LinkPath{3, float(x1), float(y1), float(m), float(y1), float(m), float(y2), float(x2), float(y2)});
                return true;
            }
        }
    }

    // Stage 2: interior rows y2 + 1 .. y1 - 1
    if (y1 - y2 > 1) {
        for (int m = y2 + 1; m <= y1 - 1; ++m) {
            cnt = 0;
            for (int i = m; i <= y1; ++i) {
                if (grid_[x1][i].occupied) ++cnt;
            }
            for (int i = y2; i <= m; ++i) {
                if (grid_[x2][i].occupied) ++cnt;
            }
            for (int i = x1; i <= x2; ++i) {
                if (grid_[i][m].occupied) ++cnt;
            }
            if (cnt == 2) {
                set_out(LinkPath{3, float(x1), float(y1), float(x1), float(m), float(x2), float(m), float(x2), float(y2)});
                return true;
            }
        }
    }

    // Stage 3: rows above y2 (y2 - 1 down to 0)
    for (int m = y2 - 1; m >= 0; --m) {
        cnt = 0;
        for (int i = m; i <= y1; ++i) {
            if (grid_[x1][i].occupied) ++cnt;
        }
        for (int i = m; i <= y2; ++i) {
            if (grid_[x2][i].occupied) ++cnt;
        }
        for (int i = x1; i <= x2; ++i) {
            if (grid_[i][m].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(x1), float(m), float(x2), float(m), float(x2), float(y2)});
            return true;
        }
    }

    // Stage 4: rows below y1 (y1 + 1 up to 10)
    for (int m = y1 + 1; m <= 10; ++m) {
        cnt = 0;
        for (int i = y1; i <= m; ++i) {
            if (grid_[x1][i].occupied) ++cnt;
        }
        for (int i = y2; i <= m; ++i) {
            if (grid_[x2][i].occupied) ++cnt;
        }
        for (int i = x1; i <= x2; ++i) {
            if (grid_[i][m].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(x1), float(m), float(x2), float(m), float(x2), float(y2)});
            return true;
        }
    }

    // Stage 5: columns left of x1 (x1 - 1 down to 0)
    for (int m = x1 - 1; m >= 0; --m) {
        cnt = 0;
        for (int i = m; i <= x1; ++i) {
            if (grid_[i][y1].occupied) ++cnt;
        }
        for (int i = m; i <= x2; ++i) {
            if (grid_[i][y2].occupied) ++cnt;
        }
        for (int i = y2; i <= y1; ++i) {
            if (grid_[m][i].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(m), float(y1), float(m), float(y2), float(x2), float(y2)});
            return true;
        }
    }

    // Stage 6: columns right of x2 (x2 + 1 up to 17)
    for (int m = x2 + 1; m <= 17; ++m) {
        cnt = 0;
        for (int i = x1; i <= m; ++i) {
            if (grid_[i][y1].occupied) ++cnt;
        }
        for (int i = x2; i <= m; ++i) {
            if (grid_[i][y2].occupied) ++cnt;
        }
        for (int i = y2; i <= y1; ++i) {
            if (grid_[m][i].occupied) ++cnt;
        }
        if (cnt == 2) {
            set_out(LinkPath{3, float(x1), float(y1), float(m), float(y1), float(m), float(y2), float(x2), float(y2)});
            return true;
        }
    }
    return false;
}

} // namespace llk2::core
