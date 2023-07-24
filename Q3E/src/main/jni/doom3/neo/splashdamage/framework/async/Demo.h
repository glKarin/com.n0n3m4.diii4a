// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DEMO_H__
#define __DEMO_H__

//#include "../../sys/sys_local.h"
#include "../UsercmdGen.h"

//===============================================================
//
//	sdDemo
//
//===============================================================

#define DEMO_IDENT				(('O'<<24)+('M'<<16)+('D'<<8)+'N')
#define DEMO_VERSION			3
#define JUMPSTART_IDENT			(('O'<<24)+('D'<<16)+('S'<<8)+'J')
#define JUMPSTART_VERSION		1
#define KEYFRAMEFILE_IDENT		(('F'<<24)+('F'<<16)+('K'<<8)+'D')
#define KEYFRAMEFILE_VERSION	1

class sdFrameBuffer;

class sdDemo {
public:
	typedef enum demoState_e {
		DS_IDLE,
		DS_RECORDING,
		DS_PLAYING
	} demoState_t;

	static const int TIMEDEMO_FRAME_MSEC;
	static const int KEYFRAME_INTERVAL;

						sdDemo();

	void				Record( const char* fileName );
	void				Stop();
	void				Play( const char* fileName, bool timeDemo = false );
	void				PlayJumpStart( const char* fileName );

	static bool			CheckVersion( const char* fileName );

	demoState_t			GetState() const { return state; }

	bool				IsRecording() const;
	bool				IsPlaying() const;
	bool				IsPaused() const;
	bool				IsTimeDemo() const;
	bool				IsRenderDemo() const;
	bool				IsGeneratingKeyFrames() const;
	bool				IsCuttingDemo() const;

	void				RecordMessage( const int time, const idBitMsg& msg, int clientPrediction );
	bool				PlayMessage( const int clientTime, void* data, int& size, const int maxSize, const int timeOut, int& clientPrediction );

	void				RecordUsercmd( const usercmd_t& usercmd );
	void				ReadUsercmd( int gameFrame );

	void				WriteUsercmd( idFile* file, const usercmd_t& usercmd );
	bool				ReadUsercmd( idFile* file, usercmd_t& usercmd );

	void				WriteKeyFrame( const int clientTime );

	void				RunFrame();

						// silent stop called from idSessionLocal::Stop, not the same as the Stop() command
	void				SessionStop();
	void				Shutdown();

	void				IncrementDemoFrame() { numDemoFrames++; }
	void				CaptureRenderDemoFrame( const int gameTime );

	const char*			GetName() const { return fileName.c_str(); }
	int					GetRenderDemoFPS() const { return renderDemoFPS; }

	int					GetStartGameTime() const;
	int					GetPosition() const;
	int					GetLength() const;
	int					GetStartPosition() const;
	int					GetEndPosition() const;
	int					GetCutStartMarker() const;
	int					GetCutEndMarker() const;
	usercmd_t			GetCachedUsercmd( void ) const { return usercmd; }

	void				ForceMega();

	static void			DemoFileName( const char* base, idStr& fileName );

	static void			Toggle_f( const idCmdArgs& args );
	static void			Record_f( const idCmdArgs& args );
	static void			Stop_f( const idCmdArgs& args );	
	static void			Play_f( const idCmdArgs& args );
	static void			PlayJumpStartDemo_f( const idCmdArgs& args );
	static void			Time_f( const idCmdArgs& args );
	static void			Render_f( const idCmdArgs& args );
	static void			GenerateKeyFrames_f( const idCmdArgs& args );
	static void			NextKeyFrame_f( const idCmdArgs& args );
	static void			PrevKeyFrame_f( const idCmdArgs& args );
	static void			Pause_f( const idCmdArgs& args );
	static void			CutPlaceStartMarker_f( const idCmdArgs& args );
	static void			CutPlaceEndMarker_f( const idCmdArgs& args );
	static void			Cut_f( const idCmdArgs& args );
	static void			WriteJumpStartDemo_f( const idCmdArgs& args );

public:
	static idCVar		debug;
	static idCVar		scale;
	static idCVar		snapshotDelay;
	static idCVar		prediction;
	static idCVar		forceMega;

private:
	void				Clear();

	void				DoRenderDemo( const int fps, const int startTime, const int endTime );
	void				TogglePause();
	void				WriteJumpStartDemo( const char* jumpStartFileName );

	void				WriteKeyFrameFile();
	void				ReadKeyFrames();
	void				PlayFromKeyFrame( const int keyFrame );
	void				StepKeyFrame( bool reverse );

	void				Cut( const char* fileName, const char* cutFileName );
	void				WriteCut();
	void				SetCutStartMarker();
	void				SetCutEndMarker();

	void				WriteHeader( idFile* file );
	int					ReadHeader( idFile* file, const idStr& headerFileName );

	void				WriteNetworkHeader( idFile* file );
	void				ReadNetworkHeader( idFile* file, bool keyFrame = false );

	void				WriteDict( idFile* file, const idDict* dict );
	void				ReadDict( idFile* file, idDict* dict );

	void				WriteByte( idFile* file, const byte value );
	void				WriteSignedChar( idFile* file, const signed char value );

	void				ReadByte( idFile* file, byte& value );
	void				ReadSignedChar( idFile* file, signed char& value );

	static bool			CheckVersion( idFile* file, const char* headerFileName, bool verbose );

private:
    demoState_t			state;

	idStr				fileName;
	idFile*				file;

	int					networkStreamOffset;

	int					usercmdOffset;	// position in file for the ucmd start position record
	int					usercmdStart;	// position in file of the ucmd stream, written at ucmd_offset in file
	idFile*				usercmdFile;
	usercmd_t			usercmd;

	int					startTime;
	int					startGameTime;
	int					nextMessageTime;
	bool				paused;

	int					numDemoFrames;

	bool				timeDemo;
	bool				renderDemo;
	bool				generatingKeyFrames;
	bool				cuttingDemo;

	idDict				userinfo;		// backup userinfo to set it to what it was after demo replay

	// demo rendering
	sdFrameBuffer*		renderDemoFrameBuffer;
	int					renderDemoFPS;
	int					renderStartTime;
	int					renderStopTime;
	int					renderStartFrame;
	int					renderEndFrame;

	// key frames
	struct keyFrame_t {
		int offset;
		int clientTime;
	};

	idFile*				keyFrameFile;
	int					lastKeyFrameTime;
	idList< keyFrame_t>	keyFrames;

	// demo cutting
	idStr				cutFileName;
	int					cutStartMarker;
	int					cutEndMarker;
};

ID_INLINE bool sdDemo::IsRecording() const {
	return state == DS_RECORDING;
}

ID_INLINE bool sdDemo::IsPlaying() const {
	return state == DS_PLAYING;
}

ID_INLINE bool sdDemo::IsPaused() const {
	return paused;
}

ID_INLINE bool sdDemo::IsTimeDemo() const {
	return timeDemo;
}

ID_INLINE bool sdDemo::IsRenderDemo() const {
	return renderDemo;
}

ID_INLINE bool sdDemo::IsGeneratingKeyFrames() const {
	return generatingKeyFrames;
}

ID_INLINE bool sdDemo::IsCuttingDemo() const {
	return cuttingDemo;
}

ID_INLINE int sdDemo::GetStartGameTime() const {
	return startGameTime;
}

ID_INLINE int sdDemo::GetPosition() const {
	return ( file != NULL ? file->Tell() : -1 );
}

ID_INLINE int sdDemo::GetLength() const {
	return ( file != NULL ? file->Length() : -1 );
}

ID_INLINE int sdDemo::GetStartPosition() const {
	return networkStreamOffset;
}

ID_INLINE int sdDemo::GetEndPosition() const {
	return ( file != NULL ? file->Length() - usercmdStart : -1 ) + networkStreamOffset;
}

ID_INLINE int sdDemo::GetCutStartMarker() const {
	return cutStartMarker;
}

ID_INLINE int sdDemo::GetCutEndMarker() const {
	return cutEndMarker;
}

#endif /* !__DEMO_H__ */
