#pragma once

#include <filesystem>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace llk2::core {

class I18n {
public:
    using ChangeCallback = std::function<void(const std::string& new_lang)>;

    I18n();

    // Loads all *.json locale files from a directory (e.g. "locales/").
    bool load_locales_dir(const std::filesystem::path& dir);

    // Loads a single <lang>.json file under lang_code.
    bool load_locale_file(const std::string& lang_code, const std::filesystem::path& file_path);

    // Switches active language (e.g. "zh-TW", "zh-CN", "en", "vi") and notifies listeners.
    bool set_language(const std::string& lang_code);
    const std::string& language() const noexcept { return current_lang_; }

    std::vector<std::string> available_languages() const;

    // Query raw OS locale tag (Win32 GetUserDefaultLocaleName / POSIX LANGUAGE, LC_ALL, LC_MESSAGES, LANG).
    static std::string raw_system_locale();

    // Map an arbitrary OS locale string (e.g. "vi_VN.UTF-8", "zh-HK", "en-US") to the best available catalog,
    // falling back to "en" (or "zh-TW" if "en" is unavailable).
    std::string map_locale_to_language(std::string_view raw_locale) const;

    // Detect the best initial language from the user's OS locale settings.
    std::string detect_system_language() const;

    // Translates a key into the active language (falling back to zh-TW, then returning key).
    std::string tr(std::string_view key) const;

    // Translates and substitutes `{placeholder}` tokens.
    std::string format(
        std::string_view key,
        std::initializer_list<std::pair<std::string_view, std::string>> args) const;

    void add_change_listener(ChangeCallback cb);

private:
    void init_builtin_catalogs();

    std::string current_lang_ = "zh-TW";
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> catalogs_;
    std::vector<ChangeCallback> listeners_;
};

} // namespace llk2::core
