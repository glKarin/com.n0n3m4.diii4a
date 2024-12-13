package com.karin.idTech4Amm.ui;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.misc.FileBrowser;
import com.n0n3m4.q3e.Q3ELang;

/**
 * file tree view
 */
public class FileTreeView extends ListView
{
    private static final String TAG = "FileTreeView";

    private FileTreeAdapter m_adapter;
    private FileBrowserCallback m_callback;
    private final FileTreeAdapter.FileTreeAdapterListener m_listener = new FileTreeAdapter.FileTreeAdapterListener()
    {
        @Override
        public void OnGrantPermission(String path)
        {
            if(null != m_callback)
                m_callback.Check(path);
        }
    };

    public FileTreeView(Context context)
    {
        super(context);
    }

    public FileTreeView(Context context, AttributeSet attrs)
    {
        super(context, attrs);
    }

    public FileTreeView(Context context, AttributeSet attrs, int defStyleAttr)
    {
        super(context, attrs, defStyleAttr);
    }

    public FileTreeView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
    {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public void SetupUI()
    {
        SetupUI(null);
    }

    public void SetupUI( String path)
    {
        Context context = getContext();

        m_adapter = new FileTreeAdapter(context, null);
        setAdapter(m_adapter);
/*        setOnItemClickListener(new AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    FileTreeView.this.Open(position);
                }
            });*/
        m_adapter.SetListener(m_listener);
        if(null != path)
            SetPath(path);
    }

    public void SetPath(String path)
    {
        m_adapter.SetPath(path);
    }

    public void SetCallback(FileBrowserCallback c)
    {
        m_callback = c;
    }

    public FileBrowser GetFileBrowser()
    {
        return m_adapter.GetFileBrowser();
    }

    public FileTreeAdapter Adapter()
    {
        return m_adapter;
    }

    public interface FileBrowserCallback
    {
        public void Check(String path);
    }

    public static void Test(Activity activity)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(activity);
        builder.setTitle("TEST File Tree");
        FileTreeView fileTreeView = new FileTreeView(activity);
        builder.setView(fileTreeView);
        builder.show();
        fileTreeView.SetupUI("XXX");
    }
}

