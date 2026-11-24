#pragma once

#include <array>
#include <filesystem>
#include <string>

namespace llk2::core {

enum class MidiBackend {
    EmbeddedSoundFont = 0, // Built-in TinySoundFont (.sf2) via SDL audio
    NativeOS = 1,          // OS native MIDI device (ALSA Sequencer / WinMM / CoreMIDI)
    OstAudio = 2           // Lossless OST recordings (FLAC/MP3/WAV) via SDL audio
};

enum class WindowScaleMode {
    Scale1x = 0,     // 768 x 550
    Scale1_5x = 1,   // 1152 x 825
    Scale2x = 2,     // 1536 x 1100
    Scale3x = 3,     // 2304 x 1650
    FreeAspect = 4   // User-resizable window maintaining 768:550 aspect ratio
};

enum class ScaleFilter {
    Nearest = 0,     // Crisp pixel art
    Bilinear = 1     // Smooth interpolation
};

struct Config {
    bool music_on = true;
    bool sound_on = true;
    MidiBackend midi_backend = MidiBackend::EmbeddedSoundFont;
    std::array<bool, 3> midi_tracks_enabled = {true, true, true}; // 101.mid, 102.mid, 103.mid
    WindowScaleMode scale_mode = WindowScaleMode::Scale1x;
    ScaleFilter scale_filter = ScaleFilter::Nearest;
    bool borderless_fullscreen = false;
    std::string language = "zh-TW";
    std::string soundfont_path;
    std::string player_name = "Player";

    bool load(const std::filesystem::path& path);
    bool save(const std::filesystem::path& path) const;

    // Returns scale factor for fixed preset modes (1.0f, 1.5f, 2.0f, 3.0f).
    float scale_factor() const noexcept;

    // Checks if track_id (101, 102, or 103) is enabled.
    bool is_track_enabled(int track_id) const noexcept;
    void set_track_enabled(int track_id, bool enabled) noexcept;

    // Given current_track_id (100..103, where 100 means before first track),
    // returns the next enabled track ID (101..103) in circular order, or 0 if none enabled.
    int next_enabled_midi_track(int current_track_id) const noexcept;
};

} // namespace llk2::core
