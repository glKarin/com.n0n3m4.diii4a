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

        rsc = new Q3EPatchResource(
                Q3EGlobals.PatchResource.QUAKE4_SABOT,
                Q3ELang.tr(context, R.string.bot_q3_bot_support_in_mp_game),
                "1",
                Q3EGlobals.GAME_QUAKE4,
                null,
                Q3EPatchResource.COPY_FILE_TO_DIR,
                "pak/q4base/sabot_a9.pk4",
                null
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource(
                Q3EGlobals.PatchResource.DOOM3_RIVENSIN_ORIGIANL_LEVELS,
                Q3ELang.tr(context, R.string.rivensin_play_original_doom3_level),
                "1",
                Q3EGlobals.GAME_DOOM3,
                "rivensin",
                Q3EPatchResource.COPY_FILE_TO_DIR,
                "pak/rivensin/play_original_doom3_level.pk4",
                ""
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource(
                Q3EGlobals.PatchResource.DOOM3BFG_HLSL_SHADER,
                Q3ELang.tr(context, R.string.rbdoom3_bfg_hlsl_shader),
                Q3EGlobals.RBDOOM3BFG_HLSL_SHADER_VERSION,
                Q3EGlobals.GAME_DOOM3BFG,
                null,
                Q3EPatchResource.EXTRACT_ZIP_TO_DIR,
                "pak/doom3bfg/renderprogs.pk4",
                "base"
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource(
                Q3EGlobals.PatchResource.TDM_GLSL_SHADER,
                Q3ELang.tr(context, R.string.the_dark_mod_glsl_shader),
                Q3EGlobals.TDM_GLSL_SHADER_VERSION,
                Q3EGlobals.GAME_TDM,
                null,
                Q3EPatchResource.EXTRACT_ZIP_TO_DIR,
                "pak/darkmod/glprogs.pk4",
                ""
        );
        resourceList.add(rsc);

        rsc = new Q3EPatchResource(
                Q3EGlobals.PatchResource.GZDOOM_RESOURCE,
                Q3ELang.tr(context, R.string.gzdoom_builtin_resource) + "(4.14.0)",
                Q3EGlobals.GZDOOM_VERSION,
                Q3EGlobals.GAME_GZDOOM,
                null,
                Q3EPatchResource.COPY_DIR_TO_DIR,
                "pak/gzdoom/4.14.0",
                ""
        );
        resourceList.add(rsc);
    }

    public String Fetch(Q3EGlobals.PatchResource type, boolean overwrite, String...fsgame)
    {
        for(Q3EPatchResource rsc : resourceList)
        {
            if(rsc.type == type)
            {
                return rsc.Fetch(context, overwrite, fsgame);
            }
        }
        throw new RuntimeException("Unexcept patch resource type: " + type);
    }
}
