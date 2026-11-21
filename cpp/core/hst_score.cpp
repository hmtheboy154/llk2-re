#include "hst_score.hpp"
#include <cstring>
#include <fstream>
#include <sstream>

namespace llk2::core {

HighScoreTable::HighScoreTable() {
    reset_empty();
}

void HighScoreTable::reset_empty() {
    for (auto& e : entries_) {
        e.name.clear();
        e.score = 0.0f;
    }
}

std::string HighScoreTable::scramble(std::string_view s) {
    // RTrim ASCII spaces (' ')
    size_t end = s.size();
    while (end > 0 && s[end - 1] == ' ') {
        --end;
    }
    std::string out(s.substr(0, end));
    for (size_t i = 0; i < out.size(); ++i) {
        uint8_t b = static_cast<uint8_t>(out[i]);
        out[i] = static_cast<char>(static_cast<uint8_t>(b + static_cast<uint8_t>(i + 1)));
    }
    return out;
}

std::string HighScoreTable::unscramble(std::string_view s) {
    size_t end = s.size();
    while (end > 0 && s[end - 1] == ' ') {
        --end;
    }
    std::string out(s.substr(0, end));
    for (size_t i = 0; i < out.size(); ++i) {
        uint8_t b = static_cast<uint8_t>(out[i]);
        out[i] = static_cast<char>(static_cast<uint8_t>(b - static_cast<uint8_t>(i + 1)));
    }
    return out;
}

int HighScoreTable::legacy_dbcs_byte_len(std::string_view utf8_str) {
    int total = 0;
    size_t i = 0;
    while (i < utf8_str.size()) {
        uint8_t c = static_cast<uint8_t>(utf8_str[i]);
        if (c < 0x80) {
            total += 1;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            total += 2;
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            total += 2;
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            total += 2;
            i += 4;
        } else {
            // Raw non-UTF8 DBCS byte pair fallback
            total += 1;
            i += 1;
        }
    }
    return total;
}

bool HighScoreTable::is_high_score(float sc) const noexcept {
    return sc > entries_[kCount - 1].score;
}

int HighScoreTable::insert_score(const std::string& player_name, float sc) {
    if (!is_high_score(sc)) {
        return -1;
    }
    entries_[kCount - 1].name = player_name;
    entries_[kCount - 1].score = sc;
    int new_rank = kCount - 1;
    for (int i = kCount - 1; i >= 1; --i) {
        if (entries_[i].score > entries_[i - 1].score) {
            std::swap(entries_[i], entries_[i - 1]);
            new_rank = i - 1;
        }
    }
    return new_rank;
}

void HighScoreTable::update_rank_name(int rank, const std::string& player_name) {
    if (rank >= 0 && rank < kCount) {
        entries_[rank].name = player_name;
    }
}

bool HighScoreTable::load_legacy_hst(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    std::array<char, kLegacyFileBytes> buf{};
    f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
    if (f.gcount() != static_cast<std::streamsize>(kLegacyFileBytes)) {
        return false;
    }
    for (int i = 0; i < kCount; ++i) {
        const char* rec = buf.data() + i * kLegacyRecordBytes;
        std::string_view raw_name(rec, 8);
        entries_[i].name = unscramble(raw_name);
        float sc = 0.0f;
        std::memcpy(&sc, rec + 8, sizeof(float));
        entries_[i].score = sc;
    }
    return true;
}

bool HighScoreTable::save_legacy_hst(const std::filesystem::path& path) const {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        return false;
    }
    std::array<char, kLegacyFileBytes> buf{};
    buf.fill(' ');
    for (int i = 0; i < kCount; ++i) {
        char* rec = buf.data() + i * kLegacyRecordBytes;
        std::string scr = scramble(entries_[i].name);
        // Fixed String * 8 padded with spaces (' ')
        std::memset(rec, ' ', 8);
        std::memcpy(rec, scr.data(), std::min<size_t>(8, scr.size()));
        float sc = entries_[i].score;
        std::memcpy(rec + 8, &sc, sizeof(float));
    }
    f.write(buf.data(), static_cast<std::streamsize>(buf.size()));
    return f.good();
}

static std::string json_escape(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (char ch : s) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += ch; break;
        }
    }
    return out;
}

bool HighScoreTable::save_json(const std::filesystem::path& path) const {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        return false;
    }
    f << "{\n  \"scores\": [\n";
    for (int i = 0; i < kCount; ++i) {
        f << "    { \"name\": \"" << json_escape(entries_[i].name)
          << "\", \"score\": " << entries_[i].score << " }"
          << (i + 1 < kCount ? "," : "") << "\n";
    }
    f << "  ]\n}\n";
    return f.good();
}

bool HighScoreTable::load_json(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();

    std::array<HighScoreEntry, kCount> parsed{};
    size_t pos = 0;
    int idx = 0;
    while (idx < kCount) {
        size_t nk = content.find("\"name\"", pos);
        if (nk == std::string::npos) break;
        size_t colon = content.find(':', nk + 6);
        if (colon == std::string::npos) break;
        size_t q1 = content.find('"', colon + 1);
        if (q1 == std::string::npos) break;
        std::string name;
        size_t cur = q1 + 1;
        while (cur < content.size()) {
            if (content[cur] == '\\' && cur + 1 < content.size()) {
                char esc = content[cur + 1];
                if (esc == '"' || esc == '\\' || esc == '/') name += esc;
                else if (esc == 'n') name += '\n';
                else if (esc == 'r') name += '\r';
                else if (esc == 't') name += '\t';
                else name += esc;
                cur += 2;
            } else if (content[cur] == '"') {
                ++cur;
                break;
            } else {
                name += content[cur++];
            }
        }
        size_t sk = content.find("\"score\"", cur);
        if (sk == std::string::npos) break;
        size_t scolon = content.find(':', sk + 7);
        if (scolon == std::string::npos) break;
        size_t num_start = content.find_first_of("-0123456789.", scolon + 1);
        if (num_start == std::string::npos) break;
        size_t num_end = content.find_first_not_of("-0123456789.eE+", num_start);
        float sc = std::stof(content.substr(num_start, num_end - num_start));
        parsed[idx].name = std::move(name);
        parsed[idx].score = sc;
        ++idx;
        pos = (num_end == std::string::npos) ? content.size() : num_end;
    }
    if (idx == 0) {
        return false;
    }
    entries_ = std::move(parsed);
    return true;
}

} // namespace llk2::core
