#include "config.hpp"
#include <fstream>
#include <sstream>

namespace llk2::core {

float Config::scale_factor() const noexcept {
    switch (scale_mode) {
        case WindowScaleMode::Scale1x: return 1.0f;
        case WindowScaleMode::Scale1_5x: return 1.5f;
        case WindowScaleMode::Scale2x: return 2.0f;
        case WindowScaleMode::Scale3x: return 3.0f;
        case WindowScaleMode::FreeAspect: return 1.0f;
    }
    return 1.0f;
}

bool Config::is_track_enabled(int track_id) const noexcept {
    if (track_id >= 101 && track_id <= 103) {
        return midi_tracks_enabled[track_id - 101];
    }
    return false;
}

void Config::set_track_enabled(int track_id, bool enabled) noexcept {
    if (track_id >= 101 && track_id <= 103) {
        midi_tracks_enabled[track_id - 101] = enabled;
    }
}

int Config::next_enabled_midi_track(int current_track_id) const noexcept {
    int start_idx = 0;
    if (current_track_id >= 101 && current_track_id <= 103) {
        start_idx = (current_track_id - 101 + 1) % 3;
    }
    for (int step = 0; step < 3; ++step) {
        int idx = (start_idx + step) % 3;
        if (midi_tracks_enabled[idx]) {
            return 101 + idx;
        }
    }
    return 0;
}

static std::string escape_json_str(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

bool Config::save(const std::filesystem::path& path) const {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        return false;
    }
    f << "{\n"
      << "  \"music_on\": " << (music_on ? "true" : "false") << ",\n"
      << "  \"sound_on\": " << (sound_on ? "true" : "false") << ",\n"
      << "  \"midi_backend\": \"" << (midi_backend == MidiBackend::NativeOS ? "native" : "embedded") << "\",\n"
      << "  \"midi_track_101\": " << (midi_tracks_enabled[0] ? "true" : "false") << ",\n"
      << "  \"midi_track_102\": " << (midi_tracks_enabled[1] ? "true" : "false") << ",\n"
      << "  \"midi_track_103\": " << (midi_tracks_enabled[2] ? "true" : "false") << ",\n"
      << "  \"scale_mode\": " << static_cast<int>(scale_mode) << ",\n"
      << "  \"scale_filter\": \"" << (scale_filter == ScaleFilter::Bilinear ? "bilinear" : "nearest") << "\",\n"
      << "  \"language\": \"" << escape_json_str(language) << "\",\n"
      << "  \"soundfont_path\": \"" << escape_json_str(soundfont_path) << "\",\n"
      << "  \"player_name\": \"" << escape_json_str(player_name) << "\"\n"
      << "}\n";
    return f.good();
}

static bool extract_bool(const std::string& json, const std::string& key, bool def_val) {
    size_t k = json.find("\"" + key + "\"");
    if (k == std::string::npos) return def_val;
    size_t c = json.find(':', k);
    if (c == std::string::npos) return def_val;
    size_t v = json.find_first_not_of(" \t\r\n", c + 1);
    if (v == std::string::npos) return def_val;
    if (json.compare(v, 4, "true") == 0) return true;
    if (json.compare(v, 5, "false") == 0) return false;
    return def_val;
}

static int extract_int(const std::string& json, const std::string& key, int def_val) {
    size_t k = json.find("\"" + key + "\"");
    if (k == std::string::npos) return def_val;
    size_t c = json.find(':', k);
    if (c == std::string::npos) return def_val;
    size_t v = json.find_first_of("-0123456789", c + 1);
    if (v == std::string::npos) return def_val;
    size_t e = json.find_first_not_of("-0123456789", v);
    try {
        return std::stoi(json.substr(v, e - v));
    } catch (...) {
        return def_val;
    }
}

static std::string extract_str(const std::string& json, const std::string& key, const std::string& def_val) {
    size_t k = json.find("\"" + key + "\"");
    if (k == std::string::npos) return def_val;
    size_t c = json.find(':', k);
    if (c == std::string::npos) return def_val;
    size_t q1 = json.find('"', c + 1);
    if (q1 == std::string::npos) return def_val;
    std::string out;
    size_t i = q1 + 1;
    while (i < json.size()) {
        if (json[i] == '\\' && i + 1 < json.size()) {
            out += json[i + 1];
            i += 2;
        } else if (json[i] == '"') {
            break;
        } else {
            out += json[i++];
        }
    }
    return out;
}

bool Config::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string s = ss.str();

    music_on = extract_bool(s, "music_on", music_on);
    sound_on = extract_bool(s, "sound_on", sound_on);
    std::string backend = extract_str(s, "midi_backend", "embedded");
    midi_backend = (backend == "native") ? MidiBackend::NativeOS : MidiBackend::EmbeddedSoundFont;
    midi_tracks_enabled[0] = extract_bool(s, "midi_track_101", true);
    midi_tracks_enabled[1] = extract_bool(s, "midi_track_102", true);
    midi_tracks_enabled[2] = extract_bool(s, "midi_track_103", true);
    int sm = extract_int(s, "scale_mode", 0);
    if (sm >= 0 && sm <= 4) {
        scale_mode = static_cast<WindowScaleMode>(sm);
    }
    std::string filt = extract_str(s, "scale_filter", "nearest");
    scale_filter = (filt == "bilinear") ? ScaleFilter::Bilinear : ScaleFilter::Nearest;
    language = extract_str(s, "language", language);
    soundfont_path = extract_str(s, "soundfont_path", soundfont_path);
    player_name = extract_str(s, "player_name", player_name);
    return true;
}

} // namespace llk2::core
