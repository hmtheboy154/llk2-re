#include "core/app_paths.hpp"
#include "core/config.hpp"
#include "core/game_state.hpp"
#include "core/hst_score.hpp"
#include "core/i18n.hpp"

#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

using namespace llk2::core;

static void test_scramble_roundtrip() {
    std::string plain = "35000";
    std::string enc = HighScoreTable::scramble(plain);
    assert(enc != plain);
    std::string dec = HighScoreTable::unscramble(enc);
    assert(dec == plain);

    // Big5/GBK raw bytes + ASCII mix
    std::string dbcs_name = "\xB3\xC2\xAA\xF9\xA6\x74"; // "陳阿土" in Big5
    std::string enc_name = HighScoreTable::scramble(dbcs_name);
    std::string dec_name = HighScoreTable::unscramble(enc_name);
    assert(dec_name == dbcs_name);
    assert(HighScoreTable::legacy_dbcs_byte_len("陳阿土") == 6);
    assert(HighScoreTable::legacy_dbcs_byte_len("Player1") == 7);
}

static void test_hst_and_json_persistence() {
    HighScoreTable table;
    table.reset_empty();
    assert(table.entries()[0].score == 0.0f);
    assert(table.is_high_score(15000.0f));

    for (int i = 0; i < 10; ++i) {
        table.insert_score("P" + std::to_string(i), static_cast<float>((i + 1) * 1000));
    }
    assert(!table.is_high_score(500.0f));
    assert(table.is_high_score(15000.0f));

    int idx = table.insert_score("Winner", 25000.0f);
    assert(idx == 0);
    assert(table.entries()[0].name == "Winner");
    assert(table.entries()[0].score == 25000.0f);
    assert(table.entries()[1].score == 10000.0f);

    auto tmp_dir = std::filesystem::temp_directory_path();
    auto hst_path = tmp_dir / "llk2_test_d4s.hst";
    auto json_path = tmp_dir / "llk2_test_scores.json";

    assert(table.save_legacy_hst(hst_path));
    assert(std::filesystem::file_size(hst_path) == HighScoreTable::kLegacyFileBytes);

    HighScoreTable loaded_hst;
    assert(loaded_hst.load_legacy_hst(hst_path));
    assert(loaded_hst.entries()[0].name == "Winner");
    assert(loaded_hst.entries()[0].score == 25000.0f);

    // Test Unicode names in JSON
    loaded_hst.insert_score("Nguyễn Văn A", 50000.0f);
    assert(loaded_hst.save_json(json_path));

    HighScoreTable loaded_json;
    assert(loaded_json.load_json(json_path));
    assert(loaded_json.entries()[0].name == "Nguyễn Văn A");
    assert(loaded_json.entries()[0].score == 50000.0f);

    std::filesystem::remove(hst_path);
    std::filesystem::remove(json_path);
}

static void test_config_and_i18n() {
    Config cfg;
    assert(cfg.is_track_enabled(101));
    assert(cfg.next_enabled_midi_track(100) == 101);
    assert(cfg.next_enabled_midi_track(101) == 102);
    assert(cfg.next_enabled_midi_track(102) == 103);
    assert(cfg.next_enabled_midi_track(103) == 101);

    // Untick track 2 (102.mid)
    cfg.set_track_enabled(102, false);
    assert(cfg.next_enabled_midi_track(101) == 103);
    assert(cfg.next_enabled_midi_track(103) == 101);

    // Untick all tracks
    cfg.set_track_enabled(101, false);
    cfg.set_track_enabled(103, false);
    assert(cfg.next_enabled_midi_track(100) == 0);

    auto cfg_path = std::filesystem::temp_directory_path() / "llk2_test_cfg.json";
    cfg.language = "zh-CN";
    cfg.midi_backend = MidiBackend::NativeOS;
    cfg.scale_mode = WindowScaleMode::Scale2x;
    assert(cfg.save(cfg_path));

    Config cfg2;
    assert(cfg2.load(cfg_path));
    assert(cfg2.language == "zh-CN");
    assert(cfg2.midi_backend == MidiBackend::NativeOS);
    assert(cfg2.scale_mode == WindowScaleMode::Scale2x);
    assert(cfg2.next_enabled_midi_track(100) == 0);
    std::filesystem::remove(cfg_path);

    // Test I18n
    I18n i18n;
    assert(i18n.set_language("zh-TW"));
    assert(i18n.tr("menu.settings") == "設定(&S)");
    std::string msg_tw = i18n.format("msg.stage_clear.body", {
        {"level", "2"},
        {"bonus", "520"}
    });
    assert(msg_tw.find("請進入第 2 關") != std::string::npos);
    assert(msg_tw.find("520") != std::string::npos);

    assert(i18n.set_language("zh-CN"));
    assert(i18n.tr("menu.settings") == "设置(&S)");

    // Verify OS locale -> language auto-detection mapping
    assert(i18n.map_locale_to_language("zh_TW.UTF-8") == "zh-TW");
    assert(i18n.map_locale_to_language("zh-Hant-TW") == "zh-TW");
    assert(i18n.map_locale_to_language("zh_HK.UTF-8") == "zh-TW");
    assert(i18n.map_locale_to_language("zh-MO") == "zh-TW");
    assert(i18n.map_locale_to_language("zh_CN.UTF-8") == "zh-CN");
    assert(i18n.map_locale_to_language("zh-Hans-CN") == "zh-CN");
    assert(i18n.map_locale_to_language("zh_SG.UTF-8") == "zh-CN");
    assert(i18n.map_locale_to_language("zh") == "zh-CN");
    assert(i18n.map_locale_to_language("vi_VN.UTF-8") == "vi");
    assert(i18n.map_locale_to_language("vi-VN") == "vi");
    assert(i18n.map_locale_to_language("en_US.UTF-8") == "en");
    assert(i18n.map_locale_to_language("en-GB") == "en");
    assert(i18n.map_locale_to_language("ja_JP.UTF-8") == "en");
    assert(i18n.map_locale_to_language("C.UTF-8") == "en");
    assert(i18n.map_locale_to_language("") == "en");
    assert(!i18n.detect_system_language().empty());
}

static void test_gamestate_progression() {
    GameState gs;
    gs.start_new_game(1000);
    assert(gs.in_title_screen() == false);
    assert(gs.board_active() == true);
    assert(gs.level() == 0);
    assert(gs.lives() == 10);
    assert(gs.time_left() == 600.0f);
    assert(gs.score() == 0.0f);

    // Tick 1000 ms (at t = 2000) -> remaining_seconds should drop by 1 (599)
    gs.tick(2000);
    assert(gs.remaining_seconds() == 599);

    // Clear all tiles except the last hinted pair on Level 0
    int c1 = static_cast<int>(gs.board().hint_x1());
    int r1 = static_cast<int>(gs.board().hint_y1());
    int c2 = static_cast<int>(gs.board().hint_x2());
    int r2 = static_cast<int>(gs.board().hint_y2());
    for (int r = 0; r < Board::kRows; ++r) {
        for (int c = 0; c < Board::kCols; ++c) {
            if ((c == c1 && r == r1) || (c == c2 && r == r2)) continue;
            gs.board().cell(c, r).occupied = false;
        }
    }
    gs.board().set_tiles_left(2);

    // Click c1 then c2 -> must enter animating_link() immediately on the 2nd click (0 ms latency)
    gs.on_mouse_down(1, 21.0f + c1 * 40 + 10, 6.0f + r1 * 50 + 10, 2010);
    gs.on_mouse_down(1, 21.0f + c2 * 40 + 10, 6.0f + r2 * 50 + 10, 2020);
    assert(gs.animating_link());

    // Advance past 200ms link flash -> stage 0 cleared!
    // Level must stay 0 while awaiting StageClearModal so Pic_Level(0) stays on screen!
    gs.tick(2250);
    assert(gs.awaiting_modal());
    assert(gs.level() == 0);
    auto evs = gs.consume_events();
    bool saw_stage_clear = false;
    for (const auto& ev : evs) {
        if (ev.type == GameEventType::StageClearModal) {
            saw_stage_clear = true;
            assert(ev.next_level_display == 2);
        }
    }
    assert(saw_stage_clear);

    // Only after acknowledging the modal does level advance to 1!
    gs.acknowledge_modal(2300);
    assert(!gs.awaiting_modal());
    assert(gs.level() == 1);
    assert(gs.board_active());
    assert(gs.board().tiles_left() == 144);

    // Rapid-click fast-forward guard on Level 1 (ShiftUp):
    // Set up col 3 with tiles (3,1) & (3,2) matching, and (3,3) = tile_id 5 that shifts up to (3,1);
    // plus col 4 at (4,1) = tile_id 5 (already at top, untouched by ShiftUp) and col 10 at (10,1)/(10,2).
    gs.board().clear_all();
    gs.board().cell(3, 1) = {true, 1, 1};
    gs.board().cell(3, 2) = {true, 1, 1};
    gs.board().cell(3, 3) = {true, 5, 5};
    gs.board().cell(4, 1) = {true, 5, 5};
    gs.board().cell(10, 1) = {true, 7, 7};
    gs.board().cell(10, 2) = {true, 7, 7};
    gs.board().set_tiles_left(6);

    // Match (3,1) and (3,2) -> enters animating_link()
    gs.on_mouse_down(1, 21.0f + 3 * 40 + 10, 6.0f + 1 * 50 + 10, 2400);
    gs.on_mouse_down(1, 21.0f + 3 * 40 + 10, 6.0f + 2 * 50 + 10, 2410);
    assert(gs.animating_link());

    // Click (3,3) during the 200ms flash: ShiftUp moves (3,3) to (3,1), vacating (3,3).
    // Fast-forward must complete the match & shift WITHOUT selecting anything at (3,3)!
    gs.on_mouse_down(1, 21.0f + 3 * 40 + 10, 6.0f + 3 * 50 + 10, 2450);
    assert(!gs.animating_link());
    assert(gs.board().cell(3, 1).occupied && gs.board().cell(3, 1).tile_id == 5);
    assert(!gs.board().cell(3, 3).occupied);
    assert(gs.sel1_c() < 0);

    // Match (10,1) and (10,2), then rapid-click untouched tile (4,1) during 200ms flash ->
    // since (4,1) did NOT move during ShiftUp, it SHOULD select immediately!
    gs.on_mouse_down(1, 21.0f + 10 * 40 + 10, 6.0f + 1 * 50 + 10, 2500);
    gs.on_mouse_down(1, 21.0f + 10 * 40 + 10, 6.0f + 2 * 50 + 10, 2510);
    assert(gs.animating_link());
    gs.on_mouse_down(1, 21.0f + 4 * 40 + 10, 6.0f + 1 * 50 + 10, 2550);
    assert(!gs.animating_link());
    assert(gs.sel1_c() == 4 && gs.sel1_r() == 1);
}

static void clear_current_stage(GameState& gs, uint64_t& t_ms) {
    int c1 = static_cast<int>(gs.board().hint_x1());
    int r1 = static_cast<int>(gs.board().hint_y1());
    int c2 = static_cast<int>(gs.board().hint_x2());
    int r2 = static_cast<int>(gs.board().hint_y2());
    for (int r = 0; r < Board::kRows; ++r) {
        for (int c = 0; c < Board::kCols; ++c) {
            if ((c == c1 && r == r1) || (c == c2 && r == r2)) continue;
            gs.board().cell(c, r).occupied = false;
        }
    }
    gs.board().set_tiles_left(2);
    gs.on_mouse_down(1, 21.0f + c1 * 40 + 10, 6.0f + r1 * 50 + 10, t_ms += 10);
    gs.on_mouse_down(1, 21.0f + c2 * 40 + 10, 6.0f + r2 * 50 + 10, t_ms += 10);
    assert(gs.animating_link());
    gs.tick(t_ms += 250);
    assert(gs.awaiting_modal());
}

static void test_endless_mode_and_fullscreen_config() {
    // 1. Verify borderless_fullscreen persists in config.json
    auto tmp_cfg = std::filesystem::temp_directory_path() / "llk2_test_fs_cfg.json";
    Config cfg;
    cfg.borderless_fullscreen = true;
    cfg.scale_mode = WindowScaleMode::Scale2x;
    assert(cfg.save(tmp_cfg));
    Config cfg2;
    assert(cfg2.load(tmp_cfg));
    assert(cfg2.borderless_fullscreen == true);
    assert(cfg2.scale_mode == WindowScaleMode::Scale2x);
    std::filesystem::remove(tmp_cfg);

    // 2. Endless All Levels (sequential): must cycle 0..6 -> 0 endlessly and never record scores
    GameState gs;
    EndlessConfig ec_seq;
    ec_seq.type = EndlessModeType::AllLevels;
    ec_seq.shuffle = false;
    ec_seq.no_time_limit = false;
    uint64_t t = 1000;
    gs.start_endless_game(ec_seq, t);
    assert(gs.is_endless() && !gs.no_time_limit() && gs.endless_stage_count() == 1 && gs.level() == 0);

    for (int step = 0; step < 8; ++step) {
        assert(gs.level() == (step % 7));
        assert(gs.endless_stage_count() == step + 1);
        clear_current_stage(gs, t);
        auto evs = gs.consume_events();
        bool saw_clear = false;
        for (const auto& ev : evs) {
            assert(ev.type != GameEventType::AllStagesClearModal);
            if (ev.type == GameEventType::StageClearModal) {
                saw_clear = true;
                assert(ev.is_endless && !ev.records_score);
                assert(ev.next_level_display == step + 2);
                assert(ev.next_shift_level == ((step + 1) % 7));
            }
        }
        assert(saw_clear);
        gs.acknowledge_modal(t += 50);
        assert(!gs.in_title_screen() && gs.board_active());
    }
    assert(gs.endless_stage_count() == 9 && gs.level() == 1);

    // Timed Endless timeout must NEVER record high scores
    gs.tick(t += 610000);
    assert(gs.awaiting_modal());
    {
        auto evs = gs.consume_events();
        bool saw_timeout = false;
        for (const auto& ev : evs) {
            if (ev.type == GameEventType::TimeoutGameOverModal) {
                saw_timeout = true;
                assert(ev.is_endless && !ev.records_score);
            }
        }
        assert(saw_timeout);
    }
    gs.acknowledge_modal(t += 50);
    assert(gs.in_title_screen() && !gs.is_endless());

    // 3. Endless All Levels (bag shuffled): 7 stages per bag, never repeats consecutively
    EndlessConfig ec_shuf;
    ec_shuf.type = EndlessModeType::AllLevels;
    ec_shuf.shuffle = true;
    ec_shuf.no_time_limit = true;
    gs.start_endless_game(ec_shuf, t += 100);
    assert(gs.is_endless() && gs.no_time_limit());

    // Advancing past 600s in No Time Limit mode must NEVER time out
    gs.tick(t += 700000);
    assert(!gs.awaiting_modal() && gs.board_active() && gs.remaining_seconds() == 600);

    int prev_lvl = -1;
    uint8_t bag_seen = 0;
    for (int step = 0; step < 14; ++step) {
        int cur = gs.level();
        assert(cur >= 0 && cur <= 6);
        assert(cur != prev_lvl);
        bag_seen |= static_cast<uint8_t>(1u << cur);
        prev_lvl = cur;
        if (step == 6 || step == 13) {
            assert(bag_seen == 0x7F);
            bag_seen = 0;
        }
        clear_current_stage(gs, t);
        gs.consume_events();
        gs.acknowledge_modal(t += 50);
    }

    // 4. Endless Specific Level(s) Loop: stages 3 (lvl 2) and 6 (lvl 5)
    EndlessConfig ec_spec;
    ec_spec.type = EndlessModeType::SpecificLevels;
    ec_spec.level_mask = static_cast<uint8_t>((1u << 2) | (1u << 5));
    ec_spec.shuffle = false;
    ec_spec.no_time_limit = true;
    gs.start_endless_game(ec_spec, t += 100);
    const int expected[4] = {2, 5, 2, 5};
    for (int step = 0; step < 4; ++step) {
        assert(gs.level() == expected[step]);
        clear_current_stage(gs, t);
        gs.consume_events();
        gs.acknowledge_modal(t += 50);
    }

    // 5. Endless Unlimited Shuffle: survives >10 deadlocks without losing lives or game-overing
    EndlessConfig ec_unlim;
    ec_unlim.type = EndlessModeType::SpecificLevels;
    ec_unlim.level_mask = 0x01; // Stage 1 (Static)
    ec_unlim.no_time_limit = false;
    ec_unlim.unlimited_shuffle = true;
    gs.start_endless_game(ec_unlim, t += 100);
    assert(gs.is_endless() && gs.unlimited_shuffle() && gs.lives() == 10);

    for (int round = 0; round < 15; ++round) {
        gs.board().clear_all();
        // 8x6 solid block (cols 2..9, rows 2..7): 24 interior cells + 24 boundary cells.
        // Pairing each boundary cell with a completely enclosed interior cell guarantees 0 valid moves!
        int16_t interior_idx = 0;
        int16_t boundary_id = 1;
        for (int r = 2; r <= 7; ++r) {
            for (int c = 2; c <= 9; ++c) {
                bool is_interior = (c >= 3 && c <= 8 && r >= 3 && r <= 6);
                int16_t tid = is_interior ? static_cast<int16_t>((interior_idx++ + 12) % 24 + 1)
                                          : boundary_id++;
                gs.board().cell(c, r) = {true, tid, tid};
            }
        }
        assert(interior_idx == 24 && boundary_id == 25);
        assert(!gs.board().has_valid_move());

        // Add one immediately clickable pair at (14, 2) & (14, 3) outside the 8x6 block
        gs.board().cell(14, 2) = {true, int16_t(30), int16_t(30)};
        gs.board().cell(14, 3) = {true, int16_t(30), int16_t(30)};
        gs.board().set_tiles_left(50);

        gs.on_mouse_down(1, 21.0f + 14 * 40 + 10, 6.0f + 2 * 50 + 10, t += 10);
        gs.on_mouse_down(1, 21.0f + 14 * 40 + 10, 6.0f + 3 * 50 + 10, t += 10);
        assert(gs.animating_link());
        gs.tick(t += 250);

        // Must have auto-shuffled out of deadlock with lives unchanged at 10 and zero Game Over modal
        assert(!gs.awaiting_modal() && gs.board_active());
        assert(gs.lives() == 10);
        assert(gs.board().has_valid_move());
        auto evs = gs.consume_events();
        bool saw_shuffle_sfx = false;
        for (const auto& ev : evs) {
            assert(ev.type != GameEventType::NoMovesGameOverModal);
            if (ev.type == GameEventType::PlaySfx && ev.sfx_id == 206) {
                saw_shuffle_sfx = true;
            }
        }
        assert(saw_shuffle_sfx);
    }

    // Stage clear with unlimited_shuffle keeps lives fixed at 10
    clear_current_stage(gs, t);
    gs.consume_events();
    assert(gs.lives() == 10);
    gs.acknowledge_modal(t += 50);
    assert(gs.lives() == 10);

    // Timeout with unlimited_shuffle awards 0 life bonus
    gs.tick(t += 610000);
    assert(gs.awaiting_modal());
    {
        auto evs = gs.consume_events();
        bool saw_timeout = false;
        for (const auto& ev : evs) {
            if (ev.type == GameEventType::TimeoutGameOverModal) {
                saw_timeout = true;
                assert(ev.bonus == 0 && !ev.records_score);
            }
        }
        assert(saw_timeout);
    }
    gs.acknowledge_modal(t += 50);
    assert(!gs.unlimited_shuffle());
}

static void test_app_paths() {
    auto def_user = AppPaths::default_user_data_dir();
    assert(!def_user.empty());
    assert(def_user.filename() == "llk2");

    auto tmp_override = std::filesystem::temp_directory_path() / "llk2_test_custom_data_dir";
    std::filesystem::remove_all(tmp_override);

    auto p_cli = AppPaths::resolve(nullptr, tmp_override);
    assert(p_cli.mode == StorageMode::CustomOverride);
    assert(p_cli.data_dir == std::filesystem::absolute(tmp_override));
    assert(p_cli.ensure_data_dir());
    assert(std::filesystem::is_directory(p_cli.data_dir));
    assert(p_cli.config_file() == p_cli.data_dir / "config.json");
    assert(p_cli.scores_file() == p_cli.data_dir / "scores.json");
    assert(p_cli.assets_dir() == p_cli.data_dir / "assets");
    assert(p_cli.mods_dir() == p_cli.data_dir / "mods");

    // Verify Config & HighScoreTable auto-create parent directories on save
    auto nested_cfg = tmp_override / "sub" / "config.json";
    auto nested_scores = tmp_override / "sub" / "scores.json";
    Config cfg;
    cfg.player_name = "Tester";
    assert(cfg.save(nested_cfg));
    assert(std::filesystem::is_regular_file(nested_cfg));

    HighScoreTable hs;
    hs.reset_empty();
    hs.insert_score("Tester", 1234.0f);
    assert(hs.save_json(nested_scores));
    assert(std::filesystem::is_regular_file(nested_scores));

    // Verify bundled resource lookup locates extract_assets.py in source tree
    auto extractor = p_cli.find_resource(std::filesystem::path("tools") / "extract_assets.py");
    assert(!extractor.empty() && std::filesystem::is_regular_file(extractor));

    std::filesystem::remove_all(tmp_override);
}

int main() {
    test_scramble_roundtrip();
    test_hst_and_json_persistence();
    test_config_and_i18n();
    test_gamestate_progression();
    test_endless_mode_and_fullscreen_config();
    test_app_paths();
    std::printf("[PASS] test_scoring_hst\n");
    return 0;
}
