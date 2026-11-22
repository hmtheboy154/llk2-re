#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

int main() {
    std::filesystem::path src_dir(LLK2_SOURCE_DIR);
    std::filesystem::path imgui_bin(LLK2_IMGUI_BINARY);
    assert(std::filesystem::exists(imgui_bin));

    bool created_temp = false;
    std::filesystem::path assets_dir;
    if (std::filesystem::exists(src_dir / "assets" / "images")) {
        assets_dir = src_dir / "assets";
    } else {
        std::filesystem::path exe_candidate = src_dir.parent_path() / "D4s.exe";
        std::filesystem::path extractor = src_dir / "tools" / "extract_assets.py";
        assert(std::filesystem::exists(exe_candidate) && std::filesystem::exists(extractor));
        assets_dir = std::filesystem::temp_directory_path() / "llk2_test_imgui_assets";
        std::string extract_cmd = "python3 \"" + extractor.string() + "\" \"" + exe_candidate.string() +
                                  "\" -o \"" + assets_dir.string() + "\" > /dev/null 2>&1";
        assert(std::system(extract_cmd.c_str()) == 0);
        created_temp = true;
    }

    auto tmp_work = std::filesystem::temp_directory_path() / "llk2_test_imgui_workdir";
    std::filesystem::create_directories(tmp_work);

    std::string cmd = "cd \"" + tmp_work.string() + "\" && SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \"" +
                      imgui_bin.string() + "\" --assets \"" + assets_dir.string() + "\" --smoke-test";
    int rc = std::system(cmd.c_str());

    if (created_temp) {
        std::filesystem::remove_all(assets_dir);
    }
    std::filesystem::remove_all(tmp_work);

    assert(rc == 0 && "llk2-imgui --smoke-test failed");
    std::printf("[PASS] test_imgui_ui\n");
    return 0;
}
