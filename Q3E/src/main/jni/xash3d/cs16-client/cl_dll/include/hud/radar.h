/*
radar.h
Copyright (C) 2015 a1batross
*/
#pragma once
#ifndef RADAR_H
#define RADAR_H

class CClientSprite;

class CHudRadar: public CHudBase
{
public:
	virtual int Init();
	virtual int VidInit();
	virtual int Draw( float flTime );
	virtual void InitHUDData();
	virtual void Reset();
	virtual void Shutdown();

	int MsgFunc_Radar(const char *pszName,  int iSize, void *pbuf);

	void UserCmd_ShowRadar();
	void UserCmd_HideRadar();
	CClientSprite m_hRadar;
	CClientSprite m_hRadarOpaque;

	int MsgFunc_BombDrop(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_BombPickup(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_HostagePos(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_HostageK(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_Location(const char *pszName, int iSize, void *pbuf);
private:

	cvar_t *cl_radartype;

	int InitBuiltinTextures();
	void DrawPlayerLocation(int y);
	void DrawRadarDot(int x, int y, int r, int g, int b, int a);
	void DrawCross(int x, int y, int r, int g, int b, int a );

	// Call DrawT, DrawFlippedT or DrawRadarDot considering z value
	inline void DrawZAxis( Vector pos, int r, int g, int b, int a );

	void DrawT( int x, int y, int r, int g, int b, int a );
	void DrawFlippedT( int x, int y, int r, int g, int b, int a );
	bool HostageFlashTime( float flTime, struct hostage_info_t *pplayer );
	bool FlashTime( float flTime, struct extra_player_info_t *pplayer );
	Vector WorldToRadar(const Vector vPlayerOrigin, const Vector vObjectOrigin, const Vector vAngles );
	inline void DrawColoredTexture( int x, int y, int size, byte r, byte g, byte b, byte a, int texHandle );

	bool bUseRenderAPI, bTexturesInitialized;
	int hCross, hT, hFlippedT;
	int iMaxRadius;
};

#endif // RADAR_H
