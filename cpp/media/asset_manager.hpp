#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace llk2::media {

// 32-bit RGBA8888 software surface (byte order R, G, B, A in memory)
struct RgbaSurface {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels; // size = width * height * 4

    bool valid() const { return width > 0 && height > 0 && pixels.size() == static_cast<size_t>(width) * height * 4; }

    void resize(int w, int h) {
        width = w;
        height = h;
        pixels.assign(static_cast<size_t>(w) * h * 4, 0);
    }

    void fill_black() {
        for (size_t i = 0; i < pixels.size(); i += 4) {
            pixels[i + 0] = 0;
            pixels[i + 1] = 0;
            pixels[i + 2] = 0;
            pixels[i + 3] = 255;
        }
    }
};

class AssetManager {
public:
    AssetManager() = default;

    // Locate and load assets from a root assets directory (or auto-search ./assets, ./src/assets, ./src-llkcn/assets)
    bool load_from_dir(const std::filesystem::path& assets_dir, std::string* err_out = nullptr);
    bool auto_load(const std::filesystem::path& exe_or_work_dir = ".", std::string* err_out = nullptr);

    bool is_loaded() const { return loaded_; }
    const std::filesystem::path& assets_dir() const { return assets_dir_; }

    const RgbaSurface& pic_title() const { return pic_title_; }
    const RgbaSurface& pic_data() const { return pic_data_; }
    const RgbaSurface& pic_mjdata() const { return pic_mjdata_; }
    const RgbaSurface& pic_level(int level) const { return pic_levels_.at(level); }
    const RgbaSurface& form_explain() const { return form_explain_; }
    const RgbaSurface& form_author() const { return form_author_; }

    const std::filesystem::path& icon_path() const { return icon_path_; }
    // sfx_id = 201..206 -> index 0..5
    std::filesystem::path sfx_path(int sfx_id) const;
    // track_idx = 0..2 -> 101.mid, 102.mid, 103.mid
    std::filesystem::path midi_path(int track_idx) const;
    // Optional lossless OST audio track (track_idx = 0..2 -> 101.flac/mp3/wav .. 103.flac/mp3/wav)
    std::filesystem::path ost_path(int track_idx) const;
    bool has_ost_tracks() const { return has_ost_; }

    static bool load_image_file(const std::filesystem::path& path, RgbaSurface& out_surf);

private:
    bool find_and_load_image(const std::filesystem::path& images_dir,
                             const std::string& stem,
                             RgbaSurface& out_surf,
                             std::string* err_out);
    void discover_ost_tracks(const std::filesystem::path& assets_dir);

    bool loaded_ = false;
    bool has_ost_ = false;
    std::filesystem::path assets_dir_;

    RgbaSurface pic_title_;
    RgbaSurface pic_data_;
    RgbaSurface pic_mjdata_;
    std::array<RgbaSurface, 7> pic_levels_;
    RgbaSurface form_explain_;
    RgbaSurface form_author_;

    std::filesystem::path icon_path_;
    std::array<std::filesystem::path, 6> sfx_paths_;
    std::array<std::filesystem::path, 3> midi_paths_;
    std::array<std::filesystem::path, 3> ost_paths_;
};

} // namespace llk2::media
