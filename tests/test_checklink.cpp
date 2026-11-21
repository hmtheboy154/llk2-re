#include "core/board.hpp"
#include "core/rng.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <map>

using namespace llk2::core;

static void test_init_board_distribution() {
    Vb5Rng rng(123456);
    Board b;
    b.init_board(rng);

    assert(b.tiles_left() == Board::kTotalTiles);

    int active_count = 0;
    std::map<int, int> match_counts;
    std::map<int, int> tile_counts;

    for (int r = 0; r < Board::kRows; ++r) {
        for (int c = 0; c < Board::kCols; ++c) {
            const auto& cell = b.cell(c, r);
            bool inside = (r >= 1 && r <= 9 && c >= 1 && c <= 16);
            if (!inside) {
                assert(!cell.occupied);
            } else {
                assert(cell.occupied);
                ++active_count;
                match_counts[cell.match_id]++;
                tile_counts[cell.tile_id]++;
                if (cell.tile_id >= 0 && cell.tile_id <= 33) {
                    assert(cell.match_id == cell.tile_id);
                } else if (cell.tile_id >= 34 && cell.tile_id <= 37) {
                    assert(cell.match_id == 34);
                } else if (cell.tile_id >= 38 && cell.tile_id <= 41) {
                    assert(cell.match_id == 35);
                } else {
                    assert(false && "Invalid tile_id");
                }
            }
        }
    }
    assert(active_count == 144);
    for (int m = 0; m <= 35; ++m) {
        assert(match_counts[m] == 4);
    }
    for (int t = 34; t <= 41; ++t) {
        assert(tile_counts[t] == 1);
    }
}

static void test_checklink0_vertical_and_uturn() {
    Board b;
    b.clear_all();

    // 1. Direct vertical adjacency (0 turns)
    b.cell(5, 3) = {true, 10, 10};
    b.cell(5, 4) = {true, 10, 10};
    LinkPath path;
    assert(b.check_link0(5, 3, 5, 4, &path));
    assert(path.link_n == 1);
    assert(static_cast<int>(std::lround(path.x1)) == 5 && static_cast<int>(std::lround(path.y1)) == 3);
    assert(static_cast<int>(std::lround(path.x2)) == 5 && static_cast<int>(std::lround(path.y2)) == 4);

    // 2. Direct vertical with clear intermediate cells (0 turns)
    b.clear_all();
    b.cell(5, 2) = {true, 10, 10};
    b.cell(5, 7) = {true, 10, 10};
    assert(b.check_link0(5, 2, 5, 7, &path));
    assert(path.link_n == 1);

    // 3. Direct vertical blocked -> must take 2-turn horizontal detour
    b.cell(5, 4) = {true, 20, 20}; // blocker
    assert(b.check_link0(5, 2, 5, 7, &path));
    assert(path.link_n == 3);

    // 4. Exterior border U-turn via col 0 when cols 1..17 are blocked
    b.clear_all();
    b.cell(3, 2) = {true, 10, 10};
    b.cell(3, 5) = {true, 10, 10};
    for (int c = 1; c <= 17; ++c) {
        b.cell(c, 3) = {true, 99, 99};
    }
    // Only col 0 is open at row 3
    assert(b.check_link0(3, 2, 3, 5, &path));
    assert(path.link_n == 3);
    assert(static_cast<int>(std::lround(path.x2)) == 0 && static_cast<int>(std::lround(path.y2)) == 2);
    assert(static_cast<int>(std::lround(path.x3)) == 0 && static_cast<int>(std::lround(path.y3)) == 5);
}

static void test_checklink1_horizontal_and_uturn() {
    Board b;
    b.clear_all();

    // 1. Direct horizontal adjacency (0 turns)
    b.cell(4, 5) = {true, 12, 12};
    b.cell(5, 5) = {true, 12, 12};
    LinkPath path;
    assert(b.check_link1(4, 5, 5, 5, &path));
    assert(path.link_n == 1);

    // 2. Direct horizontal blocked -> exterior border U-turn via row 10
    b.clear_all();
    b.cell(3, 8) = {true, 12, 12};
    b.cell(10, 8) = {true, 12, 12};
    for (int r = 0; r <= 9; ++r) {
        b.cell(6, r) = {true, 99, 99};
    }
    // Only row 10 is open across col 6
    assert(b.check_link1(3, 8, 10, 8, &path));
    assert(path.link_n == 3);
    assert(static_cast<int>(std::lround(path.x2)) == 3 && static_cast<int>(std::lround(path.y2)) == 10);
    assert(static_cast<int>(std::lround(path.x3)) == 10 && static_cast<int>(std::lround(path.y3)) == 10);
}

static void test_checklink2_lpath_and_zpath() {
    Board b;
    b.clear_all();

    // 1-turn L-path between (2, 2) and (5, 6)
    b.cell(2, 2) = {true, 7, 7};
    b.cell(5, 6) = {true, 7, 7};
    LinkPath path;
    assert(b.check_link(2, 2, 5, 6, &path));
    assert(path.link_n == 2);

    // Block both L-corners (2, 6) and (5, 2) -> forces 2-turn Z-path
    b.cell(2, 6) = {true, 99, 99};
    b.cell(5, 2) = {true, 99, 99};
    assert(b.check_link(2, 2, 5, 6, &path));
    assert(path.link_n == 3);

    // Surround (2, 2) completely -> no link possible
    b.cell(1, 2) = {true, 99, 99};
    b.cell(3, 2) = {true, 99, 99};
    b.cell(2, 1) = {true, 99, 99};
    b.cell(2, 3) = {true, 99, 99};
    assert(!b.check_link(2, 2, 5, 6, &path));
}

static void test_has_valid_move_and_shuffle() {
    Board b;
    b.clear_all();
    b.cell(2, 2) = {true, 34, 34}; // Spring flower
    b.cell(3, 2) = {true, 37, 34}; // Winter flower (same match_id 34!)
    b.set_tiles_left(2);

    LinkPath path;
    assert(b.has_valid_move(&path));
    assert(static_cast<int>(std::lround(b.hint_x1())) == 2 && static_cast<int>(std::lround(b.hint_y1())) == 2);
    assert(static_cast<int>(std::lround(b.hint_x2())) == 3 && static_cast<int>(std::lround(b.hint_y2())) == 2);
    assert(path.link_n == 1);

    Vb5Rng rng(42);
    b.shuffle_remaining(rng);
    assert(b.cell(2, 2).occupied);
    assert(b.cell(3, 2).occupied);
    assert(b.cell(2, 2).match_id == 34);
    assert(b.cell(3, 2).match_id == 34);
}

int main() {
    test_init_board_distribution();
    test_checklink0_vertical_and_uturn();
    test_checklink1_horizontal_and_uturn();
    test_checklink2_lpath_and_zpath();
    test_has_valid_move_and_shuffle();
    std::printf("[PASS] test_checklink\n");
    return 0;
}
