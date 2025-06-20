package com.n0n3m4.q3e;

import android.content.Context;

import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.karin.KStr;

import java.util.Arrays;

public abstract class Q3EPatchResource
{
    public final Q3EGameConstants.PatchResource type;
    public final String                   name;
    public final String                   version;
    public final String                   game;
    public final String                   mod;
    public final String                   assetPath;
    public final String                   fsPath;

    public Q3EPatchResource(Q3EGameConstants.PatchResource type, String name, String version, String game, String mod, String assetPath, String fsPath)
    {
        this.type = type;
        this.name = name;
        this.version = version;
        this.game = game;
        this.mod = mod;
        this.assetPath = assetPath;
        this.fsPath = fsPath;
    }

    public abstract String Fetch(Context context, boolean overwrite, String... fsgame);

    protected String MakeOutPath(String... fsgame)
    {
        String path = Q3EInterface.GetStandaloneDirectory(Q3EUtils.q3ei.standalone, game);
        if(null == fsPath)
            path = KStr.AppendPath(path, null != fsgame && fsgame.length > 0 ? fsgame[0] : null);
        else if(fsPath.isEmpty())
            path = KStr.AppendPath(path, mod);
        else
            path = KStr.AppendPath(path, fsPath);

        return KStr.AppendPath(Q3EUtils.q3ei.datadir, path);
    }
}

// public static final int COPY_FILE_TO_FILE     = 1; // copy assets/patch.pk4 -> dirname/zzz.pk4
class Q3EPatchResource_fileToFile extends Q3EPatchResource
{
    private final String prefix;

    public Q3EPatchResource_fileToFile(Q3EGameConstants.PatchResource type, String name, String version, String game, String mod, String assetPath, String fsPath, String prefix)
    {
        super(type, name, version, game, mod, assetPath, fsPath);
        this.prefix = prefix;
    }

    public String Fetch(Context context, boolean overwrite, String... fsgame)
    {
        return CopyFileToFile(context, overwrite, fsgame);
    }

    private String CopyFileToFile(Context context, boolean overwrite, String... fsgame)
    {
        String toPath = MakeOutPath(fsgame);
        String target = toPath;
        if(KStr.NotEmpty(prefix))
            target = KStr.AppendPath(toPath, prefix + KStr.Filename(assetPath));
        KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying file '%s' to file '%s'", assetPath, target);
        return Q3EUtils.ExtractCopyFile(context, assetPath, target, overwrite) ? target : null;
    }
}

// public static final int COPY_FILE_TO_DIR      = 2; // copy assets/patch.pk4 -> dirname/
class Q3EPatchResource_fileToDir extends Q3EPatchResource
{
    public Q3EPatchResource_fileToDir(Q3EGameConstants.PatchResource type, String name, String version, String game, String mod, String assetPath, String fsPath)
    {
        super(type, name, version, game, mod, assetPath, fsPath);
    }

    public String Fetch(Context context, boolean overwrite, String... fsgame)
    {
        return CopyFileToDir(context, overwrite, fsgame);
    }

    private String CopyFileToDir(Context context, boolean overwrite, String... fsgame)
    {
        String toPath = MakeOutPath(fsgame);
        KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying file '%s' to directory '%s/'", assetPath, toPath);
        return Q3EUtils.ExtractCopyFile(context, assetPath, toPath + "/", overwrite) ? toPath : null;
    }
}

// public static final int COPY_DIR_FILES_TO_DIR = 3; // copy assets/ :: patch.pk4, other/other.pk4, ... -> dirname/
class Q3EPatchResource_filesToDir extends Q3EPatchResource
{
    public final String[] assetFiles;

    public Q3EPatchResource_filesToDir(Q3EGameConstants.PatchResource type, String name, String version, String game, String mod, String assetPath, String fsPath, String... assetFiles)
    {
        super(type, name, version, game, mod, assetPath, fsPath);

        if(null != assetFiles)
        {
            this.assetFiles = new String[assetFiles.length];
            System.arraycopy(assetFiles, 0, this.assetFiles, 0, assetFiles.length);
        }
        else
            this.assetFiles = null;
    }

    public String Fetch(Context context, boolean overwrite, String... fsgame)
    {
        return CopyDirFilesToDir(context, overwrite, fsgame);
    }

    private String CopyDirFilesToDir(Context context, boolean overwrite, String... fsgame)
    {
        String toPath = MakeOutPath(fsgame);
        KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying directory '%s' files '%s' to directory '%s/'", assetPath, Arrays.toString(assetFiles), toPath);
        return Q3EUtils.ExtractCopyDirFiles(context, assetPath, toPath, overwrite, assetFiles) ? toPath : null;
    }
}

// public static final int EXTRACT_ZIP_TO_DIR    = 4; // extract assets/patch.zip -> dirname/
class Q3EPatchResource_zipToDir extends Q3EPatchResource
{
    public Q3EPatchResource_zipToDir(Q3EGameConstants.PatchResource type, String name, String version, String game, String mod, String assetPath, String fsPath)
    {
        super(type, name, version, game, mod, assetPath, fsPath);
    }

    public String Fetch(Context context, boolean overwrite, String... fsgame)
    {
        return ExtractZipToDir(context, overwrite, fsgame);
    }


    private String ExtractZipToDir(Context context, boolean overwrite, String... fsgame)
    {
        String toPath = MakeOutPath(fsgame);
        KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Extracting zip file '%s' to directory '%s/'", assetPath, toPath);
        return Q3EUtils.ExtractZip(context, assetPath, toPath, overwrite) ? toPath : null;
    }
}

// public static final int COPY_DIR_TO_DIR       = 5; // copy assets/ -> dirname/
class Q3EPatchResource_dirToDir extends Q3EPatchResource
{
    public Q3EPatchResource_dirToDir(Q3EGameConstants.PatchResource type, String name, String version, String game, String mod, String assetPath, String fsPath)
    {
        super(type, name, version, game, mod, assetPath, fsPath);
    }

    public String Fetch(Context context, boolean overwrite, String... fsgame)
    {
        return CopyDirToDir(context, overwrite, fsgame);
    }

    private String CopyDirToDir(Context context, boolean overwrite, String... fsgame)
    {
        String toPath = MakeOutPath(fsgame);
        KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying directory '%s' to directory '%s/'", assetPath, toPath);
        return Q3EUtils.ExtractCopyDir(context, assetPath, toPath, overwrite) ? toPath : null;
    }
}

// public static final int EXTRACT_ZIP_TO_DIR    = 4; // extract assets/patch.zip -> dirname/
class Q3EPatchResource_zipToZip extends Q3EPatchResource
{
    public final String   filename;
    public final String[] files;

    public Q3EPatchResource_zipToZip(Q3EGameConstants.PatchResource type, String name, String version, String game, String mod, String assetPath, String fsPath, String filename, String... files)
    {
        super(type, name, version, game, mod, assetPath, fsPath);
        this.filename = filename;

        if(null != files)
        {
            this.files = new String[files.length];
            System.arraycopy(files, 0, this.files, 0, files.length);
        }
        else
            this.files = null;
    }

    public String Fetch(Context context, boolean overwrite, String... fsgame)
    {
        return ExtractZipToZip(context, overwrite, fsgame);
    }


    private String ExtractZipToZip(Context context, boolean overwrite, String... fsgame)
    {
        String toPath = MakeOutPath(fsgame);
        String target = toPath;
        if(KStr.NotEmpty(filename))
            target = KStr.AppendPath(toPath, filename);
        KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Extracting zip file '%s' to zip '%s'", assetPath, target);
        return Q3EUtils.ExtractZipToZip(context, assetPath, target, overwrite, files) ? toPath : null;
    }
}
