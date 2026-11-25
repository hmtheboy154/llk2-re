#include "launcher/asset_mod_builder.hpp"

#include <QCryptographicHash>
#include <QFile>
#include <QPainter>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <system_error>

namespace llk2::launcher {

namespace {

constexpr const char* kHashV102 = "72c8a9882badac35faf618e40b751d4c11d1f1cdf935fb94043c41afdba64c7a";
constexpr const char* kHashV100Cn = "d5306b7fdc3125259d49cee9054eaffebf67237f9240da82b244b932af7c919b";

std::string to_lower_ascii(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::filesystem::path find_existing_image(const std::filesystem::path& images_dir, const std::string& stem) {
    static const char* kExts[] = {".bmp", ".jpg", ".gif", ".png", ".BMP", ".JPG", ".GIF", ".PNG"};
    std::error_code ec;
    for (const char* ext : kExts) {
        auto cand = images_dir / (stem + ext);
        if (std::filesystem::is_regular_file(cand, ec)) {
            return cand;
        }
    }
    return {};
}

void remove_competing_images(const std::filesystem::path& images_dir, const std::string& stem) {
    static const char* kExts[] = {".bmp", ".jpg", ".gif", ".png", ".BMP", ".JPG", ".GIF", ".PNG"};
    std::error_code ec;
    for (const char* ext : kExts) {
        auto cand = images_dir / (stem + ext);
        if (std::filesystem::exists(cand, ec)) {
            std::filesystem::remove(cand, ec);
        }
    }
}

void remove_competing_ost(const std::filesystem::path& ost_dir, int track_id) {
    static const char* kExts[] = {".flac", ".FLAC", ".wav", ".WAV", ".mp3", ".MP3"};
    std::error_code ec;
    for (const char* ext : kExts) {
        auto cand = ost_dir / (std::to_string(track_id) + ext);
        if (std::filesystem::exists(cand, ec)) {
            std::filesystem::remove(cand, ec);
        }
    }
}

bool is_supported_ost_ext(const std::string& ext_lower) {
    return ext_lower == ".flac" || ext_lower == ".wav" || ext_lower == ".mp3";
}

QImage scale_cover_crop(const QImage& src, int dst_w, int dst_h) {
    if (src.isNull() || dst_w <= 0 || dst_h <= 0) return {};
    QImage scaled = src.scaled(dst_w, dst_h, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int ox = std::max(0, (scaled.width() - dst_w) / 2);
    int oy = std::max(0, (scaled.height() - dst_h) / 2);
    return scaled.copy(ox, oy, dst_w, dst_h).convertToFormat(QImage::Format_RGB888);
}

uint8_t median4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    std::array<uint8_t, 4> arr{a, b, c, d};
    std::sort(arr.begin(), arr.end());
    return static_cast<uint8_t>((static_cast<int>(arr[1]) + static_cast<int>(arr[2])) / 2);
}

} // namespace

ExeHashInfo inspect_source_archive(const std::filesystem::path& path) {
    ExeHashInfo info;
    std::error_code ec;
    if (path.empty() || !std::filesystem::is_regular_file(path, ec)) {
        return info;
    }

    std::string ext = to_lower_ascii(path.extension().string());
    if (ext == ".zip") {
        info.ok = true;
        info.is_zip = true;
        return info;
    }

    QFile f(QString::fromStdString(path.string()));
    if (!f.open(QIODevice::ReadOnly)) {
        return info;
    }
    QCryptographicHash hasher(QCryptographicHash::Sha256);
    if (!hasher.addData(&f)) {
        return info;
    }

    info.ok = true;
    info.sha256_hex = hasher.result().toHex().toLower().toStdString();
    if (info.sha256_hex == kHashV102) {
        info.is_known = true;
        info.version_label = "v1.02 (D4s.exe zh-TW)";
    } else if (info.sha256_hex == kHashV100Cn) {
        info.is_known = true;
        info.version_label = "v1.00-cn (lianliankan2.ExE zh-CN)";
    }
    return info;
}

PackIntegrityReport check_pack_integrity(const std::filesystem::path& pack_dir) {
    PackIntegrityReport rep;
    std::error_code ec;

    const auto images_dir = pack_dir / "images";
    const auto sfx_dir = pack_dir / "sfx";
    const auto midi_dir = pack_dir / "midi";

    static const char* kRequiredImages[] = {
        "Pic_Title_Picture",
        "Pic_Data_Picture",
        "Pic_MjData_Picture",
        "Form_Explain_Picture",
        "Form_Author_Picture",
        "Pic_Level_0_Picture",
        "Pic_Level_1_Picture",
        "Pic_Level_2_Picture",
        "Pic_Level_3_Picture",
        "Pic_Level_4_Picture",
        "Pic_Level_5_Picture",
        "Pic_Level_6_Picture",
    };

    rep.required_total = 12 + 6 + 3;
    rep.present_count = 0;

    for (const char* stem : kRequiredImages) {
        if (!find_existing_image(images_dir, stem).empty()) {
            ++rep.present_count;
        } else {
            rep.missing.push_back(std::string("images/") + stem + ".*");
        }
    }

    for (int id = 201; id <= 206; ++id) {
        auto p = sfx_dir / (std::to_string(id) + ".wav");
        if (std::filesystem::is_regular_file(p, ec)) {
            ++rep.present_count;
        } else {
            rep.missing.push_back("sfx/" + std::to_string(id) + ".wav");
        }
    }

    for (int id = 101; id <= 103; ++id) {
        auto p = midi_dir / (std::to_string(id) + ".mid");
        if (std::filesystem::is_regular_file(p, ec)) {
            ++rep.present_count;
        } else {
            rep.missing.push_back("midi/" + std::to_string(id) + ".mid");
        }
    }

    // Count optional OST tracks in <pack_dir>/ost
    const auto ost_dir = pack_dir / "ost";
    if (std::filesystem::is_directory(ost_dir, ec)) {
        static const char* kOstExts[] = {".flac", ".FLAC", ".wav", ".WAV", ".mp3", ".MP3"};
        static const char* kPrefixes[3] = {"01.", "02.", "03."};
        for (int i = 0; i < 3; ++i) {
            bool found = false;
            std::string id_str = std::to_string(101 + i);
            for (const char* ext : kOstExts) {
                if (std::filesystem::is_regular_file(ost_dir / (id_str + ext), ec)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (const auto& entry : std::filesystem::directory_iterator(ost_dir, ec)) {
                    if (!entry.is_regular_file(ec)) continue;
                    std::string fn = entry.path().filename().string();
                    std::string ext_l = to_lower_ascii(entry.path().extension().string());
                    if (fn.rfind(kPrefixes[i], 0) == 0 && is_supported_ost_ext(ext_l)) {
                        found = true;
                        break;
                    }
                }
            }
            if (found) ++rep.ost_count;
        }
    }

    rep.valid = rep.missing.empty();
    return rep;
}

std::string sanitize_mod_name(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (unsigned char ch : raw) {
        if (std::isalnum(ch) || ch == '-' || ch == '_') {
            out.push_back(static_cast<char>(ch));
        } else if (ch == ' ') {
            out.push_back('_');
        }
    }
    while (!out.empty() && (out.front() == '-' || out.front() == '_')) {
        out.erase(out.begin());
    }
    if (out.size() > 48) {
        out.resize(48);
    }
    return out;
}

std::vector<std::string> list_mod_packs(const std::filesystem::path& mods_dir) {
    std::vector<std::string> packs;
    std::error_code ec;
    if (!std::filesystem::is_directory(mods_dir, ec)) {
        return packs;
    }
    for (const auto& entry : std::filesystem::directory_iterator(mods_dir, ec)) {
        if (entry.is_directory(ec)) {
            std::string name = entry.path().filename().string();
            if (!name.empty() && name[0] != '.') {
                packs.push_back(name);
            }
        }
    }
    std::sort(packs.begin(), packs.end());
    return packs;
}

bool clone_asset_pack(const std::filesystem::path& src_dir,
                      const std::filesystem::path& dst_dir,
                      std::string* err_out) {
    std::error_code ec;
    if (!std::filesystem::is_directory(src_dir, ec)) {
        if (err_out) *err_out = "Source asset pack directory not found: " + src_dir.string();
        return false;
    }
    std::filesystem::create_directories(dst_dir, ec);
    if (ec) {
        if (err_out) *err_out = "Failed to create destination folder: " + ec.message();
        return false;
    }
    std::filesystem::copy(src_dir, dst_dir,
                          std::filesystem::copy_options::recursive |
                              std::filesystem::copy_options::overwrite_existing,
                          ec);
    if (ec) {
        if (err_out) *err_out = "Failed to copy asset pack files: " + ec.message();
        return false;
    }
    return true;
}

bool replace_tile_sprite(const std::filesystem::path& pack_dir,
                         int tile_id,
                         const std::filesystem::path& image_path,
                         std::string* err_out) {
    if (tile_id < 0 || tile_id >= 45) {
        if (err_out) *err_out = "Tile ID out of range (expected 0..41).";
        return false;
    }

    auto images_dir = pack_dir / "images";
    auto sheet_path = find_existing_image(images_dir, "Pic_MjData_Picture");
    if (sheet_path.empty()) {
        if (err_out) *err_out = "Missing Pic_MjData_Picture in " + images_dir.string();
        return false;
    }

    QImage sheet(QString::fromStdString(sheet_path.string()));
    if (sheet.isNull()) {
        if (err_out) *err_out = "Failed to load sprite sheet: " + sheet_path.string();
        return false;
    }
    constexpr int kSheetW = 360;
    constexpr int kSheetH = 612;
    constexpr int kCellW = 40;
    constexpr int kCellH = 50;
    constexpr int kMaskY0 = 250;
    constexpr int kFitW = 32;
    constexpr int kFitH = 32;
    constexpr int kBoxX = 4;
    constexpr int kBoxY = 9;

    if (sheet.width() != kSheetW || sheet.height() != kSheetH) {
        sheet = sheet.scaled(kSheetW, kSheetH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    sheet = sheet.convertToFormat(QImage::Format_RGB888);

    QImage src_raw(QString::fromStdString(image_path.string()));
    if (src_raw.isNull()) {
        if (err_out) *err_out = "Failed to load replacement tile image: " + image_path.string();
        return false;
    }

    const bool has_alpha = src_raw.hasAlphaChannel();
    QImage src_rgba = src_raw.convertToFormat(QImage::Format_RGBA8888);

    // If image has no alpha channel, synthesize transparency from median of the 4 corner pixels
    if (!has_alpha && src_rgba.width() > 0 && src_rgba.height() > 0) {
        const int w = src_rgba.width();
        const int h = src_rgba.height();
        const QColor c0 = src_rgba.pixelColor(0, 0);
        const QColor c1 = src_rgba.pixelColor(w - 1, 0);
        const QColor c2 = src_rgba.pixelColor(0, h - 1);
        const QColor c3 = src_rgba.pixelColor(w - 1, h - 1);
        const int bg_r = median4(static_cast<uint8_t>(c0.red()), static_cast<uint8_t>(c1.red()),
                                 static_cast<uint8_t>(c2.red()), static_cast<uint8_t>(c3.red()));
        const int bg_g = median4(static_cast<uint8_t>(c0.green()), static_cast<uint8_t>(c1.green()),
                                 static_cast<uint8_t>(c2.green()), static_cast<uint8_t>(c3.green()));
        const int bg_b = median4(static_cast<uint8_t>(c0.blue()), static_cast<uint8_t>(c1.blue()),
                                 static_cast<uint8_t>(c2.blue()), static_cast<uint8_t>(c3.blue()));

        for (int y = 0; y < h; ++y) {
            uint8_t* row = src_rgba.scanLine(y);
            for (int x = 0; x < w; ++x) {
                int dr = std::abs(static_cast<int>(row[x * 4 + 0]) - bg_r);
                int dg = std::abs(static_cast<int>(row[x * 4 + 1]) - bg_g);
                int db = std::abs(static_cast<int>(row[x * 4 + 2]) - bg_b);
                int dist = std::max({dr, dg, db});
                row[x * 4 + 3] = (dist > 30) ? 255 : 0;
            }
        }
    }

    QImage scaled = src_rgba.scaled(kFitW, kFitH, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                        .convertToFormat(QImage::Format_RGBA8888);

    const int col = tile_id % 9;
    const int row = tile_id / 9;
    const int art_x0 = col * kCellW;
    const int art_y0 = row * kCellH;
    const int mask_x0 = col * kCellW;
    const int mask_y0 = kMaskY0 + row * kCellH;

    // Clear art cell to pure black (0,0,0) and mask cell to pure white (255,255,255)
    for (int y = 0; y < kCellH; ++y) {
        uint8_t* art_line = sheet.scanLine(art_y0 + y) + art_x0 * 3;
        uint8_t* mask_line = sheet.scanLine(mask_y0 + y) + mask_x0 * 3;
        std::memset(art_line, 0, kCellW * 3);
        std::memset(mask_line, 255, kCellW * 3);
    }

    // Center scaled sprite inside the 32x32 flat face box (x=4..35, y=9..40)
    const int dst_ox = kBoxX + (kFitW - scaled.width()) / 2;
    const int dst_oy = kBoxY + (kFitH - scaled.height()) / 2;

    for (int sy = 0; sy < scaled.height(); ++sy) {
        const uint8_t* sline = scaled.constScanLine(sy);
        const int cy = dst_oy + sy;
        if (cy < 0 || cy >= kCellH) continue;
        uint8_t* art_line = sheet.scanLine(art_y0 + cy) + art_x0 * 3;
        uint8_t* mask_line = sheet.scanLine(mask_y0 + cy) + mask_x0 * 3;
        for (int sx = 0; sx < scaled.width(); ++sx) {
            const int cx = dst_ox + sx;
            if (cx < 0 || cx >= kCellW) continue;
            const uint8_t a = sline[sx * 4 + 3];
            if (a >= 128) {
                art_line[cx * 3 + 0] = sline[sx * 4 + 0];
                art_line[cx * 3 + 1] = sline[sx * 4 + 1];
                art_line[cx * 3 + 2] = sline[sx * 4 + 2];
                mask_line[cx * 3 + 0] = 0;
                mask_line[cx * 3 + 1] = 0;
                mask_line[cx * 3 + 2] = 0;
            }
        }
    }

    // Save losslessly as uncompressed 24-bit BMP so SRCAND/SRCPAINT mask edges never have JPEG artifacts
    remove_competing_images(images_dir, "Pic_MjData_Picture");
    auto out_path = images_dir / "Pic_MjData_Picture.bmp";
    if (!sheet.save(QString::fromStdString(out_path.string()), "BMP")) {
        if (err_out) *err_out = "Failed to write modified sprite sheet: " + out_path.string();
        return false;
    }
    return true;
}

bool replace_level_background(const std::filesystem::path& pack_dir,
                              int level_idx,
                              const std::filesystem::path& image_path,
                              std::string* err_out) {
    if (level_idx < 0 || level_idx > 6) {
        if (err_out) *err_out = "Level index out of range (expected 0..6).";
        return false;
    }
    QImage src(QString::fromStdString(image_path.string()));
    if (src.isNull()) {
        if (err_out) *err_out = "Failed to load background image: " + image_path.string();
        return false;
    }
    QImage fitted = scale_cover_crop(src, 600, 450);
    auto images_dir = pack_dir / "images";
    std::error_code ec;
    std::filesystem::create_directories(images_dir, ec);
    std::string stem = "Pic_Level_" + std::to_string(level_idx) + "_Picture";
    remove_competing_images(images_dir, stem);
    auto out_path = images_dir / (stem + ".png");
    if (!fitted.save(QString::fromStdString(out_path.string()), "PNG")) {
        if (err_out) *err_out = "Failed to save level background: " + out_path.string();
        return false;
    }
    return true;
}

bool replace_title_image(const std::filesystem::path& pack_dir,
                         const std::filesystem::path& image_path,
                         std::string* err_out) {
    QImage src(QString::fromStdString(image_path.string()));
    if (src.isNull()) {
        if (err_out) *err_out = "Failed to load title image: " + image_path.string();
        return false;
    }
    QImage fitted = scale_cover_crop(src, 500, 375);
    auto images_dir = pack_dir / "images";
    std::error_code ec;
    std::filesystem::create_directories(images_dir, ec);
    remove_competing_images(images_dir, "Pic_Title_Picture");
    auto out_path = images_dir / "Pic_Title_Picture.png";
    if (!fitted.save(QString::fromStdString(out_path.string()), "PNG")) {
        if (err_out) *err_out = "Failed to save title image: " + out_path.string();
        return false;
    }
    return true;
}

bool replace_audio_asset(const std::filesystem::path& pack_dir,
                         const std::filesystem::path& rel_target,
                         const std::filesystem::path& src_file,
                         std::string* err_out) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(src_file, ec)) {
        if (err_out) *err_out = "Audio source file not found: " + src_file.string();
        return false;
    }
    auto dst = pack_dir / rel_target;
    std::filesystem::create_directories(dst.parent_path(), ec);
    std::filesystem::copy_file(src_file, dst, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        if (err_out) *err_out = "Failed to copy audio file: " + ec.message();
        return false;
    }
    return true;
}

int import_ost_from_directory(const std::filesystem::path& pack_dir,
                              const std::filesystem::path& src_dir,
                              std::string* err_out) {
    std::error_code ec;
    if (!std::filesystem::is_directory(src_dir, ec)) {
        if (err_out) *err_out = "OST folder not found: " + src_dir.string();
        return 0;
    }

    std::filesystem::path scan_dir = src_dir;
    auto nested = src_dir / "上海步天软件有限公司 - KAWAI连连看OST";
    if (std::filesystem::is_directory(nested, ec)) {
        scan_dir = nested;
    }

    auto dst_ost = pack_dir / "ost";
    std::filesystem::create_directories(dst_ost, ec);

    static const char* kPrefixes[3] = {"01.", "02.", "03."};
    int copied = 0;

    for (int i = 0; i < 3; ++i) {
        int track_id = 101 + i;
        std::string id_str = std::to_string(track_id);
        std::filesystem::path chosen;
        std::filesystem::path mp3_fallback;

        for (const auto& entry : std::filesystem::directory_iterator(scan_dir, ec)) {
            if (!entry.is_regular_file(ec)) continue;
            std::string fn = entry.path().filename().string();
            std::string stem = entry.path().stem().string();
            std::string ext_l = to_lower_ascii(entry.path().extension().string());
            if (!is_supported_ost_ext(ext_l)) continue;

            bool matches = (stem == id_str) || (fn.rfind(kPrefixes[i], 0) == 0);
            if (!matches) continue;

            if (ext_l == ".flac" || ext_l == ".wav") {
                chosen = entry.path();
                break;
            } else if (mp3_fallback.empty()) {
                mp3_fallback = entry.path();
            }
        }
        if (chosen.empty()) chosen = mp3_fallback;
        if (!chosen.empty()) {
            remove_competing_ost(dst_ost, track_id);
            auto dst = dst_ost / (id_str + to_lower_ascii(chosen.extension().string()));
            std::filesystem::copy_file(chosen, dst, std::filesystem::copy_options::overwrite_existing, ec);
            if (!ec) ++copied;
        }
    }

    if (copied == 0 && err_out) {
        *err_out = "No matching 01..03 or 101..103 (.flac/.wav/.mp3) OST files found in " + scan_dir.string();
    }
    return copied;
}

int import_ost_files(const std::filesystem::path& pack_dir,
                     const std::vector<std::filesystem::path>& files,
                     std::string* err_out) {
    if (files.empty()) return 0;
    auto dst_ost = pack_dir / "ost";
    std::error_code ec;
    std::filesystem::create_directories(dst_ost, ec);

    int copied = 0;
    int next_fallback = 101;
    for (const auto& f : files) {
        if (!std::filesystem::is_regular_file(f, ec)) continue;
        std::string ext_l = to_lower_ascii(f.extension().string());
        if (!is_supported_ost_ext(ext_l)) continue;

        std::string fn = f.filename().string();
        std::string stem = f.stem().string();
        int track_id = 0;
        if (stem == "101" || fn.rfind("01.", 0) == 0 || fn.rfind("1.", 0) == 0) track_id = 101;
        else if (stem == "102" || fn.rfind("02.", 0) == 0 || fn.rfind("2.", 0) == 0) track_id = 102;
        else if (stem == "103" || fn.rfind("03.", 0) == 0 || fn.rfind("3.", 0) == 0) track_id = 103;
        else if (next_fallback <= 103) track_id = next_fallback++;

        if (track_id >= 101 && track_id <= 103) {
            remove_competing_ost(dst_ost, track_id);
            auto dst = dst_ost / (std::to_string(track_id) + ext_l);
            std::filesystem::copy_file(f, dst, std::filesystem::copy_options::overwrite_existing, ec);
            if (!ec) ++copied;
        }
    }
    if (copied == 0 && err_out) {
        *err_out = "No supported (.flac/.wav/.mp3) OST files were copied.";
    }
    return copied;
}

QImage surface_to_qimage(const media::RgbaSurface& surf) {
    if (!surf.valid()) return {};
    QImage img(surf.pixels.data(), surf.width, surf.height, surf.width * 4, QImage::Format_RGBA8888);
    return img.copy();
}

QImage render_composited_tile_preview(const media::AssetManager& assets, int palette_off) {
    const auto& mj = assets.pic_mjdata();
    if (!mj.valid() || mj.width < 360 || mj.height < 612) {
        return {};
    }

    constexpr int kCols = 7;
    constexpr int kRows = 6; // 42 tiles
    constexpr int kShellW = 46;
    constexpr int kShellH = 56;
    constexpr int kTileW = 40;
    constexpr int kTileH = 50;
    constexpr int kPadX = 6;
    constexpr int kPadY = 6;
    constexpr int kCellStepX = kShellW + 4;
    constexpr int kCellStepY = kShellH + 4;

    const int out_w = kPadX * 2 + kCols * kCellStepX - 4;
    const int out_h = kPadY * 2 + kRows * kCellStepY - 4;

    media::RgbaSurface canvas;
    canvas.resize(out_w, out_h);
    // Dark charcoal background matching classic board contrast
    for (size_t i = 0; i < canvas.pixels.size(); i += 4) {
        canvas.pixels[i + 0] = 18;
        canvas.pixels[i + 1] = 20;
        canvas.pixels[i + 2] = 26;
        canvas.pixels[i + 3] = 255;
    }

    auto blit = [&](int dst_x, int dst_y, int w, int h, int src_x, int src_y, int op) {
        for (int ry = 0; ry < h; ++ry) {
            int dy = dst_y + ry;
            int sy = src_y + ry;
            if (dy < 0 || dy >= canvas.height || sy < 0 || sy >= mj.height) continue;
            for (int rx = 0; rx < w; ++rx) {
                int dx = dst_x + rx;
                int sx = src_x + rx;
                if (dx < 0 || dx >= canvas.width || sx < 0 || sx >= mj.width) continue;
                size_t di = (static_cast<size_t>(dy) * canvas.width + dx) * 4;
                size_t si = (static_cast<size_t>(sy) * mj.width + sx) * 4;
                if (op == 0) { // SRCAND
                    canvas.pixels[di + 0] &= mj.pixels[si + 0];
                    canvas.pixels[di + 1] &= mj.pixels[si + 1];
                    canvas.pixels[di + 2] &= mj.pixels[si + 2];
                } else { // SRCPAINT
                    canvas.pixels[di + 0] |= mj.pixels[si + 0];
                    canvas.pixels[di + 1] |= mj.pixels[si + 1];
                    canvas.pixels[di + 2] |= mj.pixels[si + 2];
                }
                canvas.pixels[di + 3] = 255;
            }
        }
    };

    int pal = std::clamp(palette_off, 0, 5);
    for (int tid = 0; tid < 42; ++tid) {
        int gx = tid % kCols;
        int gy = tid / kCols;
        int px = kPadX + gx * kCellStepX;
        int py = kPadY + gy * kCellStepY;
        int t1 = tid % 9;
        int t2 = tid / 9;

        // 1. 3D tile shell mask + art
        blit(px, py, kShellW, kShellH, 276, 500, 0);
        blit(px, py, kShellW, kShellH, 46 * pal, 500, 1);
        // 2. Tile sprite mask + art
        blit(px, py, kTileW, kTileH, t1 * kTileW, (t2 + 5) * kTileH, 0);
        blit(px, py, kTileW, kTileH, t1 * kTileW, t2 * kTileH, 1);
    }

    return surface_to_qimage(canvas);
}

} // namespace llk2::launcher
