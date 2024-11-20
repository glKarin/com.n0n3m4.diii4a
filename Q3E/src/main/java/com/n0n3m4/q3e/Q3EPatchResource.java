package com.n0n3m4.q3e;

import android.app.Activity;
import android.content.Context;
import android.util.Log;

import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.karin.KStr;

import java.util.Arrays;

public class Q3EPatchResource
{
    public static final int COPY_FILE_TO_FILE     = 1; // copy assets/patch.pk4 -> dirname/zzz.pk4
    public static final int COPY_FILE_TO_DIR      = 2; // copy assets/patch.pk4 -> dirname/
    public static final int COPY_DIR_FILES_TO_DIR = 3; // copy assets/ :: patch.pk4, other/other.pk4, ... -> dirname/
    public static final int EXTRACT_ZIP_TO_DIR    = 4; // extract assets/patch.zip -> dirname/

    public final Q3EGlobals.PatchResource type;
    public final String                   name;
    public final String                   version;
    public final String                   game;
    public final String                   mod;
    public final int                      extract;
    public final String                   assetPath;
    public final String                   fsPath;
    public final String[]                 assetFiles;

    public Q3EPatchResource(Q3EGlobals.PatchResource type, String name, String version, String game, String mod, int extract, String assetPath, String fsPath, String...assetFiles)
    {
        this.type = type;
        this.name = name;
        this.version = version;
        this.game = game;
        this.mod = mod;
        this.extract = extract;
        this.assetPath = assetPath;
        this.fsPath = fsPath;
        if(null != assetFiles)
        {
            this.assetFiles = new String[assetFiles.length];
            System.arraycopy(assetFiles, 0, this.assetFiles, 0, assetFiles.length);
        }
        else
            this.assetFiles = null;
    }

    public String Fetch(Context context, boolean overwrite, String...fsgame)
    {
        switch(extract)
        {
            case COPY_FILE_TO_FILE:
                return CopyFileToFile(context, overwrite, fsgame);
            case COPY_FILE_TO_DIR:
                return CopyFileToDir(context, overwrite, fsgame);
            case COPY_DIR_FILES_TO_DIR:
                return CopyDirFilesToDir(context, overwrite, fsgame);
            case EXTRACT_ZIP_TO_DIR:
                return ExtractZipToDir(context, overwrite, fsgame);
            default:
                throw new RuntimeException("Unexcept extract type:" + extract);
        }
    }

    private String MakeOutPath(String...fsgame)
    {
        String path = Q3EInterface.GetStandaloneDirectory(Q3EUtils.q3ei.standalone, game);
        if(null == fsPath)
            path = KStr.AppendPath(path, null != fsgame ? fsgame[0] : null);
        else if(fsPath.isEmpty())
            path = KStr.AppendPath(path, mod);
        else
            path = KStr.AppendPath(path, fsPath);

        return KStr.AppendPath(Q3EUtils.q3ei.datadir, path);
    }

    private String CopyFileToFile(Context context, boolean overwrite, String...fsgame)
    {
        String toPath = MakeOutPath(fsgame);
        KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying file '%s' to file '%s'", assetPath, toPath);
        return Q3EUtils.ExtractCopyFile(context, assetPath, toPath, overwrite) ? toPath : null;
    }

    private String CopyFileToDir(Context context, boolean overwrite, String...fsgame)
    {
        String toPath = MakeOutPath(fsgame);
        KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying file '%s' to directory '%s/'", assetPath, toPath);
        return Q3EUtils.ExtractCopyFile(context, assetPath, toPath + "/", overwrite) ? toPath : null;
    }

    private String CopyDirFilesToDir(Context context, boolean overwrite, String...fsgame)
    {
        String toPath = MakeOutPath(fsgame);
        KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying directory '%s' files '%s' to directory '%s/'", assetPath, Arrays.toString(assetFiles), toPath);
        return Q3EUtils.ExtractCopyDir(context, assetPath, toPath, overwrite, assetFiles) ? toPath : null;
    }

    private String ExtractZipToDir(Context context, boolean overwrite, String...fsgame)
    {
        String toPath = MakeOutPath(fsgame);
        KLog.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Extracting zip file '%s' to directory '%s/'", assetPath, toPath);
        return Q3EUtils.ExtractZip(context, assetPath, toPath, overwrite) ? toPath : null;
    }
}
