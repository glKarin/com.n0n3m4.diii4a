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
import com.karin.idTech4Amm.sys.GameManager;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EMain;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class CreateShortcutFunc extends GameLauncherFunc
{
    private static final int TYPE_GAME     = 1;
    private static final int TYPE_LAUNCHER = 1 << 1;
    private static final int TYPE_ALL      = TYPE_GAME | TYPE_LAUNCHER;

    private final int m_code;
    private String m_game;

    public CreateShortcutFunc(GameLauncher gameLauncher, int code)
    {
        super(gameLauncher);
        m_code = code;
    }

    public void Reset()
    {
        m_game = null;
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();
        //k check external storage permission
        int res = ContextUtility.CheckPermission(m_gameLauncher, android.Manifest.permission.INSTALL_SHORTCUT, m_code);
        if (res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long("android.Manifest.permission.INSTALL_SHORTCUT not granted");
        if (res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    public void run()
    {
        final List<CharSequence> items = new ArrayList<>();
        final List<String> values = new ArrayList<>();
        // Map<String, Integer> pinnedShortcuts = GetPinnedShortcuts();
        int selected = -1;
        for(int i = 0; i < GameManager.Games.length; i++)
        {
            String game = GameManager.Games[i];
            items.add(Tr(GameManager.GetGameNameRS(game)));
            values.add(game);
            if(game.equals(m_game))
                selected = i;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.create_desktop_shortcut);
        builder.setSingleChoiceItems(items.toArray(new CharSequence[0]), selected, new DialogInterface.OnClickListener(){
            public void onClick(DialogInterface dialog, int p)
            {
                String g = values.get(p);
                UpdateDialogButtonState((AlertDialog)dialog, g);
            }
        });
        builder.setPositiveButton(R.string.game, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                if(null != m_game)
                    CreateShortcut(m_game, TYPE_GAME);
                dialog.dismiss();
            }
        });
        builder.setNeutralButton(R.string.launcher, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                if(null != m_game)
                    CreateShortcut(m_game, TYPE_LAUNCHER);
                dialog.dismiss();
            }
        });
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                dialog.dismiss();
            }
        });
        AlertDialog dialog = builder.create();
        dialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface dl)
            {
                UpdateDialogButtonState(dialog, m_game);
            }
        });
        dialog.show();
    }

    private void UpdateDialogButtonState(AlertDialog d, String g)
    {
        UpdateDialogButtonState(d, g, null != g ? GetPinnedShortcut(g) : TYPE_ALL);
    }

    private void UpdateDialogButtonState(AlertDialog d, String g, Integer mask)
    {
        boolean canGame = true;
        boolean canLauncher = true;
        if(null != mask)
        {
            if((mask & TYPE_GAME) != 0)
                canGame = false;
            if((mask & TYPE_LAUNCHER) != 0)
                canLauncher = false;
        }
        d.getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(canGame);
        d.getButton(DialogInterface.BUTTON_NEUTRAL).setEnabled(canLauncher);
        m_game = canGame || canLauncher ? g : null;
    }

    private void ChooseType(String game)
    {
        final List<CharSequence> items = new ArrayList<>();
        final List<Integer> values = new ArrayList<>();
        int mask = GetPinnedShortcut(game);
        if((mask & TYPE_GAME) == 0)
        {
            items.add(Tr(R.string.game));
            values.add(TYPE_GAME);
        }
        if((mask & TYPE_LAUNCHER) == 0)
        {
            items.add(Tr(R.string.launcher));
            values.add(TYPE_LAUNCHER);
        }
/*        if(mask == 0)
        {
            items.add(Tr(R.string.all));
            values.add(TYPE_ALL);
        }*/

        if(items.isEmpty())
        {
            run();
            return;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.create_desktop_shortcut);
        builder.setSingleChoiceItems(items.toArray(new CharSequence[0]), -1, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                int mask = values.get(p);
                if((mask & TYPE_GAME) != 0)
                    CreateShortcut(game, TYPE_GAME);
                if((mask & TYPE_LAUNCHER) != 0)
                    CreateShortcut(game, TYPE_LAUNCHER);
                dialog.dismiss();
            }
        });
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                dialog.dismiss();
            }
        });
        builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog)
            {
                run();
            }
        });
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private Map<String, Integer> GetPinnedShortcuts()
    {
        if(android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.N_MR1)
            return new HashMap<>();

        ShortcutManager shortcutManager = (ShortcutManager) m_gameLauncher.getSystemService(Context.SHORTCUT_SERVICE);
        if (null == shortcutManager)
            return new HashMap<>();

        List<ShortcutInfo> pinnedShortcuts = shortcutManager.getPinnedShortcuts();
        Map<String, Integer> res = new HashMap<>();
        for(ShortcutInfo info : pinnedShortcuts)
        {
            String id = info.getId();
            if(!id.startsWith("idTech4Amm_"))
                continue;
            id = id.substring("idTech4Amm_".length());
            String[] arr = id.split("_");
            String game = arr[0];
            String type = arr[1];
            int mask = 0;
            if("launcher".equals(type))
                mask |= TYPE_LAUNCHER;
            else if("game".equals(type))
                mask |= TYPE_GAME;
            else
                continue;
            if(res.containsKey(game))
                mask |= res.get(game);
            res.put(game, mask);
        }
        return res;
    }

    private int GetPinnedShortcut(String game)
    {
        if(android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.N_MR1)
            return 0;

        ShortcutManager shortcutManager = (ShortcutManager) m_gameLauncher.getSystemService(Context.SHORTCUT_SERVICE);
        if (null == shortcutManager)
            return 0;

        List<ShortcutInfo> pinnedShortcuts = shortcutManager.getPinnedShortcuts();
        String prefix = "idTech4Amm_" + game + "_";
        int mask = 0;
        for(ShortcutInfo info : pinnedShortcuts)
        {
            String id = info.getId();
            if(!id.startsWith(prefix))
                continue;
            String type = id.substring(prefix.length());
            if("launcher".equals(type))
                mask |= TYPE_LAUNCHER;
            else if("game".equals(type))
                mask |= TYPE_GAME;
        }
        return mask;
    }

    private void CreateShortcut(String game, int type)
    {
        if(android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.N_MR1)
            return;

        ShortcutManager shortcutManager = (ShortcutManager) m_gameLauncher.getSystemService(Context.SHORTCUT_SERVICE);
        if (null != shortcutManager && shortcutManager.isRequestPinShortcutSupported())
        {
            boolean isLauncher = TYPE_LAUNCHER == type;
            int iconId = GameManager.GetGameIcon(game);
            Class<?> activity = isLauncher ? GameLauncher.class : Q3EMain.class;
            String shortcutId = "idTech4Amm_" + game + "_" + (isLauncher ? "launcher" : "game");
            String gameName = Tr(GameManager.GetGameNameRS(game));
            String longName = gameName;
            if(isLauncher)
                longName += " " + Tr(R.string.launcher);
            String typeName = isLauncher ? Tr(R.string.launcher) : Tr(R.string.game);

            Intent intent = new Intent(m_gameLauncher, activity)
                    .putExtra("game", game)
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
                    Toast_long(Tr(R.string.create_desktop_shortcut_success, gameName, typeName));
                else
                    Toast_long(Tr(R.string.create_desktop_shortcut_fail, gameName, typeName));
            }
            catch(Exception e)
            {
                e.printStackTrace();
                Toast_long(Tr(R.string.create_desktop_shortcut_fail, gameName, typeName) + ": " + e.getMessage());
            }
            run();
        }
        else
        {
            Toast_long(R.string.unsupport_create_desktop_shortcut);
        }
    }
}
