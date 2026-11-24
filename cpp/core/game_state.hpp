#pragma once

#include "board.hpp"
#include "rng.hpp"
#include <cstdint>
#include <vector>

namespace llk2::core {

enum class EndlessModeType {
    AllLevels = 0,     // Loop all 7 shift stages (0..6) endlessly
    SpecificLevels = 1 // Loop only the checked shift stage(s) in level_mask
};

struct EndlessConfig {
    EndlessModeType type = EndlessModeType::AllLevels;
    uint8_t level_mask = 0x01;       // Bit 0..6 -> Stages 1..7 (used when type == SpecificLevels)
    bool shuffle = false;            // Shuffle stage order (bag shuffle so stages don't repeat consecutively)
    bool no_time_limit = false;      // Optional relaxed timer (stays full, never times out)
    bool unlimited_shuffle = false;  // Optional unlimited auto-shuffles / lives (never game-overs on deadlock)
};

enum class GameEventType {
    PlaySfx,              // sfx_id = 201..206
    StageClearModal,      // Stage 0..5 cleared (or Endless stage cleared)
    AllStagesClearModal,  // Classic Stage 6 cleared -> victory!
    NoMovesGameOverModal, // Lives < 0 on deadlock
    TimeoutGameOverModal  // LastTick >= TimeLeft * 1000
};

struct GameEvent {
    GameEventType type;
    int sfx_id = 0;
    int next_level_display = 0; // Classic: Level + 2 (2..7); Endless: next stage count (2, 3, ...)
    int bonus = 0;
    int life_bonus = 0;
    float total_score = 0.0f;
    bool is_endless = false;
    bool records_score = true;  // False in Endless Mode (never writes to high scores)
    int next_shift_level = 0;   // 0..6 shift rule of the upcoming stage
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

    // VB5 Mem_NewGame_Click + GameLoop initialization (Classic 7-stage mode, records high scores)
    void start_new_game(uint64_t now_ms);

    // Endless Mode (never saves scores to HighScoreTable, loops indefinitely)
    void start_endless_game(const EndlessConfig& cfg, uint64_t now_ms);

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
    bool is_endless() const noexcept { return endless_active_; }
    bool no_time_limit() const noexcept { return no_time_limit_; }
    bool unlimited_shuffle() const noexcept { return unlimited_shuffle_; }
    int endless_stage_count() const noexcept { return endless_stage_count_; }
    const EndlessConfig& endless_config() const noexcept { return endless_cfg_; }

    int level() const noexcept { return level_; }          // 0..6 (current stage shift rule)
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
    void refill_endless_queue();
    int pop_next_endless_level();
    int peek_next_endless_level();
    void process_selection(uint64_t now_ms);
    // Returns true if completing the pair triggered a deadlock shuffle.
    bool complete_matched_pair(uint64_t now_ms);
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

    // Endless Mode state
    bool endless_active_ = false;
    bool no_time_limit_ = false;
    bool unlimited_shuffle_ = false;
    int endless_stage_count_ = 0; // 1-based stages played in current endless run
    EndlessConfig endless_cfg_{};
    std::vector<int> endless_pool_;
    std::vector<int> endless_queue_;
    int last_endless_level_ = -1;

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
