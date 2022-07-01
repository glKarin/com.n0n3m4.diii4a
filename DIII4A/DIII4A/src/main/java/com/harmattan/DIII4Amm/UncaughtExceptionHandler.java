package com.harmattan.DIII4Amm;
import android.content.Context;
import java.text.SimpleDateFormat;
import java.util.Date;
import android.app.ActivityManager;
import android.os.Build;
import android.os.Debug;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.os.Process;

public class UncaughtExceptionHandler implements Thread.UncaughtExceptionHandler
{
    private Context m_context;
    
    public UncaughtExceptionHandler(Context context)
    {
        m_context = context;
    }

    @Override
    public void uncaughtException(Thread t, Throwable e) {
        try
        {
            String str = ExceptionStr(m_context, t, e);
            SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(m_context).edit();
            editor.putString(Constants.CONST_PREFERENCE_APP_CRASH_INFO, str);
            editor.commit();
        }
        catch (Exception ex)
        {
            ex.printStackTrace();
        }
        finally {
            Thread.UncaughtExceptionHandler defaultUncaughtExceptionHandler = Thread.getDefaultUncaughtExceptionHandler();
            if(defaultUncaughtExceptionHandler != null)
                defaultUncaughtExceptionHandler.uncaughtException(t, e);
        }
    }
    
    public static String ExceptionStr(Context context, Thread t, Throwable e)
    {
            StringBuilder sb = new StringBuilder();
            StackTraceElement[] arr = e.getStackTrace();

            sb.append("********** DUMP **********\n");
            sb.append("----- Time: " + new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(new Date())).append('\n');
            sb.append('\n');

            sb.append("----- Thread: " + t).append('\n');
            sb.append("\tID: " + t.getId()).append('\n');
            sb.append("\tName: " + t.getName()).append('\n');
            sb.append('\n');

            sb.append("----- Throwable: " + e).append('\n');
            sb.append("\tInfo: " + e.getMessage()).append('\n');
            sb.append("\tStack: ").append('\n');
            for(StackTraceElement ste : arr)
            {
                sb.append("\t\t" + ste.toString()).append('\n');
            }
            sb.append('\n');

            sb.append("----- Memory:").append('\n');
            ActivityManager am = (ActivityManager)(context.getSystemService(Context.ACTIVITY_SERVICE));
            int[] processs = {Process.myPid()};
            ActivityManager.MemoryInfo outInfo = new ActivityManager.MemoryInfo();
            am.getMemoryInfo(outInfo);

            sb.append("\tSystem: ").append('\n');
            sb.append("\t\tAvail: " + outInfo.availMem + " bytes").append('\n');
            if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) // 16
            {
                sb.append("\t\tTotal: " + outInfo.totalMem + " bytes").append('\n');
            }

            sb.append("\tApplication: ").append('\n');
            Debug.MemoryInfo memInfos[] = am.getProcessMemoryInfo(processs);
            Debug.MemoryInfo memInfo = memInfos[0];
            if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) // 23
            {
                sb.append("\t\tNative heap: " + memInfo.getMemoryStat("summary.native-heap") + " kb").append('\n');
                sb.append("\t\tDalvik heap: " + memInfo.getMemoryStat("summary.java-heap") + " kb").append('\n');
                sb.append("\t\tGraphics: " + memInfo.getMemoryStat("summary.graphics") + " kb").append('\n');   
                sb.append("\t\tStack: " + memInfo.getMemoryStat("summary.stack") + " kb").append('\n');
            }
            else
            {
                sb.append("\t\tNative heap: " + memInfo.nativePrivateDirty + " kb").append('\n');
                sb.append("\t\tDalvik heap: " + memInfo.dalvikPrivateDirty + " kb").append('\n');
            }
            sb.append('\n');

            sb.append("********** END **********\n");
            sb.append("Application exit.\n");
            return sb.toString();
    }
    
    public static void DumpException(Context context, Thread t, Throwable e) {
        try
        {
            String str = ExceptionStr(context, t, e);
            SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(context).edit();
            editor.putString(Constants.CONST_PREFERENCE_EXCEPTION_DEBUG, str);
            editor.commit();
        }
        catch (Exception ex)
        {
            ex.printStackTrace();
        }
    }
}
