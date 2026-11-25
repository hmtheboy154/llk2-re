#include "media/audio_engine.hpp"
#include "core/app_paths.hpp"

#define TSF_IMPLEMENTATION
#include "tsf.h"

#define TML_IMPLEMENTATION
#include "tml.h"

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace llk2::media {

namespace {

#if defined(LLK2_USE_SDL3)
void SDLCALL sdl3_audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int /*total_amount*/) {
    if (additional_amount <= 0) return;
    auto* engine = static_cast<AudioEngine*>(userdata);
    int num_frames = additional_amount / (static_cast<int>(sizeof(float)) * AudioEngine::CHANNELS);
    if (num_frames <= 0) return;

    // Render in fixed zero-allocation chunks so PipeWire low-latency quanta never stall on malloc
    constexpr int kMaxChunkFrames = 2048;
    while (num_frames > 0) {
        int chunk = std::min(num_frames, kMaxChunkFrames);
        float* buf = engine->scratch_buffer(static_cast<size_t>(chunk) * AudioEngine::CHANNELS);
        engine->mix_audio_frames(buf, chunk);
        SDL_PutAudioStreamData(stream, buf, static_cast<int>(chunk * AudioEngine::CHANNELS * sizeof(float)));
        num_frames -= chunk;
    }
}
#else
void SDLCALL sdl2_audio_callback(void* userdata, Uint8* stream, int len) {
    auto* engine = static_cast<AudioEngine*>(userdata);
    int num_frames = len / (static_cast<int>(sizeof(float)) * AudioEngine::CHANNELS);
    auto* out = reinterpret_cast<float*>(stream);
    engine->mix_audio_frames(out, num_frames);
}
#endif

bool is_valid_sf2_file(const std::filesystem::path& p) {
    if (p.empty() || !std::filesystem::exists(p) || !std::filesystem::is_regular_file(p)) {
        return false;
    }
    std::string ext = p.extension().string();
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext == ".sf2" || ext == ".sf3";
}

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

AudioEngine::AudioEngine() {
    sdl_scratch_.resize(2048 * CHANNELS, 0.0f);
    active_voices_.reserve(16);
}

AudioEngine::~AudioEngine() {
    shutdown();
}

float* AudioEngine::scratch_buffer(size_t num_floats) {
    if (sdl_scratch_.size() < num_floats) {
        sdl_scratch_.resize(num_floats, 0.0f);
    }
    return sdl_scratch_.data();
}

std::filesystem::path AudioEngine::discover_soundfont_path(const std::string& configured_path) {
    // 1. User-selected custom SoundFont from config.json
    if (is_valid_sf2_file(configured_path)) {
        return configured_path;
    }

    // 2. Bundled GeneralUser-GS SoundFont inside app / build / install / data directories (never scans user drive)
    std::vector<std::filesystem::path> candidates = {
        "soundfont.sf2",
        "GeneralUser-GS.sf2",
        "assets/soundfont.sf2",
        "assets/GeneralUser-GS.sf2",
        "subprojects/GeneralUser-GS/GeneralUser-GS.sf2",
        "build/soundfont.sf2",
    };

    for (const auto& dir : core::AppPaths::bundled_resource_dirs()) {
        candidates.push_back(dir / "soundfont.sf2");
        candidates.push_back(dir / "GeneralUser-GS.sf2");
        candidates.push_back(dir / "assets" / "soundfont.sf2");
    }
    auto user_dir = core::AppPaths::default_user_data_dir();
    candidates.push_back(user_dir / "soundfont.sf2");
    candidates.push_back(user_dir / "GeneralUser-GS.sf2");

#if defined(LLK2_BUNDLED_SOUNDFONT)
    candidates.emplace_back(LLK2_BUNDLED_SOUNDFONT);
#endif

    for (const auto& c : candidates) {
        if (is_valid_sf2_file(c)) {
            return c;
        }
    }

    return {};
}

// Emergency in-memory SoundFont fallback if someone compiles without the GeneralUser-GS submodule
std::vector<uint8_t> AudioEngine::build_minimal_sf2_bytes() {
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

    constexpr int kLoopLen = 100;
    constexpr int kTotalSamples = kLoopLen + 46;
    std::vector<uint8_t> smpl_data;
    smpl_data.reserve(kTotalSamples * 2);
    for (int i = 0; i < kTotalSamples; ++i) {
        int16_t s = 0;
        if (i < kLoopLen) {
            double phase = 2.0 * 3.14159265358979323846 * static_cast<double>(i) / static_cast<double>(kLoopLen);
            double v = 0.65 * std::sin(phase) + 0.22 * std::sin(2.0 * phase) + 0.08 * std::sin(3.0 * phase);
            s = static_cast<int16_t>(std::clamp(v * 14000.0, -32767.0, 32767.0));
        }
        append_u16(smpl_data, static_cast<uint16_t>(s));
    }
    std::vector<uint8_t> sdta_list;
    append_fourcc(sdta_list, "sdta");
    append_chunk(sdta_list, "smpl", smpl_data);

    constexpr int kNumPresets = 129;
    std::vector<uint8_t> phdr_data;
    for (int p = 0; p < kNumPresets; ++p) {
        uint16_t preset_num = (p < 128) ? static_cast<uint16_t>(p) : 0;
        uint16_t bank_num = (p < 128) ? 0 : 128;
        append_fixed_str(phdr_data, "GM Preset", 20);
        append_u16(phdr_data, preset_num);
        append_u16(phdr_data, bank_num);
        append_u16(phdr_data, static_cast<uint16_t>(p));
        append_u32(phdr_data, 0);
        append_u32(phdr_data, 0);
        append_u32(phdr_data, 0);
    }
    append_fixed_str(phdr_data, "EOP", 20);
    append_u16(phdr_data, 0);
    append_u16(phdr_data, 0);
    append_u16(phdr_data, static_cast<uint16_t>(kNumPresets));
    append_u32(phdr_data, 0);
    append_u32(phdr_data, 0);
    append_u32(phdr_data, 0);

    std::vector<uint8_t> pbag_data;
    for (int p = 0; p <= kNumPresets; ++p) {
        append_u16(pbag_data, static_cast<uint16_t>(p));
        append_u16(pbag_data, 0);
    }

    std::vector<uint8_t> pmod_data(10, 0);

    std::vector<uint8_t> pgen_data;
    for (int p = 0; p < kNumPresets; ++p) {
        append_u16(pgen_data, 41);
        append_u16(pgen_data, 0);
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
    append_u16(ibag_data, 5);
    append_u16(ibag_data, 0);

    std::vector<uint8_t> imod_data(10, 0);

    std::vector<uint8_t> igen_data;
    append_u16(igen_data, 54);
    append_u16(igen_data, 1);
    append_u16(igen_data, 36);
    append_u16(igen_data, static_cast<uint16_t>(static_cast<int16_t>(-800)));
    append_u16(igen_data, 37);
    append_u16(igen_data, 900);
    append_u16(igen_data, 38);
    append_u16(igen_data, static_cast<uint16_t>(static_cast<int16_t>(-1800)));
    append_u16(igen_data, 53);
    append_u16(igen_data, 0);
    append_u16(igen_data, 0);
    append_u16(igen_data, 0);

    std::vector<uint8_t> shdr_data;
    append_fixed_str(shdr_data, "SynthWave", 20);
    append_u32(shdr_data, 0);
    append_u32(shdr_data, kLoopLen);
    append_u32(shdr_data, 0);
    append_u32(shdr_data, kLoopLen);
    append_u32(shdr_data, 44100);
    shdr_data.push_back(69);
    shdr_data.push_back(0);
    append_u16(shdr_data, 0);
    append_u16(shdr_data, 1);

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
    if (!is_valid_sf2_file(sf2_path)) return false;
    tsf* loaded = tsf_load_filename(sf2_path.string().c_str());
    if (!loaded) return false;
    tsf_set_max_voices(loaded, 128);
    tsf_set_output(loaded, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, -8.0f);
    tsf_channel_set_bank_preset(loaded, 9, 128, 0);

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

    auto mem_sf2 = build_minimal_sf2_bytes();
    tsf* loaded = tsf_load_memory(mem_sf2.data(), static_cast<int>(mem_sf2.size()));
    if (!loaded) return false;
    tsf_set_max_voices(loaded, 64);
    tsf_set_output(loaded, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, -10.0f);
    tsf_channel_set_bank_preset(loaded, 9, 128, 0);

    std::lock_guard<std::mutex> lock(audio_mutex_);
    if (tsf_synth_) {
        tsf_close(tsf_synth_);
    }
    tsf_synth_ = loaded;
    active_sf2_name_ = "Builtin-GM-Fallback.sf2";
    return true;
}

void AudioEngine::close_ost_stream_locked() {
    ost_playing_.store(false, std::memory_order_release);
    if (ost_flac_) {
        drflac_close(static_cast<drflac*>(ost_flac_));
        ost_flac_ = nullptr;
    }
    if (ost_mp3_) {
        auto* m = static_cast<drmp3*>(ost_mp3_);
        drmp3_uninit(m);
        delete m;
        ost_mp3_ = nullptr;
    }
}

bool AudioEngine::open_ost_stream(const std::filesystem::path& ost_path) {
    if (ost_path.empty() || !std::filesystem::exists(ost_path)) {
        return false;
    }
    std::string ext = ost_path.extension().string();
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    std::lock_guard<std::mutex> lock(audio_mutex_);
    close_ost_stream_locked();

    if (ext == ".flac") {
        drflac* f = drflac_open_file(ost_path.string().c_str(), nullptr);
        if (!f) return false;
        ost_flac_ = f;
        ost_channels_ = std::max<uint32_t>(1, f->channels);
        ost_finished_.store(false, std::memory_order_release);
        ost_playing_.store(true, std::memory_order_release);
        return true;
    } else if (ext == ".mp3") {
        auto* m = new drmp3{};
        if (!drmp3_init_file(m, ost_path.string().c_str(), nullptr)) {
            delete m;
            return false;
        }
        ost_mp3_ = m;
        ost_channels_ = std::max<uint32_t>(1, m->channels);
        ost_finished_.store(false, std::memory_order_release);
        ost_playing_.store(true, std::memory_order_release);
        return true;
    }
    return false;
}

bool AudioEngine::init(const AssetManager& assets, const core::Config& cfg, std::string* err_out) {
    shutdown();

    if (!assets.is_loaded()) {
        if (err_out) *err_out = "AssetManager is not loaded";
        return false;
    }

    for (int i = 0; i < 6; ++i) {
        int id = 201 + i;
        if (!load_wav_to_float(assets.sfx_path(id), sfx_bank_[i])) {
            if (err_out) *err_out = "Failed to decode WAV: " + assets.sfx_path(id).string();
            return false;
        }
    }

    has_ost_ = assets.has_ost_tracks();
    for (int i = 0; i < 3; ++i) {
        midi_files_[i] = assets.midi_path(i);
        ost_files_[i] = assets.ost_path(i);
        preloaded_midi_[i] = tml_load_filename(midi_files_[i].string().c_str());
        if (!preloaded_midi_[i]) {
            if (err_out) *err_out = "Failed to parse MIDI: " + midi_files_[i].string();
            shutdown();
            return false;
        }
    }

    if (!init_embedded_synth(cfg)) {
        if (err_out) *err_out = "Failed to initialize embedded TinySoundFont synthesizer";
        shutdown();
        return false;
    }

    native_midi_.init();

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

    sfx_enabled_.store(cfg.sound_on, std::memory_order_release);
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
    sfx_read_.store(0, std::memory_order_relaxed);
    sfx_write_.store(0, std::memory_order_relaxed);
    close_ost_stream_locked();
    ost_finished_.store(false, std::memory_order_release);
    embedded_cursor_ = nullptr;
    embedded_playing_.store(false, std::memory_order_release);
    embedded_finished_.store(false, std::memory_order_release);

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
    // 100% lock-free push onto SPSC ring buffer — never stalls the GUI thread!
    if (!initialized_ || !sfx_enabled_.load(std::memory_order_acquire)) return;
    if (sfx_id < 201 || sfx_id > 206) return;
    int idx = sfx_id - 201;
    if (sfx_bank_[idx].stereo_samples.empty()) return;

    uint32_t w = sfx_write_.load(std::memory_order_relaxed);
    uint32_t next_w = (w + 1) % kSfxQueueCap;
    if (next_w != sfx_read_.load(std::memory_order_acquire)) {
        sfx_ring_[w] = idx;
        sfx_write_.store(next_w, std::memory_order_release);
    }
}

void AudioEngine::stop_all_midi() {
    native_midi_.stop();
    std::lock_guard<std::mutex> lock(audio_mutex_);
    close_ost_stream_locked();
    ost_finished_.store(false, std::memory_order_release);
    embedded_playing_.store(false, std::memory_order_release);
    embedded_finished_.store(false, std::memory_order_release);
    embedded_cursor_ = nullptr;
    if (tsf_synth_) {
        tsf_note_off_all(tsf_synth_);
    }
}

bool AudioEngine::start_midi_track(int track_idx, core::MidiBackend backend) {
    if (track_idx < 0 || track_idx >= 3) return false;
    stop_all_midi();

    if (backend == core::MidiBackend::OstAudio && has_ost_) {
        if (open_ost_stream(ost_files_[track_idx])) {
            current_track_ = track_idx;
            return true;
        }
    }

    if (backend == core::MidiBackend::NativeOS && native_midi_.is_available()) {
        if (native_midi_.play_file(midi_files_[track_idx])) {
            current_track_ = track_idx;
            return true;
        }
    }

    std::lock_guard<std::mutex> lock(audio_mutex_);
    if (!tsf_synth_ || !preloaded_midi_[track_idx]) return false;
    tsf_reset(tsf_synth_);
    tsf_channel_set_bank_preset(tsf_synth_, 9, 128, 0);
    embedded_cursor_ = preloaded_midi_[track_idx];
    embedded_msec_ = 0.0;
    embedded_finished_.store(false, std::memory_order_release);
    embedded_playing_.store(true, std::memory_order_release);
    current_track_ = track_idx;
    return true;
}

bool AudioEngine::is_midi_playing() const {
    if (!initialized_) return false;
    if (active_backend_ == core::MidiBackend::NativeOS && native_midi_.is_available()) {
        return native_midi_.is_playing();
    }
    if (active_backend_ == core::MidiBackend::OstAudio && has_ost_) {
        return ost_playing_.load(std::memory_order_acquire);
    }
    return embedded_playing_.load(std::memory_order_acquire);
}

void AudioEngine::update(const core::Config& cfg) {
    // Lock-free fast path during normal playback — never contends with audio callback!
    if (!initialized_) return;
    sfx_enabled_.store(cfg.sound_on, std::memory_order_release);
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
    } else if (active_backend_ == core::MidiBackend::OstAudio && has_ost_) {
        track_done = !ost_playing_.load(std::memory_order_acquire) || ost_finished_.load(std::memory_order_acquire);
    } else {
        track_done = !embedded_playing_.load(std::memory_order_acquire) || embedded_finished_.load(std::memory_order_acquire);
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

    // Drain lock-free SFX trigger queue into active_voices_
    uint32_t r = sfx_read_.load(std::memory_order_relaxed);
    uint32_t w = sfx_write_.load(std::memory_order_acquire);
    while (r != w) {
        active_voices_.push_back(ActiveVoice{sfx_ring_[r], 0});
        r = (r + 1) % kSfxQueueCap;
    }
    sfx_read_.store(r, std::memory_order_release);

    // Try-lock so a track change on the GUI thread NEVER stalls PipeWire's real-time audio callback
    std::unique_lock<std::mutex> lock(audio_mutex_, std::try_to_lock);
    if (lock.owns_lock()) {
        // 1A. Stream Lossless OST Audio (FLAC / MP3) if active
        if (ost_playing_.load(std::memory_order_acquire) && (ost_flac_ || ost_mp3_)) {
            auto* flac = static_cast<drflac*>(ost_flac_);
            auto* mp3 = static_cast<drmp3*>(ost_mp3_);
            uint64_t frames_read = 0;
            if (ost_channels_ == 2) {
                if (flac) {
                    frames_read = drflac_read_pcm_frames_f32(flac, static_cast<drflac_uint64>(num_frames), out_stereo_frames);
                } else if (mp3) {
                    frames_read = drmp3_read_pcm_frames_f32(mp3, static_cast<drmp3_uint64>(num_frames), out_stereo_frames);
                }
            } else if (ost_channels_ == 1) {
                float mono_stack[2048];
                int chunk_lim = std::min(num_frames, 2048);
                if (flac) {
                    frames_read = drflac_read_pcm_frames_f32(flac, static_cast<drflac_uint64>(chunk_lim), mono_stack);
                } else if (mp3) {
                    frames_read = drmp3_read_pcm_frames_f32(mp3, static_cast<drmp3_uint64>(chunk_lim), mono_stack);
                }
                for (uint64_t i = 0; i < frames_read; ++i) {
                    out_stereo_frames[i * 2 + 0] = mono_stack[i];
                    out_stereo_frames[i * 2 + 1] = mono_stack[i];
                }
            }
            if (frames_read < static_cast<uint64_t>(num_frames)) {
                close_ost_stream_locked();
                ost_finished_.store(true, std::memory_order_release);
            }
        }
        // 1B. Render Embedded SoundFont MIDI in 64-frame blocks for sample-accurate event timing
        else if (embedded_playing_.load(std::memory_order_acquire) && tsf_synth_ && embedded_cursor_) {
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
                    embedded_playing_.store(false, std::memory_order_release);
                    embedded_finished_.store(true, std::memory_order_release);
                    break;
                }
            }
        }
    }

    // 2. Mix active WAV sound effects (zero locks required!)
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
