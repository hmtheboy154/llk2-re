#pragma once

#include "board.hpp"
#include "rng.hpp"
#include <cstdint>
#include <vector>

namespace llk2::core {

enum class GameEventType {
    PlaySfx,              // sfx_id = 201..206
    StageClearModal,      // Stage 0..5 cleared -> advance to next_level_display (2..7)
    AllStagesClearModal,  // Stage 6 cleared -> victory!
    NoMovesGameOverModal, // Lives < 0 on deadlock
    TimeoutGameOverModal  // LastTick >= TimeLeft * 1000
};

struct GameEvent {
    GameEventType type;
    int sfx_id = 0;
    int next_level_display = 0; // Level + 2 (2..7)
    int bonus = 0;
    int life_bonus = 0;
    float total_score = 0.0f;
};

class GameState {
public:
    static constexpr int kCanvasWidth = 768;
    static constexpr int kCanvasHeight = 550;
    static constexpr int kOffX = (kCanvasWidth - 726) / 2;      // 21
    static constexpr int kOffY = (kCanvasHeight - 550) / 2 + 6; // 6
    static constexpr float kStageTimeLimitSec = 600.0f;
    static constexpr uint32_t kTickIntervalMs = 30;
    static constexpr uint32_t kLinkFlashDurationMs = 200;

    GameState();

    // VB5 Form_Load: Randomize Timer
    void randomize_from_clock() noexcept;

    // VB5 Mem_NewGame_Click + GameLoop initialization
    void start_new_game(uint64_t now_ms);

    // VB5 Mem_QuitGame_Click
    void give_up_game();

    // VB5 slot 36: Pic_Show_MouseDown (button: 1 = left, 2 = right)
    void on_mouse_down(int button, float x, float y, uint64_t now_ms);

    // VB5 slot 37: Pic_Show_MouseMove
    void on_mouse_move(float x, float y);

    // VB5 slot 19: GameLoop 30ms tick body
    void tick(uint64_t now_ms);

    // Called by UI when user dismisses the active modal dialog (StageClear / AllClear / GameOver).
    void acknowledge_modal(uint64_t now_ms);

    std::vector<GameEvent> consume_events();

    const Board& board() const noexcept { return board_; }
    Board& board() noexcept { return board_; }
    Vb5Rng& rng() noexcept { return rng_; }

    bool in_title_screen() const noexcept { return in_title_screen_; }
    bool board_active() const noexcept { return board_active_; }
    bool awaiting_modal() const noexcept { return awaiting_modal_; }
    bool animating_link() const noexcept { return animating_link_; }

    int level() const noexcept { return level_; }          // 0..6
    int lives() const noexcept { return lives_; }          // starts at 10
    float score() const noexcept { return score_; }
    float time_left() const noexcept { return time_left_; } // 600.0f
    float last_tick_ms() const noexcept { return last_tick_ms_; }

    // VB5 v1.02 integer remaining seconds: CLng(TimeLeft - (LastTick \ 1000))
    int remaining_seconds() const noexcept;

    int sel1_c() const noexcept { return sel1_c_; }
    int sel1_r() const noexcept { return sel1_r_; }
    int sel2_c() const noexcept { return sel2_c_; }
    int sel2_r() const noexcept { return sel2_r_; }
    int hover_c() const noexcept { return hover_c_; }
    int hover_r() const noexcept { return hover_r_; }

    const LinkPath& active_link_path() const noexcept { return active_link_path_; }

    void clear_selection() noexcept;

private:
    void init_current_stage_board();
    void process_selection(uint64_t now_ms);
    void complete_matched_pair(uint64_t now_ms);
    void update_hover_from_last_mouse() noexcept;

    Vb5Rng rng_;
    Board board_;

    bool in_title_screen_ = true;
    bool board_active_ = false;
    bool awaiting_modal_ = false;
    bool quit_after_modal_ = false;

    bool animating_link_ = false;
    uint64_t link_flash_until_ms_ = 0;
    int pending_c1_ = -1;
    int pending_r1_ = -1;
    int pending_c2_ = -1;
    int pending_r2_ = -1;
    LinkPath active_link_path_{};

    int level_ = 0;
    int lives_ = 10;
    float score_ = 0.0f;
    float time_left_ = kStageTimeLimitSec;
    float last_tick_ms_ = 0.0f;
    uint64_t last_wall_ms_ = 0;

    int sel1_c_ = -1;
    int sel1_r_ = -1;
    int sel2_c_ = -1;
    int sel2_r_ = -1;
    int hover_c_ = -1;
    int hover_r_ = -1;
    float last_mouse_x_ = -1.0f;
    float last_mouse_y_ = -1.0f;

    std::vector<GameEvent> events_;
};

} // namespace llk2::core
