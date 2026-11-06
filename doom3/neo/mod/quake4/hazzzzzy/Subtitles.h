// karin: Quake 4 简体中文汉化项目新增 —— HL 风格语音字幕
// 文本来源: rvDeclLipSync::GetTranscribeText()（经 GetLocalizedString 本地化）
#ifndef __GAME_SUBTITLES_H__
#define __GAME_SUBTITLES_H__

#define MAX_SUBTITLE_LINES	8

// 字幕配色类型：按说话人来源区分前景色（取代旧的 dimmed 二态淡色）
enum subColor_t {
	SUB_COLOR_NORMAL = 0,	// 角色对白 / 无名环境音 — 白
	SUB_COLOR_BROADCAST,	// Strogg广播(PA) — 黄
	SUB_COLOR_RADIO,		// 无线电 — 青
	SUB_COLOR_MAKRON,		// Makron — 紫
	SUB_COLOR_HUMANCAST,	// 人类/舰船广播 — 绿
	SUB_COLOR_ENEMY,		// 敌军语音 — 红
	SUB_COLOR_WEAPONMOD		// 武器升级 — 粉
};

class idSoundShader;

class rvSubtitles {
public:
						rvSubtitles( void );

	void				Clear( void );
	// speaker 可为 NULL；durationMs 为语音时长（毫秒）
	void				Add( const char *speaker, const char *text, int durationMs, subColor_t colorOverride = SUB_COLOR_NORMAL );
	// 从说话实体推断说话人名后加入；shader 非 NULL 时做可听性门控；
	// fallbackSpeaker：推断不出名字时的兜底前缀（如 speaker 实体传"广播"）
	void				AddFromEntity( idEntity *bodyEnt, const char *text, int durationMs, const idSoundShader *shader = NULL, const char *fallbackSpeaker = NULL );
	// 查声音 shader 名 → 说话人映射（speaker_map.txt）。返回非空=角色台词（用角色名），
	// 返回 NULL=环境音（fallback 到"广播"/"无线电"默认前缀+淡色）。供 Sound.cpp/Misc.cpp 挂钩调用
	static const char *	LookupSpeaker( const char *shaderName );
	// \xE5\xB0\x86\xE6\x96\x87\xE6\x9C\xAC\xE4\xB8\xAD\xE7\x9A\x84\xE8\x8B\xB1\xE6\x96\x87\xE4\xBA\xBA\xE5\x90\x8D\xE7\xBF\xBB\xE8\xAF\x91\xE4\xB8\xBA\xE4\xB8\xAD\xE6\x96\x87\xEF\xBC\x8C\xE5\x86\x9B\xE8\xA1\x94\xE8\xA7\x84\xE8\x8C\x83\xE5\x8C\x96
	// \xE4\xBE\x9B Player.cpp \xE5\x87\x86\xE5\xBF\x83\xE5\x90\x8D\xE5\xAD\x97\xE7\xAD\x89\xE5\x9C\xBA\xE6\x99\xAF\xE8\xB0\x83\xE7\x94\xA8\xEF\xBC\x8C\xE5\x8F\x97 harm_g_cnNames \xE6\x8E\xA7\xE5\x88\xB6
	static void			LocalizeText( idStr &text );
	// 每帧在 idGameLocal::Draw 尾部调用
	void				Draw( void );

private:
	void				AddLine( const char *text, int endTime, subColor_t color );
	// 说话点相对玩家是否可听（距离超出声音衰减半径 / 近距离外不在 PVS 内则不可听）
	bool				IsAudible( idEntity *bodyEnt, const idSoundShader *shader ) const;
	// 剥离 {emotion} 标记并折叠多余空格
	static void			Sanitize( idStr &text );

	struct subLine_t {
		idStr			text;
		int				startTime;
		int				endTime;
		subColor_t		color;		// 字幕配色类型(角色白/广播黄/无线电青)
	};

	subLine_t			lines[MAX_SUBTITLE_LINES];
	int					numLines;
	// 注意：不得缓存 gui 指针跨地图使用——换图时 uiManager 释放重建 GUI 实例，
	// 缓存指针悬空导致堆损坏概率崩溃（c0000409，2026-07-17 转储定位），
	// Draw 内每帧按名 FindGui
	float				smoothH;
	int					lastDrawTime;
};

extern rvSubtitles		gameSubtitles;
extern idCVar			harm_g_subtitleDebug;

// 主菜单设置页选分辨率/比例后由 GUI onActionRelease("harm_applyVideo")触发，
// 经 game->HandleMainMenuCommands 直接调用（不经控制台命令：主菜单时 Clear 未执行、
// 命令未注册）。解析 harm_g_resIndex → r_mode -1+r_customWidth/Height+vid_restart。
void				Harm_ApplyResolution( void );

#endif /* !__GAME_SUBTITLES_H__ */
