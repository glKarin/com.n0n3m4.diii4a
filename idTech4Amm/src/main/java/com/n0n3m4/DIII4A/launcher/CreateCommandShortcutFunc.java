package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ShortcutInfo;
import android.content.pm.ShortcutManager;
import android.graphics.drawable.Icon;
import android.os.Bundle;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.Utility;
import com.karin.idTech4Amm.misc.Function;
import com.karin.idTech4Amm.sys.GameManager;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EMain;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class CreateCommandShortcutFunc extends GameLauncherFunc
{
    private final int m_code;
    private String m_game;
    private String m_cmd;

    public CreateCommandShortcutFunc(GameLauncher gameLauncher, int code)
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
        m_cmd = data.getString("command");
        m_game = data.getString("game");
        int res = ContextUtility.CheckPermission(m_gameLauncher, android.Manifest.permission.INSTALL_SHORTCUT, m_code);
        if (res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Tr(R.string.permission_not_granted, "android.Manifest.permission.INSTALL_SHORTCUT"));
        if (res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    public void run()
    {
        String rs = Tr(GameManager.GetGameNameRS(m_game));
        if(CheckPinnedShortcut(m_game, m_cmd))
        {
            Toast_long(Tr(R.string.game_desktop_shortcut_has_created, rs, m_cmd));
            return;
        }

        String[] args = { rs };
        AlertDialog input = ContextUtility.Input(m_gameLauncher, Tr(R.string.create_shortcut_with_current_command_and_game), Tr(R.string.input_desktop_shortcut_name), args, new Runnable() {
            @Override
            public void run()
            {
                String name = args[0];
                if(KStr.IsEmpty(name))
                    name = rs;
                CreateShortcut(m_game, name, m_cmd);
            }
        }, null, null, null, null);
    }

    private String GenShortcutId(String game, String command)
    {
        return "idTech4Amm_" + game + "_" + "command" + "-" + Utility.MD5(command);
    }

    private boolean CheckPinnedShortcut(String game, String command)
    {
        if(android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.N_MR1)
            return false;

        ShortcutManager shortcutManager = (ShortcutManager) m_gameLauncher.getSystemService(Context.SHORTCUT_SERVICE);
        if (null == shortcutManager)
            return false;

        List<ShortcutInfo> pinnedShortcuts = shortcutManager.getPinnedShortcuts();
        String shortcutId = GenShortcutId(game, command);
        for(ShortcutInfo info : pinnedShortcuts)
        {
            if(info.getId().equals(shortcutId))
                return true;
        }
        return false;
    }

    private void CreateShortcut(String game, String name, String command)
    {
        if(android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.N_MR1)
            return;

        ShortcutManager shortcutManager = (ShortcutManager) m_gameLauncher.getSystemService(Context.SHORTCUT_SERVICE);
        if (null != shortcutManager && shortcutManager.isRequestPinShortcutSupported())
        {
            int iconId = GameManager.GetGameIcon(game);
            Class<?> activity = Q3EMain.class;
            String shortcutId = GenShortcutId(game, command);
            String gameName = name;
            String longName = gameName;

            Intent intent = new Intent(m_gameLauncher, activity)
                    .putExtra("game", game)
                    .putExtra("command", command)
                    .setAction(Intent.ACTION_VIEW)
                    ;
            ShortcutInfo shortcutInfo = new ShortcutInfo.Builder(m_gameLauncher, shortcutId)
                    .setShortLabel(gameName)
                    .setLongLabel(longName)
                    .setIcon(Icon.createWithResource(m_gameLauncher, iconId))
                    .setIntent(intent)
                    .build();

            Intent pinnedShortcutCallbackIntent = shortcutManager.createShortcutResultIntent(shortcutInfo);
            PendingIntent successCallback = PendingIntent.getBroadcast(m_gameLauncher, 0, pinnedShortcutCallbackIntent, 0);

            try
            {
                if(shortcutManager.requestPinShortcut(shortcutInfo, successCallback.getIntentSender()))
                    Toast_long(Tr(R.string.create_desktop_shortcut_success, gameName, command));
                else
                    Toast_long(Tr(R.string.create_desktop_shortcut_fail, gameName, command));
            }
            catch(Exception e)
            {
                e.printStackTrace();
                Toast_long(Tr(R.string.create_desktop_shortcut_fail, gameName, command) + ": " + e.getMessage());
            }
        }
        else
        {
            Toast_long(R.string.unsupport_create_desktop_shortcut);
        }
    }
}
