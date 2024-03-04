package com.n0n3m4.q3e.onscreen;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.widget.Toast;

public class Q3EGUI
{
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

    public void MessageDialog(String title, String text)
    {
        m_context.runOnUiThread(new Runnable() {
            @Override
            public void run()
            {
                AlertDialog.Builder builder = new AlertDialog.Builder(m_context);
                builder.setTitle(title)
                        .setMessage(text)
                        .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which)
                            {
                                dialog.dismiss();
                            }
                        });
                builder.create().show();
            }
        });
    }
}
