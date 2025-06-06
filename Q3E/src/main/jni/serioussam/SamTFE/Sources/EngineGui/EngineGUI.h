/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include <Engine/Engine.h>

#ifdef ENGINEGUI_EXPORTS

  #define ENGINEGUI_API __declspec(dllexport)

#else

  #define ENGINEGUI_API __declspec(dllimport)

  #ifdef NDEBUG
    #pragma comment(lib, "EngineGUI.lib")
  #else
    #pragma comment(lib, "EngineGUID.lib")
  #endif

#endif

class CEngineGUI
{
public:
  /* Functions used by application for getting and setting registry keys concerning modes */
  ENGINEGUI_API void GetFullScreenModeFromRegistry( CTString strSectionName, CDisplayMode &dm, GfxAPIType &gat);
  ENGINEGUI_API void SetFullScreenModeToRegistry(   CTString strSectionName, CDisplayMode  dm, GfxAPIType  gat);
  /* Call select mode dialog */
  ENGINEGUI_API void SelectMode( CDisplayMode &dm, GfxAPIType &gat);

  /* Call create texture dialog */
  ENGINEGUI_API CTFileName CreateTexture( CTFileName fnTexFileToRecreate = CTString(""),
                                          CDynamicArray<CTFileName> *pafnCreatedTextures=NULL);

/* Predefined registry key names */
#define KEY_NAME_REQUEST_FILE_DIR "Request file directory"
#define KEY_NAME_DETAIL_TEXTURE_DIR "Detail texture directory"
#define KEY_NAME_BASE_TEXTURE_DIR   "Base texture directory"
#define KEY_NAME_CREATE_TEXTURE_DIR "Create texture directory"
#define KEY_NAME_CREATE_ANIMATED_TEXTURE_DIR "Create animated texture directory"
#define KEY_NAME_CREATE_EFFECT_TEXTURE_DIR "Create effect texture directory"
#define KEY_NAME_BACKGROUND_TEXTURE_DIR "Background texture directory"
#define KEY_NAME_REPLACE_TEXTURE_DIR "Replace texture directory"
#define KEY_NAME_SCREEN_SHOT_DIR "Screen shots directory"

/* Predefined file filters for file requester */
#define FILTER_ALL            "All files (*.*)\0*.*\0"
#define FILTER_PICTURES       "Pictures (*.pcx;*.tga)\0*.pcx;*.tga\0"
#define FILTER_3DOBJ          "3D object\0*.lwo;*.obj;*.3ds\0"
#define FILTER_LWO            "Lightwave object (*.lwo)\0*.lwo\0"
#define FILTER_OBJ            "Alias Wavefront Object (*.obj)\0*.obj\0"
#define FILTER_3DS            "3DS object (*.3ds)\0*.3ds\0"
#define FILTER_SCR            "Scripts (*.scr)\0*.scr\0"
#define FILTER_PCX            "PCX files (*.pcx)\0*.pcx\0"
#define FILTER_TGA            "TGA files (*.tga)\0*.tga\0"
#define FILTER_TEX            "Textures (*.tex)\0*.tex\0"
#define FILTER_TBN            "Thumbnails (*.tbn)\0*.tbn\0"
#define FILTER_MDL            "Models (*.mdl)\0*.mdl\0"
#define FILTER_WLD            "Worlds (*.wld)\0*.wld\0"
#define FILTER_WAV            "Sounds (*.wav)\0*.wav\0"
#define FILTER_ANI            "Animations (*.ani)\0*.ani\0"
#define FILTER_TXT            "Text files (*.txt)\0*.txt\0"
#define FILTER_LST            "List files (*.lst)\0*.lst\0"
#define FILTER_TGA            "TGA files (*.tga)\0*.tga\0"
#define FILTER_SMC            "SMC files (*.smc)\0*.smc\0"
#define FILTER_END            "\0"
  /* File requester with thumbnail display */
  ENGINEGUI_API CTFileName FileRequester( char *pchrTitle="Choose file",
                            char *pchrFilters=FILTER_ALL FILTER_END,
                            char *pchrRegistry=KEY_NAME_REQUEST_FILE_DIR,
                            CTString strDefaultDir="", CTString strFileSelectedByDefault="",
                            CDynamicArray<CTFileName> *pafnCreatedTextures=NULL,
                            BOOL bIfOpen=TRUE);

  /* Call browse texture requester */
  ENGINEGUI_API CTFileName BrowseTexture(CTFileName fnDefaultSelected=CTString(""),
    char *pchrIniKeyName=KEY_NAME_REQUEST_FILE_DIR,
    char *pchrWindowTitle="Choose texture",
    BOOL bIfOpen=TRUE);
};

// global engine gui handling object
ENGINEGUI_API extern CEngineGUI _EngineGUI;
