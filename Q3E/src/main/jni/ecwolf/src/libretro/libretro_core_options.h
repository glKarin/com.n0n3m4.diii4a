#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include "libretro.h"

/*
 ********************************
 * VERSION: 1.3
 ********************************
 *
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition */

#define BOOL_OPTIONS                  \
	{			      \
		{ "disabled", NULL},  \
		{ "enabled",  NULL},  \
		{ NULL,       NULL }, \
	}

#define SLIDER_OPTIONS		 \
	{			 \
		{ "Off", NULL }, \
		{ "1", NULL },   \
		{ "2", NULL },   \
		{ "3", NULL },   \
		{ "4", NULL },   \
		{ "5", NULL },   \
		{ "6", NULL },   \
		{ "7", NULL },   \
		{ "8", NULL },   \
		{ "9", NULL },   \
		{ "10", NULL },	 \
		{ "11", NULL },  \
		{ "12", NULL },  \
		{ "13", NULL },  \
		{ "14", NULL },  \
		{ "15", NULL },	 \
		{ "16", NULL },  \
		{ "17", NULL },  \
		{ "18", NULL },  \
		{ "19", NULL },  \
		{ "20", NULL },  \
		{ NULL, NULL },  \
  }

struct retro_core_option_definition option_defs_us[] = {
	{
		"ecwolf-resolution",
		"Internal resolution",
		"Configure the resolution.",
		{
		        { "240x160", NULL },
			{ "320x200", NULL },
			{ "320x240", NULL },
			{ "400x240", NULL },
			{ "420x240", NULL }, // third of 720p
			{ "480x270", NULL }, // psvita half-resolution
			{ "640x360", NULL }, // half of 720p
			{ "640x400", NULL },
			{ "640x480", NULL },
			{ "800x500", NULL },
			{ "960x540", NULL }, // psvita resolution
			{ "960x600", NULL },
			{ "1024x768", NULL },
			{ "1280x720", NULL }, // 720p
			{ "1280x800", NULL },
			{ "1600x1000", NULL },
			{ "1920x1200", NULL },
			{ "2240x1400", NULL },
			{ "2560x1600", NULL },
			{ NULL, NULL },
		},
#if defined(RS90)
		"240x160",
#elif defined(VITA) || defined(PSP)
		"480x270",
#elif defined (_3DS)
		"400x240",
#else
		"320x200",
#endif
	},
	{
		"ecwolf-analog-deadzone",
		"Analog deadzone",
		"Configure the deadzone for analog axis.",
		{
			{ "0%",  NULL },
			{ "5%",  NULL },
			{ "10%",  NULL },
			{ "15%",  NULL },
			{ "20%", NULL },
			{ "25%", NULL },
			{ "30%", NULL },
			{ NULL, NULL },
		},
		"15%"
	},
	{
		"ecwolf-fps",
		"Refresh rate (FPS)",
		"Configure the FPS.",
		{
			{ "7", NULL },
			{ "7.8", NULL },
			{ "8.8", NULL },
			{ "10", NULL },
			{ "14", NULL },
			{ "17.5", NULL },
			{ "25", NULL },
			{ "30", NULL },
			{ "35", NULL },
			{ "50", NULL },
			{ "60", NULL },
			{ "70", NULL },
			{ "72", NULL },
			{ "75", NULL },
			{ "90", NULL },
			{ "100", NULL },
			{ "119", NULL },
			{ "120", NULL },
			{ "140", NULL },
			{ "144", NULL },
			{ "240", NULL },
			{ "244", NULL },
			{ "300", NULL },
			{ "360", NULL },
			{ NULL, NULL },
		},
#if defined(PSP) || defined(RS90)
		"17.5",
#else
		"35",
#endif
	},
	{
		"ecwolf-palette",
		"Preferred palette format (Restart)",
		"Actual format is subject to hardware support.",
		{
			{ "rgb565",           "RGB565 (16-bit)"},
			{ "xrgb8888",         "XRGB8888 (24-bit)"},
			{ NULL, NULL },
		},
		"rgb565"
	},
	{
		"ecwolf-alwaysrun",
		"Always run",
		"Run by default. Use RUN button to walk.",
		BOOL_OPTIONS,
		"disabled"
	},
	{
		"ecwolf-viewsize",
		"Screen size",
		NULL,
		{
			{ "4",              "Smallest"},
			{ "5",              "5"},
			{ "6",              "6"},
			{ "7",              "7"},
			{ "8",              "8"},
			{ "9",              "9"},
			{ "10",            "10"},
			{ "11",            "11"},
			{ "12",            "12"},
			{ "13",            "13"},
			{ "14",            "14"},
			{ "15",            "15"},
			{ "16",            "16"},
			{ "17",            "17"},
			{ "18",            "18"},
			{ "19",            "19"},
			{ "20",            "Largest with statusbar"},
			{ "21",            "Without statusbar"},
			{ NULL, NULL },
		},
		"20"
	},
	{
		"ecwolf-am-overlay",
		"Show map as overlay",
		"Draw automap on top of the game.",
		{
			{ "off",              "Off"},
			{ "on",               "On"},
			{ "both",             "On + Normal"},
			{ NULL, NULL },
		},
		"off"
	},
	{
		"ecwolf-am-rotate",
		"Rotate map",
		"Rotate map to match looking direction.",
		{
			{ "off",              "Off"},
			{ "on",               "On"},
			{ "overlay_only",     "Overlay only"},
			{ NULL, NULL },
		},
		"off"
	},
	{
		"ecwolf-am-drawtexturedwalls",
		"Textured walls in automap",
		"Draw textured walls in automap.",
		BOOL_OPTIONS,
		"enabled"
	},
	{
		"ecwolf-am-drawtexturedfloors",
		"Textured floors in automap",
		"Draw textured floors in automap.",
		BOOL_OPTIONS,
		"disabled"
	},
	{
		"ecwolf-am-texturedoverlay",
		"Textured Overlay in automap",
		"Draw textures in overlay automap.",
		BOOL_OPTIONS,
		"disabled"
	},
	{
		"ecwolf-am-showratios",
		"Show level ratios in automap",
		"Show level ratios in automap.",
		BOOL_OPTIONS,
		"disabled"
	},
	{
		"ecwolf-am-pause",
		"Pause game in automap",
		"Pause game when in automap.",
		BOOL_OPTIONS,
		"enabled"
	},
#ifndef DISABLE_ADLIB
	{
		"ecwolf-music-volume",
		"Volume of music",
		"Volume of music from Off to 20",
		SLIDER_OPTIONS,
		"20"
	},
#endif
	{
		"ecwolf-digi-volume",
		"Volume of digitized sound effects",
		"Volume of digitized sound effects from Off to 20",
		SLIDER_OPTIONS,
		"20"
	},
#ifndef DISABLE_ADLIB
	{
		"ecwolf-adlib-volume",
		"Volume of Adlib sound effects",
		"Volume of Adlib sound effects from Off to 20",
		SLIDER_OPTIONS,
		"20"
	},
#endif
	{
		"ecwolf-speaker-volume",
		"Volume of Speaker sound effects",
		"Volume of Speaker sound effects from Off to 20",
		SLIDER_OPTIONS,
		"20"
	},
	{
		"ecwolf-analog-move-sensitivity",
		"Analog move and strafe sensitivity",
		"Sensitivity of move with analog stick from Off to 20",
		SLIDER_OPTIONS,
		"20"
	},
	{
		"ecwolf-analog-turn-sensitivity",
		"Analog turn sensitivity",
		"Sensitivity of turn with analog stick from Off to 20",
		SLIDER_OPTIONS,
		"20"
	},
	{
		"ecwolf-effects-priority",
		"Order of lookup for effects",
		"Which variants of assets are used in priority",
		{
#ifndef DISABLE_ADLIB
			{ "digi-adlib-speaker", "Digitized, Adlib, Speaker" },
			{ "digi-adlib", "Digitized, Adlib" },
#endif
			{ "digi-speaker", "Digitized, Speaker" },
			{ "digi", "Digitized only" },
#ifndef DISABLE_ADLIB
			{ "adlib", "Adlib only" },
#endif
			{ "speaker", "Speaker only" },
			{ NULL, NULL },
		},
#ifndef DISABLE_ADLIB
		"digi-adlib-speaker",
#else
		"digi-speaker",
#endif
	},
	{
		"ecwolf-aspect",
		"Aspect",
		"Force a video aspect",
		{
			{ "auto", "Auto" },
			{ "16:9", NULL },
			{ "4:3", NULL },
			{ "16:10", NULL },
			{ "17:10", NULL },
			{ "5:4", NULL },
			{ "21:9", NULL },
			{ NULL, NULL },
		},
		"auto",
	},
	{
		"ecwolf-invulnerability",
		"Invulnerability",
		"Enable invulnerability (cheat).",
		BOOL_OPTIONS,
		"disabled"
	},
	{
		"ecwolf-dynamic-fps",
		"Dynamic FPS",
		"Try to adjust FPS automatically",
		BOOL_OPTIONS,
		"disabled"
	},
#if !defined(_3DS) && !defined(GEKKO) && !defined(RS90)
	{
		"ecwolf-memstore",
		"Store files in memory",
		"Increases speed at cost of memory and initial load times",
		BOOL_OPTIONS,
		"disabled"
	},
#endif
	{
		"ecwolf-preload-digisounds",
		"Preload digitized sounds",
		"Increases speed at cost of memory and initial load times",
		BOOL_OPTIONS,
		"enabled"
	},
	{
		"ecwolf-panx-adjustment",
		"Horizontal panning speed in automap",
		"Multiplier for horizontal panning from 0 to 20",
		SLIDER_OPTIONS,
		"5"
	},
	{
		"ecwolf-pany-adjustment",
		"Vertical panning speed in automap",
		"Multiplier for vertical panning from 0 to 20",
		SLIDER_OPTIONS,
		"5"
	},
	{ NULL, NULL, NULL, {{0}}, NULL },
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_option_definition *option_defs_intl[RETRO_LANGUAGE_LAST] = {
   option_defs_us, /* RETRO_LANGUAGE_ENGLISH */
   NULL,           /* RETRO_LANGUAGE_JAPANESE */
   NULL,           /* RETRO_LANGUAGE_FRENCH */
   NULL,           /* RETRO_LANGUAGE_SPANISH */
   NULL,           /* RETRO_LANGUAGE_GERMAN */
   NULL,           /* RETRO_LANGUAGE_ITALIAN */
   NULL,           /* RETRO_LANGUAGE_DUTCH */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,           /* RETRO_LANGUAGE_RUSSIAN */
   NULL,           /* RETRO_LANGUAGE_KOREAN */
   NULL,           /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,           /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,           /* RETRO_LANGUAGE_ESPERANTO */
   NULL,           /* RETRO_LANGUAGE_POLISH */
   NULL,           /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,           /* RETRO_LANGUAGE_ARABIC */
   NULL,           /* RETRO_LANGUAGE_GREEK */
   NULL,           /* RETRO_LANGUAGE_TURKISH */
};
#endif

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should be called as early as possible - ideally inside
 * retro_set_environment(), and no later than retro_load_game()
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static void libretro_set_core_options(retro_environment_t environ_cb)
{
   unsigned version = 0;

   if (!environ_cb)
      return;

   if (environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && (version >= 1))
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_intl core_options_intl;
      unsigned language = 0;

      core_options_intl.us    = option_defs_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = option_defs_intl[language];

      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_intl);
#else
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, &option_defs_us);
#endif
   }
   else
   {
      size_t i;
      size_t num_options               = 0;
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine number of options */
      for (;;)
      {
         if (!option_defs_us[num_options].key)
            break;
         num_options++;
      }

      /* Allocate arrays */
      variables  = (struct retro_variable *)calloc(num_options + 1, sizeof(struct retro_variable));
      values_buf = (char **)calloc(num_options, sizeof(char *));

      if (!variables || !values_buf)
         goto error;

      /* Copy parameters from option_defs_us array */
      for (i = 0; i < num_options; i++)
      {
         const char *key                        = option_defs_us[i].key;
         const char *desc                       = option_defs_us[i].desc;
         const char *default_value              = option_defs_us[i].default_value;
         struct retro_core_option_value *values = option_defs_us[i].values;
         size_t buf_len                         = 3;
         size_t default_index                   = 0;

         values_buf[i] = NULL;

         if (desc)
         {
            size_t num_values = 0;

            /* Determine number of values */
            for (;;)
            {
               if (!values[num_values].value)
                  break;

               /* Check if this is the default value */
               if (default_value)
                  if (strcmp(values[num_values].value, default_value) == 0)
                     default_index = num_values;

               buf_len += strlen(values[num_values].value);
               num_values++;
            }

            /* Build values string */
            if (num_values > 0)
            {
               size_t j;

               buf_len += num_values - 1;
               buf_len += strlen(desc);

               values_buf[i] = (char *)calloc(buf_len, sizeof(char));
               if (!values_buf[i])
                  goto error;

               strcpy(values_buf[i], desc);
               strcat(values_buf[i], "; ");

               /* Default value goes first */
               strcat(values_buf[i], values[default_index].value);

               /* Add remaining values */
               for (j = 0; j < num_values; j++)
               {
                  if (j != default_index)
                  {
                     strcat(values_buf[i], "|");
                     strcat(values_buf[i], values[j].value);
                  }
               }
            }
         }

         variables[i].key   = key;
         variables[i].value = values_buf[i];
      }

      /* Set variables */
      environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);

error:

      /* Clean up */
      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
