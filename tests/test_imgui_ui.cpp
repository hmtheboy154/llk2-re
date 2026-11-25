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
#if defined(_WIN32)
        std::string extract_cmd = "\"python \"" + extractor.string() + "\" \"" + exe_candidate.string() +
                                  "\" -o \"" + assets_dir.string() + "\" > NUL 2>&1\"";
#else
        std::string extract_cmd = "python3 \"" + extractor.string() + "\" \"" + exe_candidate.string() +
                                  "\" -o \"" + assets_dir.string() + "\" > /dev/null 2>&1";
#endif
        assert(std::system(extract_cmd.c_str()) == 0);
        created_temp = true;
    }

    auto tmp_work = std::filesystem::temp_directory_path() / "llk2_test_imgui_workdir";
    std::filesystem::create_directories(tmp_work);
    auto orig_cwd = std::filesystem::current_path();
    std::filesystem::current_path(tmp_work);

#if defined(_WIN32)
    _putenv_s("SDL_VIDEODRIVER", "dummy");
    _putenv_s("SDL_AUDIODRIVER", "dummy");
    std::string cmd = "\"\"" + imgui_bin.string() + "\" --data-dir \"" + tmp_work.string() +
                      "\" --assets \"" + assets_dir.string() + "\" --smoke-test\"";
#else
    setenv("SDL_VIDEODRIVER", "dummy", 1);
    setenv("SDL_AUDIODRIVER", "dummy", 1);
    std::string cmd = "\"" + imgui_bin.string() + "\" --data-dir \"" + tmp_work.string() +
                      "\" --assets \"" + assets_dir.string() + "\" --smoke-test";
#endif
    int rc = std::system(cmd.c_str());
    std::filesystem::current_path(orig_cwd);

    if (created_temp) {
        std::filesystem::remove_all(assets_dir);
    }
    std::filesystem::remove_all(tmp_work);

    assert(rc == 0 && "llk2-imgui --smoke-test failed");
    std::printf("[PASS] test_imgui_ui\n");
    return 0;
}
