#include "game_state.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>

namespace llk2::core {

GameState::GameState() {
    randomize_from_clock();
}

void GameState::randomize_from_clock() noexcept {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms_since_epoch = duration_cast<milliseconds>(now.time_since_epoch()).count();
    // Seconds since midnight (0.0 .. 86400.0), matching VB5 `Randomize Timer`
    double sec_since_midnight = static_cast<double>(ms_since_epoch % 86400000LL) / 1000.0;
    rng_.randomize(sec_since_midnight);
}

void GameState::clear_selection() noexcept {
    sel1_c_ = -1;
    sel1_r_ = -1;
    sel2_c_ = -1;
    sel2_r_ = -1;
    update_hover_from_last_mouse();
}

void GameState::update_hover_from_last_mouse() noexcept {
    if (!board_active_ || in_title_screen_ || awaiting_modal_) {
        hover_c_ = -1;
        hover_r_ = -1;
        return;
    }
    if (cursor_active_) {
        if (cursor_c_ >= 1 && cursor_c_ <= Board::kPlayCols &&
            cursor_r_ >= 1 && cursor_r_ <= Board::kPlayRows &&
            board_.cell(cursor_c_, cursor_r_).occupied) {
            hover_c_ = cursor_c_;
            hover_r_ = cursor_r_;
            return;
        }
        int pref_c = (cursor_c_ >= 1 && cursor_c_ <= Board::kPlayCols) ? cursor_c_ : 1;
        int pref_r = (cursor_r_ >= 1 && cursor_r_ <= Board::kPlayRows) ? cursor_r_ : 1;
        if (snap_cursor_to_nearest_occupied(pref_c, pref_r)) {
            hover_c_ = cursor_c_;
            hover_r_ = cursor_r_;
            return;
        }
        hover_c_ = -1;
        hover_r_ = -1;
        return;
    }
    float dx = last_mouse_x_ - static_cast<float>(kOffX);
    float dy = last_mouse_y_ - static_cast<float>(kOffY);
    if (dx > 0.0f && dx < 720.0f && dy > 0.0f && dy < 550.0f) {
        int col = static_cast<int>(dx) / 40;
        int row = static_cast<int>(dy) / 50;
        if (col >= 0 && col < Board::kCols && row >= 0 && row < Board::kRows && board_.cell(col, row).occupied) {
            hover_c_ = col;
            hover_r_ = row;
            return;
        }
    }
    hover_c_ = -1;
    hover_r_ = -1;
}

bool GameState::snap_cursor_to_nearest_occupied(int pref_c, int pref_r) noexcept {
    int best_c = -1;
    int best_r = -1;
    int best_dist = 1000000;
    for (int r = 1; r <= Board::kPlayRows; ++r) {
        for (int c = 1; c <= Board::kPlayCols; ++c) {
            if (!board_.cell(c, r).occupied) continue;
            int d = std::abs(c - pref_c) + std::abs(r - pref_r);
            if (d < best_dist) {
                best_dist = d;
                best_c = c;
                best_r = r;
            }
        }
    }
    if (best_c < 0) {
        cursor_c_ = -1;
        cursor_r_ = -1;
        return false;
    }
    cursor_c_ = best_c;
    cursor_r_ = best_r;
    return true;
}

void GameState::init_current_stage_board() {
    board_.init_board(rng_);
    do {
        board_.shuffle_remaining(rng_);
    } while (!board_.has_valid_move());
    board_active_ = true;
    in_title_screen_ = false;
    awaiting_modal_ = false;
    animating_link_ = false;
    last_tick_ms_ = 0.0f;
    clear_selection();
}

void GameState::refill_endless_queue() {
    endless_queue_ = endless_pool_;
    if (endless_queue_.empty()) {
        endless_queue_.push_back(0);
    }
    if (endless_cfg_.shuffle && endless_queue_.size() > 1) {
        // Fisher-Yates bag shuffle using rng_
        for (size_t i = endless_queue_.size() - 1; i > 0; --i) {
            size_t j = static_cast<size_t>(rng_.next_float() * static_cast<float>(i + 1));
            if (j > i) j = i;
            std::swap(endless_queue_[i], endless_queue_[j]);
        }
        // Avoid repeating the immediately previous stage across bag boundaries
        if (last_endless_level_ >= 0 && endless_queue_.front() == last_endless_level_) {
            std::swap(endless_queue_.front(), endless_queue_.back());
        }
    }
}

int GameState::peek_next_endless_level() {
    if (endless_queue_.empty()) {
        refill_endless_queue();
    }
    return endless_queue_.front();
}

int GameState::pop_next_endless_level() {
    if (endless_queue_.empty()) {
        refill_endless_queue();
    }
    int next = endless_queue_.front();
    endless_queue_.erase(endless_queue_.begin());
    last_endless_level_ = next;
    return next;
}

void GameState::start_new_game(uint64_t now_ms) {
    endless_active_ = false;
    no_time_limit_ = false;
    unlimited_shuffle_ = false;
    endless_stage_count_ = 0;
    endless_pool_.clear();
    endless_queue_.clear();
    last_endless_level_ = -1;

    level_ = 0;
    score_ = 0.0f;
    lives_ = 10;
    time_left_ = kStageTimeLimitSec;
    quit_after_modal_ = false;
    events_.clear();
    init_current_stage_board();
    last_wall_ms_ = now_ms;
}

void GameState::start_endless_game(const EndlessConfig& cfg, uint64_t now_ms) {
    endless_active_ = true;
    endless_cfg_ = cfg;
    no_time_limit_ = cfg.no_time_limit;
    unlimited_shuffle_ = cfg.unlimited_shuffle;
    endless_stage_count_ = 1;
    endless_pool_.clear();
    endless_queue_.clear();
    last_endless_level_ = -1;

    if (cfg.type == EndlessModeType::AllLevels) {
        for (int lvl = 0; lvl <= 6; ++lvl) {
            endless_pool_.push_back(lvl);
        }
    } else {
        for (int lvl = 0; lvl <= 6; ++lvl) {
            if ((cfg.level_mask & (1u << lvl)) != 0) {
                endless_pool_.push_back(lvl);
            }
        }
        if (endless_pool_.empty()) {
            endless_pool_.push_back(0);
            endless_cfg_.level_mask = 0x01;
        }
    }

    level_ = pop_next_endless_level();
    score_ = 0.0f;
    lives_ = 10;
    time_left_ = kStageTimeLimitSec;
    quit_after_modal_ = false;
    events_.clear();
    init_current_stage_board();
    last_wall_ms_ = now_ms;
}

void GameState::give_up_game() {
    endless_active_ = false;
    no_time_limit_ = false;
    unlimited_shuffle_ = false;
    endless_stage_count_ = 0;
    endless_pool_.clear();
    endless_queue_.clear();
    last_endless_level_ = -1;

    board_active_ = false;
    in_title_screen_ = true;
    awaiting_modal_ = false;
    animating_link_ = false;
    hover_c_ = -1;
    hover_r_ = -1;
    cursor_active_ = false;
    cursor_c_ = -1;
    cursor_r_ = -1;
    clear_selection();
}

int GameState::remaining_seconds() const noexcept {
    if (no_time_limit_) {
        return static_cast<int>(time_left_);
    }
    int elapsed_sec = static_cast<int>(last_tick_ms_) / 1000;
    return static_cast<int>(time_left_) - elapsed_sec;
}

void GameState::on_mouse_move(float x, float y) {
    bool moved = (x >= 0.0f && y >= 0.0f) &&
                 (std::abs(x - last_mouse_x_) > 0.5f || std::abs(y - last_mouse_y_) > 0.5f);
    last_mouse_x_ = x;
    last_mouse_y_ = y;
    if (moved) {
        cursor_active_ = false;
    }
    update_hover_from_last_mouse();
}

void GameState::select_tile_at(int col, int row, uint64_t now_ms) {
    if (!board_active_ || in_title_screen_ || awaiting_modal_) {
        return;
    }

    // If the user activates a tile while a previous pair's 200ms lightning flash is still showing,
    // immediately finish removing that pair so rapid input never drops — EXCEPT if a deadlock
    // shuffle or level gravity shift altered the tile at (col, row)!
    if (animating_link_) {
        int pre_col = -1;
        int pre_row = -1;
        BoardCell pre_cell{};
        if (col >= 0 && col < Board::kCols && row >= 0 && row < Board::kRows) {
            pre_col = col;
            pre_row = row;
            pre_cell = board_.cell(col, row);
        }

        bool did_deadlock_shuffle = complete_matched_pair(now_ms);
        if (!board_active_ || in_title_screen_ || awaiting_modal_ || did_deadlock_shuffle) {
            update_hover_from_last_mouse();
            return;
        }
        if (pre_col >= 0 && pre_row >= 0) {
            const BoardCell& post_cell = board_.cell(pre_col, pre_row);
            if (!pre_cell.occupied || !post_cell.occupied ||
                post_cell.tile_id != pre_cell.tile_id || post_cell.match_id != pre_cell.match_id) {
                // Gravity shift moved a different tile under the target; don't mis-select it.
                update_hover_from_last_mouse();
                return;
            }
        }
    }

    if (col >= 0 && col < Board::kCols && row >= 0 && row < Board::kRows) {
        if (sel1_c_ < 0) {
            if (board_.cell(col, row).occupied) {
                sel1_c_ = col;
                sel1_r_ = row;
                events_.push_back(GameEvent{GameEventType::PlaySfx, 202});
            }
        } else if (sel2_c_ < 0) {
            if (board_.cell(col, row).occupied) {
                if (!(col == sel1_c_ && row == sel1_r_)) {
                    sel2_c_ = col;
                    sel2_r_ = row;
                    // Evaluate pair immediately for zero-latency response!
                    process_selection(now_ms);
                }
            }
        }
    }
    update_hover_from_last_mouse();
}

void GameState::cancel_selection() {
    if (!board_active_ || in_title_screen_ || awaiting_modal_) {
        return;
    }
    if (sel1_c_ > 0) {
        clear_selection();
        events_.push_back(GameEvent{GameEventType::PlaySfx, 201});
    }
}

void GameState::on_mouse_down(int button, float x, float y, uint64_t now_ms) {
    last_mouse_x_ = x;
    last_mouse_y_ = y;
    cursor_active_ = false;

    if (!board_active_ || in_title_screen_ || awaiting_modal_) {
        return;
    }

    if (button == 1) {
        float dx = x - static_cast<float>(kOffX);
        float dy = y - static_cast<float>(kOffY);
        if (dx > 0.0f && dx < 720.0f && dy > 0.0f && dy < 550.0f) {
            int col = static_cast<int>(dx) / 40;
            int row = static_cast<int>(dy) / 50;
            select_tile_at(col, row, now_ms);
            return;
        }
        if (animating_link_) {
            complete_matched_pair(now_ms);
        }
        update_hover_from_last_mouse();
    } else {
        if (animating_link_) {
            complete_matched_pair(now_ms);
        }
        cancel_selection();
    }
}

bool GameState::move_cursor(int dc, int dr) noexcept {
    if (!board_active_ || in_title_screen_ || awaiting_modal_ || board_.tiles_left() <= 0) {
        return false;
    }
    dc = std::clamp(dc, -1, 1);
    dr = std::clamp(dr, -1, 1);
    if (dc == 0 && dr == 0) {
        return false;
    }

    // Activate cursor from current hover/selection or snap to an occupied tile on first press
    if (!cursor_active_) {
        cursor_active_ = true;
        if (hover_c_ >= 1 && hover_c_ <= Board::kPlayCols &&
            hover_r_ >= 1 && hover_r_ <= Board::kPlayRows &&
            board_.cell(hover_c_, hover_r_).occupied) {
            cursor_c_ = hover_c_;
            cursor_r_ = hover_r_;
        } else if (sel1_c_ >= 1 && sel1_c_ <= Board::kPlayCols &&
                   sel1_r_ >= 1 && sel1_r_ <= Board::kPlayRows &&
                   board_.cell(sel1_c_, sel1_r_).occupied) {
            cursor_c_ = sel1_c_;
            cursor_r_ = sel1_r_;
        } else {
            int pref_c = (cursor_c_ >= 1 && cursor_c_ <= Board::kPlayCols) ? cursor_c_ : 1;
            int pref_r = (cursor_r_ >= 1 && cursor_r_ <= Board::kPlayRows) ? cursor_r_ : 1;
            if (snap_cursor_to_nearest_occupied(pref_c, pref_r)) {
                hover_c_ = cursor_c_;
                hover_r_ = cursor_r_;
                return true;
            }
            return false;
        }
    }

    if (cursor_c_ < 1 || cursor_c_ > Board::kPlayCols ||
        cursor_r_ < 1 || cursor_r_ > Board::kPlayRows ||
        !board_.cell(cursor_c_, cursor_r_).occupied) {
        int pref_c = std::clamp(cursor_c_, 1, Board::kPlayCols);
        int pref_r = std::clamp(cursor_r_, 1, Board::kPlayRows);
        if (snap_cursor_to_nearest_occupied(pref_c, pref_r)) {
            hover_c_ = cursor_c_;
            hover_r_ = cursor_r_;
            return true;
        }
        return false;
    }

    const int start_c = cursor_c_;
    const int start_r = cursor_r_;

    // Stage 1: Exact row / column auto-skip with wrap-around
    if (dr == 0) {
        for (int step = 1; step < Board::kPlayCols; ++step) {
            int nc = 1 + ((start_c - 1 + dc * step + Board::kPlayCols) % Board::kPlayCols);
            if (board_.cell(nc, start_r).occupied) {
                cursor_c_ = nc;
                hover_c_ = cursor_c_;
                hover_r_ = cursor_r_;
                return true;
            }
        }
    } else if (dc == 0) {
        for (int step = 1; step < Board::kPlayRows; ++step) {
            int nr = 1 + ((start_r - 1 + dr * step + Board::kPlayRows) % Board::kPlayRows);
            if (board_.cell(start_c, nr).occupied) {
                cursor_r_ = nr;
                hover_c_ = cursor_c_;
                hover_r_ = cursor_r_;
                return true;
            }
        }
    }

    // Stage 2: Current row/column has no other occupied tile — move smoothly to the
    // nearest occupied tile in the pressed half-plane (or wrap across the board edge).
    int best_c = -1;
    int best_r = -1;
    int best_score = 1000000;
    int best_perp = 1000000;

    auto eval_candidates = [&](bool strict_half_plane) {
        const int span = (dr == 0) ? Board::kPlayCols : Board::kPlayRows;
        for (int r = 1; r <= Board::kPlayRows; ++r) {
            for (int c = 1; c <= Board::kPlayCols; ++c) {
                if ((c == start_c && r == start_r) || !board_.cell(c, r).occupied) {
                    continue;
                }
                int delta = (dr == 0) ? ((c - start_c) * dc) : ((r - start_r) * dr);
                int perp = (dr == 0) ? std::abs(r - start_r) : std::abs(c - start_c);
                if (strict_half_plane && delta <= 0) {
                    continue;
                }
                int fwd = (delta > 0) ? delta : (delta + span);
                int score = perp * 3 + fwd;
                if (score < best_score || (score == best_score && perp < best_perp)) {
                    best_score = score;
                    best_perp = perp;
                    best_c = c;
                    best_r = r;
                }
            }
        }
    };

    eval_candidates(true);
    if (best_c < 0) {
        eval_candidates(false);
    }

    if (best_c >= 1) {
        cursor_c_ = best_c;
        cursor_r_ = best_r;
        hover_c_ = cursor_c_;
        hover_r_ = cursor_r_;
        return true;
    }
    return false;
}

bool GameState::jump_cursor_tile(int dir) noexcept {
    if (!board_active_ || in_title_screen_ || awaiting_modal_ || board_.tiles_left() <= 0) {
        return false;
    }
    const int step = (dir < 0) ? -1 : 1;
    const int total = Board::kPlayCols * Board::kPlayRows;

    if (!cursor_active_) {
        cursor_active_ = true;
        if (hover_c_ >= 1 && hover_c_ <= Board::kPlayCols &&
            hover_r_ >= 1 && hover_r_ <= Board::kPlayRows &&
            board_.cell(hover_c_, hover_r_).occupied) {
            cursor_c_ = hover_c_;
            cursor_r_ = hover_r_;
        } else if (sel1_c_ >= 1 && sel1_c_ <= Board::kPlayCols &&
                   sel1_r_ >= 1 && sel1_r_ <= Board::kPlayRows &&
                   board_.cell(sel1_c_, sel1_r_).occupied) {
            cursor_c_ = sel1_c_;
            cursor_r_ = sel1_r_;
        } else {
            int start_idx = (step > 0) ? 0 : (total - 1);
            for (int k = 0; k < total; ++k) {
                int idx = (start_idx + step * k + total) % total;
                int c = 1 + (idx % Board::kPlayCols);
                int r = 1 + (idx / Board::kPlayCols);
                if (board_.cell(c, r).occupied) {
                    cursor_c_ = c;
                    cursor_r_ = r;
                    hover_c_ = cursor_c_;
                    hover_r_ = cursor_r_;
                    return true;
                }
            }
            return false;
        }
    }

    if (cursor_c_ < 1 || cursor_c_ > Board::kPlayCols ||
        cursor_r_ < 1 || cursor_r_ > Board::kPlayRows) {
        int start_idx = (step > 0) ? 0 : (total - 1);
        for (int k = 0; k < total; ++k) {
            int idx = (start_idx + step * k + total) % total;
            int c = 1 + (idx % Board::kPlayCols);
            int r = 1 + (idx / Board::kPlayCols);
            if (board_.cell(c, r).occupied) {
                cursor_c_ = c;
                cursor_r_ = r;
                hover_c_ = cursor_c_;
                hover_r_ = cursor_r_;
                return true;
            }
        }
        return false;
    }

    int cur_idx = (cursor_r_ - 1) * Board::kPlayCols + (cursor_c_ - 1);
    for (int k = 1; k <= total; ++k) {
        int idx = (cur_idx + step * k + total) % total;
        int c = 1 + (idx % Board::kPlayCols);
        int r = 1 + (idx / Board::kPlayCols);
        if (board_.cell(c, r).occupied) {
            cursor_c_ = c;
            cursor_r_ = r;
            hover_c_ = cursor_c_;
            hover_r_ = cursor_r_;
            return true;
        }
    }
    return false;
}

void GameState::activate_cursor(uint64_t now_ms) {
    if (!board_active_ || in_title_screen_ || awaiting_modal_ || board_.tiles_left() <= 0) {
        return;
    }
    if (!cursor_active_) {
        cursor_active_ = true;
        if (hover_c_ >= 1 && hover_c_ <= Board::kPlayCols &&
            hover_r_ >= 1 && hover_r_ <= Board::kPlayRows &&
            board_.cell(hover_c_, hover_r_).occupied) {
            cursor_c_ = hover_c_;
            cursor_r_ = hover_r_;
        } else {
            int pref_c = (cursor_c_ >= 1 && cursor_c_ <= Board::kPlayCols) ? cursor_c_ : 1;
            int pref_r = (cursor_r_ >= 1 && cursor_r_ <= Board::kPlayRows) ? cursor_r_ : 1;
            if (!snap_cursor_to_nearest_occupied(pref_c, pref_r)) {
                return;
            }
        }
    }
    if (cursor_c_ >= 1 && cursor_c_ <= Board::kPlayCols &&
        cursor_r_ >= 1 && cursor_r_ <= Board::kPlayRows) {
        select_tile_at(cursor_c_, cursor_r_, now_ms);
    }
}

void GameState::process_selection(uint64_t now_ms) {
    if (sel2_c_ <= 0) {
        return;
    }
    int c1 = sel1_c_;
    int r1 = sel1_r_;
    int c2 = sel2_c_;
    int r2 = sel2_r_;
    LinkPath path{};
    if (board_.cell(c1, r1).match_id == board_.cell(c2, r2).match_id &&
        board_.check_link(c1, r1, c2, r2, &path)) {
        events_.push_back(GameEvent{GameEventType::PlaySfx, 203});
        animating_link_ = true;
        link_flash_until_ms_ = now_ms + kLinkFlashDurationMs;
        pending_c1_ = c1;
        pending_r1_ = r1;
        pending_c2_ = c2;
        pending_r2_ = r2;
        active_link_path_ = path;
        if (kLinkFlashDurationMs == 0) {
            complete_matched_pair(now_ms);
        }
    } else {
        events_.push_back(GameEvent{GameEventType::PlaySfx, 204});
        clear_selection();
    }
}

bool GameState::complete_matched_pair(uint64_t now_ms) {
    animating_link_ = false;
    bool did_deadlock_shuffle = false;
    board_.cell(pending_c1_, pending_r1_).occupied = false;
    board_.cell(pending_c2_, pending_r2_).occupied = false;

    board_.apply_level_shift(level_);
    score_ += 20.0f;
    board_.set_tiles_left(board_.tiles_left() - 2);
    clear_selection();

    if (board_.tiles_left() < 1) {
        // GameState = 1: LevelClear (slot 21)
        // Note: Do NOT increment level_ until acknowledge_modal() so the completed
        // stage's background image (Pic_Level(level_)) stays visible behind the dialog!
        events_.push_back(GameEvent{GameEventType::PlaySfx, 205});
        if (!unlimited_shuffle_) {
            lives_ += 1;
        }
        int rem_bonus = no_time_limit_ ? 0 : remaining_seconds();
        if (endless_active_) {
            score_ += static_cast<float>(rem_bonus);
            board_active_ = false;
            awaiting_modal_ = true;
            quit_after_modal_ = false;
            hover_c_ = -1;
            hover_r_ = -1;
            int upcoming_shift = peek_next_endless_level();
            events_.push_back(GameEvent{
                GameEventType::StageClearModal, 0, endless_stage_count_ + 1,
                rem_bonus, 0, score_, true, false, upcoming_shift
            });
        } else if (level_ >= 6) {
            int life_bonus = lives_ * 50;
            score_ += static_cast<float>(rem_bonus + life_bonus);
            board_active_ = false;
            awaiting_modal_ = true;
            quit_after_modal_ = true;
            hover_c_ = -1;
            hover_r_ = -1;
            events_.push_back(GameEvent{
                GameEventType::AllStagesClearModal, 0, level_ + 2,
                rem_bonus, life_bonus, score_, false, true, 6
            });
        } else {
            score_ += static_cast<float>(rem_bonus);
            board_active_ = false;
            awaiting_modal_ = true;
            quit_after_modal_ = false;
            hover_c_ = -1;
            hover_r_ = -1;
            events_.push_back(GameEvent{
                GameEventType::StageClearModal, 0, level_ + 2,
                rem_bonus, 0, score_, false, true, level_ + 1
            });
        }
    } else if (!board_.has_valid_move()) {
        // GameState = 2: DeadlockHandler (slot 22)
        events_.push_back(GameEvent{GameEventType::PlaySfx, 206});
        if (!unlimited_shuffle_) {
            lives_ -= 1;
        }
        if (unlimited_shuffle_ || lives_ >= 0) {
            do {
                board_.shuffle_remaining(rng_);
            } while (!board_.has_valid_move());
            did_deadlock_shuffle = true;
            update_hover_from_last_mouse();
        } else {
            awaiting_modal_ = true;
            quit_after_modal_ = true;
            hover_c_ = -1;
            hover_r_ = -1;
            events_.push_back(GameEvent{
                GameEventType::NoMovesGameOverModal, 0, 0, 0, 0, score_,
                endless_active_, !endless_active_, level_
            });
        }
    }
    last_wall_ms_ = now_ms;
    return did_deadlock_shuffle;
}

void GameState::tick(uint64_t now_ms) {
    if (in_title_screen_ || awaiting_modal_ || !board_active_) {
        last_wall_ms_ = now_ms;
        return;
    }

    if (now_ms > last_wall_ms_) {
        if (!no_time_limit_) {
            last_tick_ms_ += static_cast<float>(now_ms - last_wall_ms_);
        }
        last_wall_ms_ = now_ms;
    }

    // If currently in the 200ms yellow lightning link animation, wait until it finishes
    if (animating_link_) {
        if (now_ms >= link_flash_until_ms_) {
            complete_matched_pair(now_ms);
        }
        return;
    }

    // VB5 slot 20: ProcessSelection (if not already processed in on_mouse_down)
    if (sel2_c_ > 0) {
        process_selection(now_ms);
        if (animating_link_) {
            return;
        }
    }

    // Check timeout: If LastTick >= TimeLeft * 1000 Then GameState = 3 (TimeoutOver, slot 23)
    if (!no_time_limit_ && last_tick_ms_ >= time_left_ * 1000.0f) {
        events_.push_back(GameEvent{GameEventType::PlaySfx, 204});
        int bonus = unlimited_shuffle_ ? 0 : (lives_ * 50);
        score_ += static_cast<float>(bonus);
        awaiting_modal_ = true;
        quit_after_modal_ = true;
        hover_c_ = -1;
        hover_r_ = -1;
        events_.push_back(GameEvent{
            GameEventType::TimeoutGameOverModal, 0, 0, bonus, bonus, score_,
            endless_active_, !endless_active_, level_
        });
    }
}

void GameState::acknowledge_modal(uint64_t now_ms) {
    if (!awaiting_modal_) {
        return;
    }
    awaiting_modal_ = false;
    if (quit_after_modal_) {
        endless_active_ = false;
        no_time_limit_ = false;
        unlimited_shuffle_ = false;
        endless_stage_count_ = 0;
        endless_pool_.clear();
        endless_queue_.clear();
        last_endless_level_ = -1;
        board_active_ = false;
        in_title_screen_ = true;
        hover_c_ = -1;
        hover_r_ = -1;
        cursor_active_ = false;
        cursor_c_ = -1;
        cursor_r_ = -1;
        clear_selection();
    } else {
        // Advance to the next stage only AFTER the StageClear modal is dismissed!
        if (endless_active_) {
            endless_stage_count_ += 1;
            level_ = pop_next_endless_level();
        } else {
            level_ += 1;
        }
        init_current_stage_board();
        last_wall_ms_ = now_ms;
    }
}

std::vector<GameEvent> GameState::consume_events() {
    std::vector<GameEvent> out = std::move(events_);
    events_.clear();
    return out;
}

} // namespace llk2::core
