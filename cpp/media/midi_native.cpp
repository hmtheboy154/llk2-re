#include "media/midi_native.hpp"

#include "tml.h"

#include <chrono>

#if defined(__linux__) && defined(LLK2_HAVE_ALSA)
#include <alsa/asoundlib.h>
#elif defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif

namespace llk2::media {

NativeMidiPlayer::NativeMidiPlayer() = default;

NativeMidiPlayer::~NativeMidiPlayer() {
    shutdown();
}

bool NativeMidiPlayer::init(std::string* err_out) {
    shutdown();
    quit_.store(false);

#if defined(__linux__) && defined(LLK2_HAVE_ALSA)
    snd_seq_t* seq = nullptr;
    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_OUTPUT, 0) < 0 || !seq) {
        if (err_out) *err_out = "Failed to open ALSA sequencer";
        return false;
    }
    seq_ = seq;
    snd_seq_set_client_name(seq, "LLK2 Remastered");
    port_ = snd_seq_create_simple_port(
        seq, "LLK2 MIDI Out",
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);

    if (port_ < 0) {
        if (err_out) *err_out = "Failed to create ALSA sequencer output port";
        snd_seq_close(seq);
        seq_ = nullptr;
        return false;
    }

    int my_client = snd_seq_client_id(seq);
    dest_client_ = -1;
    dest_port_ = -1;
    int fallback_client = -1;
    int fallback_port = -1;
    std::string fallback_name;

    snd_seq_client_info_t* cinfo = nullptr;
    snd_seq_port_info_t* pinfo = nullptr;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    while (snd_seq_query_next_client(seq, cinfo) >= 0) {
        int client = snd_seq_client_info_get_client(cinfo);
        if (client == 0 || client == my_client) continue;

        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(seq, pinfo) >= 0) {
            unsigned int cap = snd_seq_port_info_get_capability(pinfo);
            unsigned int req = SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;
            if ((cap & req) == req) {
                int port = snd_seq_port_info_get_port(pinfo);
                std::string cname = snd_seq_client_info_get_name(cinfo);
                std::string pname = snd_seq_port_info_get_name(pinfo);
                if (client == 14) { // Midi Through
                    if (fallback_client < 0) {
                        fallback_client = client;
                        fallback_port = port;
                        fallback_name = cname + ":" + pname;
                    }
                } else {
                    dest_client_ = client;
                    dest_port_ = port;
                    port_desc_ = cname + ":" + pname + " (" + std::to_string(client) + ":" + std::to_string(port) + ")";
                    break;
                }
            }
        }
        if (dest_client_ >= 0) break;
    }

    if (dest_client_ < 0 && fallback_client >= 0) {
        dest_client_ = fallback_client;
        dest_port_ = fallback_port;
        port_desc_ = fallback_name + " (" + std::to_string(dest_client_) + ":" + std::to_string(dest_port_) + ")";
    }

    if (dest_client_ >= 0) {
        snd_seq_connect_to(seq, port_, dest_client_, dest_port_);
    } else {
        port_desc_ = "ALSA Port " + std::to_string(my_client) + ":" + std::to_string(port_) + " (unconnected)";
    }

    available_ = true;
    worker_ = std::thread(&NativeMidiPlayer::worker_loop, this);
    return true;
#elif defined(_WIN32)
    HMIDIOUT h = nullptr;
    if (midiOutOpen(&h, MIDI_MAPPER, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        if (err_out) *err_out = "Failed to open WinMM MIDI_MAPPER";
        return false;
    }
    hmidi_ = h;
    port_desc_ = "WinMM MIDI_MAPPER";
    available_ = true;
    worker_ = std::thread(&NativeMidiPlayer::worker_loop, this);
    return true;
#else
    if (err_out) *err_out = "Native OS MIDI backend not compiled for this platform";
    return false;
#endif
}

void NativeMidiPlayer::shutdown() {
    stop();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        quit_.store(true);
    }
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }

    if (midi_head_) {
        tml_free(midi_head_);
        midi_head_ = nullptr;
        midi_cursor_ = nullptr;
    }

#if defined(__linux__) && defined(LLK2_HAVE_ALSA)
    if (seq_) {
        auto* seq = static_cast<snd_seq_t*>(seq_);
        if (port_ >= 0) {
            snd_seq_delete_simple_port(seq, port_);
            port_ = -1;
        }
        snd_seq_close(seq);
        seq_ = nullptr;
    }
#elif defined(_WIN32)
    if (hmidi_) {
        midiOutClose(static_cast<HMIDIOUT>(hmidi_));
        hmidi_ = nullptr;
    }
#endif
    available_ = false;
}

bool NativeMidiPlayer::play_file(const std::filesystem::path& midi_path) {
    if (!available_) return false;
    tml_message* loaded = tml_load_filename(midi_path.string().c_str());
    if (!loaded) return false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        send_all_notes_off();
        if (midi_head_) {
            tml_free(midi_head_);
        }
        midi_head_ = loaded;
        midi_cursor_ = loaded;
        finished_.store(false);
        playing_.store(true);
    }
    cv_.notify_all();
    return true;
}

void NativeMidiPlayer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (playing_.load()) {
        playing_.store(false);
        send_all_notes_off();
    }
    midi_cursor_ = nullptr;
    cv_.notify_all();
}

void NativeMidiPlayer::send_all_notes_off() {
    if (!available_) return;
#if defined(__linux__) && defined(LLK2_HAVE_ALSA)
    if (!seq_ || port_ < 0) return;
    auto* seq = static_cast<snd_seq_t*>(seq_);
    for (int ch = 0; ch < 16; ++ch) {
        snd_seq_event_t ev;
        snd_seq_ev_clear(&ev);
        snd_seq_ev_set_source(&ev, port_);
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);
        snd_seq_ev_set_controller(&ev, ch, 123, 0); // All Notes Off
        snd_seq_event_output(seq, &ev);
        snd_seq_ev_set_controller(&ev, ch, 120, 0); // All Sound Off
        snd_seq_event_output(seq, &ev);
    }
    snd_seq_drain_output(seq);
#elif defined(_WIN32)
    if (!hmidi_) return;
    auto h = static_cast<HMIDIOUT>(hmidi_);
    for (int ch = 0; ch < 16; ++ch) {
        DWORD msg1 = static_cast<DWORD>(0xB0 | ch) | (123 << 8);
        DWORD msg2 = static_cast<DWORD>(0xB0 | ch) | (120 << 8);
        midiOutShortMsg(h, msg1);
        midiOutShortMsg(h, msg2);
    }
#endif
}

void NativeMidiPlayer::dispatch_event(const tml_message* msg) {
    if (!available_ || !msg) return;

#if defined(__linux__) && defined(LLK2_HAVE_ALSA)
    if (!seq_ || port_ < 0) return;
    auto* seq = static_cast<snd_seq_t*>(seq_);
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, port_);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);

    switch (msg->type) {
        case TML_NOTE_ON:
            if (msg->velocity > 0) {
                snd_seq_ev_set_noteon(&ev, msg->channel, msg->key, msg->velocity);
            } else {
                snd_seq_ev_set_noteoff(&ev, msg->channel, msg->key, 0);
            }
            break;
        case TML_NOTE_OFF:
            snd_seq_ev_set_noteoff(&ev, msg->channel, msg->key, msg->velocity);
            break;
        case TML_PROGRAM_CHANGE:
            snd_seq_ev_set_pgmchange(&ev, msg->channel, msg->program);
            break;
        case TML_CONTROL_CHANGE:
            snd_seq_ev_set_controller(&ev, msg->channel, msg->control, msg->control_value);
            break;
        case TML_PITCH_BEND:
            // ALSA pitchbend is centered at 0 (-8192..8191), MIDI is 0..16383 (center 8192)
            snd_seq_ev_set_pitchbend(&ev, msg->channel, static_cast<int>(msg->pitch_bend) - 8192);
            break;
        default:
            return;
    }
    snd_seq_event_output_direct(seq, &ev);
#elif defined(_WIN32)
    if (!hmidi_) return;
    auto h = static_cast<HMIDIOUT>(hmidi_);
    DWORD status = 0;
    DWORD d1 = 0;
    DWORD d2 = 0;
    switch (msg->type) {
        case TML_NOTE_ON:
            status = 0x90 | (msg->channel & 0x0F);
            d1 = msg->key & 0x7F;
            d2 = msg->velocity & 0x7F;
            break;
        case TML_NOTE_OFF:
            status = 0x80 | (msg->channel & 0x0F);
            d1 = msg->key & 0x7F;
            d2 = msg->velocity & 0x7F;
            break;
        case TML_PROGRAM_CHANGE:
            status = 0xC0 | (msg->channel & 0x0F);
            d1 = msg->program & 0x7F;
            break;
        case TML_CONTROL_CHANGE:
            status = 0xB0 | (msg->channel & 0x0F);
            d1 = msg->control & 0x7F;
            d2 = msg->control_value & 0x7F;
            break;
        case TML_PITCH_BEND:
            status = 0xE0 | (msg->channel & 0x0F);
            d1 = msg->pitch_bend & 0x7F;
            d2 = (msg->pitch_bend >> 7) & 0x7F;
            break;
        default:
            return;
    }
    midiOutShortMsg(h, status | (d1 << 8) | (d2 << 16));
#endif
}

void NativeMidiPlayer::worker_loop() {
    using clock = std::chrono::steady_clock;
    auto start_time = clock::now();
    const tml_message* last_head = nullptr;

    std::unique_lock<std::mutex> lock(mutex_);
    while (!quit_.load()) {
        if (!playing_.load() || !midi_cursor_) {
            last_head = nullptr;
            cv_.wait(lock, [this] { return quit_.load() || (playing_.load() && midi_cursor_ != nullptr); });
            if (quit_.load()) break;
            start_time = clock::now();
            last_head = midi_head_;
        }

        if (midi_head_ != last_head) {
            start_time = clock::now();
            last_head = midi_head_;
        }

        double elapsed_ms = std::chrono::duration<double, std::milli>(clock::now() - start_time).count();
        while (midi_cursor_ && elapsed_ms >= static_cast<double>(midi_cursor_->time)) {
            dispatch_event(midi_cursor_);
            midi_cursor_ = midi_cursor_->next;
        }

        if (!midi_cursor_) {
            playing_.store(false);
            finished_.store(true);
            send_all_notes_off();
            continue;
        }

        double wait_ms = static_cast<double>(midi_cursor_->time) - elapsed_ms;
        if (wait_ms > 5.0) wait_ms = 5.0;
        if (wait_ms > 0.5) {
            cv_.wait_for(lock, std::chrono::microseconds(static_cast<int64_t>(wait_ms * 1000.0)));
        }
    }
}

} // namespace llk2::media
