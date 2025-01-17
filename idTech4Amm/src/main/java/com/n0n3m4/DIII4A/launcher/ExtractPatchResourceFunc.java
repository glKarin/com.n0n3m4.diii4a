package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.widget.Toast;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.sys.GameResourceUrl;
import com.karin.idTech4Amm.sys.Game;
import com.karin.idTech4Amm.sys.GameManager;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EPatchResource;
import com.n0n3m4.q3e.Q3EPatchResourceManager;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class ExtractPatchResourceFunc extends GameLauncherFunc
{
    private static final int RESOURCE_TYPE_PATCH = 1;
    private static final int RESOURCE_TYPE_URL = 2;

    private       String       m_path;
    private final int          m_code;
    private       boolean      m_all;
    private       String       m_game;
    private       String       m_mod;

    public ExtractPatchResourceFunc(GameLauncher gameLauncher, int code)
    {
        super(gameLauncher);
        m_code = code;
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        m_path = data.getString("path");
        m_all = data.getBoolean("all");
        m_game = data.getString("game");
        m_mod = data.getString("mod");
        run();
    }

    private static class ResourceObject
    {
        public int type;
        public Object data;

        public ResourceObject(Q3EPatchResource data)
        {
            type = RESOURCE_TYPE_PATCH;
            this.data = data;
        }

        public ResourceObject(GameResourceUrl data)
        {
            type = RESOURCE_TYPE_URL;
            this.data = data;
        }
    }

    public void run()
    {
        Q3EPatchResourceManager manager = new Q3EPatchResourceManager(m_gameLauncher);
        List<Q3EPatchResource> resources = manager.ResourceList();
        List<CharSequence> nameList = new ArrayList<>();
        List<ResourceObject> rscList = new ArrayList<>();

        for(Q3EPatchResource resource : resources)
        {
            if(!m_all)
            {
                if(!resource.game.equals(m_game))
                    continue;
                if(null != resource.mod && !resource.mod.equals(m_mod))
                    continue;
            }
            nameList.add(m_all ? resource.name : GetResourcePatchName(resource));
            rscList.add(new ResourceObject(resource));
        }

        if(!m_all)
        {
            List<GameResourceUrl> gameResourceUrls = GameResourceUrl.GetResourceUrlList(m_game, m_mod);
            for(GameResourceUrl resource : gameResourceUrls)
            {
                nameList.add(GetResourceUrlName(resource));
                rscList.add(new ResourceObject(resource));
            }
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(m_all ? R.string.game_patch_resource : R.string.game_data_resource)
                .setItems(nameList.toArray(new CharSequence[0]), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int p)
                    {
                        ResourceObject resourceObject = rscList.get(p);
                        if(resourceObject.type == RESOURCE_TYPE_PATCH)
                            ExtractPatchResource((Q3EPatchResource) resourceObject.data);
                        else
                            OpenResourceUrl((GameResourceUrl) resourceObject.data);
                    }
                })
                .setNegativeButton(R.string.cancel, null)
                .create()
                .show()
        ;
    }

    private void OpenResourceUrl(GameResourceUrl resource)
    {
        ContextUtility.OpenUrlExternally(m_gameLauncher, resource.url);
    }

    private boolean ExtractPatchResource(Q3EPatchResource resource)
    {
        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Tr(R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Tr(R.string.access_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return false;

        String toPath = resource.Fetch(m_gameLauncher, true, m_mod);

        if(null != toPath)
            Toast_short(Tr(R.string.extract_path_resource_) + toPath);
        else
            Toast_short(Tr(R.string.extract_path_resource_) + Tr(R.string.fail));
        //run();
        return null != toPath;
    }

    private String GetResourceUrlName(GameResourceUrl resource)
    {
        Game game = Game.GetGameMod(resource.type, resource.game);
        String name;
        if(null != game)
            name = game.GetName(m_gameLauncher);
        else
        {
            int resId = GameManager.GetGameNameTs(resource.type);
            name = Q3ELang.tr(m_gameLauncher, resId);
        }
        if(KStr.NotEmpty(resource.name))
            name += " " + resource.name;
        switch(resource.source)
        {
            case GameResourceUrl.SOURCE_HOMEPAGE:
                return "[" + Q3ELang.tr(m_gameLauncher, R.string.homepage) + "]" + " " + name;
            case GameResourceUrl.SOURCE_STEAM:
                return "[" + Q3ELang.tr(m_gameLauncher, R.string.steam) + "]" + " " + name;
            case GameResourceUrl.SOURCE_MODDB:
                return "[" + Q3ELang.tr(m_gameLauncher, R.string.moddb) + "]" + " " + name;
            case GameResourceUrl.SOURCE_OTHER:
            default:
                return "[" + Q3ELang.tr(m_gameLauncher, R.string.url) + "]" + " " + name;
        }
    }

    private String GetResourcePatchName(Q3EPatchResource resource)
    {
        return "[" + Q3ELang.tr(m_gameLauncher, R.string.patch) + "]" + " " + resource.name;
    }
}
