#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

struct tml_message;

namespace llk2::media {

class NativeMidiPlayer {
public:
    NativeMidiPlayer();
    ~NativeMidiPlayer();

    NativeMidiPlayer(const NativeMidiPlayer&) = delete;
    NativeMidiPlayer& operator=(const NativeMidiPlayer&) = delete;

    bool init(std::string* err_out = nullptr);
    void shutdown();

    bool is_available() const { return available_; }
    const std::string& port_description() const { return port_desc_; }

    bool play_file(const std::filesystem::path& midi_path);
    void stop();

    bool is_playing() const { return playing_.load(); }
    bool track_finished() const { return finished_.load(); }

private:
    void worker_loop();
    void send_all_notes_off();
    void dispatch_event(const tml_message* msg);

    bool available_ = false;
    std::string port_desc_ = "None";

#if defined(__linux__) && defined(LLK2_HAVE_ALSA)
    void* seq_ = nullptr; // snd_seq_t*
    int port_ = -1;
    int dest_client_ = -1;
    int dest_port_ = -1;
#elif defined(_WIN32)
    void* hmidi_ = nullptr; // HMIDIOUT
#endif

    std::atomic<bool> quit_{false};
    std::atomic<bool> playing_{false};
    std::atomic<bool> finished_{false};

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::thread worker_;
    tml_message* midi_head_ = nullptr;
    tml_message* midi_cursor_ = nullptr;
};

} // namespace llk2::media
