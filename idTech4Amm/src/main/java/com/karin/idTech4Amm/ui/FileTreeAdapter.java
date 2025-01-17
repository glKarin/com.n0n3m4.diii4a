package com.karin.idTech4Amm.ui;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.graphics.Color;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.TextView;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.misc.FileBrowser;
import com.karin.idTech4Amm.sys.Theme;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;

public class FileTreeAdapter extends ArrayAdapter_base<FileTreeAdapter.FileTreeModel>
{
    private static final String TAG = "FileTreeAdapter";
    private static final int INDENT_LENGTH = 50;

    private final Set<FileTreeModel> m_list = new TreeSet<>();

    private FileTreeAdapterListener m_listener;

    private final FileBrowser m_fileBrowser;

    public FileTreeAdapter(Context context, String path)
    {
        super(context, R.layout.file_tree_delegate);
        m_fileBrowser = new FileBrowser(context);
        m_fileBrowser.SetIgnoreDotDot(true);
        m_fileBrowser.SetListener(new FileBrowser.Listener() {
            @Override
            public void OnPathCannotAccess(String path)
            {
                if(null != m_listener)
                    m_listener.OnGrantPermission(path);
            }
        });

        SetData(new ArrayList<>());

        if(null != path && !path.isEmpty())
            SetPath(path);
    }

    public View GenerateView(int position, View view, ViewGroup parent, FileTreeAdapter.FileTreeModel data)
    {
        TextView textView;
        CheckBox selectView;
        TextView arrowView;
        TextView indentView;

        textView = view.findViewById(R.id.file_tree_name);
        textView.setText(data.name);
        textView.setTextColor(data.IsDirectory() ? Theme.BlackColor(getContext()) : Color.GRAY);

        arrowView = view.findViewById(R.id.file_tree_arrow);
        arrowView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v)
            {
                if(data.IsDirectory())
                {
                    if(data.expanded)
                        ClosePath(position, data);
                    else
                        OpenPath(position, data);
                }
            }
        });
        if(data.IsDirectory())
        {
            arrowView.setText(">");
            arrowView.setVisibility(View.VISIBLE);
            arrowView.setRotation(data.expanded ? 90 : 0);
        }
        else
        {
            arrowView.setText(" ");
            arrowView.setVisibility(View.INVISIBLE);
        }

        indentView = view.findViewById(R.id.file_tree_indent);
        indentView.setWidth(data.level * INDENT_LENGTH);

        selectView = view.findViewById(R.id.file_tree_checkbox);
        selectView.setOnCheckedChangeListener(null);
        selectView.setChecked(data.selected);
        selectView.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                Select(position, data, isChecked);
            }
        });

        return view;
    }

    private void Select(int index, FileTreeAdapter.FileTreeModel file, boolean selected)
    {
        if(file.type != FileBrowser.FileModel.ID_FILE_TYPE_FILE)
            return;

        if(selected)
            m_list.add(file);
        else
            m_list.remove(file);

        // Update(index);
    }

    public List<String> GetSelectFiles(String prefix)
    {
        return null;
    }

    private List<FileTreeAdapter.FileTreeModel> ConvertFiles(FileTreeAdapter.FileTreeModel parent, List<FileBrowser.FileModel> fileModels)
    {
        List<FileTreeAdapter.FileTreeModel> list = new ArrayList<>();
        if(null == fileModels)
            return list;
        for(FileBrowser.FileModel fileModel : fileModels)
        {
            FileTreeModel fileTreeModel = new FileTreeModel(fileModel);
            if(null != parent)
            {
                fileTreeModel.level = parent.level + 1;
            }
            else
            {
                fileTreeModel.level = 0;
            }
            fileTreeModel.expanded = false;
            list.add(fileTreeModel);
        }
        return list;
    }

    public void SetPath(String path)
    {
        boolean res;

        Log.d(TAG, "Open directory: " + path);
        res = m_fileBrowser.SetCurrentPath(path);

        m_list.clear();
        clear();
        if(res)
        {
            addAll(ConvertFiles(null, m_fileBrowser.FileList()));
        }
    }

    private void ClosePath_r(FileTreeAdapter.FileTreeModel file)
    {
        if(!file.IsDirectory())
            return;

        if(!file.expanded)
            return;

        Log.d(TAG, "UnExpand: " + file.path);

        for(FileTreeModel child : file.children)
        {
            int i = FindIndex(child);
            if(i == -1)
                continue;
            if(child.IsDirectory())
            {
                if(child.expanded)
                {
                    ClosePath_r(child);
                    child.expanded = false;
                }
            }
            remove(child);
        }
        file.expanded = false;
    }

    private void ClosePath(int index, FileTreeAdapter.FileTreeModel file)
    {
        if(!file.IsDirectory())
            return;

        if(!file.expanded)
            return;

        ClosePath_r(file);

        Update(index);
    }

    private void OpenPath(int index, FileTreeAdapter.FileTreeModel file)
    {
        boolean res;

        if(!file.IsDirectory())
            return;

        if(file.expanded)
            return;

        Log.d(TAG, "Expand: " + file.path);

        List<FileTreeAdapter.FileTreeModel> fileTreeModels = null;

        if(null != file.children)
        {
            file.expanded = true;
            fileTreeModels = file.children;
            for(FileTreeModel fileTreeModel : fileTreeModels)
            {
                fileTreeModel.expanded = false;
            }
        }
        else
        {
            res = m_fileBrowser.SetCurrentPath(file.path);

            if(res)
            {
                file.expanded = true;
                fileTreeModels = ConvertFiles(file, m_fileBrowser.FileList());
            }
            // else fileTreeModels = new ArrayList<>();
            file.children = fileTreeModels;
        }

        if(null != fileTreeModels)
            Insert(index + 1, fileTreeModels);

        Update(index);
    }

    public FileBrowser GetFileBrowser()
    {
        return m_fileBrowser;
    }

    public interface FileTreeAdapterListener
    {
        public void OnGrantPermission(String path);
    }

    public void SetListener(FileTreeAdapterListener l)
    {
        m_listener = l;
    }


    public static class FileTreeModel extends FileBrowser.FileModel
    {
        public boolean expanded;
        public boolean selected;
        public int level;
        public List<FileTreeModel> children;

        public FileTreeModel() {}

        public FileTreeModel(FileBrowser.FileModel fileModel)
        {
            path = fileModel.path;
            name = fileModel.name;
            size = fileModel.size;
            type = fileModel.type;
            time = fileModel.time;
            permission = fileModel.permission;
        }
    }
}
