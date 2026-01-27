package com.n0n3m4.q3e;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Build;
import android.view.View;
import android.widget.Toast;

import com.n0n3m4.q3e.device.Q3EOuya;

public class Q3EGUI
{
    public final static int DIALOG_ERROR  = -1;
    public final static int DIALOG_CANCEL = 0;
    public final static int DIALOG_YES    = 1;
    public final static int DIALOG_NO     = 2;
    public final static int DIALOG_OTHER  = 3;

    public static int UI_FULLSCREEN_HIDE_NAV_OPTIONS = 0;
    public static int UI_FULLSCREEN_OPTIONS = 0;

    private final Activity m_context;

    static
    {
        UI_FULLSCREEN_HIDE_NAV_OPTIONS = GetFullScreenFlags(true);
        UI_FULLSCREEN_OPTIONS = GetFullScreenFlags(false);
    }

    /*
    @SuppressLint("InlinedApi")
    private final int m_uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_FULLSCREEN;
    @SuppressLint("InlinedApi")
    private final int m_uiOptions_def = View.SYSTEM_UI_FLAG_FULLSCREEN
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
     */
    public static int GetFullScreenFlags(boolean hideNav)
    {
        int m_uiOptions = 0;

        if(hideNav)
        {
            m_uiOptions |= View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
            {
                m_uiOptions |= View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
            }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
        {
            m_uiOptions |= View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
            m_uiOptions |= View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
            m_uiOptions |= View.SYSTEM_UI_FLAG_FULLSCREEN;
        }
        return m_uiOptions;
    }

    public Q3EGUI(Activity context)
    {
        this.m_context = context;
    }

    public void Toast(String text)
    {
        m_context.runOnUiThread(new Runnable() {
            @Override
            public void run()
            {
                Toast.makeText(m_context, text, Toast.LENGTH_LONG).show();
            }
        });
    }

    // must run on non-UI thread
    public int MessageDialog(String title, String text, String[] buttons)
    {
        final Object lock = new Object();
        int[] res = { DIALOG_CANCEL };
        synchronized(lock) {
            try
            {
                Q3E.post(new Runnable() {
                    @Override
                    public void run()
                    {
                        AlertDialog.Builder builder = new AlertDialog.Builder(m_context);
                        builder.setTitle(title)
                                .setMessage(text)
                        ;
                        if(null != buttons)
                        {
                            if(buttons.length > 0 && null != buttons[0])
                                builder.setPositiveButton(buttons[0], new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which)
                                    {
                                        res[0] = DIALOG_YES;
                                        dialog.dismiss();
                                    }
                                });
                            if(buttons.length > 1 && null != buttons[1])
                                builder.setNegativeButton(buttons[1], new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which)
                                    {
                                        res[0] = DIALOG_NO;
                                        dialog.dismiss();
                                    }
                                });
                            if(buttons.length > 2 && null != buttons[2])
                                builder.setNeutralButton(buttons[2], new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which)
                                    {
                                        res[0] = DIALOG_OTHER;
                                        dialog.dismiss();
                                    }
                                });
                        }
                        AlertDialog dialog = builder.create();
                        dialog.setOnDismissListener(new DialogInterface.OnDismissListener()
                        {
                            @Override
                            public void onDismiss(DialogInterface dialog)
                            {
                                synchronized(lock) {
                                    lock.notifyAll();
                                }
                            }
                        });
                        dialog.show();
                    }
                });
                lock.wait();
            }
            catch(Exception e)
            {
                res[0] = DIALOG_ERROR;
                e.printStackTrace();
            }
        }

        return res[0];
    }

    public boolean BacktraceDialog(String title, String text)
    {
        final Object lock = new Object();
        boolean[] res = { false };
        synchronized(lock) {
            try
            {
                Q3E.post(new Runnable() {
                    @Override
                    public void run()
                    {
                        AlertDialog.Builder builder = new AlertDialog.Builder(m_context);
                        builder.setTitle(title)
                                .setMessage(text)
                                .setCancelable(false)
                        ;
                        builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which)
                            {
                                dialog.dismiss();
                            }
                        });

                        builder.setNeutralButton(R.string.copy, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which)
                            {
                                Q3EContextUtils.CopyToClipboard(m_context, text);
                                dialog.dismiss();
                            }
                        });
                        builder.setNegativeButton(R.string.exit, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which)
                            {
                                res[0] = true;
                                Q3E.Finish();
                                dialog.dismiss();
                            }
                        });
                        AlertDialog dialog = builder.create();
                        dialog.setOnDismissListener(new DialogInterface.OnDismissListener()
                        {
                            @Override
                            public void onDismiss(DialogInterface dialog)
                            {
                                synchronized(lock) {
                                    lock.notifyAll();
                                }
                            }
                        });
                        dialog.show();
                    }
                });
                lock.wait();
            }
            catch(Exception e)
            {
                e.printStackTrace();
            }
        }
        return res[0];
    }
}
