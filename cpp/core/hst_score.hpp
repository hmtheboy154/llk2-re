#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace llk2::core {

struct HighScoreEntry {
    std::string name;
    float score = 0.0f;
};

class HighScoreTable {
public:
    static constexpr int kCount = 10;
    static constexpr size_t kLegacyRecordBytes = 12; // 8-byte fixed name + 4-byte float
    static constexpr size_t kLegacyFileBytes = kCount * kLegacyRecordBytes; // 120 bytes

    HighScoreTable();

    // VB5 slot 40: Scramble (RTrim spaces, then shift byte[i] by +(i+1))
    static std::string scramble(std::string_view s);

    // VB5 slot 41: Unscramble (RTrim spaces, then shift byte[i] by -(i+1))
    static std::string unscramble(std::string_view s);

    // VB5 slot 45: ByteLen — counts legacy DBCS byte width (ASCII = 1, non-ASCII = 2)
    static int legacy_dbcs_byte_len(std::string_view utf8_str);

    // VB5 slot 24: InitHighScoreFile — resets all 10 entries to empty name & score 0
    void reset_empty();

    // VB5 slot 25: IsHighScore — true if sc > entries_[9].score
    bool is_high_score(float sc) const noexcept;

    // VB5 slot 44: InsertScore — inserts at slot 9 and bubble-sorts upward; returns new rank (0..9) or -1
    int insert_score(const std::string& player_name, float sc);

    // VB5 slot 48/49: update name at given rank
    void update_rank_name(int rank, const std::string& player_name);

    // Legacy 120-byte d4s.hst binary I/O
    bool load_legacy_hst(const std::filesystem::path& path);
    bool save_legacy_hst(const std::filesystem::path& path) const;

    // Modern UTF-8 scores.json I/O
    bool load_json(const std::filesystem::path& path);
    bool save_json(const std::filesystem::path& path) const;

    const std::array<HighScoreEntry, kCount>& entries() const noexcept { return entries_; }
    HighScoreEntry& entry(int idx) { return entries_.at(idx); }
    const HighScoreEntry& entry(int idx) const { return entries_.at(idx); }

private:
    std::array<HighScoreEntry, kCount> entries_{};
};

} // namespace llk2::core
