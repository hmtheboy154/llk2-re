#include "core/board.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>

using namespace llk2::core;

static void test_shift_down() {
    Board b;
    b.clear_all();
    // Column 4: tiles at row 3 and row 7, empty at row 9 (playable rows 1..9)
    b.cell(4, 3) = {true, 11, 11};
    b.cell(4, 7) = {true, 22, 22};

    b.shift_down();
    assert(b.cell(4, 9).occupied && b.cell(4, 9).tile_id == 22);
    assert(b.cell(4, 8).occupied && b.cell(4, 8).tile_id == 11);
    assert(!b.cell(4, 7).occupied);
    assert(!b.cell(4, 3).occupied);
}

static void test_shift_up() {
    Board b;
    b.clear_all();
    // Column 5: tiles at row 5 and row 9 -> packs up to rows 1, 2
    b.cell(5, 5) = {true, 15, 15};
    b.cell(5, 9) = {true, 25, 25};

    b.shift_up();
    assert(b.cell(5, 1).occupied && b.cell(5, 1).tile_id == 15);
    assert(b.cell(5, 2).occupied && b.cell(5, 2).tile_id == 25);
    assert(!b.cell(5, 5).occupied);
    assert(!b.cell(5, 9).occupied);
}

static void test_shift_left() {
    Board b;
    b.clear_all();
    // Row 4: tiles at col 6 and col 12 -> packs left to cols 1, 2
    b.cell(6, 4) = {true, 3, 3};
    b.cell(12, 4) = {true, 8, 8};

    b.shift_left();
    assert(b.cell(1, 4).occupied && b.cell(1, 4).tile_id == 3);
    assert(b.cell(2, 4).occupied && b.cell(2, 4).tile_id == 8);
    assert(!b.cell(6, 4).occupied);
    assert(!b.cell(12, 4).occupied);
}

static void test_shift_right() {
    Board b;
    b.clear_all();
    // Row 6: tiles at col 3 and col 10 -> packs right to cols 15, 16
    b.cell(3, 6) = {true, 4, 4};
    b.cell(10, 6) = {true, 9, 9};

    b.shift_right();
    assert(b.cell(16, 6).occupied && b.cell(16, 6).tile_id == 9);
    assert(b.cell(15, 6).occupied && b.cell(15, 6).tile_id == 4);
    assert(!b.cell(3, 6).occupied);
    assert(!b.cell(10, 6).occupied);
}

static void test_shift_outward_h() {
    Board b;
    b.clear_all();
    // Row 5: left half (cols 1..8) has tiles at 5 and 8 -> packs left to 1, 2
    // right half (cols 9..16) has tiles at 9 and 12 -> packs right to 15, 16
    b.cell(5, 5) = {true, 1, 1};
    b.cell(8, 5) = {true, 2, 2};
    b.cell(9, 5) = {true, 3, 3};
    b.cell(12, 5) = {true, 4, 4};

    b.shift_outward_h();
    assert(b.cell(1, 5).occupied && b.cell(1, 5).tile_id == 1);
    assert(b.cell(2, 5).occupied && b.cell(2, 5).tile_id == 2);
    assert(!b.cell(5, 5).occupied);
    assert(!b.cell(8, 5).occupied);

    assert(b.cell(16, 5).occupied && b.cell(16, 5).tile_id == 4);
    assert(b.cell(15, 5).occupied && b.cell(15, 5).tile_id == 3);
    assert(!b.cell(9, 5).occupied);
    assert(!b.cell(12, 5).occupied);
}

static void test_shift_inward_h() {
    Board b;
    b.clear_all();
    // Row 5: left half (cols 1..8) has tiles at 2 and 5 -> packs right toward center col 8 (cols 7, 8)
    // right half (cols 9..16) has tiles at 12 and 15 -> packs left toward center col 9 (cols 9, 10)
    b.cell(2, 5) = {true, 1, 1};
    b.cell(5, 5) = {true, 2, 2};
    b.cell(12, 5) = {true, 3, 3};
    b.cell(15, 5) = {true, 4, 4};

    b.shift_inward_h();
    assert(b.cell(8, 5).occupied && b.cell(8, 5).tile_id == 2);
    assert(b.cell(7, 5).occupied && b.cell(7, 5).tile_id == 1);
    assert(!b.cell(2, 5).occupied);
    assert(!b.cell(5, 5).occupied);

    assert(b.cell(9, 5).occupied && b.cell(9, 5).tile_id == 3);
    assert(b.cell(10, 5).occupied && b.cell(10, 5).tile_id == 4);
    assert(!b.cell(12, 5).occupied);
    assert(!b.cell(15, 5).occupied);
}

int main() {
    test_shift_down();
    test_shift_up();
    test_shift_left();
    test_shift_right();
    test_shift_outward_h();
    test_shift_inward_h();
    std::printf("[PASS] test_shifts\n");
    return 0;
}
