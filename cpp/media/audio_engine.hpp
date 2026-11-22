#pragma once

#include "core/config.hpp"
#include "media/asset_manager.hpp"
#include "media/midi_native.hpp"
#include "media/sdl_compat.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

struct tsf;
struct tml_message;

namespace llk2::media {

class AudioEngine {
public:
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int CHANNELS = 2;

    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // Initialize SDL audio + WAV SFX + OST / Embedded SoundFont / Native OS MIDI backends
    bool init(const AssetManager& assets, const core::Config& cfg, std::string* err_out = nullptr);
    void shutdown();

    bool is_initialized() const { return initialized_; }
    bool has_ost_available() const { return has_ost_; }

    // Trigger one of the 6 WAV sound effects (sfx_id = 201..206) — lock-free for 0ms GUI latency
    void play_sfx(int sfx_id);

    // Synchronize BGM state, backend choice, and enabled tracks with Config, and advance playlist rotation
    void update(const core::Config& cfg);

    // Load a specific .sf2 SoundFont file into the embedded synth
    bool load_soundfont(const std::filesystem::path& sf2_path);

    // Locate bundled or explicitly configured .sf2 SoundFont (never scans user home or external configs)
    static std::filesystem::path discover_soundfont_path(const std::string& configured_path = "");

    int current_midi_track() const { return current_track_; }
    bool is_midi_playing() const;
    const std::string& active_soundfont_name() const { return active_sf2_name_; }
    const NativeMidiPlayer& native_midi() const { return native_midi_; }

    // Render stereo float PCM frames into out_stereo_frames (frames * 2 floats).
    // Called automatically by SDL audio callback, or manually in headless tests.
    void mix_audio_frames(float* out_stereo_frames, int num_frames);

    // Zero-allocation scratch buffer helper for SDL3 audio stream callback
    float* scratch_buffer(size_t num_floats);

private:
    struct DecodedWav {
        std::vector<float> stereo_samples; // interleaved L, R at 44100 Hz
    };

    struct ActiveVoice {
        int sfx_index = -1; // 0..5
        size_t cursor = 0;  // index in stereo_samples
    };

    bool load_wav_to_float(const std::filesystem::path& wav_path, DecodedWav& out_wav);
    bool init_embedded_synth(const core::Config& cfg);
    bool start_midi_track(int track_idx, core::MidiBackend backend);
    bool open_ost_stream(const std::filesystem::path& ost_path);
    void close_ost_stream_locked();
    void stop_all_midi();

    static std::vector<uint8_t> build_minimal_sf2_bytes();

    bool initialized_ = false;
    bool audio_device_open_ = false;
    bool has_ost_ = false;

#if defined(LLK2_USE_SDL3)
    SDL_AudioStream* sdl_stream_ = nullptr;
#else
    SDL_AudioDeviceID sdl_dev_ = 0;
#endif
    std::vector<float> sdl_scratch_;

    std::array<DecodedWav, 6> sfx_bank_;
    std::array<std::filesystem::path, 3> midi_files_;
    std::array<std::filesystem::path, 3> ost_files_;

    // Lock-free SPSC ring buffer for instant GUI -> Audio SFX triggers
    static constexpr size_t kSfxQueueCap = 32;
    std::array<int, kSfxQueueCap> sfx_ring_{};
    std::atomic<uint32_t> sfx_write_{0};
    std::atomic<uint32_t> sfx_read_{0};

    mutable std::mutex audio_mutex_;
    std::vector<ActiveVoice> active_voices_;

    // Embedded TinySoundFont + TinyMidiLoader state
    tsf* tsf_synth_ = nullptr;
    std::string active_sf2_name_ = "None";
    std::array<tml_message*, 3> preloaded_midi_{nullptr, nullptr, nullptr};
    tml_message* embedded_cursor_ = nullptr;
    double embedded_msec_ = 0.0;
    std::atomic<bool> embedded_playing_{false};
    std::atomic<bool> embedded_finished_{false};

    // Streaming OST Audio (FLAC / MP3) state
    void* ost_flac_ = nullptr; // drflac*
    void* ost_mp3_ = nullptr;  // drmp3*
    uint32_t ost_channels_ = 2;
    std::atomic<bool> ost_playing_{false};
    std::atomic<bool> ost_finished_{false};

    // Native OS MIDI device backend
    NativeMidiPlayer native_midi_;

    // Playlist rotation state
    std::atomic<bool> sfx_enabled_{true};
    bool bgm_enabled_ = true;
    core::MidiBackend active_backend_ = core::MidiBackend::EmbeddedSoundFont;
    int current_track_ = -1; // 0, 1, 2
};

} // namespace llk2::media
