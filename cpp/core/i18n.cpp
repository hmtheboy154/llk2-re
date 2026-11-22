#include "i18n.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace llk2::core {

I18n::I18n() {
    init_builtin_catalogs();
}

void I18n::init_builtin_catalogs() {
    auto& tw = catalogs_["zh-TW"];
    tw["app.title"] = "連連看2";
    tw["dialog.explain.title"] = "遊戲說明";
    tw["dialog.author.title"] = "作者";
    tw["dialog.table.title"] = "連連看2";
    tw["menu.game"] = "遊戲(&G)";
    tw["menu.game.new"] = "新局(&N)";
    tw["menu.game.giveup"] = "放棄(&Q)";
    tw["menu.game.record"] = "最佳記錄(&R)";
    tw["menu.game.exit"] = "結束離開(&X)";
    tw["menu.audio"] = "背景聲音(&B)";
    tw["menu.audio.music"] = "音樂(&M)";
    tw["menu.audio.sound"] = "音效(&S)";
    tw["menu.settings"] = "設定(&S)";
    tw["menu.settings.language"] = "語言(&L)";
    tw["lang.zh_TW"] = "繁體中文 (zh-TW)";
    tw["lang.zh_CN"] = "简体中文 (zh-CN)";
    tw["menu.settings.midi_tracks"] = "背景音樂音軌";
    tw["menu.settings.track_101"] = "音軌 1：Michael P. Sirotkin - Aquarelle";
    tw["menu.settings.track_102"] = "音軌 2：Miyuki Nakajima - Sora to Kimi no Aida ni";
    tw["menu.settings.track_103"] = "音軌 3：Air Supply - Making Love Out Of Nothing At All";
    tw["menu.settings.midi_device"] = "音樂播放裝置";
    tw["menu.settings.midi_ost"] = "原聲帶無損音訊 (OST FLAC/MP3)";
    tw["menu.settings.midi_native"] = "系統原生 MIDI 裝置";
    tw["menu.settings.midi_embedded"] = "內建 SoundFont 合成器";
    tw["menu.settings.select_soundfont"] = "選擇自訂 SoundFont (.sf2)...";
    tw["menu.settings.scale"] = "畫面縮放";
    tw["menu.settings.scale_1x"] = "1× (768×550 原始大小)";
    tw["menu.settings.scale_1_5x"] = "1.5× (1152×825)";
    tw["menu.settings.scale_2x"] = "2× (1536×1100)";
    tw["menu.settings.scale_3x"] = "3× (2304×1650)";
    tw["menu.settings.scale_free"] = "自由縮放 (保持比例)";
    tw["menu.settings.filter_nearest"] = "像素銳利 (Nearest)";
    tw["menu.settings.filter_bilinear"] = "平滑柔化 (Bilinear)";
    tw["menu.settings.import_hst"] = "匯入舊版紀錄 (d4s.hst)...";
    tw["menu.settings.export_hst"] = "匯出舊版紀錄 (d4s.hst)...";
    tw["menu.about"] = "關於(&A)";
    tw["menu.about.explain"] = "遊戲說明(&E)";
    tw["menu.about.author"] = "作者(&P)";
    tw["table.col.rank"] = "名次";
    tw["table.col.name"] = "姓名";
    tw["table.col.score"] = "積分";
    tw["table.prompt.name"] = "玩家姓名";
    tw["table.btn.ok"] = "確定";
    tw["dialog.btn.close"] = "關閉";
    tw["table.default_player"] = "Player";
    tw["msg.stage_clear.title"] = "成功";
    tw["msg.stage_clear.body"] = "過關成功，請進入第 {level} 關。\n剩餘時間能量可以獲得 {bonus} 分";
    tw["msg.all_clear.title"] = "恭喜破關";
    tw["msg.all_clear.body"] = "真是厲害!!恭喜完成所有關卡。\n剩餘時間及生命能量可以獲得 {bonus} 分\n累計總得分為 {score} 分";
    tw["msg.no_moves.title"] = "殘局";
    tw["msg.no_moves.body"] = "已無牌可消除，遊戲結束。";
    tw["msg.timeout.title"] = "殘局";
    tw["msg.timeout.body"] = "時間能量用盡，遊戲結束。\n剩餘生命能量可以獲得 {bonus} 分\n累計總得分為 {score} 分";
    tw["msg.missing_assets.title"] = "缺少遊戲素材";
    tw["msg.missing_assets.body"] = "找不到遊戲素材資料夾 (assets/)。\n請先執行以下指令從原版主程式提取素材：\npython3 tools/extract_assets.py D4s.exe";

    auto& cn = catalogs_["zh-CN"];
    cn["app.title"] = "连连看2";
    cn["dialog.explain.title"] = "游戏规则";
    cn["dialog.author.title"] = "作者";
    cn["dialog.table.title"] = "连连看2";
    cn["menu.game"] = "游戏(&G)";
    cn["menu.game.new"] = "新局(&N)";
    cn["menu.game.giveup"] = "放弃(&Q)";
    cn["menu.game.record"] = "最佳记录(&R)";
    cn["menu.game.exit"] = "结束离开(&X)";
    cn["menu.audio"] = "背景声音(&B)";
    cn["menu.audio.music"] = "音乐(&M)";
    cn["menu.audio.sound"] = "音效(&S)";
    cn["menu.settings"] = "设置(&S)";
    cn["menu.settings.language"] = "语言(&L)";
    cn["lang.zh_TW"] = "繁體中文 (zh-TW)";
    cn["lang.zh_CN"] = "简体中文 (zh-CN)";
    cn["menu.settings.midi_tracks"] = "背景音乐音轨";
    cn["menu.settings.track_101"] = "音轨 1：Michael P. Sirotkin - Aquarelle";
    cn["menu.settings.track_102"] = "音轨 2：Miyuki Nakajima - Sora to Kimi no Aida ni";
    cn["menu.settings.track_103"] = "音轨 3：Air Supply - Making Love Out Of Nothing At All";
    cn["menu.settings.midi_device"] = "音乐播放设备";
    cn["menu.settings.midi_ost"] = "原声带无损音频 (OST FLAC/MP3)";
    cn["menu.settings.midi_native"] = "系统原生 MIDI 设备";
    cn["menu.settings.midi_embedded"] = "内置 SoundFont 合成器";
    cn["menu.settings.select_soundfont"] = "选择自定义 SoundFont (.sf2)...";
    cn["menu.settings.scale"] = "画面缩放";
    cn["menu.settings.scale_1x"] = "1× (768×550 原始大小)";
    cn["menu.settings.scale_1_5x"] = "1.5× (1152×825)";
    cn["menu.settings.scale_2x"] = "2× (1536×1100)";
    cn["menu.settings.scale_3x"] = "3× (2304×1650)";
    cn["menu.settings.scale_free"] = "自由缩放 (保持比例)";
    cn["menu.settings.filter_nearest"] = "像素锐利 (Nearest)";
    cn["menu.settings.filter_bilinear"] = "平滑柔化 (Bilinear)";
    cn["menu.settings.import_hst"] = "导入旧版记录 (d4s.hst)...";
    cn["menu.settings.export_hst"] = "导出旧版记录 (d4s.hst)...";
    cn["menu.about"] = "关于(&A)";
    cn["menu.about.explain"] = "游戏规则(&E)";
    cn["menu.about.author"] = "作者(&P)";
    cn["table.col.rank"] = "名次";
    cn["table.col.name"] = "姓名";
    cn["table.col.score"] = "积分";
    cn["table.prompt.name"] = "玩家姓名";
    cn["table.btn.ok"] = "确定";
    cn["dialog.btn.close"] = "关闭";
    cn["table.default_player"] = "Player";
    cn["msg.stage_clear.title"] = "成功";
    cn["msg.stage_clear.body"] = "过关成功，请进入第 {level} 关。\n剩余时间能量可以获得 {bonus} 分";
    cn["msg.all_clear.title"] = "恭喜破关";
    cn["msg.all_clear.body"] = "真是厉害!!恭喜完成所有关卡。\n剩余时间及生命能量可以获得 {bonus} 分\n累计总得分为 {score} 分";
    cn["msg.no_moves.title"] = "残局";
    cn["msg.no_moves.body"] = "已无牌可消除，游戏结束。";
    cn["msg.timeout.title"] = "残局";
    cn["msg.timeout.body"] = "时间能量用尽，游戏结束。\n剩余生命能量可以获得 {bonus} 分\n累计总得分为 {score} 分";
    cn["msg.missing_assets.title"] = "缺少游戏素材";
    cn["msg.missing_assets.body"] = "找不到游戏素材文件夹 (assets/)。\n请先运行以下命令从原版主程序提取素材：\npython3 tools/extract_assets.py lianliankan2.ExE";
}

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
