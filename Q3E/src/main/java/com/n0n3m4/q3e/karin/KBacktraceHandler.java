package com.n0n3m4.q3e.karin;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Build;

import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EUtils;

import java.io.File;
import java.io.FileWriter;
import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * backtrace handler
 */
public class KBacktraceHandler
{
    public static final String CONST_BACKTRACE = "_BACKTRACE";

    private Context m_context;
    @SuppressLint("StaticFieldLeak")
    private static KBacktraceHandler               _instance = null;

    private KBacktraceHandler() {}

    public String Backtrace(int signnum, int pid, int tid, int mask, String[] strs)
    {
        try
        {
            String str = BacktraceStr(signnum, pid, tid, mask, strs);
            WriteToPreference(str);
            WriteToStorage(str);
            return str;
        }
        catch (Exception ex)
        {
            ex.printStackTrace();
            return null;
        }
    }

    public static String BacktraceStr(int signnum, int pid, int tid, int mask, String[] strs)
    {
        StringBuilder buf = new StringBuilder();
        buf.append("********** backtrace **********\n");
        buf.append("----- Time: ").append(Q3EUtils.date_format("yyyy-MM-dd HH:mm:ss")).append('\n');
        buf.append("----- Signal: ").append(signnum);
        buf.append(", pid=").append(pid);
        buf.append(", tid=").append(tid);
        buf.append("\n\n");

        if((mask & 1) != 0)
        {
            buf.append("----- CFI (Call Frame Info)\n");
            buf.append(null != strs[0] ? strs[0] : "<NULL>");
            buf.append('\n');
        }

        if((mask & 2) != 0)
        {
            buf.append("----- FP (Frame Pointer)\n");
            buf.append(null != strs[1] ? strs[1] : "<NULL>");
            buf.append('\n');
        }

        if((mask & 4) != 0)
        {
            buf.append("----- EH (Exception handling GCC extension)\n");
            buf.append(null != strs[2] ? strs[2] : "<NULL>");
            buf.append('\n');
        }

        buf.append("********** END **********\n");
        return buf.toString();
    }

    private String GetDumpPath()
    {
        if(null == m_context)
            return null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            return m_context.getDataDir().getAbsolutePath();
        else
            return m_context.getCacheDir().getAbsolutePath();
    }

    private String GetBacktraceDumpFile()
    {
        String dir = GetDumpPath();
        if(null == dir)
            return null;
        return dir + "/" + CONST_BACKTRACE;
    }

    public static KBacktraceHandler Instance()
    {
        return _instance;
    }

    public static String GetDumpExceptionContent()
    {
        if(null == _instance)
            return "";
        try
        {
            String str;

            str = Q3EUtils.file_get_contents(_instance.GetBacktraceDumpFile());
            return null != str ? str : "";
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return "";
        }
    }

    public static boolean ClearDumpBacktraceContent()
    {
        if(null == _instance)
            return false;
        try
        {
            return Q3EUtils.rm(_instance.GetBacktraceDumpFile());
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    private boolean WriteToPreference(String str)
    {
        if(null == m_context)
            return false;
        try
        {
            Q3EUtils.file_put_contents(GetBacktraceDumpFile(), str);
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    private boolean WriteToStorage(String str)
    {
        FileWriter out = null;
        try
        {
            String fileName = String.format("%s_%s.backtrace.log", Q3EGlobals.CONST_APP_NAME, new SimpleDateFormat("yyyy-MM-dd HH-mm-ss-SSS").format(new Date()));
            String logPath = KStr.AppendPath(Q3E.q3ei.app_storage_path, Q3EGlobals.FOLDER_BACKTRACE_LOG);
            File dir = new File(logPath);
            if(!dir.exists())
                dir.mkdirs();
            out = new FileWriter(KStr.AppendPath(logPath, fileName));
            out.append(str);
            out.flush();
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        finally
        {
            try
            {
                if(null != out)
                    out.close();
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }
        return false;
    }

    private void Register(Context context)
    {
        if(null != m_context)
            return;
        if(null == context)
            return;
        m_context = context;
    }

    private void Unregister()
    {
        if(null == m_context)
            return;
        m_context = null;
    }

    public static void HandleBacktrace(Context context)
    {
        try
        {
            if(null == _instance)
                _instance = new KBacktraceHandler();
            else
                _instance.Unregister();
            _instance.Register(context);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}
