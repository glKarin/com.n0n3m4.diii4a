package com.n0n3m4.q3e;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.widget.Toast;

public class Q3EGUI
{
    public final static int DIALOG_ERROR  = -1;
    public final static int DIALOG_CANCEL = 0;
    public final static int DIALOG_YES    = 1;
    public final static int DIALOG_NO     = 2;
    public final static int DIALOG_OTHER  = 3;

    private final Activity m_context;

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
                                Q3EUtils.CopyToClipboard(m_context, text);
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
