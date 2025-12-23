package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.widget.ListView;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.sys.Constants;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EGameConstants;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.karin.KLog;

import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

public final class UpdateCompatFunc extends GameLauncherFunc
{
    public UpdateCompatFunc(GameLauncher gameLauncher)
    {
        super(gameLauncher);
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        run();
    }

    public void run()
    {
        int[] vers = { 0, 0, };
        if (IsUpdateRelease(vers))
        {
            Object[] args = new Object[1];
            Runnable callback = new Runnable() {
                @Override
                public void run()
                {
                    AlertDialog.Builder builder = (AlertDialog.Builder)args[0];
                    builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                        @Override
                        public void onDismiss(DialogInterface dialog)
                        {
                            OpenUpdateTips();
                        }
                    });
                }
            };
            ContextUtility.OpenMessageDialog(m_gameLauncher, Q3ELang.tr(m_gameLauncher, R.string.update_) + Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")", TextHelper.GetUpdateText(m_gameLauncher), callback, args);

            ResetPreferences(vers[0], vers[1]);
        }
        else
            OpenUpdateTips();
        ResetPreferences(vers[0], vers[1]);
    }

    private boolean IsUpdateRelease(int[] vers)
    {
        final String UPDATE_RELEASE = "UPDATE_RELEASE";
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        int r = pref.getInt(UPDATE_RELEASE, 0);
        vers[0] = r;
        vers[1] = Constants.CONST_UPDATE_RELEASE;
        KLog.I("App version: current=%d, new=%d, update=%s", vers[0], vers[1], r != Constants.CONST_UPDATE_RELEASE);
        if (r == Constants.CONST_UPDATE_RELEASE)
            return false;
        pref.edit().putInt(UPDATE_RELEASE, Constants.CONST_UPDATE_RELEASE).commit();
        return true;
    }

    private void OpenUpdateTips()
    {
        Map<String, Integer> defaultTips = new LinkedHashMap<>();
        defaultTips.put("STANDALONE_DIRECTORY", 57);

        OpenUpdateTipsDialog(defaultTips, null);
    }

    private void OpenUpdateTipsDialog(Map<String, Integer> defaultTips, Set<String> ignore)
    {
        final String UPDATE_TIPS = "UPDATE_TIPS";
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        Set<String> stateSet = pref.getStringSet(UPDATE_TIPS, new TreeSet<>());
        Set<String> writeSet = new TreeSet<>(stateSet);

        Set<String> handled = new HashSet<>();
        if(null != ignore)
            handled.addAll(ignore);

        for(Map.Entry<String, Integer> def : defaultTips.entrySet())
        {
            if(handled.contains(def.getKey()))
                continue;

            handled.add(def.getKey());

            TipState tmp = null;
            for(String state : writeSet)
            {
                TipState tt = new TipState(state);
                if(def.getKey().equals(tt.name))
                {
                    tmp = tt;
                    writeSet.remove(state);
                    break;
                }
            }
            final TipState t = null == tmp ? new TipState(def.getKey(), def.getValue(), 0) : tmp;

            if(t.NeedCheck("STANDALONE_DIRECTORY") || !pref.contains(Q3EPreference.GAME_STANDALONE_DIRECTORY))
            {
                String Endl = TextHelper.GetDialogMessageEndl();
                String message = Q3ELang.tr(m_gameLauncher, R.string.idtech4amm_requires_enable_game_standalone_directory_since_version_57) + Endl
                        + Q3ELang.tr(m_gameLauncher, R.string.you_can_also_disable_it_like_older_version) + Endl
                        + Q3ELang.tr(m_gameLauncher, R.string.you_can_also_change_it_manually_on_launcher_settings) + Endl
                        + Q3ELang.tr(m_gameLauncher, R.string.you_can_view_all_support_games_data_directory_path_by_tips_button_above_chooser_button_in_launcher)
                        ;

                AlertDialog.Builder builder = ContextUtility.CreateMessageDialogBuilder(m_gameLauncher, Q3ELang.tr(m_gameLauncher, R.string.enable_game_standalone_directory), TextHelper.GetDialogMessage(message));
                builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        t.checked = 1;
                        writeSet.add(t.ToString());
                        pref.edit().putBoolean(Q3EPreference.GAME_STANDALONE_DIRECTORY, true)
                                .putStringSet(UPDATE_TIPS, writeSet)
                                .commit();
                    }
                });
                builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog)
                    {
                        OpenUpdateTipsDialog(defaultTips, handled);
                    }
                });
                AlertDialog dialog = builder.create();
                dialog.show();
            }

            break;
        }
    }

    private void ResetPreferences(int curVer, int newVer)
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        // Version 69: remove RealRTCW v5.0 and The Dark Mod v2.12
        if(newVer == 64)
        {
            if(curVer >= 62 && curVer <= 63)
            {
                preferences.edit()
                        .remove(Q3EPreference.pref_harm_realrtcw_version)
                        .commit();
            }
            if(curVer == 63)
            {
                preferences.edit()
                        .remove(Q3EPreference.pref_harm_tdm_version)
                        .commit();
            }
        }
        // Version 69: remove RealRTCW v5.1
        if(newVer == 69)
        {
            if((curVer >= 62 && curVer <= 63) || curVer == 68)
            {
                preferences.edit()
                        .remove(Q3EPreference.pref_harm_realrtcw_version)
                        .commit();
            }
        }
        // Version 69: q3e_params_XXX rename to q3e_harm_XXX_params
        if(curVer <= 68)
        {
            Map<String, ?> kvs = preferences.getAll();
            if(null != kvs)
            {
                SharedPreferences.Editor editor = preferences.edit();
                for(String key : kvs.keySet())
                {
                    String prefix = "q3e_params_";
                    if(!key.startsWith(prefix))
                        continue;
                    Object val = kvs.get(key);
                    if(!(val instanceof String))
                        continue;
                    String game = key.substring(prefix.length());
                    if("xash".equals(game))
                    {
                        editor.remove(key);
                        continue;
                    }
                    if("quake4".equals(game))
                        game = "q4";
                    editor.putString("q3e_harm_" + game + "_params", (String)val);
                    editor.remove(key);
                }
                editor.commit();
            }
        }
    }

    private static class TipState
    {
        public final String name;
        public final int    version;
        public       int    checked;

        public TipState(String name, int version, int checked)
        {
            this.name = name;
            this.version = version;
            this.checked = checked;
        }

        public TipState(String str)
        {
            String[] part = str.trim().split(" ");
            name = part[0];
            version = Integer.parseInt(part[1]);
            checked = Integer.parseInt(part[2]);
        }

        public boolean NeedCheck(String n)
        {
            return name.equals(n) && checked == 0 && Constants.CONST_UPDATE_RELEASE >= version;
        }

        public String ToString()
        {
            return name + " " + version + " " + checked;
        }
    }
}
