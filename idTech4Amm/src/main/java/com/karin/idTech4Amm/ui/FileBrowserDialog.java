package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.os.Bundle;

import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;

import android.app.AlertDialog;
import android.util.Log;
import android.graphics.Color;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.misc.FileBrowser;
import com.n0n3m4.q3e.Q3ELang;

/**
 * Simple file chooser
 */
public class FileBrowserDialog extends AlertDialog {
    private static final String TAG = "FileBrowserDialog";

    private FileViewAdapter m_adapter;
    private ListView m_listView;
    private String m_path;
    private String m_file;
    private String m_title;
    private FileBrowserDialogListener m_listener;
    
    public FileBrowserDialog(Context context)
    {
        super(context);
        m_title = Q3ELang.tr(context, R.string.file_chooser);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //SetupUI();
    }

    public void SetupUI(String title, String path)
    {
        Context context = getContext();
        
        m_title = title;
        m_path = path;
       
        setTitle(m_title);
        m_listView = new ListView(context);
        setView(m_listView);
        
        m_adapter = new FileViewAdapter(context, null);
        m_listView.setAdapter(m_adapter);
        m_listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    FileViewAdapter adapter;

                    adapter = (FileViewAdapter)parent.getAdapter();
                    adapter.Open(position);
                }
            });
        m_adapter.SetPath(m_path);
    }
    
    public interface FileBrowserDialogListener
    {
        public void OnPathChanged(String path);
        public void OnFileSelected(String file, String path);
    }
    
    public FileBrowserDialog SetListener(FileBrowserDialogListener l)
    {
        m_listener = l;
        return this;
    }
    
    private void SetPath(String path)
    {
        m_path = path;
        if(m_listener != null)
            m_listener.OnPathChanged(m_path);
    }

    private void SetFile(String file)
    {
        m_file = file;
        if(m_listener != null)
            m_listener.OnFileSelected(m_file, m_path);
    }
    
    public String Path()
    {
        return m_path;
    }

    public String File()
    {
        return m_file;
    }

    // internal
    private class FileViewAdapter extends ArrayAdapter_base<FileBrowser.FileModel>
    {
        private final FileBrowser m_fileBrowser;

        public FileViewAdapter(Context context, String path)
        {
            super(context, android.R.layout.select_dialog_item);
            m_fileBrowser = new FileBrowser(path);
            SetData(m_fileBrowser.FileList());
        }

        public View GenerateView(int position, View view, ViewGroup parent, FileBrowser.FileModel data)
        {
            TextView textView;

            textView = (TextView)view;
            textView.setText(data.name);
            textView.setTextColor(data.IsDirectory() ? Color.BLACK : Color.GRAY);
            return view;
        }

        public void Open(int index)
        {
            FileBrowser.FileModel item;

            item = m_fileBrowser.GetFileModel(index);
            if(item == null)
                return;
            if(item.type != FileBrowser.FileModel.ID_FILE_TYPE_DIRECTORY) // TODO: symbol file
            {
                Log.d(TAG, "Open file: " + item.path);
                FileBrowserDialog.this.SetFile(item.path);
                // open
                return;
            }

            SetPath(item.path);
        }

        public void SetPath(String path)
        {
            Log.d(TAG, "Open directory: " + path);
            boolean res = m_fileBrowser.SetCurrentPath(path);
            if(true || res) // always update
            {
                SetData(m_fileBrowser.FileList());
                FileBrowserDialog.this.SetPath(m_fileBrowser.CurrentPath());
                FileBrowserDialog.this.SetFile(null);
                FileBrowserDialog.this.setTitle(FileBrowserDialog.this.m_title + ": \n" + FileBrowserDialog.this.m_path);
                m_listView.setSelection(0);
            }
        }
    }
}

