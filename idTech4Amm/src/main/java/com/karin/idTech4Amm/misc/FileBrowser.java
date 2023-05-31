package com.karin.idTech4Amm.misc;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class FileBrowser
{
    public static final int ID_ORDER_BY_NAME = 1;
    public static final int ID_ORDER_BY_TIME = 2;

    public static final int ID_SEQUENCE_ASC = 1;
    public static final int ID_SEQUENCE_DESC = 2;

    private String m_currentPath;
    private final Set<String> m_history = new HashSet<>();
    private final List<FileBrowser.FileModel> m_fileList = new ArrayList<>();
    private int m_sequence = ID_SEQUENCE_ASC;
    private int m_filter = 0;
    private int m_order = ID_ORDER_BY_NAME;
    private List<String> m_extensions = new ArrayList<>();
    private boolean m_showHidden = true;
    private boolean m_ignoreDotDot = false;

    public FileBrowser()
    {
        this(System.getProperty("user.home"));
    }

    public FileBrowser(String path)
    {
        if (path == null || path.isEmpty())
            SetCurrentPath(path);
    }

    protected boolean ListFiles(String path)
    {
        File dir;
        File[] files;
        FileBrowser.FileModel item;

        if (path == null || path.isEmpty())
            return false;

        dir = new File(path);
        if (!dir.isDirectory())
            return false;

        files = dir.listFiles();
        if (files == null)
        {
            if (!path.equals(m_currentPath))
            {
                m_currentPath = path;
                m_fileList.clear();
            }
            return false;
        }

        m_fileList.clear();

        for (File f : files)
        {
            String name = f.getName();
            if (".".equals(name))
                continue;
            if (f.isDirectory())
                name += File.separator;

            item = new FileBrowser.FileModel();
            item.name = name;
            item.path = f.getAbsolutePath();
            item.size = f.length();
            item.time = f.lastModified();
            item.type = f.isDirectory() ? FileBrowser.FileModel.ID_FILE_TYPE_DIRECTORY : FileBrowser.FileModel.ID_FILE_TYPE_FILE;
            m_fileList.add(item);
        }

        Collections.sort(m_fileList, m_fileComparator);
        //m_fileList.sort(m_fileComparator);

        // add parent directory
        if (!m_ignoreDotDot)
        {
            item = new FileBrowser.FileModel();
            item.name = "../";
            item.path = dir.getParent();
            item.size = dir.length();
            item.time = dir.lastModified();
            item.type = dir.isDirectory() ? FileBrowser.FileModel.ID_FILE_TYPE_DIRECTORY : FileBrowser.FileModel.ID_FILE_TYPE_FILE;
            m_fileList.add(0, item);
        }
        m_currentPath = path;

        return true;
    }

    public boolean SetCurrentPath(String path)
    {
        if (path != null && !path.equals(m_currentPath))
        {
            if (ListFiles(path))
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

    public String CurrentPath()
    {
        return m_currentPath;
    }

    public List<FileBrowser.FileModel> FileList()
    {
        return m_fileList;
    }

    public FileBrowser.FileModel GetFileModel(int index)
    {
        if (index >= m_fileList.size())
            return null;
        return m_fileList.get(index);
    }

    public boolean ShowHidden()
    {
        return m_showHidden;
    }

    public FileBrowser SetShowHidden(boolean showHidden)
    {
        if (m_showHidden != showHidden)
        {
            this.m_showHidden = showHidden;
            ListFiles(m_currentPath);
        }
        return this;
    }

    public FileBrowser SetIgnoreDotDot(boolean b)
    {
        if (m_ignoreDotDot != b)
        {
            this.m_ignoreDotDot = b;
            ListFiles(m_currentPath);
        }
        return this;
    }

    public FileBrowser SetOrder(int i)
    {
        if (m_order != i)
        {
            this.m_order = i;
            ListFiles(m_currentPath);
        }
        return this;
    }

    public FileBrowser SetSequence(int i)
    {
        if (m_sequence != i)
        {
            this.m_sequence = i;
            ListFiles(m_currentPath);
        }
        return this;
    }

    public static class FileModel
    {
        public static final int ID_FILE_TYPE_FILE = 0;
        public static final int ID_FILE_TYPE_DIRECTORY = 1;
        public static final int ID_FILE_TYPE_SYMBOL = 2;

        public String path;
        public String name;
        public long size;
        public int type;
        public long time;
        public String permission;

        public boolean IsDirectory()
        {
            return type == ID_FILE_TYPE_DIRECTORY;
        }
    }

    private Comparator<FileBrowser.FileModel> m_fileComparator = new Comparator<FileBrowser.FileModel>()
    {
        @Override
        public int compare(FileBrowser.FileModel a, FileBrowser.FileModel b)
        {
            if ("./".equals(a.name))
                return -1;
            if ("../".equals(a.name))
                return -1;
            if (a.type != b.type)
            {
                if (a.type == FileBrowser.FileModel.ID_FILE_TYPE_DIRECTORY)
                    return -1;
                if (b.type == FileBrowser.FileModel.ID_FILE_TYPE_DIRECTORY)
                    return 1;
            }

            int res = 0;
            if (m_order == ID_ORDER_BY_TIME)
                res = Long.signum(a.time - b.time);
            else if (m_order == ID_ORDER_BY_NAME)
                res = a.name.compareToIgnoreCase(b.name);

            if (m_sequence == ID_SEQUENCE_DESC)
                res = -res;

            return res;
        }
    };
}
