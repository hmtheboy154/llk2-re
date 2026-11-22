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
    tw["lang.en"] = "English (en)";
    tw["lang.vi"] = "Tiếng Việt (vi)";
    tw["lang.bui.zh_TW"] = "繁體中文 (zh-TW)";
    tw["lang.bui.zh_CN"] = "简体中文 (zh-CN)";
    tw["lang.bui.en"] = "English (en)";
    tw["lang.bui.vi"] = "越南語 / Vietnamese (vi)";
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
    tw["menu.settings.theme"] = "介面主題 (Blender)";
    tw["menu.settings.theme_dark"] = "Blender 深色 (Dark)";
    tw["menu.settings.theme_light"] = "Blender 淺色 (Light)";
    tw["menu.settings.import_hst"] = "匯入舊版紀錄 (d4s.hst)...";
    tw["menu.settings.export_hst"] = "匯出舊版紀錄 (d4s.hst)...";
    tw["menu.settings.inspector"] = "狀態監視器 / 提示輔助";
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
    tw["inspector.title"] = "連連看2 狀態監視器 (F12)";
    tw["inspector.mode"] = "狀態";
    tw["inspector.mode.title"] = "標題畫面";
    tw["inspector.mode.playing"] = "遊戲中";
    tw["inspector.stage"] = "關卡";
    tw["inspector.tiles_left"] = "剩餘牌數";
    tw["inspector.lives"] = "生命";
    tw["inspector.score"] = "積分";
    tw["inspector.time_left"] = "剩餘時間";
    tw["inspector.audio_backend"] = "音訊後端";
    tw["inspector.soundfont"] = "音色庫";
    tw["inspector.track"] = "音軌";
    tw["inspector.playing"] = "播放中";
    tw["inspector.stopped"] = "已停止";
    tw["inspector.next_hint"] = "可消除提示";
    tw["inspector.highlight_hint"] = "在棋盤上標示提示配對";
    tw["inspector.shift.0"] = "固定無移動";
    tw["inspector.shift.1"] = "向上集中";
    tw["inspector.shift.2"] = "向下集中";
    tw["inspector.shift.3"] = "向左集中";
    tw["inspector.shift.4"] = "向右集中";
    tw["inspector.shift.5"] = "左右分離";
    tw["inspector.shift.6"] = "上下集中";
    tw["rules.use_overlay"] = "0";
    tw["rules.turn_0"] = "無轉折";
    tw["rules.turn_1"] = "一個轉折";
    tw["rules.turn_2"] = "兩個轉折";
    tw["rules.body"] = "";
    tw["rules.controls"] = "";
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
    cn["lang.en"] = "English (en)";
    cn["lang.vi"] = "Tiếng Việt (vi)";
    cn["lang.bui.zh_TW"] = "繁體中文 (zh-TW)";
    cn["lang.bui.zh_CN"] = "简体中文 (zh-CN)";
    cn["lang.bui.en"] = "English (en)";
    cn["lang.bui.vi"] = "越南语 / Vietnamese (vi)";
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
    cn["menu.settings.theme"] = "界面主题 (Blender)";
    cn["menu.settings.theme_dark"] = "Blender 深色 (Dark)";
    cn["menu.settings.theme_light"] = "Blender 浅色 (Light)";
    cn["menu.settings.import_hst"] = "导入旧版记录 (d4s.hst)...";
    cn["menu.settings.export_hst"] = "导出旧版记录 (d4s.hst)...";
    cn["menu.settings.inspector"] = "状态监视器 / 提示辅助";
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
    cn["inspector.title"] = "连连看2 状态监视器 (F12)";
    cn["inspector.mode"] = "状态";
    cn["inspector.mode.title"] = "标题画面";
    cn["inspector.mode.playing"] = "游戏中";
    cn["inspector.stage"] = "关卡";
    cn["inspector.tiles_left"] = "剩余牌数";
    cn["inspector.lives"] = "生命";
    cn["inspector.score"] = "积分";
    cn["inspector.time_left"] = "剩余时间";
    cn["inspector.audio_backend"] = "音频后端";
    cn["inspector.soundfont"] = "音色库";
    cn["inspector.track"] = "音轨";
    cn["inspector.playing"] = "播放中";
    cn["inspector.stopped"] = "已停止";
    cn["inspector.next_hint"] = "可消除提示";
    cn["inspector.highlight_hint"] = "在棋盘上标示提示配对";
    cn["inspector.shift.0"] = "固定无移动";
    cn["inspector.shift.1"] = "向上集中";
    cn["inspector.shift.2"] = "向下集中";
    cn["inspector.shift.3"] = "向左集中";
    cn["inspector.shift.4"] = "向右集中";
    cn["inspector.shift.5"] = "左右分离";
    cn["inspector.shift.6"] = "上下集中";
    cn["rules.use_overlay"] = "0";
    cn["rules.turn_0"] = "无转折";
    cn["rules.turn_1"] = "一个转折";
    cn["rules.turn_2"] = "两个转折";
    cn["rules.body"] = "";
    cn["rules.controls"] = "";
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

    auto& en = catalogs_["en"];
    en["app.title"] = "Link-Link 2";
    en["dialog.explain.title"] = "Game Rules";
    en["dialog.author.title"] = "Author";
    en["dialog.table.title"] = "Link-Link 2";
    en["menu.game"] = "&Game";
    en["menu.game.new"] = "&New Game";
    en["menu.game.giveup"] = "&Give Up";
    en["menu.game.record"] = "High &Scores";
    en["menu.game.exit"] = "E&xit";
    en["menu.audio"] = "&Audio";
    en["menu.audio.music"] = "&Music";
    en["menu.audio.sound"] = "Sound &Effects";
    en["menu.settings"] = "&Settings";
    en["menu.settings.language"] = "&Language";
    en["lang.zh_TW"] = "繁體中文 (zh-TW)";
    en["lang.zh_CN"] = "简体中文 (zh-CN)";
    en["lang.en"] = "English (en)";
    en["lang.vi"] = "Tiếng Việt (vi)";
    en["lang.bui.zh_TW"] = "Traditional Chinese (zh-TW)";
    en["lang.bui.zh_CN"] = "Simplified Chinese (zh-CN)";
    en["lang.bui.en"] = "English (en)";
    en["lang.bui.vi"] = "Tiếng Việt (vi)";
    en["menu.settings.midi_tracks"] = "Background Music Tracks";
    en["menu.settings.track_101"] = "Track 1: Michael P. Sirotkin - Aquarelle";
    en["menu.settings.track_102"] = "Track 2: Miyuki Nakajima - Sora to Kimi no Aida ni";
    en["menu.settings.track_103"] = "Track 3: Air Supply - Making Love Out Of Nothing At All";
    en["menu.settings.midi_device"] = "Music Playback Device";
    en["menu.settings.midi_ost"] = "Lossless OST Audio (FLAC/MP3)";
    en["menu.settings.midi_native"] = "Native OS MIDI Device";
    en["menu.settings.midi_embedded"] = "Embedded SoundFont Synth";
    en["menu.settings.select_soundfont"] = "Select Custom SoundFont (.sf2)...";
    en["menu.settings.scale"] = "Window Scale";
    en["menu.settings.scale_1x"] = "1× (768×550 Native)";
    en["menu.settings.scale_1_5x"] = "1.5× (1152×825)";
    en["menu.settings.scale_2x"] = "2× (1536×1100)";
    en["menu.settings.scale_3x"] = "3× (2304×1650)";
    en["menu.settings.scale_free"] = "Free Resize (Keep Aspect)";
    en["menu.settings.filter_nearest"] = "Crisp Pixels (Nearest)";
    en["menu.settings.filter_bilinear"] = "Smooth (Bilinear)";
    en["menu.settings.theme"] = "UI Theme (Blender)";
    en["menu.settings.theme_dark"] = "Blender Dark";
    en["menu.settings.theme_light"] = "Blender Light";
    en["menu.settings.import_hst"] = "Import Legacy Scores (d4s.hst)...";
    en["menu.settings.export_hst"] = "Export Legacy Scores (d4s.hst)...";
    en["menu.settings.inspector"] = "Inspector / Hint Assistant";
    en["menu.about"] = "A&bout";
    en["menu.about.explain"] = "Game &Rules";
    en["menu.about.author"] = "&Author";
    en["table.col.rank"] = "Rank";
    en["table.col.name"] = "Name";
    en["table.col.score"] = "Score";
    en["table.prompt.name"] = "Player Name";
    en["table.btn.ok"] = "OK";
    en["dialog.btn.close"] = "Close";
    en["table.default_player"] = "Player";
    en["inspector.title"] = "Link-Link 2 Inspector (F12)";
    en["inspector.mode"] = "Status";
    en["inspector.mode.title"] = "Title Screen";
    en["inspector.mode.playing"] = "Playing";
    en["inspector.stage"] = "Stage";
    en["inspector.tiles_left"] = "Tiles Left";
    en["inspector.lives"] = "Lives";
    en["inspector.score"] = "Score";
    en["inspector.time_left"] = "Time Left";
    en["inspector.audio_backend"] = "Audio Backend";
    en["inspector.soundfont"] = "SoundFont";
    en["inspector.track"] = "Track";
    en["inspector.playing"] = "Playing";
    en["inspector.stopped"] = "Stopped";
    en["inspector.next_hint"] = "Valid Pair Hint";
    en["inspector.highlight_hint"] = "Highlight Hint Pair on Board";
    en["inspector.shift.0"] = "Static";
    en["inspector.shift.1"] = "Shift Up";
    en["inspector.shift.2"] = "Shift Down";
    en["inspector.shift.3"] = "Shift Left";
    en["inspector.shift.4"] = "Shift Right";
    en["inspector.shift.5"] = "Split Left/Right";
    en["inspector.shift.6"] = "Gather Top/Bottom";
    en["rules.use_overlay"] = "1";
    en["rules.turn_0"] = "No\nTurns";
    en["rules.turn_1"] = "1 Turn";
    en["rules.turn_2"] = "2 Turns";
    en["rules.body"] = "Match two identical tiles with a clear path of at most 2 turns (see examples) to remove them and score points.\n\nClear all tiles before time runs out to reach the next stage. When no moves remain, the board shuffles automatically. You start with 10 shuffles and earn +1 per stage cleared. Top scores enter the leaderboard.";
    en["rules.controls"] = "Left Click = Select\nRight Click = Cancel";
    en["msg.stage_clear.title"] = "Stage Clear";
    en["msg.stage_clear.body"] = "Stage cleared! Advancing to Stage {level}.\nRemaining time energy awards {bonus} bonus points.";
    en["msg.all_clear.title"] = "Congratulations!";
    en["msg.all_clear.body"] = "Amazing!! You have cleared all stages.\nRemaining time and life energy award {bonus} bonus points.\nTotal final score: {score}";
    en["msg.no_moves.title"] = "Game Over";
    en["msg.no_moves.body"] = "No remaining moves available. Game over.";
    en["msg.timeout.title"] = "Time Up";
    en["msg.timeout.body"] = "Time energy depleted. Game over.\nRemaining life energy awards {bonus} bonus points.\nTotal final score: {score}";
    en["msg.missing_assets.title"] = "Missing Game Assets";
    en["msg.missing_assets.body"] = "Could not find the game assets directory (assets/).\nPlease extract assets from the original executable first:\npython3 tools/extract_assets.py D4s.exe";

    auto& vi = catalogs_["vi"];
    vi["app.title"] = "Liên Liên Khán 2";
    vi["dialog.explain.title"] = "Luật chơi";
    vi["dialog.author.title"] = "Tác giả";
    vi["dialog.table.title"] = "Liên Liên Khán 2";
    vi["menu.game"] = "Trò chơi(&G)";
    vi["menu.game.new"] = "Ván mới(&N)";
    vi["menu.game.giveup"] = "Bỏ cuộc(&Q)";
    vi["menu.game.record"] = "Bảng xếp hạng(&R)";
    vi["menu.game.exit"] = "Thoát(&X)";
    vi["menu.audio"] = "Âm thanh(&B)";
    vi["menu.audio.music"] = "Nhạc nền(&M)";
    vi["menu.audio.sound"] = "Hiệu ứng(&S)";
    vi["menu.settings"] = "Cài đặt(&S)";
    vi["menu.settings.language"] = "Ngôn ngữ(&L)";
    vi["lang.zh_TW"] = "繁體中文 (zh-TW)";
    vi["lang.zh_CN"] = "简体中文 (zh-CN)";
    vi["lang.en"] = "English (en)";
    vi["lang.vi"] = "Tiếng Việt (vi)";
    vi["lang.bui.zh_TW"] = "Tiếng Trung Phồn Thể (zh-TW)";
    vi["lang.bui.zh_CN"] = "Tiếng Trung Giản Thể (zh-CN)";
    vi["lang.bui.en"] = "English (en)";
    vi["lang.bui.vi"] = "Tiếng Việt (vi)";
    vi["menu.settings.midi_tracks"] = "Danh sách nhạc nền";
    vi["menu.settings.track_101"] = "Bài 1: Michael P. Sirotkin - Aquarelle";
    vi["menu.settings.track_102"] = "Bài 2: Miyuki Nakajima - Sora to Kimi no Aida ni";
    vi["menu.settings.track_103"] = "Bài 3: Air Supply - Making Love Out Of Nothing At All";
    vi["menu.settings.midi_device"] = "Thiết bị phát nhạc";
    vi["menu.settings.midi_ost"] = "Nhạc OST gốc (FLAC/MP3)";
    vi["menu.settings.midi_native"] = "Thiết bị MIDI hệ thống";
    vi["menu.settings.midi_embedded"] = "Bộ tổng hợp SoundFont tích hợp";
    vi["menu.settings.select_soundfont"] = "Chọn SoundFont tùy chỉnh (.sf2)...";
    vi["menu.settings.scale"] = "Tỷ lệ hiển thị";
    vi["menu.settings.scale_1x"] = "1× (768×550 gốc)";
    vi["menu.settings.scale_1_5x"] = "1.5× (1152×825)";
    vi["menu.settings.scale_2x"] = "2× (1536×1100)";
    vi["menu.settings.scale_3x"] = "3× (2304×1650)";
    vi["menu.settings.scale_free"] = "Tự do (Giữ đúng tỷ lệ)";
    vi["menu.settings.filter_nearest"] = "Sắc nét pixel (Nearest)";
    vi["menu.settings.filter_bilinear"] = "Làm mịn (Bilinear)";
    vi["menu.settings.theme"] = "Giao diện (Blender)";
    vi["menu.settings.theme_dark"] = "Blender Tối (Dark)";
    vi["menu.settings.theme_light"] = "Blender Sáng (Light)";
    vi["menu.settings.import_hst"] = "Nhập kỷ lục cũ (d4s.hst)...";
    vi["menu.settings.export_hst"] = "Xuất kỷ lục cũ (d4s.hst)...";
    vi["menu.settings.inspector"] = "Trình giám sát / Gợi ý";
    vi["menu.about"] = "Giới thiệu(&A)";
    vi["menu.about.explain"] = "Luật chơi(&E)";
    vi["menu.about.author"] = "Tác giả(&P)";
    vi["table.col.rank"] = "Hạng";
    vi["table.col.name"] = "Tên";
    vi["table.col.score"] = "Điểm";
    vi["table.prompt.name"] = "Tên người chơi";
    vi["table.btn.ok"] = "Đồng ý";
    vi["dialog.btn.close"] = "Đóng";
    vi["table.default_player"] = "Người chơi";
    vi["inspector.title"] = "Liên Liên Khán 2 - Trình giám sát (F12)";
    vi["inspector.mode"] = "Trạng thái";
    vi["inspector.mode.title"] = "Màn hình chờ";
    vi["inspector.mode.playing"] = "Đang chơi";
    vi["inspector.stage"] = "Màn";
    vi["inspector.tiles_left"] = "Ô còn lại";
    vi["inspector.lives"] = "Lượt đổi";
    vi["inspector.score"] = "Điểm";
    vi["inspector.time_left"] = "Thời gian";
    vi["inspector.audio_backend"] = "Nguồn âm thanh";
    vi["inspector.soundfont"] = "SoundFont";
    vi["inspector.track"] = "Bài nhạc";
    vi["inspector.playing"] = "Đang phát";
    vi["inspector.stopped"] = "Đã dừng";
    vi["inspector.next_hint"] = "Gợi ý cặp nối";
    vi["inspector.highlight_hint"] = "Đánh dấu cặp gợi ý trên bàn";
    vi["inspector.shift.0"] = "Cố định";
    vi["inspector.shift.1"] = "Dồn lên trên";
    vi["inspector.shift.2"] = "Dồn xuống dưới";
    vi["inspector.shift.3"] = "Dồn sang trái";
    vi["inspector.shift.4"] = "Dồn sang phải";
    vi["inspector.shift.5"] = "Tách trái / phải";
    vi["inspector.shift.6"] = "Gom trên / dưới";
    vi["rules.use_overlay"] = "1";
    vi["rules.turn_0"] = "Đường\nthẳng";
    vi["rules.turn_1"] = "1 góc\nvuông";
    vi["rules.turn_2"] = "2 góc\nvuông";
    vi["rules.body"] = "Chọn hai ô giống nhau sao cho đường nối tránh các ô khác không quá 2 góc gấp khúc (xem minh họa) để xóa cặp ô và ghi điểm.\n\nXóa hết ô trước khi hết giờ để qua màn. Khi hết nước đi, bàn tự xáo trộn. Ban đầu có 10 lượt đổi, qua mỗi màn cộng thêm 1 lượt. Điểm cao sẽ lưu vào bảng xếp hạng.";
    vi["rules.controls"] = "Chuột trái = Chọn\nChuột phải = Hủy";
    vi["msg.stage_clear.title"] = "Qua màn";
    vi["msg.stage_clear.body"] = "Qua màn thành công! Chuẩn bị vào màn {level}.\nNăng lượng thời gian còn lại thưởng {bonus} điểm.";
    vi["msg.all_clear.title"] = "Chúc mừng phá đảo!";
    vi["msg.all_clear.body"] = "Xuất sắc!! Bạn đã vượt qua tất cả các màn chơi.\nThời gian và lượt đổi còn lại thưởng {bonus} điểm.\nTổng điểm đạt được: {score} điểm.";
    vi["msg.no_moves.title"] = "Hết nước đi";
    vi["msg.no_moves.body"] = "Không còn cặp hình nào có thể nối. Trò chơi kết thúc.";
    vi["msg.timeout.title"] = "Hết giờ";
    vi["msg.timeout.body"] = "Năng lượng thời gian đã cạn. Trò chơi kết thúc.\nLượt đổi còn lại thưởng {bonus} điểm.\nTổng điểm đạt được: {score} điểm.";
    vi["msg.missing_assets.title"] = "Thiếu dữ liệu trò chơi";
    vi["msg.missing_assets.body"] = "Không tìm thấy thư mục tài nguyên (assets/).\nVui lòng trích xuất tài nguyên từ tệp gốc trước:\npython3 tools/extract_assets.py D4s.exe";
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
