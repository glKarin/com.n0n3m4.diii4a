/*
android_nosdl.c - android backend
Copyright (C) 2016-2019 mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#include "platform/platform.h"
#include "input.h"
#include "client.h"
#include "sound.h"
#include "errno.h"
#include <pthread.h>
#include <sys/prctl.h>

#include <android/log.h>
#include <jni.h>
#if XASH_SDL
#include <SDL.h>
#endif // XASH_SDL

#ifdef _DIII4A //karin: default fake Android ID
#define ANDROID_ID "idTech4Amm"
#define ANDROID_ID_FILE "xash3d.idTech4Amm.aid"
extern const char * Sys_GameDataDefaultPath();
#endif

struct jnimethods_s
{
	JNIEnv *env;
	jobject activity;
	jclass actcls;
	jmethodID loadAndroidID;
	jmethodID getAndroidID;
	jmethodID saveAndroidID;
} jni;

void Android_Init( void )
{
	memset( &jni, 0, sizeof( jni ));

#if XASH_SDL
#if !defined(_DIII4A) //karin: unuse any JNI
	jni.env = (JNIEnv *)SDL_AndroidGetJNIEnv();
	jni.activity = (jobject)SDL_AndroidGetActivity();
	jni.actcls = (*jni.env)->GetObjectClass( jni.env, jni.activity );
	jni.loadAndroidID = (*jni.env)->GetMethodID( jni.env, jni.actcls, "loadAndroidID", "()Ljava/lang/String;" );
	jni.getAndroidID = (*jni.env)->GetMethodID( jni.env, jni.actcls, "getAndroidID", "()Ljava/lang/String;" );
	jni.saveAndroidID = (*jni.env)->GetMethodID( jni.env, jni.actcls, "saveAndroidID", "(Ljava/lang/String;)V" );
#endif

	SDL_SetHint( SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight" );
	SDL_SetHint( SDL_HINT_JOYSTICK_HIDAPI_STEAM, "1" );
	SDL_SetHint( SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "0" );
	SDL_SetHint( SDL_HINT_ANDROID_BLOCK_ON_PAUSE_PAUSEAUDIO, "0" );
	SDL_SetHint( SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1" );
#endif // !XASH_SDL
}

/*
========================
Android_GetNativeObject
========================
*/

void *Android_GetNativeObject( const char *name )
{
	if( !strcasecmp( name, "JNIEnv" ) )
	{
		return (void *)jni.env;
	}
	else if( !strcasecmp( name, "ActivityClass" ) )
	{
		return (void *)jni.actcls;
	}

	return NULL;
}

/*
========================
Android_GetAndroidID
========================
*/
const char *Android_GetAndroidID( void )
{
#ifdef _DIII4A //karin: return fake Android ID
	printf("Android_GetAndroidID() -> %s\n", ANDROID_ID);
	return ANDROID_ID;
#else
	static char id[32];
	jstring resultJNIStr;
	const char *resultCStr;

	if( COM_CheckString( id ) ) return id;

	resultJNIStr = (*jni.env)->CallObjectMethod( jni.env, jni.activity, jni.getAndroidID );
	resultCStr = (*jni.env)->GetStringUTFChars( jni.env, resultJNIStr, NULL );
	Q_strncpy( id, resultCStr, sizeof( id ) );
	(*jni.env)->ReleaseStringUTFChars( jni.env, resultJNIStr, resultCStr );
	(*jni.env)->DeleteLocalRef( jni.env, resultJNIStr );

	return id;
#endif
}

/*
========================
Android_LoadID
========================
*/
const char *Android_LoadID( void )
{
#ifdef _DIII4A //karin: load from cwd
	char path[MAX_OSPATH] = { 0 };
	Q_snprintf(path, sizeof(path), "%s/" ANDROID_ID_FILE, Sys_GameDataDefaultPath());
	FILE *file = fopen(path, "r");
	if(!file)
		return ANDROID_ID;
	static char id[32];
	int i = 0;
	while(!feof(file))
	{
		int ch = fgetc(file);
		if(ch < 0)
			break;
		id[i++] = ch;
	}
	id[i] = '\0';
	fclose(file);
	printf("Android_LoadID() -> %s at %s\n", id, path);
	return id;
#else
	static char id[32];
	jstring resultJNIStr;
	const char *resultCStr;

	resultJNIStr = (*jni.env)->CallObjectMethod( jni.env, jni.activity, jni.loadAndroidID );
	resultCStr = (*jni.env)->GetStringUTFChars( jni.env, resultJNIStr, NULL );
	Q_strncpy( id, resultCStr, sizeof( id ) );
	(*jni.env)->ReleaseStringUTFChars( jni.env, resultJNIStr, resultCStr );
	(*jni.env)->DeleteLocalRef( jni.env, resultJNIStr );

	return id;
#endif
}

/*
========================
Android_SaveID
========================
*/
void Android_SaveID( const char *id )
{
#ifdef _DIII4A //karin: save to cwd
	char path[MAX_OSPATH] = { 0 };
	Q_snprintf(path, sizeof(path), "%s/" ANDROID_ID_FILE, Sys_GameDataDefaultPath());
	FILE *file = fopen(path, "w");
	fwrite(id, 1, strlen(id), file);
	fflush(file);
	fclose(file);
	printf("Android_SaveID() -> %s to %s\n", id, path);
#else
	jstring JStr = (*jni.env)->NewStringUTF( jni.env, id );
	(*jni.env)->CallVoidMethod( jni.env, jni.activity, jni.saveAndroidID, JStr );
	(*jni.env)->DeleteLocalRef( jni.env, JStr );
#endif
}

/*
========================
Android_ShellExecute
========================
*/
void Platform_ShellExecute( const char *path, const char *parms )
{
#if XASH_SDL
	SDL_OpenURL( path );
#endif // XASH_SDL
}
