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
    private       String       m_path;
    private final int          m_code;
    private       boolean      m_all;
    private       String       m_game;

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
        run();
    }


    public void run()
    {
        Q3EPatchResourceManager manager = new Q3EPatchResourceManager(m_gameLauncher);
        List<Q3EPatchResource> resources = manager.ResourceList();
        List<CharSequence> nameList = new ArrayList<>();
        List<Q3EPatchResource> rscList = new ArrayList<>();
        String mod = GetGameMod(Q3EUtils.q3ei.game);

        for(Q3EPatchResource resource : resources)
        {
            if(!m_all)
            {
                if(!resource.game.equals(m_game))
                    continue;
                if(null != resource.mod && !resource.mod.equals(mod))
                    continue;
            }
            nameList.add(resource.name);
            rscList.add(resource);
        }

        // D3-format fonts don't need on longer
                // Tr(R.string.opengles_shader),
                //Tr(R.string.bot_q3_bot_support_in_mp_game),
                //Tr(R.string.rivensin_play_original_doom3_level),
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.game_patch_resource)
                .setItems(nameList.toArray(new CharSequence[0]), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int p)
                    {
                        ExtractPatchResource(rscList.get(p));
                    }
                })
                .setNegativeButton(R.string.cancel, null)
                /*.setPositiveButton(R.string.extract_all, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int p)
                    {
                        m_files.addAll(Arrays.asList(m_patchResources));
                        ExtractPatchResource();
                    }
                })*/
                .create()
                .show()
        ;
    }

    private boolean ExtractPatchResource(Q3EPatchResource resource)
    {
        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Tr(R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Tr(R.string.access_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return false;

        String base = GetGameMod(resource.game);
        String toPath = resource.Fetch(m_gameLauncher, true, base);

        if(null != toPath)
            Toast_short(Tr(R.string.extract_path_resource_) + toPath);
        else
            Toast_short(Tr(R.string.extract_path_resource_) + Tr(R.string.fail));
        //run();
        return null != toPath;
    }

    private String GetGameMod(String game)
    {
        SharedPreferences preferences = SharedPreferences();
        boolean userMod = preferences.getBoolean(Q3EInterface.GetEnableModPreferenceKey(game), false);
        String gamebase = Q3EInterface.GetGameBaseDirectory(game);
        String mod = preferences.getString(Q3EInterface.GetGameModPreferenceKey(game), gamebase);
        String base;
        if(userMod)
            base = KStr.IsEmpty(mod) ? gamebase : mod;
        else
            base = gamebase;
        return base;
    }

/*    public void run2()
    {
        int r = 0;
        String gamePath = m_path;
        final String BasePath = gamePath + File.separator;
        for (String str : m_files)
        {
            File f = new File(str);
            String path = f.getParent();
            if(null == path)
                path = "";
            String name = f.getName();
            String newFileName = "z_" + FileUtility.GetFileBaseName(name) + "_idTech4Amm." + FileUtility.GetFileExtension(name);
            boolean ok = ContextUtility.ExtractAsset(m_gameLauncher, "pak/" + str, BasePath + path + File.separator + newFileName);
            if(ok)
                r++;
        }
        Toast_short(Tr(R.string.extract_path_resource_) + r);
    }*/
}
