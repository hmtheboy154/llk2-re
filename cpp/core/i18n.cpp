#include "i18n.hpp"
#include "builtin_locales.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace llk2::core {

static bool parse_flat_json_object(const std::string& content, std::unordered_map<std::string, std::string>& out) {
    size_t i = 0;
    auto skip_ws = [&]() {
        while (i < content.size() && (content[i] == ' ' || content[i] == '\t' || content[i] == '\r' || content[i] == '\n' || content[i] == ',' || content[i] == '{' || content[i] == '}')) {
            ++i;
        }
    };
    auto read_quoted = [&](std::string& s) -> bool {
        if (i >= content.size() || content[i] != '"') return false;
        ++i;
        s.clear();
        while (i < content.size()) {
            if (content[i] == '\\' && i + 1 < content.size()) {
                char esc = content[i + 1];
                if (esc == 'n') s += '\n';
                else if (esc == 'r') s += '\r';
                else if (esc == 't') s += '\t';
                else s += esc;
                i += 2;
            } else if (content[i] == '"') {
                ++i;
                return true;
            } else {
                s += content[i++];
            }
        }
        return false;
    };

    int count = 0;
    while (i < content.size()) {
        skip_ws();
        if (i >= content.size()) break;
        std::string key;
        if (!read_quoted(key)) break;
        while (i < content.size() && (content[i] == ' ' || content[i] == '\t' || content[i] == ':')) {
            ++i;
        }
        std::string val;
        if (!read_quoted(val)) break;
        out[key] = std::move(val);
        ++count;
    }
    return count > 0;
}

I18n::I18n() {
    init_builtin_catalogs();
}

void I18n::init_builtin_catalogs() {
    for (const auto& blob : detail::kBuiltinLocales) {
        std::string json(reinterpret_cast<const char*>(blob.data), blob.size);
        std::unordered_map<std::string, std::string> parsed;
        if (parse_flat_json_object(json, parsed)) {
            catalogs_[blob.lang] = std::move(parsed);
        }
    }
}

bool I18n::load_locale_file(const std::string& lang_code, const std::filesystem::path& file_path) {
    std::ifstream f(file_path, std::ios::binary);
    if (!f) {
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::unordered_map<std::string, std::string> parsed = catalogs_[lang_code];
    if (!parse_flat_json_object(ss.str(), parsed)) {
        return false;
    }
    catalogs_[lang_code] = std::move(parsed);
    return true;
}

bool I18n::load_locales_dir(const std::filesystem::path& dir) {
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        return false;
    }
    bool any = false;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::string lang = entry.path().stem().string();
            if (load_locale_file(lang, entry.path())) {
                any = true;
            }
        }
    }
    return any;
}

bool I18n::set_language(const std::string& lang_code) {
    if (catalogs_.find(lang_code) == catalogs_.end()) {
        return false;
    }
    if (current_lang_ != lang_code) {
        current_lang_ = lang_code;
        for (const auto& cb : listeners_) {
            cb(current_lang_);
        }
    }
    return true;
}

std::vector<std::string> I18n::available_languages() const {
    std::vector<std::string> langs;
    for (const auto& kv : catalogs_) {
        langs.push_back(kv.first);
    }
    std::sort(langs.begin(), langs.end());
    return langs;
}

static std::string to_lower_ascii_str(std::string_view sv) {
    std::string out(sv);
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

std::string I18n::raw_system_locale() {
#if defined(_WIN32)
    wchar_t wbuf[LOCALE_NAME_MAX_LENGTH] = {};
    if (GetUserDefaultLocaleName(wbuf, LOCALE_NAME_MAX_LENGTH) > 0) {
        std::string out;
        for (size_t i = 0; wbuf[i] != L'\0'; ++i) {
            if (wbuf[i] < 128) {
                out.push_back(static_cast<char>(wbuf[i]));
            }
        }
        if (!out.empty()) {
            return out;
        }
    }
#endif
    auto get_env_nonempty = [](const char* name) -> std::string {
#if defined(_WIN32)
        char* val = nullptr;
        size_t len = 0;
        if (_dupenv_s(&val, &len, name) == 0 && val) {
            std::string s(val);
            std::free(val);
            return s;
        }
        return {};
#else
        const char* val = std::getenv(name);
        return val ? std::string(val) : std::string();
#endif
    };

    // POSIX LANGUAGE can be a colon-separated preference list (e.g. "vi_VN:vi:en_US")
    std::string language_list = get_env_nonempty("LANGUAGE");
    if (!language_list.empty()) {
        size_t start = 0;
        while (start <= language_list.size()) {
            size_t end = language_list.find(':', start);
            std::string seg = (end == std::string::npos)
                                  ? language_list.substr(start)
                                  : language_list.substr(start, end - start);
            std::string low = to_lower_ascii_str(seg);
            if (!low.empty() && low != "c" && low != "posix" && low.rfind("c.", 0) != 0) {
                return seg;
            }
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }

    for (const char* var : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
        std::string val = get_env_nonempty(var);
        if (!val.empty()) {
            return val;
        }
    }
    return {};
}

std::string I18n::map_locale_to_language(std::string_view raw_locale) const {
    auto fallback = [&]() -> std::string {
        if (catalogs_.find("en") != catalogs_.end()) return "en";
        if (catalogs_.find("zh-TW") != catalogs_.end()) return "zh-TW";
        return catalogs_.empty() ? std::string("en") : catalogs_.begin()->first;
    };

    // Strip encoding (".UTF-8") and modifier ("@euro"), normalize '_' -> '-' and lowercase
    size_t cut = raw_locale.find_first_of(".@");
    std::string norm = to_lower_ascii_str(cut == std::string_view::npos ? raw_locale : raw_locale.substr(0, cut));
    std::replace(norm.begin(), norm.end(), '_', '-');

    if (norm.empty() || norm == "c" || norm == "posix") {
        return fallback();
    }

    // 1. Exact case-insensitive match against loaded catalogs (supports zh-TW, zh-CN, or custom pt-BR.json)
    for (const auto& kv : catalogs_) {
        if (to_lower_ascii_str(kv.first) == norm) {
            return kv.first;
        }
    }

    // 2. Chinese script / region disambiguation (Traditional vs Simplified)
    if (norm == "zh" || norm.rfind("zh-", 0) == 0) {
        bool traditional = (norm.find("hant") != std::string::npos) ||
                           (norm.find("-tw") != std::string::npos) ||
                           (norm.find("-hk") != std::string::npos) ||
                           (norm.find("-mo") != std::string::npos);
        std::string target = traditional ? "zh-TW" : "zh-CN";
        if (catalogs_.find(target) != catalogs_.end()) {
            return target;
        }
    }

    // 3. Primary language subtag match (e.g. "vi-vn" -> "vi", "en-us" -> "en", "ja-jp" -> custom "ja")
    size_t dash = norm.find('-');
    std::string primary = (dash == std::string::npos) ? norm : norm.substr(0, dash);
    for (const auto& kv : catalogs_) {
        if (to_lower_ascii_str(kv.first) == primary) {
            return kv.first;
        }
    }

    return fallback();
}

std::string I18n::detect_system_language() const {
    return map_locale_to_language(raw_system_locale());
}

std::string I18n::tr(std::string_view key) const {
    std::string k(key);
    auto it_lang = catalogs_.find(current_lang_);
    if (it_lang != catalogs_.end()) {
        auto it_val = it_lang->second.find(k);
        if (it_val != it_lang->second.end()) {
            return it_val->second;
        }
    }
    auto it_tw = catalogs_.find("zh-TW");
    if (it_tw != catalogs_.end()) {
        auto it_val = it_tw->second.find(k);
        if (it_val != it_tw->second.end()) {
            return it_val->second;
        }
    }
    return k;
}

std::string I18n::format(
    std::string_view key,
    std::initializer_list<std::pair<std::string_view, std::string>> args) const {
    std::string text = tr(key);
    for (const auto& kv : args) {
        std::string token = "{" + std::string(kv.first) + "}";
        size_t pos = 0;
        while ((pos = text.find(token, pos)) != std::string::npos) {
            text.replace(pos, token.size(), kv.second);
            pos += kv.second.size();
        }
    }
    return text;
}

void I18n::add_change_listener(ChangeCallback cb) {
    listeners_.push_back(std::move(cb));
}

} // namespace llk2::core
