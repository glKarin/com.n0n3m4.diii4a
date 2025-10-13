REM Quite clearly this is a stupid way to do this.
REM But it keeps it all in one file, so that's always fun.
REM To use: Stick in the q3 quake3-1.32b/code/botlib directory.
REM Install gcc as either cygwin or mingw
REM Double click it your batch file.
REM Copy the resultant botlib.dll to your quake directory.
REM Run FTE in Q3 with bots.

REM note that botlib doesn't work the same as other addons, and must be compiled to native.
REM it could potentially be compiled in, but that results in a lot of conflicts and much bloat when the engine is primarily targeted for q1.
REM botlib works as a dll, and FTE links dynamically. botlib calls fail without the dll.
REM The bots_enabled q3 cvar is readonly and forced to 0 without the dll.
REM The primary reason for the library as a dll is to stop the damn thing from crashing on cached memory references on map changes.
REM FTE likes freeing memory, while q3 reuses it. Botlib has some really unhealthy reads. The only ways around that I could find were rewriting botlib or closing the dll between maps. DLLs help combat q1 bloat though, and requires less maintainence in botlib.


echo off
echo. >standalone.c

echo #include "../game/q_shared.h" >>standalone.c

echo void Com_Memset (void* dest, const int val, const size_t count) >>standalone.c
echo { >>standalone.c
echo 	memset(dest, val, count); >>standalone.c
echo } >>standalone.c
echo void Com_Memcpy (void* dest, const void* src, const size_t count) >>standalone.c
echo { >>standalone.c
echo 	memcpy(dest, src, count); >>standalone.c
echo } >>standalone.c

echo void	QDECL Com_Error( int level, const char *error, ... ) >>standalone.c
echo { >>standalone.c
echo 	exit(0); >>standalone.c
echo } >>standalone.c

gcc -mno-cygwin *.c ../game/q_shared.c ../game/q_math.c -DBOTLIB -D__LCC__ -shared -o botlib.dll -DCom_Printf=printf