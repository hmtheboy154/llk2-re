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

void GameState::start_new_game(uint64_t now_ms) {
    level_ = 0;
    score_ = 0.0f;
    lives_ = 10;
    time_left_ = kStageTimeLimitSec;
    quit_after_modal_ = false;
    events_.clear();
    init_current_stage_board();
    last_wall_ms_ = now_ms;
}

void GameState::give_up_game() {
    board_active_ = false;
    in_title_screen_ = true;
    awaiting_modal_ = false;
    animating_link_ = false;
    hover_c_ = -1;
    hover_r_ = -1;
    clear_selection();
}

int GameState::remaining_seconds() const noexcept {
    int elapsed_sec = static_cast<int>(last_tick_ms_) / 1000;
    return static_cast<int>(time_left_) - elapsed_sec;
}

void GameState::on_mouse_move(float x, float y) {
    last_mouse_x_ = x;
    last_mouse_y_ = y;
    update_hover_from_last_mouse();
}

void GameState::on_mouse_down(int button, float x, float y, uint64_t now_ms) {
    last_mouse_x_ = x;
    last_mouse_y_ = y;

    if (!board_active_ || in_title_screen_ || awaiting_modal_) {
        return;
    }

    // If the user clicks while a previous pair's 200ms lightning flash is still showing,
    // immediately finish removing that pair so rapid clicking never drops inputs.
    if (animating_link_) {
        complete_matched_pair(now_ms);
        if (!board_active_ || in_title_screen_ || awaiting_modal_) {
            return;
        }
    }

    if (button == 1) {
        float dx = x - static_cast<float>(kOffX);
        float dy = y - static_cast<float>(kOffY);
        if (dx > 0.0f && dx < 720.0f && dy > 0.0f && dy < 550.0f) {
            int col = static_cast<int>(dx) / 40;
            int row = static_cast<int>(dy) / 50;
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
                            // Evaluate pair immediately on click for zero-latency response!
                            process_selection(now_ms);
                        }
                    }
                }
            }
        }
    } else {
        // Right/other mouse button clears selection and plays SFX 201
        if (sel1_c_ > 0) {
            clear_selection();
            events_.push_back(GameEvent{GameEventType::PlaySfx, 201});
        }
    }
    update_hover_from_last_mouse();
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

void GameState::complete_matched_pair(uint64_t now_ms) {
    animating_link_ = false;
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
        lives_ += 1;
        int rem_bonus = remaining_seconds();
        if (level_ >= 6) {
            int life_bonus = lives_ * 50;
            score_ += static_cast<float>(rem_bonus + life_bonus);
            board_active_ = false;
            awaiting_modal_ = true;
            quit_after_modal_ = true;
            hover_c_ = -1;
            hover_r_ = -1;
            events_.push_back(GameEvent{
                GameEventType::AllStagesClearModal, 0, level_ + 2, rem_bonus, life_bonus, score_
            });
        } else {
            score_ += static_cast<float>(rem_bonus);
            board_active_ = false;
            awaiting_modal_ = true;
            quit_after_modal_ = false;
            hover_c_ = -1;
            hover_r_ = -1;
            events_.push_back(GameEvent{
                GameEventType::StageClearModal, 0, level_ + 2, rem_bonus, 0, score_
            });
        }
    } else if (!board_.has_valid_move()) {
        // GameState = 2: DeadlockHandler (slot 22)
        events_.push_back(GameEvent{GameEventType::PlaySfx, 206});
        lives_ -= 1;
        if (lives_ >= 0) {
            do {
                board_.shuffle_remaining(rng_);
            } while (!board_.has_valid_move());
            update_hover_from_last_mouse();
        } else {
            awaiting_modal_ = true;
            quit_after_modal_ = true;
            hover_c_ = -1;
            hover_r_ = -1;
            events_.push_back(GameEvent{
                GameEventType::NoMovesGameOverModal, 0, 0, 0, 0, score_
            });
        }
    }
    last_wall_ms_ = now_ms;
}

void GameState::tick(uint64_t now_ms) {
    if (in_title_screen_ || awaiting_modal_ || !board_active_) {
        last_wall_ms_ = now_ms;
        return;
    }

    if (now_ms > last_wall_ms_) {
        last_tick_ms_ += static_cast<float>(now_ms - last_wall_ms_);
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
    if (last_tick_ms_ >= time_left_ * 1000.0f) {
        events_.push_back(GameEvent{GameEventType::PlaySfx, 204});
        int bonus = lives_ * 50;
        score_ += static_cast<float>(bonus);
        awaiting_modal_ = true;
        quit_after_modal_ = true;
        hover_c_ = -1;
        hover_r_ = -1;
        events_.push_back(GameEvent{
            GameEventType::TimeoutGameOverModal, 0, 0, bonus, bonus, score_
        });
    }
}

void GameState::acknowledge_modal(uint64_t now_ms) {
    if (!awaiting_modal_) {
        return;
    }
    awaiting_modal_ = false;
    if (quit_after_modal_) {
        board_active_ = false;
        in_title_screen_ = true;
        hover_c_ = -1;
        hover_r_ = -1;
        clear_selection();
    } else {
        // Advance to the next stage (Level 1..6) only AFTER the StageClear modal is dismissed!
        level_ += 1;
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
