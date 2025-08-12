package com.n0n3m4.q3e;

import android.content.Context;

import java.util.ArrayList;
import java.util.List;

public class Q3EPatchResourceManager
{
    private Context context;
    private final List<Q3EPatchResource> resourceList = new ArrayList<>();

    public Q3EPatchResourceManager(Context context)
    {
        SetContext(context);
        Init();
    }

    public void SetContext(Context context)
    {
        if(null == context)
            throw new RuntimeException("Q3EPatchResourceManager::Context is null");
        this.context = context;
    }

    public List<Q3EPatchResource> ResourceList()
    {
        return resourceList;
    }

    private void Init()
    {
        Q3EPatchResource rsc;

        rsc = new Q3EPatchResource_fileToFile(
                Q3EGameConstants.PatchResource.QUAKE4_SABOT,
                Q3ELang.tr(context, R.string.bot_q3_bot_support_in_mp_game),
                "1",
                Q3EGameConstants.GAME_QUAKE4,
                null,
                "pak/q4base/q4_sabot_a9.pk4",
                null,
                "zzz_idTech4Amm_"
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_fileToFile(
                Q3EGameConstants.PatchResource.DOOM3_SABOT,
                Q3ELang.tr(context, R.string.doom3_bot_sabot_a7_mod),
                "1",
                Q3EGameConstants.GAME_DOOM3,
                null,
                "pak/doom3/d3_sabot_a7.pk4",
                null,
                "zzz_idTech4Amm_"
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_fileToFile(
                Q3EGameConstants.PatchResource.DOOM3_RIVENSIN_ORIGIANL_LEVELS,
                Q3ELang.tr(context, R.string.rivensin_play_original_doom3_level),
                "1",
                Q3EGameConstants.GAME_DOOM3,
                "rivensin",
                "pak/rivensin/play_original_doom3_level.pk4",
                "",
                "zzz_idTech4Amm_"
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_zipToDir(
                Q3EGameConstants.PatchResource.DOOM3BFG_HLSL_SHADER,
                Q3ELang.tr(context, R.string.rbdoom3_bfg_hlsl_shader),
                Q3EGameConstants.RBDOOM3BFG_HLSL_SHADER_VERSION,
                Q3EGameConstants.GAME_DOOM3BFG,
                null,
                "pak/doom3bfg/renderprogs.pk4",
                "base"
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_zipToDir(
                Q3EGameConstants.PatchResource.TDM_GLSL_SHADER,
                Q3ELang.tr(context, R.string.the_dark_mod_glsl_shader) + "(" + Q3EGameConstants.GAME_VERSION_TDM + ")",
                Q3EGameConstants.TDM_GLSL_SHADER_VERSION,
                Q3EGameConstants.GAME_TDM,
                null,
                "pak/darkmod/glprogs.pk4",
                ""
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_dirToDir(
                Q3EGameConstants.PatchResource.GZDOOM_RESOURCE,
                Q3ELang.tr(context, R.string.gzdoom_builtin_resource) + "(" + Q3EGameConstants.GAME_VERSION_GZDOOM + ")",
                Q3EGameConstants.GZDOOM_VERSION,
                Q3EGameConstants.GAME_GZDOOM,
                null,
                "pak/gzdoom/" + Q3EGameConstants.GAME_VERSION_GZDOOM,
                ""
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_fileToDir(
                Q3EGameConstants.PatchResource.DOOM3_BFG_CHINESE_TRANSLATION,
                Q3ELang.tr(context, R.string.doom3_chinese_translation_doom3bfg),
                "1",
                Q3EGameConstants.GAME_DOOM3,
                null,
                "pak/doom3/doom3_chinese_translation_doom3bfg.pk4",
                "base"
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_zipToZip(
                Q3EGameConstants.PatchResource.QUAKE4_SABOT,
                Q3ELang.tr(context, R.string.bot_q3_bot_support_in_mp_game) + " " + Q3ELang.tr(context, R.string.mp_game_map_aas),
                "1",
                Q3EGameConstants.GAME_QUAKE4,
                null,
                "pak/q4base/q4_sabot_a9.pk4",
                null,
                "zzz_idTech4Amm_q4_sabot_a9_aas.pk4",
                "maps/"
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_zipToZip(
                Q3EGameConstants.PatchResource.DOOM3_SABOT,
                Q3ELang.tr(context, R.string.doom3_bot_sabot_a7_mod) + " " + Q3ELang.tr(context, R.string.mp_game_map_aas),
                "1",
                Q3EGameConstants.GAME_DOOM3,
                null,
                "pak/doom3/d3_sabot_a7.pk4",
                null,
                "zzz_idTech4Amm_d3_sabot_a7_aas.pk4",
                "maps/"
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_fileToDir(
                Q3EGameConstants.PatchResource.XASH3D_EXTRAS,
                Q3ELang.tr(context, R.string.xash3d_extras),
                Q3EGameConstants.XASH3D_VERSION,
                Q3EGameConstants.GAME_XASH3D,
                null,
                "pak/xash3d/extras.pk3",
                ""
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_fileToDir(
                Q3EGameConstants.PatchResource.XASH3D_CS16_EXTRAS,
                Q3ELang.tr(context, R.string.cs16_xash3d_extras),
                "1",
                Q3EGameConstants.GAME_XASH3D,
                null,
                "pak/xash3d/cs16client-extras.pk3",
                ""
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_zipToDir(
                Q3EGameConstants.PatchResource.SOURCE_ENGINE_EXTRAS,
                Q3ELang.tr(context, R.string.sourceengine_extras),
                Q3EGameConstants.SOURCE_ENGINE_VERSION,
                Q3EGameConstants.GAME_SOURCE,
                null,
                "pak/source/extras.zip",
                ""
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource_fileToDir(
                Q3EGameConstants.PatchResource.ET_LEGACY_EXTRAS,
                Q3ELang.tr(context, R.string.etlegacy_extras) + "(" + Q3EGameConstants.GAME_VERSION_ETW + ")",
                Q3EGameConstants.ETW_VERSION,
                Q3EGameConstants.GAME_ETW,
                null,
                "pak/etw/legacy_v" + Q3EGameConstants.GAME_VERSION_ETW + ".pk3",
                "legacy"
        );
        resourceList.add(rsc);
    }

    public String Fetch(Q3EGameConstants.PatchResource type, boolean overwrite, String...fsgame)
    {
        for(Q3EPatchResource rsc : resourceList)
        {
            if(rsc.type == type)
            {
                return rsc.Fetch(context, overwrite, fsgame);
            }
        }
        throw new RuntimeException("Unexpected patch resource type: " + type);
    }
}
