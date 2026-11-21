#include "media/asset_manager.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_BMP
#define STBI_ONLY_JPEG
#define STBI_ONLY_GIF
#define STBI_ONLY_PNG
#include "stb_image.h"

namespace llk2::media {

bool AssetManager::load_image_file(const std::filesystem::path& path, RgbaSurface& out_surf) {
    int w = 0, h = 0, comp = 0;
    stbi_uc* data = stbi_load(path.string().c_str(), &w, &h, &comp, 4);
    if (!data || w <= 0 || h <= 0) {
        if (data) stbi_image_free(data);
        return false;
    }
    out_surf.width = w;
    out_surf.height = h;
    out_surf.pixels.assign(data, data + static_cast<size_t>(w) * h * 4);
    stbi_image_free(data);
    return true;
}

bool AssetManager::find_and_load_image(const std::filesystem::path& images_dir,
                                       const std::string& stem,
                                       RgbaSurface& out_surf,
                                       std::string* err_out) {
    static const char* kExts[] = {".bmp", ".jpg", ".gif", ".png", ".BMP", ".JPG", ".GIF", ".PNG"};
    for (const char* ext : kExts) {
        auto candidate = images_dir / (stem + ext);
        if (std::filesystem::exists(candidate)) {
            if (load_image_file(candidate, out_surf)) {
                return true;
            }
        }
    }
    if (err_out) {
        *err_out = "Missing or unreadable image asset: " + (images_dir / stem).string() + "(.bmp/.jpg/.gif/.png)";
    }
    return false;
}

bool AssetManager::load_from_dir(const std::filesystem::path& assets_dir, std::string* err_out) {
    loaded_ = false;
    if (!std::filesystem::exists(assets_dir)) {
        if (err_out) *err_out = "Assets directory does not exist: " + assets_dir.string();
        return false;
    }

    auto images_dir = assets_dir / "images";
    if (!find_and_load_image(images_dir, "Pic_Title_Picture", pic_title_, err_out)) return false;
    if (!find_and_load_image(images_dir, "Pic_Data_Picture", pic_data_, err_out)) return false;
    if (!find_and_load_image(images_dir, "Pic_MjData_Picture", pic_mjdata_, err_out)) return false;

    for (int i = 0; i < 7; ++i) {
        std::string stem = "Pic_Level_" + std::to_string(i) + "_Picture";
        if (!find_and_load_image(images_dir, stem, pic_levels_[i], err_out)) return false;
    }

    if (!find_and_load_image(images_dir, "Form_Explain_Picture", form_explain_, err_out)) return false;
    if (!find_and_load_image(images_dir, "Form_Author_Picture", form_author_, err_out)) return false;

    icon_path_ = assets_dir / "icons" / "Form_Main_Icon.ico";

    for (int i = 0; i < 6; ++i) {
        int id = 201 + i;
        sfx_paths_[i] = assets_dir / "sfx" / (std::to_string(id) + ".wav");
        if (!std::filesystem::exists(sfx_paths_[i])) {
            if (err_out) *err_out = "Missing SFX asset: " + sfx_paths_[i].string();
            return false;
        }
    }

    for (int i = 0; i < 3; ++i) {
        int id = 101 + i;
        midi_paths_[i] = assets_dir / "midi" / (std::to_string(id) + ".mid");
        if (!std::filesystem::exists(midi_paths_[i])) {
            if (err_out) *err_out = "Missing MIDI asset: " + midi_paths_[i].string();
            return false;
        }
    }

    assets_dir_ = assets_dir;
    loaded_ = true;
    return true;
}

bool AssetManager::auto_load(const std::filesystem::path& exe_or_work_dir, std::string* err_out) {
    std::vector<std::filesystem::path> candidates = {
        exe_or_work_dir / "assets",
        exe_or_work_dir / "src" / "assets",
        exe_or_work_dir / "src-llkcn" / "assets",
        std::filesystem::current_path() / "assets",
        std::filesystem::current_path() / "src" / "assets",
        std::filesystem::current_path() / "src-llkcn" / "assets",
    };

    std::string last_err;
    for (const auto& dir : candidates) {
        if (std::filesystem::exists(dir) && load_from_dir(dir, &last_err)) {
            return true;
        }
    }
    if (err_out) {
        *err_out = last_err.empty()
            ? "Could not locate extracted assets in ./assets, ./src/assets, or ./src-llkcn/assets. Run: python3 tools/extract_assets.py <D4s.exe|llk-cn.exe> -o assets/"
            : last_err;
    }
    return false;
}

std::filesystem::path AssetManager::sfx_path(int sfx_id) const {
    if (sfx_id >= 201 && sfx_id <= 206) {
        return sfx_paths_[sfx_id - 201];
    }
    return {};
}

std::filesystem::path AssetManager::midi_path(int track_idx) const {
    if (track_idx >= 0 && track_idx < 3) {
        return midi_paths_[track_idx];
    }
    return {};
}

} // namespace llk2::media
