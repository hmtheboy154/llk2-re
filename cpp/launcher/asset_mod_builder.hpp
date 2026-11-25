#pragma once

#include "media/asset_manager.hpp"

#include <QImage>
#include <filesystem>
#include <string>
#include <vector>

namespace llk2::launcher {

struct ExeHashInfo {
    bool ok = false;
    bool is_zip = false;
    bool is_known = false;
    std::string sha256_hex;
    std::string version_label; // e.g. "v1.02 (D4s.exe zh-TW)" or "v1.00-cn (lianliankan2.ExE zh-CN)"
};

struct PackIntegrityReport {
    bool valid = false;
    int required_total = 21; // 12 images + 6 sfx + 3 midi
    int present_count = 0;
    int ost_count = 0;
    std::vector<std::string> missing;
};

// Inspect a local EXE (computes SHA-256 and checks known versions) or ZIP archive
ExeHashInfo inspect_source_archive(const std::filesystem::path& path);

// Check whether an asset pack directory contains all required images, SFX, MIDIs, and optional OSTs
PackIntegrityReport check_pack_integrity(const std::filesystem::path& pack_dir);

// Sanitize a user-entered mod pack folder name (keeps ASCII letters, digits, '-', '_')
std::string sanitize_mod_name(const std::string& raw);

// List valid mod pack subdirectories under <mods_dir> in alphabetical order
std::vector<std::string> list_mod_packs(const std::filesystem::path& mods_dir);

// Recursively clone an existing asset pack directory (e.g. assets/ -> mods/<name>/)
bool clone_asset_pack(const std::filesystem::path& src_dir,
                      const std::filesystem::path& dst_dir,
                      std::string* err_out = nullptr);

// Replace a single tile sprite (tile_id 0..41) inside <pack_dir>/images/Pic_MjData_Picture.bmp
// Generates exact SRCAND/SRCPAINT art & silhouette mask cells inside the 32x32 flat face region.
bool replace_tile_sprite(const std::filesystem::path& pack_dir,
                         int tile_id,
                         const std::filesystem::path& image_path,
                         std::string* err_out = nullptr);

// Replace Stage 1..7 background (level_idx 0..6) scaled & center-cropped to 600x450
bool replace_level_background(const std::filesystem::path& pack_dir,
                              int level_idx,
                              const std::filesystem::path& image_path,
                              std::string* err_out = nullptr);

// Replace Title screen image scaled & center-cropped to 500x375
bool replace_title_image(const std::filesystem::path& pack_dir,
                         const std::filesystem::path& image_path,
                         std::string* err_out = nullptr);

// Replace a single SFX (sfx/201..206.wav) or MIDI (midi/101..103.mid) file in a pack
bool replace_audio_asset(const std::filesystem::path& pack_dir,
                         const std::filesystem::path& rel_target,
                         const std::filesystem::path& src_file,
                         std::string* err_out = nullptr);

// Import OST tracks from a folder (scanning 01..03.* / 101..103.* + nested OST dir)
// or from an explicit list of audio files into <pack_dir>/ost/{101..103}.<ext>.
// Returns number of tracks imported (0..3).
int import_ost_from_directory(const std::filesystem::path& pack_dir,
                              const std::filesystem::path& src_dir,
                              std::string* err_out = nullptr);

int import_ost_files(const std::filesystem::path& pack_dir,
                     const std::vector<std::filesystem::path>& files,
                     std::string* err_out = nullptr);

// Render a 7x6 composited preview of all 42 tiles on their authentic 3D tile shell
QImage render_composited_tile_preview(const media::AssetManager& assets, int palette_off = 0);

// Convert an RgbaSurface to a deep-copied QImage
QImage surface_to_qimage(const media::RgbaSurface& surf);

} // namespace llk2::launcher
