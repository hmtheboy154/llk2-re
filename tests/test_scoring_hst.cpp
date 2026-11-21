#include "core/config.hpp"
#include "core/game_state.hpp"
#include "core/hst_score.hpp"
#include "core/i18n.hpp"

#include <cassert>
#include <cstdio>
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
}

int main() {
    test_scramble_roundtrip();
    test_hst_and_json_persistence();
    test_config_and_i18n();
    test_gamestate_progression();
    std::printf("[PASS] test_scoring_hst\n");
    return 0;
}
