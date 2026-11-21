#include "media/audio_engine.hpp"

#define TSF_IMPLEMENTATION
#include "tsf.h"

#define TML_IMPLEMENTATION
#include "tml.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace llk2::media {

namespace {

#if defined(LLK2_USE_SDL3)
void SDLCALL sdl3_audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int /*total_amount*/) {
    if (additional_amount <= 0) return;
    auto* engine = static_cast<AudioEngine*>(userdata);
    int num_frames = additional_amount / (static_cast<int>(sizeof(float)) * AudioEngine::CHANNELS);
    if (num_frames <= 0) return;

    std::vector<float> buf(static_cast<size_t>(num_frames) * AudioEngine::CHANNELS, 0.0f);
    engine->mix_audio_frames(buf.data(), num_frames);
    SDL_PutAudioStreamData(stream, buf.data(), static_cast<int>(buf.size() * sizeof(float)));
}
#else
void SDLCALL sdl2_audio_callback(void* userdata, Uint8* stream, int len) {
    auto* engine = static_cast<AudioEngine*>(userdata);
    int num_frames = len / (static_cast<int>(sizeof(float)) * AudioEngine::CHANNELS);
    auto* out = reinterpret_cast<float*>(stream);
    engine->mix_audio_frames(out, num_frames);
}
#endif

void append_u16(std::vector<uint8_t>& v, uint16_t val) {
    v.push_back(static_cast<uint8_t>(val & 0xFF));
    v.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

void append_u32(std::vector<uint8_t>& v, uint32_t val) {
    v.push_back(static_cast<uint8_t>(val & 0xFF));
    v.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    v.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    v.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

void append_fourcc(std::vector<uint8_t>& v, const char tag[4]) {
    v.insert(v.end(), tag, tag + 4);
}

void append_fixed_str(std::vector<uint8_t>& v, const char* s, size_t len) {
    size_t slen = std::strlen(s);
    for (size_t i = 0; i < len; ++i) {
        v.push_back(i < slen ? static_cast<uint8_t>(s[i]) : 0);
    }
}

void append_chunk(std::vector<uint8_t>& dst, const char tag[4], const std::vector<uint8_t>& payload) {
    append_fourcc(dst, tag);
    append_u32(dst, static_cast<uint32_t>(payload.size()));
    dst.insert(dst.end(), payload.begin(), payload.end());
    if (payload.size() & 1) dst.push_back(0);
}

} // namespace

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
    shutdown();
}

std::filesystem::path AudioEngine::discover_soundfont_path(const std::string& configured_path) {
    if (!configured_path.empty() && std::filesystem::exists(configured_path)) {
        return configured_path;
    }

    // 1. Check local workspace / assets
    std::vector<std::filesystem::path> direct_candidates = {
        "assets/soundfont.sf2",
        "soundfont.sf2",
        "/usr/share/soundfonts/FluidR3_GM.sf2",
        "/usr/share/soundfonts/default.sf2",
        "/usr/share/soundfonts/freepats-general-midi.sf2",
        "/usr/share/sounds/sf2/FluidR3_GM.sf2",
        "/usr/share/sounds/sf2/TimGM6mb.sf2",
        "/usr/share/sounds/sf2/default-GM.sf2",
    };

    // 2. Parse ~/.config/rncbc.org/Qsynth.conf if available
    if (const char* home = std::getenv("HOME")) {
        auto qsynth_conf = std::filesystem::path(home) / ".config" / "rncbc.org" / "Qsynth.conf";
        if (std::filesystem::exists(qsynth_conf)) {
            std::ifstream in(qsynth_conf);
            std::string line;
            while (std::getline(in, line)) {
                auto eq = line.find('=');
                if (eq != std::string::npos && line.find("SoundFont") != std::string::npos) {
                    std::string val = line.substr(eq + 1);
                    while (!val.empty() && (val.back() == '\r' || val.back() == '\n' || val.back() == ' ')) {
                        val.pop_back();
                    }
                    if (!val.empty() && std::filesystem::exists(val)) {
                        direct_candidates.insert(direct_candidates.begin(), val);
                    }
                }
            }
        }
    }

    for (const auto& c : direct_candidates) {
        if (std::filesystem::exists(c)) return c;
    }

    // 3. Scan /usr/share/soundfonts and /usr/share/sounds/sf2 for any .sf2 file
    for (const char* dir_str : {"/usr/share/soundfonts", "/usr/share/sounds/sf2"}) {
        std::filesystem::path d(dir_str);
        if (std::filesystem::exists(d) && std::filesystem::is_directory(d)) {
            for (const auto& entry : std::filesystem::directory_iterator(d)) {
                auto ext = entry.path().extension().string();
                if (ext == ".sf2" || ext == ".SF2") {
                    return entry.path();
                }
            }
        }
    }

    return {};
}

// Builds a valid in-memory SoundFont 2.01 (.sf2) with 128 GM presets + Drum bank 128
// backed by a rich harmonic wavetable sample so TinySoundFont always works even without an external .sf2 file.
std::vector<uint8_t> AudioEngine::build_minimal_sf2_bytes() {
    // 1. INFO-list
    std::vector<uint8_t> ifil_data;
    append_u16(ifil_data, 2);
    append_u16(ifil_data, 1);
    std::vector<uint8_t> isng_data;
    append_fixed_str(isng_data, "EMU8000", 8);
    std::vector<uint8_t> inam_data;
    append_fixed_str(inam_data, "LLK2 Builtin GM Synth", 22);

    std::vector<uint8_t> info_list;
    append_fourcc(info_list, "INFO");
    append_chunk(info_list, "ifil", ifil_data);
    append_chunk(info_list, "isng", isng_data);
    append_chunk(info_list, "INAM", inam_data);

    // 2. sdta-list: 100-sample loopable single cycle at 44100 Hz (441 Hz ≈ A4, MIDI key 69) + 46 zero trailer samples
    constexpr int kLoopLen = 100;
    constexpr int kTotalSamples = kLoopLen + 46;
    std::vector<uint8_t> smpl_data;
    smpl_data.reserve(kTotalSamples * 2);
    for (int i = 0; i < kTotalSamples; ++i) {
        int16_t s = 0;
        if (i < kLoopLen) {
            double phase = 2.0 * 3.14159265358979323846 * static_cast<double>(i) / static_cast<double>(kLoopLen);
            double v = 0.65 * std::sin(phase) + 0.22 * std::sin(2.0 * phase) + 0.08 * std::sin(3.0 * phase);
            s = static_cast<int16_t>(std::clamp(v * 18000.0, -32767.0, 32767.0));
        }
        append_u16(smpl_data, static_cast<uint16_t>(s));
    }
    std::vector<uint8_t> sdta_list;
    append_fourcc(sdta_list, "sdta");
    append_chunk(sdta_list, "smpl", smpl_data);

    // 3. pdta-list
    // Create 129 presets: Bank 0 Presets 0..127 + Bank 128 Preset 0 (Drums)
    constexpr int kNumPresets = 129;
    std::vector<uint8_t> phdr_data;
    for (int p = 0; p < kNumPresets; ++p) {
        uint16_t preset_num = (p < 128) ? static_cast<uint16_t>(p) : 0;
        uint16_t bank_num = (p < 128) ? 0 : 128;
        append_fixed_str(phdr_data, "GM Preset", 20);
        append_u16(phdr_data, preset_num);
        append_u16(phdr_data, bank_num);
        append_u16(phdr_data, static_cast<uint16_t>(p)); // pbagNdx
        append_u32(phdr_data, 0);
        append_u32(phdr_data, 0);
        append_u32(phdr_data, 0);
    }
    // Terminal phdr record (EOP)
    append_fixed_str(phdr_data, "EOP", 20);
    append_u16(phdr_data, 0);
    append_u16(phdr_data, 0);
    append_u16(phdr_data, static_cast<uint16_t>(kNumPresets));
    append_u32(phdr_data, 0);
    append_u32(phdr_data, 0);
    append_u32(phdr_data, 0);

    std::vector<uint8_t> pbag_data;
    for (int p = 0; p <= kNumPresets; ++p) {
        append_u16(pbag_data, static_cast<uint16_t>(p)); // pgenNdx
        append_u16(pbag_data, 0);                        // pmodNdx
    }

    std::vector<uint8_t> pmod_data(10, 0);

    std::vector<uint8_t> pgen_data;
    for (int p = 0; p < kNumPresets; ++p) {
        append_u16(pgen_data, 41); // Gen_Instrument = 41
        append_u16(pgen_data, 0);  // Instrument 0
    }
    append_u16(pgen_data, 0);
    append_u16(pgen_data, 0);

    std::vector<uint8_t> inst_data;
    append_fixed_str(inst_data, "GM Inst", 20);
    append_u16(inst_data, 0);
    append_fixed_str(inst_data, "EOI", 20);
    append_u16(inst_data, 1);

    std::vector<uint8_t> ibag_data;
    append_u16(ibag_data, 0);
    append_u16(ibag_data, 0);
    append_u16(ibag_data, 3);
    append_u16(ibag_data, 0);

    std::vector<uint8_t> imod_data(10, 0);

    // igen: 0) sampleModes (54) = 1 (loop continuously), 1) releaseVolEnv (38) = -1200, 2) sampleID (53) = 0
    std::vector<uint8_t> igen_data;
    append_u16(igen_data, 54);
    append_u16(igen_data, 1);
    append_u16(igen_data, 38);
    append_u16(igen_data, static_cast<uint16_t>(static_cast<int16_t>(-1200)));
    append_u16(igen_data, 53);
    append_u16(igen_data, 0);
    append_u16(igen_data, 0);
    append_u16(igen_data, 0);

    // shdr (46 bytes per record)
    std::vector<uint8_t> shdr_data;
    append_fixed_str(shdr_data, "SynthWave", 20);
    append_u32(shdr_data, 0);         // dwStart
    append_u32(shdr_data, kLoopLen);  // dwEnd
    append_u32(shdr_data, 0);         // dwStartloop
    append_u32(shdr_data, kLoopLen);  // dwEndloop
    append_u32(shdr_data, 44100);     // dwSampleRate
    shdr_data.push_back(69);          // byOriginalPitch (A4 = 69)
    shdr_data.push_back(0);           // chPitchCorrection
    append_u16(shdr_data, 0);         // wSampleLink
    append_u16(shdr_data, 1);         // sfSampleType = monoSample

    append_fixed_str(shdr_data, "EOS", 20);
    append_u32(shdr_data, 0);
    append_u32(shdr_data, 0);
    append_u32(shdr_data, 0);
    append_u32(shdr_data, 0);
    append_u32(shdr_data, 0);
    shdr_data.push_back(0);
    shdr_data.push_back(0);
    append_u16(shdr_data, 0);
    append_u16(shdr_data, 0);

    std::vector<uint8_t> pdta_list;
    append_fourcc(pdta_list, "pdta");
    append_chunk(pdta_list, "phdr", phdr_data);
    append_chunk(pdta_list, "pbag", pbag_data);
    append_chunk(pdta_list, "pmod", pmod_data);
    append_chunk(pdta_list, "pgen", pgen_data);
    append_chunk(pdta_list, "inst", inst_data);
    append_chunk(pdta_list, "ibag", ibag_data);
    append_chunk(pdta_list, "imod", imod_data);
    append_chunk(pdta_list, "igen", igen_data);
    append_chunk(pdta_list, "shdr", shdr_data);

    std::vector<uint8_t> riff_payload;
    append_fourcc(riff_payload, "sfbk");
    append_chunk(riff_payload, "LIST", info_list);
    append_chunk(riff_payload, "LIST", sdta_list);
    append_chunk(riff_payload, "LIST", pdta_list);

    std::vector<uint8_t> sf2;
    append_chunk(sf2, "RIFF", riff_payload);
    return sf2;
}

bool AudioEngine::load_wav_to_float(const std::filesystem::path& wav_path, DecodedWav& out_wav) {
    out_wav.stereo_samples.clear();
#if defined(LLK2_USE_SDL3)
    SDL_AudioSpec src_spec{};
    Uint8* wav_buf = nullptr;
    Uint32 wav_len = 0;
    if (!SDL_LoadWAV(wav_path.string().c_str(), &src_spec, &wav_buf, &wav_len)) {
        return false;
    }
    SDL_AudioSpec dst_spec{};
    dst_spec.format = SDL_AUDIO_F32;
    dst_spec.channels = CHANNELS;
    dst_spec.freq = SAMPLE_RATE;

    Uint8* cvt_buf = nullptr;
    int cvt_len = 0;
    bool ok = SDL_ConvertAudioSamples(&src_spec, wav_buf, static_cast<int>(wav_len), &dst_spec, &cvt_buf, &cvt_len);
    SDL_free(wav_buf);
    if (!ok || !cvt_buf || cvt_len <= 0) {
        if (cvt_buf) SDL_free(cvt_buf);
        return false;
    }
    size_t num_floats = static_cast<size_t>(cvt_len) / sizeof(float);
    const auto* fptr = reinterpret_cast<const float*>(cvt_buf);
    out_wav.stereo_samples.assign(fptr, fptr + num_floats);
    SDL_free(cvt_buf);
    return true;
#else
    SDL_AudioSpec src_spec{};
    Uint8* wav_buf = nullptr;
    Uint32 wav_len = 0;
    if (!SDL_LoadWAV(wav_path.string().c_str(), &src_spec, &wav_buf, &wav_len)) {
        return false;
    }
    SDL_AudioCVT cvt{};
    if (SDL_BuildAudioCVT(&cvt, src_spec.format, src_spec.channels, src_spec.freq,
                          AUDIO_F32SYS, CHANNELS, SAMPLE_RATE) < 0) {
        SDL_FreeWAV(wav_buf);
        return false;
    }
    cvt.len = static_cast<int>(wav_len);
    cvt.buf = static_cast<Uint8*>(SDL_malloc(static_cast<size_t>(cvt.len) * cvt.len_mult));
    std::memcpy(cvt.buf, wav_buf, wav_len);
    SDL_FreeWAV(wav_buf);

    if (SDL_ConvertAudio(&cvt) < 0) {
        SDL_free(cvt.buf);
        return false;
    }
    size_t num_floats = static_cast<size_t>(cvt.len_cvt) / sizeof(float);
    const auto* fptr = reinterpret_cast<const float*>(cvt.buf);
    out_wav.stereo_samples.assign(fptr, fptr + num_floats);
    SDL_free(cvt.buf);
    return true;
#endif
}

bool AudioEngine::load_soundfont(const std::filesystem::path& sf2_path) {
    if (!std::filesystem::exists(sf2_path)) return false;
    tsf* loaded = tsf_load_filename(sf2_path.string().c_str());
    if (!loaded) return false;
    tsf_set_output(loaded, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, 0.0f);
    tsf_channel_set_bank_preset(loaded, 9, 128, 0); // MIDI channel 10 (index 9) drum bank

    std::lock_guard<std::mutex> lock(audio_mutex_);
    if (tsf_synth_) {
        tsf_close(tsf_synth_);
    }
    tsf_synth_ = loaded;
    active_sf2_name_ = sf2_path.filename().string();
    return true;
}

bool AudioEngine::init_embedded_synth(const core::Config& cfg) {
    auto sf2_path = discover_soundfont_path(cfg.soundfont_path);
    if (!sf2_path.empty() && load_soundfont(sf2_path)) {
        return true;
    }

    // Fallback to builtin in-memory minimal SoundFont
    auto mem_sf2 = build_minimal_sf2_bytes();
    tsf* loaded = tsf_load_memory(mem_sf2.data(), static_cast<int>(mem_sf2.size()));
    if (!loaded) return false;
    tsf_set_output(loaded, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, 0.0f);
    tsf_channel_set_bank_preset(loaded, 9, 128, 0);

    std::lock_guard<std::mutex> lock(audio_mutex_);
    if (tsf_synth_) {
        tsf_close(tsf_synth_);
    }
    tsf_synth_ = loaded;
    active_sf2_name_ = "Builtin-GM-Fallback.sf2";
    return true;
}

bool AudioEngine::init(const AssetManager& assets, const core::Config& cfg, std::string* err_out) {
    shutdown();

    if (!assets.is_loaded()) {
        if (err_out) *err_out = "AssetManager is not loaded";
        return false;
    }

    // 1. Decode all 6 WAV sound effects (201..206.wav)
    for (int i = 0; i < 6; ++i) {
        int id = 201 + i;
        if (!load_wav_to_float(assets.sfx_path(id), sfx_bank_[i])) {
            if (err_out) *err_out = "Failed to decode WAV: " + assets.sfx_path(id).string();
            return false;
        }
    }

    // 2. Preload all 3 MIDI tracks (101..103.mid)
    for (int i = 0; i < 3; ++i) {
        midi_files_[i] = assets.midi_path(i);
        preloaded_midi_[i] = tml_load_filename(midi_files_[i].string().c_str());
        if (!preloaded_midi_[i]) {
            if (err_out) *err_out = "Failed to parse MIDI: " + midi_files_[i].string();
            shutdown();
            return false;
        }
    }

    // 3. Initialize Embedded SoundFont synthesizer
    if (!init_embedded_synth(cfg)) {
        if (err_out) *err_out = "Failed to initialize embedded TinySoundFont synthesizer";
        shutdown();
        return false;
    }

    // 4. Initialize Native OS MIDI player (non-fatal if OS has no ALSA/WinMM sequencer device)
    native_midi_.init();

    // 5. Try to open SDL audio output stream/device (non-fatal in headless CI)
    if (sdl_init_audio()) {
#if defined(LLK2_USE_SDL3)
        SDL_AudioSpec spec{};
        spec.format = SDL_AUDIO_F32;
        spec.channels = CHANNELS;
        spec.freq = SAMPLE_RATE;
        sdl_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, sdl3_audio_callback, this);
        if (sdl_stream_) {
            SDL_ResumeAudioStreamDevice(sdl_stream_);
            audio_device_open_ = true;
        }
#else
        SDL_AudioSpec desired{};
        desired.freq = SAMPLE_RATE;
        desired.format = AUDIO_F32SYS;
        desired.channels = CHANNELS;
        desired.samples = 1024;
        desired.callback = sdl2_audio_callback;
        desired.userdata = this;
        sdl_dev_ = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
        if (sdl_dev_ != 0) {
            SDL_PauseAudioDevice(sdl_dev_, 0);
            audio_device_open_ = true;
        }
#endif
    }

    sfx_enabled_ = cfg.sound_on;
    bgm_enabled_ = cfg.music_on;
    active_backend_ = cfg.midi_backend;
    current_track_ = -1;
    initialized_ = true;

    update(cfg);
    return true;
}

void AudioEngine::shutdown() {
#if defined(LLK2_USE_SDL3)
    if (sdl_stream_) {
        SDL_DestroyAudioStream(sdl_stream_);
        sdl_stream_ = nullptr;
    }
#else
    if (sdl_dev_ != 0) {
        SDL_CloseAudioDevice(sdl_dev_);
        sdl_dev_ = 0;
    }
#endif
    audio_device_open_ = false;

    native_midi_.shutdown();

    std::lock_guard<std::mutex> lock(audio_mutex_);
    active_voices_.clear();
    embedded_cursor_ = nullptr;
    embedded_playing_ = false;
    embedded_finished_ = false;

    for (auto*& m : preloaded_midi_) {
        if (m) {
            tml_free(m);
            m = nullptr;
        }
    }
    if (tsf_synth_) {
        tsf_close(tsf_synth_);
        tsf_synth_ = nullptr;
    }
    initialized_ = false;
}

void AudioEngine::play_sfx(int sfx_id) {
    if (!initialized_ || !sfx_enabled_) return;
    if (sfx_id < 201 || sfx_id > 206) return;
    int idx = sfx_id - 201;
    if (sfx_bank_[idx].stereo_samples.empty()) return;

    std::lock_guard<std::mutex> lock(audio_mutex_);
    active_voices_.push_back(ActiveVoice{idx, 0});
}

void AudioEngine::stop_all_midi() {
    native_midi_.stop();
    std::lock_guard<std::mutex> lock(audio_mutex_);
    embedded_playing_ = false;
    embedded_finished_ = false;
    embedded_cursor_ = nullptr;
    if (tsf_synth_) {
        tsf_note_off_all(tsf_synth_);
    }
}

bool AudioEngine::start_midi_track(int track_idx, core::MidiBackend backend) {
    if (track_idx < 0 || track_idx >= 3) return false;
    stop_all_midi();

    if (backend == core::MidiBackend::NativeOS && native_midi_.is_available()) {
        if (native_midi_.play_file(midi_files_[track_idx])) {
            current_track_ = track_idx;
            return true;
        }
    }

    // Embedded SoundFont Synth (or fallback if NativeOS unavailable)
    std::lock_guard<std::mutex> lock(audio_mutex_);
    if (!tsf_synth_ || !preloaded_midi_[track_idx]) return false;
    tsf_reset(tsf_synth_);
    tsf_channel_set_bank_preset(tsf_synth_, 9, 128, 0);
    embedded_cursor_ = preloaded_midi_[track_idx];
    embedded_msec_ = 0.0;
    embedded_finished_ = false;
    embedded_playing_ = true;
    current_track_ = track_idx;
    return true;
}

bool AudioEngine::is_midi_playing() const {
    if (!initialized_) return false;
    if (active_backend_ == core::MidiBackend::NativeOS && native_midi_.is_available()) {
        return native_midi_.is_playing();
    }
    std::lock_guard<std::mutex> lock(audio_mutex_);
    return embedded_playing_;
}

void AudioEngine::update(const core::Config& cfg) {
    if (!initialized_) return;
    sfx_enabled_ = cfg.sound_on;
    bgm_enabled_ = cfg.music_on;

    bool any_enabled = cfg.midi_tracks_enabled[0] || cfg.midi_tracks_enabled[1] || cfg.midi_tracks_enabled[2];
    if (!cfg.music_on || !any_enabled) {
        if (is_midi_playing() || current_track_ != -1) {
            stop_all_midi();
            current_track_ = -1;
        }
        active_backend_ = cfg.midi_backend;
        return;
    }

    bool backend_changed = (cfg.midi_backend != active_backend_);
    active_backend_ = cfg.midi_backend;

    bool track_disabled = (current_track_ < 0 || current_track_ >= 3 || !cfg.midi_tracks_enabled[current_track_]);
    bool track_done = false;
    if (active_backend_ == core::MidiBackend::NativeOS && native_midi_.is_available()) {
        track_done = !native_midi_.is_playing() || native_midi_.track_finished();
    } else {
        std::lock_guard<std::mutex> lock(audio_mutex_);
        track_done = !embedded_playing_ || embedded_finished_;
    }

    int cur_track_id = (current_track_ >= 0 && current_track_ < 3) ? (101 + current_track_) : 100;

    if (backend_changed) {
        int next_id = track_disabled ? cfg.next_enabled_midi_track(cur_track_id) : cur_track_id;
        if (next_id >= 101 && next_id <= 103) {
            start_midi_track(next_id - 101, active_backend_);
        }
        return;
    }

    if (track_disabled || track_done) {
        int next_id = cfg.next_enabled_midi_track(cur_track_id);
        if (next_id >= 101 && next_id <= 103) {
            start_midi_track(next_id - 101, active_backend_);
        }
    }
}

void AudioEngine::mix_audio_frames(float* out_stereo_frames, int num_frames) {
    if (!out_stereo_frames || num_frames <= 0) return;
    std::fill(out_stereo_frames, out_stereo_frames + static_cast<size_t>(num_frames) * CHANNELS, 0.0f);

    std::lock_guard<std::mutex> lock(audio_mutex_);

    // 1. Render Embedded SoundFont MIDI in 64-frame blocks for sample-accurate event timing
    if (embedded_playing_ && tsf_synth_ && embedded_cursor_) {
        int frames_left = num_frames;
        float* write_ptr = out_stereo_frames;
        while (frames_left > 0) {
            int block = std::min(frames_left, 64);
            embedded_msec_ += static_cast<double>(block) * (1000.0 / static_cast<double>(SAMPLE_RATE));

            while (embedded_cursor_ && embedded_msec_ >= static_cast<double>(embedded_cursor_->time)) {
                switch (embedded_cursor_->type) {
                    case TML_NOTE_ON:
                        tsf_channel_note_on(tsf_synth_, embedded_cursor_->channel, embedded_cursor_->key,
                                            static_cast<float>(embedded_cursor_->velocity) / 127.0f);
                        break;
                    case TML_NOTE_OFF:
                        tsf_channel_note_off(tsf_synth_, embedded_cursor_->channel, embedded_cursor_->key);
                        break;
                    case TML_PROGRAM_CHANGE:
                        tsf_channel_set_presetnumber(tsf_synth_, embedded_cursor_->channel,
                                                     embedded_cursor_->program, (embedded_cursor_->channel == 9));
                        break;
                    case TML_CONTROL_CHANGE:
                        tsf_channel_midi_control(tsf_synth_, embedded_cursor_->channel,
                                                 embedded_cursor_->control, embedded_cursor_->control_value);
                        break;
                    case TML_PITCH_BEND:
                        tsf_channel_set_pitchwheel(tsf_synth_, embedded_cursor_->channel,
                                                   embedded_cursor_->pitch_bend);
                        break;
                    default:
                        break;
                }
                embedded_cursor_ = embedded_cursor_->next;
            }

            tsf_render_float(tsf_synth_, write_ptr, block, 1);
            write_ptr += block * CHANNELS;
            frames_left -= block;

            if (!embedded_cursor_) {
                embedded_playing_ = false;
                embedded_finished_ = true;
                break;
            }
        }
    }

    // 2. Mix active WAV sound effects
    size_t total_samples = static_cast<size_t>(num_frames) * CHANNELS;
    for (auto it = active_voices_.begin(); it != active_voices_.end();) {
        const auto& wav = sfx_bank_[it->sfx_index].stereo_samples;
        size_t avail = wav.size() - it->cursor;
        size_t to_mix = std::min(total_samples, avail);
        for (size_t i = 0; i < to_mix; ++i) {
            out_stereo_frames[i] = std::clamp(out_stereo_frames[i] + wav[it->cursor + i], -1.0f, 1.0f);
        }
        it->cursor += to_mix;
        if (it->cursor >= wav.size()) {
            it = active_voices_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace llk2::media
