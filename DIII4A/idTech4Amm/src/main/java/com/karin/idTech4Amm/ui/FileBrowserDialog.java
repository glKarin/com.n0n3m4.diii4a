package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.os.Bundle;

import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;
import android.app.AlertDialog;
import java.util.Comparator;
import java.util.Collections;
import java.io.File;
import java.util.Set;
import java.util.HashSet;
import android.util.Log;
import android.graphics.Color;

/**
 * Simple file chooser
 */
public class FileBrowserDialog extends AlertDialog {
    private static final String TAG = "FileBrowserDialog";

    private FileViewAdapter m_adapter;
    private ListView m_listView;
    private String m_path;
    private String m_file;
    private String m_title = "File chooser";
    private FileBrowserDialogListener m_listener;
    
    public FileBrowserDialog(Context context)
    {
        super(context);
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
        private FileBrowser m_fileBrowser;

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

class FileBrowser {
	public static final int ID_ORDER_BY_NAME = 1;
	public static final int ID_ORDER_BY_TIME = 2;

	public static final int ID_SEQUENCE_ASC = 1;
	public static final int ID_SEQUENCE_DESC = 2;

	private String m_currentPath;
	private Set<String> m_history;
	private List<FileModel> m_fileList = null;
	private int m_sequence = ID_SEQUENCE_ASC;
	private int m_filter = 0;
	private int m_order = ID_ORDER_BY_NAME;
	private List<String> m_extensions;
	private boolean m_showHidden = true;
	private boolean m_ignoreDotDot = false;

	public FileBrowser()
	{
		this(System.getProperty("user.home"));
	}

	public FileBrowser(String path)
	{
		m_history = new HashSet<String>();
		m_fileList = new ArrayList<FileModel>();
		m_extensions = new ArrayList<String>();
		m_showHidden = true;
		if(path == null || path.isEmpty())
			SetCurrentPath(path);
	}

	protected boolean ListFiles(String path)
	{
		File dir;
		File files[];
		FileModel item;

		if(path == null || path.isEmpty())
			return false;

		dir = new File(path);
		if(!dir.isDirectory())
			return false;

		files = dir.listFiles();
		if(files == null)
		{
			if(m_currentPath != path)
			{
				m_currentPath = path;
                m_fileList.clear();
			}
			return false;
		}

		m_fileList.clear();

		for(File f : files)
		{
			String name = f.getName();
			if(".".equals(name))
				continue;
			if(f.isDirectory())
				name += File.separator;

			item = new FileModel();
			item.name = name;
			item.path = f.getAbsolutePath();
			item.size = f.length();
			item.time = f.lastModified();
			item.type = f.isDirectory() ? FileModel.ID_FILE_TYPE_DIRECTORY : FileModel.ID_FILE_TYPE_FILE;
			m_fileList.add(item);
		}

		Collections.sort(m_fileList, m_fileComparator);
		//m_fileList.sort(m_fileComparator);

		// add parent directory
		if(!m_ignoreDotDot)
		{
			item = new FileModel();
			item.name = "../";
			item.path = dir.getParent();
			item.size = dir.length();
			item.time = dir.lastModified();
			item.type = dir.isDirectory() ? FileModel.ID_FILE_TYPE_DIRECTORY : FileModel.ID_FILE_TYPE_FILE;
			m_fileList.add(0, item);
		}

		if(m_currentPath != path)
		{
			m_currentPath = path;
		}

		return true;
	}

	public boolean SetCurrentPath(String path)
	{
		if(path != null && !path.equals(m_currentPath))
		{
			if(ListFiles(path))
			{
				//m_currentPath = path;
				m_history.add(m_currentPath);
                return true;
			}
		}
		return false;
	}

	public void Rescan()
	{
		m_fileList.clear();
		ListFiles(m_currentPath);
	}

	public String CurrentPath() {
		return m_currentPath;
	}

	public List<FileModel> FileList() {
		return m_fileList;
	}

	public FileModel GetFileModel(int index)
	{
		if(index >= m_fileList.size())
			return null;
		return m_fileList.get(index);
	}

	public boolean ShowHidden() {
		return m_showHidden;
	}

	public FileBrowser SetShowHidden(boolean showHidden) {
		if(m_showHidden != showHidden)
		{
			this.m_showHidden = showHidden;
			ListFiles(m_currentPath);
		}
		return this;
	}

	public FileBrowser SetIgnoreDotDot(boolean b) {
		if(m_ignoreDotDot != b)
		{
			this.m_ignoreDotDot = b;
			ListFiles(m_currentPath);
		}
		return this;
	}

	public FileBrowser SetOrder(int i) {
		if(m_order != i)
		{
			this.m_order = i;
			ListFiles(m_currentPath);
		}
		return this;
	}

	public FileBrowser SetSequence(int i) {
		if(m_sequence != i)
		{
			this.m_sequence = i;
			ListFiles(m_currentPath);
		}
		return this;
	}

	public class FileModel
	{
		public static final int ID_FILE_TYPE_FILE = 0;
		public static final int ID_FILE_TYPE_DIRECTORY = 1;
		public static final int ID_FILE_TYPE_SYMBOL = 2;

		public String path;
		public String name;
		public long size;
		public int type;
		public String permission;
		public long time;
        
        public boolean IsDirectory()
        {
            return type == ID_FILE_TYPE_DIRECTORY;
        }
	}

	private Comparator<FileModel> m_fileComparator = new Comparator<FileModel>(){
		@Override
		public int compare(FileModel a, FileModel b) {
			if("./".equals(a.name))
				return -1;
			if("../".equals(a.name))
				return -1;
			if(a.type != b.type)
			{
				if(a.type == FileModel.ID_FILE_TYPE_DIRECTORY)
					return -1;
				if(b.type == FileModel.ID_FILE_TYPE_DIRECTORY)
					return 1;
			}

			int res = 0;
			if(m_order == ID_ORDER_BY_TIME)
				res = Long.signum(a.time - b.time);
			else if(m_order == ID_ORDER_BY_NAME)
				res = a.name.compareToIgnoreCase(b.name);

			if(m_sequence == ID_SEQUENCE_DESC)
				res = -res;

			return res;
		}
	};
}
