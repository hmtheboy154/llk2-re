#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace llk2::core {

enum class StorageMode {
    UserDirectory = 0, // ~/.local/share/llk2 (Linux), %APPDATA%\llk2 (Windows), ~/Library/Application Support/llk2 (macOS)
    Portable = 1,      // Self-contained portable.txt or local dev/zip directory
    EnvOverride = 2,   // Explicit LLK2_DATA_DIR environment variable
    CustomOverride = 3 // Explicit --data-dir CLI argument
};

struct AppPaths {
    std::filesystem::path data_dir;      // Unified writable dir for config.json, scores.json, assets/, mods/, locales/
    std::filesystem::path exe_dir;       // Directory containing the running executable
    std::vector<std::filesystem::path> resource_dirs; // Ordered search paths for read-only bundled resources
    StorageMode mode = StorageMode::UserDirectory;

    // Resolve paths using priority:
    //  1. Explicit cli_data_dir (--data-dir)
    //  2. LLK2_DATA_DIR environment variable
    //  3. Portable marker (portable.txt next to exe or $APPIMAGE)
    //  4. Local writable dev/portable folder (assets/, config.json, scores.json, or d4s.hst beside exe/cwd/build-parent)
    //  5. Standard OS user data directory (~/.local/share/llk2, %APPDATA%\llk2, ~/Library/Application Support/llk2)
    static AppPaths resolve(const char* argv0 = nullptr,
                            const std::filesystem::path& cli_data_dir = {});

    // Standard OS-native per-user data directory (always returns ~/.local/share/llk2, %APPDATA%\llk2, etc.)
    static std::filesystem::path default_user_data_dir();

    // Directory of the currently running executable
    static std::filesystem::path executable_dir(const char* argv0 = nullptr);

    // Ordered bundled read-only resource directories (exe_dir, AppImage $APPDIR/usr/share/llk2, /usr/share/llk2, etc.)
    static std::vector<std::filesystem::path> bundled_resource_dirs(const std::filesystem::path& exe_dir_hint = {});

    // Check whether a directory is writable without leaving probe files behind
    static bool is_dir_writable(const std::filesystem::path& dir);

    std::filesystem::path config_file() const { return data_dir / "config.json"; }
    std::filesystem::path scores_file() const { return data_dir / "scores.json"; }
    std::filesystem::path legacy_hst_file() const { return data_dir / "d4s.hst"; }
    std::filesystem::path assets_dir() const { return data_dir / "assets"; }
    std::filesystem::path mods_dir() const { return data_dir / "mods"; }
    std::filesystem::path locales_dir() const { return data_dir / "locales"; }

    bool ensure_data_dir() const;

    // Locate a read-only resource (e.g. "soundfont.sf2", "tools/extract_assets.py") across resource_dirs
    std::filesystem::path find_resource(const std::filesystem::path& rel_path) const;

    // Locate a sibling game executable (e.g. "llk2-qt", "llk2-imgui", "llk2-bui") next to launcher or in PATH
    std::filesystem::path find_binary(const std::string& base_name) const;
};

} // namespace llk2::core
