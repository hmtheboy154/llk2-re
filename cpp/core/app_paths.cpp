#include "core/app_paths.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif

namespace llk2::core {

namespace {

std::string get_env_str(const char* name) {
    if (!name) return {};
#if defined(_WIN32)
    char* val = nullptr;
    size_t len = 0;
    if (_dupenv_s(&val, &len, name) == 0 && val) {
        std::string out(val);
        std::free(val);
        return out;
    }
    return {};
#else
    const char* val = std::getenv(name);
    return val ? std::string(val) : std::string();
#endif
}

std::filesystem::path get_home_dir() {
#if defined(_WIN32)
    std::string up = get_env_str("USERPROFILE");
    if (!up.empty()) return std::filesystem::path(up);
    std::string hd = get_env_str("HOMEDRIVE");
    std::string hp = get_env_str("HOMEPATH");
    if (!hd.empty() && !hp.empty()) return std::filesystem::path(hd + hp);
    return {};
#else
    std::string home = get_env_str("HOME");
    return home.empty() ? std::filesystem::path{} : std::filesystem::path(home);
#endif
}

bool paths_equivalent_safe(const std::filesystem::path& a, const std::filesystem::path& b) {
    if (a.empty() || b.empty()) return false;
    std::error_code ec;
    if (std::filesystem::exists(a, ec) && std::filesystem::exists(b, ec)) {
        bool eq = std::filesystem::equivalent(a, b, ec);
        if (!ec) return eq;
    }
    return a.lexically_normal() == b.lexically_normal();
}

bool is_inside_dir(const std::filesystem::path& path, const std::filesystem::path& dir) {
    if (path.empty() || dir.empty()) return false;
    std::error_code ec;
    auto p = std::filesystem::weakly_canonical(path, ec);
    auto d = std::filesystem::weakly_canonical(dir, ec);
    if (p.empty() || d.empty()) return false;
    auto pit = p.begin();
    for (auto dit = d.begin(); dit != d.end(); ++dit, ++pit) {
        if (pit == p.end() || *pit != *dit) return false;
    }
    return true;
}

bool has_local_llk2_markers(const std::filesystem::path& dir) {
    if (dir.empty()) return false;
    std::error_code ec;
    return std::filesystem::is_directory(dir / "assets", ec) ||
           std::filesystem::is_regular_file(dir / "config.json", ec) ||
           std::filesystem::is_regular_file(dir / "scores.json", ec) ||
           std::filesystem::is_regular_file(dir / "d4s.hst", ec);
}

void push_unique_dir(std::vector<std::filesystem::path>& out, const std::filesystem::path& dir) {
    if (dir.empty()) return;
    std::error_code ec;
    std::filesystem::path norm = std::filesystem::weakly_canonical(dir, ec);
    if (ec || norm.empty()) norm = dir.lexically_normal();
    for (const auto& existing : out) {
        if (paths_equivalent_safe(existing, norm)) return;
    }
    out.push_back(norm);
}

} // namespace

std::filesystem::path AppPaths::default_user_data_dir() {
#if defined(_WIN32)
    std::string appdata = get_env_str("APPDATA");
    if (!appdata.empty()) {
        return std::filesystem::path(appdata) / "llk2";
    }
    auto home = get_home_dir();
    if (!home.empty()) {
        return home / "AppData" / "Roaming" / "llk2";
    }
    std::error_code ec;
    return std::filesystem::current_path(ec) / "llk2-data";
#elif defined(__APPLE__)
    auto home = get_home_dir();
    if (!home.empty()) {
        return home / "Library" / "Application Support" / "llk2";
    }
    std::error_code ec;
    return std::filesystem::current_path(ec) / ".llk2";
#else
    std::string xdg = get_env_str("XDG_DATA_HOME");
    if (!xdg.empty() && xdg[0] == '/') {
        return std::filesystem::path(xdg) / "llk2";
    }
    auto home = get_home_dir();
    if (!home.empty()) {
        return home / ".local" / "share" / "llk2";
    }
    std::error_code ec;
    return std::filesystem::current_path(ec) / ".llk2";
#endif
}

std::filesystem::path AppPaths::executable_dir(const char* argv0) {
    std::error_code ec;
#if defined(_WIN32)
    std::wstring buf(MAX_PATH, L'\0');
    DWORD len = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    while (len == buf.size() && buf.size() < 32768) {
        buf.resize(buf.size() * 2, L'\0');
        len = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    }
    if (len > 0) {
        buf.resize(len);
        return std::filesystem::path(buf).parent_path();
    }
#elif defined(__APPLE__)
    uint32_t sz = 0;
    _NSGetExecutablePath(nullptr, &sz);
    if (sz > 0) {
        std::string buf(sz, '\0');
        if (_NSGetExecutablePath(buf.data(), &sz) == 0) {
            auto p = std::filesystem::weakly_canonical(std::filesystem::path(buf.c_str()), ec);
            if (!ec && !p.empty()) return p.parent_path();
        }
    }
#else
    auto self_exe = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec && !self_exe.empty()) {
        return self_exe.parent_path();
    }
#endif
    if (argv0 && *argv0) {
        auto p = std::filesystem::weakly_canonical(std::filesystem::path(argv0), ec);
        if (!ec && !p.empty() && p.has_parent_path()) {
            return p.parent_path();
        }
    }
    return std::filesystem::current_path(ec);
}

bool AppPaths::is_dir_writable(const std::filesystem::path& dir) {
    if (dir.empty()) return false;
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) {
        return false;
    }
#if defined(_WIN32)
    // Probe write permission without leaving anything behind
    auto probe = dir / ".llk2_write_probe.tmp";
    HANDLE h = CreateFileW(probe.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_NEW, FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        return true;
    }
    return false;
#else
    return ::access(dir.c_str(), W_OK | X_OK) == 0;
#endif
}

std::vector<std::filesystem::path> AppPaths::bundled_resource_dirs(const std::filesystem::path& exe_dir_hint) {
    std::vector<std::filesystem::path> dirs;
    std::filesystem::path ed = exe_dir_hint.empty() ? executable_dir() : exe_dir_hint;
    push_unique_dir(dirs, ed);
    if (ed.has_parent_path()) {
        push_unique_dir(dirs, ed.parent_path());
    }

    std::string appdir = get_env_str("APPDIR");
    if (!appdir.empty()) {
        push_unique_dir(dirs, std::filesystem::path(appdir) / "usr" / "share" / "llk2");
    }

#if defined(__APPLE__)
    if (!ed.empty()) {
        push_unique_dir(dirs, ed / ".." / "Resources");
    }
#elif !defined(_WIN32)
    push_unique_dir(dirs, "/usr/share/llk2");
    push_unique_dir(dirs, "/usr/local/share/llk2");
#endif

#if defined(LLK2_SOURCE_DIR)
    push_unique_dir(dirs, std::filesystem::path(LLK2_SOURCE_DIR));
#endif

    return dirs;
}

AppPaths AppPaths::resolve(const char* argv0, const std::filesystem::path& cli_data_dir) {
    AppPaths out;
    std::error_code ec;
    out.exe_dir = executable_dir(argv0);
    std::filesystem::path cwd = std::filesystem::current_path(ec);
    std::filesystem::path home = get_home_dir();

    std::string appdir_env = get_env_str("APPDIR");
    std::filesystem::path appdir = appdir_env.empty() ? std::filesystem::path{} : std::filesystem::path(appdir_env);

    std::string appimage_env = get_env_str("APPIMAGE");
    std::filesystem::path appimage_dir = appimage_env.empty()
                                             ? std::filesystem::path{}
                                             : std::filesystem::path(appimage_env).parent_path();

    const bool exe_in_appdir = !appdir.empty() && is_inside_dir(out.exe_dir, appdir);

    // 1. Explicit CLI --data-dir override
    if (!cli_data_dir.empty()) {
        out.data_dir = std::filesystem::absolute(cli_data_dir, ec);
        if (ec || out.data_dir.empty()) out.data_dir = cli_data_dir;
        out.mode = StorageMode::CustomOverride;
    }
    // 2. Explicit LLK2_DATA_DIR environment variable
    else if (std::string env_dir = get_env_str("LLK2_DATA_DIR"); !env_dir.empty()) {
        out.data_dir = std::filesystem::path(env_dir);
        out.mode = StorageMode::EnvOverride;
    }
    // 3. Explicit portable.txt marker (next to $APPIMAGE or executable)
    else if (!appimage_dir.empty() &&
             std::filesystem::exists(appimage_dir / "portable.txt", ec) &&
             is_dir_writable(appimage_dir)) {
        out.data_dir = appimage_dir;
        out.mode = StorageMode::Portable;
    } else if (!exe_in_appdir &&
               std::filesystem::exists(out.exe_dir / "portable.txt", ec) &&
               is_dir_writable(out.exe_dir)) {
        out.data_dir = out.exe_dir;
        out.mode = StorageMode::Portable;
    }
    // 4. Local dev / portable folder auto-detection (never treats $HOME itself as portable!)
    else if (!exe_in_appdir &&
             !paths_equivalent_safe(out.exe_dir, home) &&
             has_local_llk2_markers(out.exe_dir) &&
             is_dir_writable(out.exe_dir)) {
        out.data_dir = out.exe_dir;
        out.mode = StorageMode::Portable;
    } else if (!cwd.empty() &&
               !paths_equivalent_safe(cwd, home) &&
               has_local_llk2_markers(cwd) &&
               is_dir_writable(cwd)) {
        out.data_dir = cwd;
        out.mode = StorageMode::Portable;
    } else if (!exe_in_appdir &&
               std::filesystem::is_regular_file(out.exe_dir / "CMakeCache.txt", ec) &&
               out.exe_dir.has_parent_path() &&
               !paths_equivalent_safe(out.exe_dir.parent_path(), home) &&
               has_local_llk2_markers(out.exe_dir.parent_path()) &&
               is_dir_writable(out.exe_dir.parent_path())) {
        out.data_dir = out.exe_dir.parent_path();
        out.mode = StorageMode::Portable;
    }
    // 5. Standard OS user data directory (~/.local/share/llk2, %APPDATA%\llk2, ~/Library/Application Support/llk2)
    else {
        out.data_dir = default_user_data_dir();
        out.mode = StorageMode::UserDirectory;
    }

    // Build ordered read-only resource search list
    push_unique_dir(out.resource_dirs, out.data_dir);
    for (const auto& d : bundled_resource_dirs(out.exe_dir)) {
        push_unique_dir(out.resource_dirs, d);
    }
    if (!cwd.empty()) {
        push_unique_dir(out.resource_dirs, cwd);
    }

    return out;
}

bool AppPaths::ensure_data_dir() const {
    if (data_dir.empty()) return false;
    std::error_code ec;
    if (std::filesystem::is_directory(data_dir, ec)) return true;
    return std::filesystem::create_directories(data_dir, ec);
}

std::filesystem::path AppPaths::find_resource(const std::filesystem::path& rel_path) const {
    if (rel_path.empty()) return {};
    std::error_code ec;
    for (const auto& base : resource_dirs) {
        auto cand = base / rel_path;
        if (std::filesystem::exists(cand, ec)) {
            return cand;
        }
    }
    return {};
}

std::filesystem::path AppPaths::find_binary(const std::string& base_name) const {
    if (base_name.empty()) return {};
    std::error_code ec;
#if defined(_WIN32)
    std::string fname = base_name + ".exe";
#else
    std::string fname = base_name;
#endif

    // 1. Sibling binary next to launcher (same build/install directory)
    if (!exe_dir.empty()) {
        auto sibling = exe_dir / fname;
        if (std::filesystem::is_regular_file(sibling, ec)) {
            return sibling;
        }
    }

    // 2. Check PATH entries
    std::string path_env = get_env_str("PATH");
#if defined(_WIN32)
    const char sep = ';';
#else
    const char sep = ':';
#endif
    size_t start = 0;
    while (start <= path_env.size()) {
        size_t end = path_env.find(sep, start);
        std::string seg = (end == std::string::npos)
                              ? path_env.substr(start)
                              : path_env.substr(start, end - start);
        if (!seg.empty()) {
            auto cand = std::filesystem::path(seg) / fname;
            if (std::filesystem::is_regular_file(cand, ec)) {
                return cand;
            }
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return {};
}

} // namespace llk2::core
