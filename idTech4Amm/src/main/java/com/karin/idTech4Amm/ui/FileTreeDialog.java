package com.karin.idTech4Amm.ui;

import android.app.AlertDialog;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.misc.FileBrowser;
import com.n0n3m4.q3e.Q3ELang;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * Simple file chooser
 */
public class FileTreeDialog extends AlertDialog {
    private static final String TAG = "FileTreeDialog";

    private FileTreeView m_listView;
    private String m_title;
    private FileBrowserCallback m_callback;
    private final FileTreeAdapter.FileTreeAdapterListener m_listener = new FileTreeAdapter.FileTreeAdapterListener() {
        @Override
        public void OnGrantPermission(String path)
        {
            if(null != m_callback)
                m_callback.Check(path);
        }

    };

    public FileTreeDialog(Context context)
    {
        super(context);
        m_title = Q3ELang.tr(context, R.string.file_chooser);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //SetupUI();
    }

    public void SetupUI(String title)
    {
        SetupUI(title, null);
    }

    public void SetupUI(String title, String path)
    {
        Context context = getContext();
        
        m_title = title;
       
        setTitle(m_title);
        m_listView = new FileTreeView(context);
        m_listView.SetupUI();
        setView(m_listView);

        m_listView.Adapter().SetListener(m_listener);
        if(null != path)
            SetPath(path);
    }

    public FileBrowser GetFileBrowser()
    {
        return m_listView.GetFileBrowser();
    }

    public void SetPath(String path)
    {
        m_listView.SetPath(path);
    }

    public void SetCallback(FileBrowserCallback c)
    {
        m_callback = c;
    }

    public interface FileBrowserCallback
    {
        public void Check(String path);
    }
}

