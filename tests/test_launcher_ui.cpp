#include "launcher/asset_mod_builder.hpp"
#include "media/asset_manager.hpp"

#include <QColor>
#include <QImage>

#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    std::filesystem::path src_dir(LLK2_SOURCE_DIR);
    std::filesystem::path launcher_bin(LLK2_LAUNCHER_BINARY);
    assert(std::filesystem::exists(launcher_bin));

    // Locate or temporarily extract base assets
    bool created_temp = false;
    std::filesystem::path base_assets;
    if (std::filesystem::exists(src_dir / "assets" / "images")) {
        base_assets = src_dir / "assets";
    } else {
        std::filesystem::path exe_candidate = src_dir.parent_path() / "D4s.exe";
        std::filesystem::path extractor = src_dir / "tools" / "extract_assets.py";
        assert(std::filesystem::exists(exe_candidate) && std::filesystem::exists(extractor));
        base_assets = std::filesystem::temp_directory_path() / "llk2_test_launcher_base_assets";
#if defined(_WIN32)
        std::string extract_cmd = "\"python \"" + extractor.string() + "\" \"" + exe_candidate.string() +
                                  "\" -o \"" + base_assets.string() + "\" > NUL 2>&1\"";
#else
        std::string extract_cmd = "python3 \"" + extractor.string() + "\" \"" + exe_candidate.string() +
                                  "\" -o \"" + base_assets.string() + "\" > /dev/null 2>&1";
#endif
        assert(std::system(extract_cmd.c_str()) == 0);
        created_temp = true;
    }

    auto tmp_work = std::filesystem::temp_directory_path() / "llk2_test_launcher_workdir";
    std::filesystem::remove_all(tmp_work);
    std::filesystem::create_directories(tmp_work);

    // 1. Test sanitize_mod_name
    assert(llk2::launcher::sanitize_mod_name("My Cool Mod! ../evil") == "My_Cool_Mod_evil");
    assert(llk2::launcher::sanitize_mod_name("---test_01") == "test_01");

    // 2. Test pack integrity checker on incomplete vs complete packs
    auto bad_rep = llk2::launcher::check_pack_integrity(tmp_work / "empty_pack");
    assert(!bad_rep.valid && bad_rep.missing.size() == 21);

    auto good_rep = llk2::launcher::check_pack_integrity(base_assets);
    assert(good_rep.valid && good_rep.present_count == 21 && good_rep.missing.empty());

    // 3. Test cloning base assets into <tmp_work>/assets and <tmp_work>/mods/unit_mod
    std::string err;
    assert(llk2::launcher::clone_asset_pack(base_assets, tmp_work / "assets", &err));
    assert(llk2::launcher::clone_asset_pack(base_assets, tmp_work / "mods" / "unit_mod", &err));
    auto mods = llk2::launcher::list_mod_packs(tmp_work / "mods");
    assert(mods.size() == 1 && mods[0] == "unit_mod");

    // 4. Test native C++/Qt tile sprite replacement (Pic_MjData_Picture.bmp art + SRCAND mask)
    auto custom_tile_png = tmp_work / "custom_tile.png";
    {
        QImage tile(32, 32, QImage::Format_RGBA8888);
        tile.fill(QColor(0, 0, 0, 0)); // transparent outer border
        for (int y = 6; y < 26; ++y) {
            for (int x = 6; x < 26; ++x) {
                tile.setPixelColor(x, y, QColor(240, 40, 60, 255));
            }
        }
        assert(tile.save(QString::fromStdString(custom_tile_png.string()), "PNG"));
    }
    assert(llk2::launcher::replace_tile_sprite(tmp_work / "mods" / "unit_mod", 0, custom_tile_png, &err));
    auto modded_sheet_path = tmp_work / "mods" / "unit_mod" / "images" / "Pic_MjData_Picture.bmp";
    assert(std::filesystem::is_regular_file(modded_sheet_path));
    {
        QImage modded(QString::fromStdString(modded_sheet_path.string()));
        assert(!modded.isNull() && modded.width() == 360 && modded.height() == 612);
        // Center of Tile #0 art cell (x=4+16=20, y=9+16=25) must be our red color; mask cell (20, 250+25) must be black
        QColor art_c = modded.pixelColor(20, 25);
        QColor mask_c = modded.pixelColor(20, 275);
        assert(art_c.red() >= 230 && art_c.green() <= 50 && art_c.blue() <= 70);
        assert(mask_c.red() == 0 && mask_c.green() == 0 && mask_c.blue() == 0);
        // Corner of Tile #0 (0, 0) outside sprite must be black art & white mask (255,255,255)
        QColor art_bg = modded.pixelColor(0, 0);
        QColor mask_bg = modded.pixelColor(0, 250);
        assert(art_bg.red() == 0 && art_bg.green() == 0 && art_bg.blue() == 0);
        assert(mask_bg.red() == 255 && mask_bg.green() == 255 && mask_bg.blue() == 255);
    }

    // 5. Test level background (600x450) and title screen (500x375) replacement
    auto custom_bg_png = tmp_work / "custom_bg.png";
    {
        QImage bg(800, 400, QImage::Format_RGB888);
        bg.fill(QColor(30, 90, 180));
        assert(bg.save(QString::fromStdString(custom_bg_png.string()), "PNG"));
    }
    assert(llk2::launcher::replace_level_background(tmp_work / "mods" / "unit_mod", 0, custom_bg_png, &err));
    {
        QImage out_bg(QString::fromStdString((tmp_work / "mods" / "unit_mod" / "images" / "Pic_Level_0_Picture.png").string()));
        assert(!out_bg.isNull() && out_bg.width() == 600 && out_bg.height() == 450);
    }
    assert(llk2::launcher::replace_title_image(tmp_work / "mods" / "unit_mod", custom_bg_png, &err));
    {
        QImage out_t(QString::fromStdString((tmp_work / "mods" / "unit_mod" / "images" / "Pic_Title_Picture.png").string()));
        assert(!out_t.isNull() && out_t.width() == 500 && out_t.height() == 375);
    }

    // 6. Test composited 42-tile preview rendering via AssetManager
    {
        llk2::media::AssetManager am;
        assert(am.load_from_dir(tmp_work / "mods" / "unit_mod", &err));
        QImage preview = llk2::launcher::render_composited_tile_preview(am, 0);
        assert(!preview.isNull() && preview.width() > 300 && preview.height() > 300);
    }

    // 7. Test SHA-256 inspector if D4s.exe is available in parent workspace
    auto orig_exe = src_dir.parent_path() / "D4s.exe";
    if (std::filesystem::is_regular_file(orig_exe)) {
        auto hinfo = llk2::launcher::inspect_source_archive(orig_exe);
        assert(hinfo.ok && hinfo.is_known && !hinfo.is_zip);
    }

    // 8. Run headless Qt 6 llk2-launcher --smoke-test
#if defined(_WIN32)
    _putenv_s("QT_QPA_PLATFORM", "offscreen");
    std::string cmd = "\"\"" + launcher_bin.string() + "\" --data-dir \"" + tmp_work.string() + "\" --smoke-test\"";
#else
    setenv("QT_QPA_PLATFORM", "offscreen", 1);
    std::string cmd = "\"" + launcher_bin.string() + "\" --data-dir \"" + tmp_work.string() + "\" --smoke-test";
#endif
    int rc = std::system(cmd.c_str());

    if (created_temp) {
        std::filesystem::remove_all(base_assets);
    }
    std::filesystem::remove_all(tmp_work);

    assert(rc == 0 && "llk2-launcher --smoke-test failed");
    std::printf("[PASS] test_launcher_ui\n");
    return 0;
}
