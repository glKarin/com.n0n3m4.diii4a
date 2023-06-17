package com.karin.idTech4Amm.lib;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.Fragment;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.graphics.Point;
import android.os.Environment;
import android.preference.PreferenceFragment;
import android.provider.Settings;
import android.content.Intent;
import android.net.Uri;
import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.ApplicationInfo;
import android.os.Build;
import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.util.Log;
import android.view.Display;
import android.view.DisplayCutout;
import android.view.WindowInsets;
import android.widget.TextView;
import android.text.util.Linkify;
import android.text.method.LinkMovementMethod;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.misc.TextHelper;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.function.Consumer;
// import android.widget.Magnifier.Builder;

/**
 * Android context/activity utility
 */
public final class ContextUtility
{
    public static final int CHECK_PERMISSION_RESULT_GRANTED = 0;
    public static final int CHECK_PERMISSION_RESULT_REQUEST = 1;
    public static final int CHECK_PERMISSION_RESULT_REJECT = 2;
    
    public static final int SCREEN_ORIENTATION_AUTOMATIC = 0;
    public static final int SCREEN_ORIENTATION_PORTRAIT = 1;
    public static final int SCREEN_ORIENTATION_LANDSCAPE = 2;
    
    public static AlertDialog.Builder CreateMessageDialogBuilder(Context context, CharSequence title, CharSequence message)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(title);
        builder.setMessage(message);
        builder.setPositiveButton(R.string.ok, new AlertDialog.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) { 
                    dialog.dismiss();
                }
            });
            
        return builder;
    }

    public static AlertDialog OpenMessageDialog(Context context, CharSequence title, CharSequence message)
    {
        AlertDialog.Builder builder = ContextUtility.CreateMessageDialogBuilder(context, title, message);

        AlertDialog dialog = builder.create();
        dialog.show();

        TextView messageText = (TextView)(dialog.findViewById(android.R.id.message));
        if(messageText != null) // never
        {
            if(!TextHelper.USING_HTML)
                messageText.setAutoLinkMask(Linkify.ALL);
            messageText.setMovementMethod(LinkMovementMethod.getInstance());   
        }

        return dialog;
    }
    
    public static void OpenAppSetting(Context context)
    {
        Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
        Uri uri = Uri.fromParts("package", context.getPackageName(), null);
        intent.setData(uri);
        context.startActivity(intent);
    }
    
    public static String GetAppVersion(Context context)
    {
        String version = "UNKNOWN";
        try
        {
            PackageManager manager = context.getPackageManager();
            PackageInfo info = manager.getPackageInfo(context.getPackageName(), 0);
            version = info.versionName;
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        return version;
    }

    public static boolean BuildIsDebug(Context context)
    {
        try
        {
            ApplicationInfo info = context.getApplicationInfo();
            return (info.flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false; // default is release
        }
    }

    public static int CheckPermission(Activity context, String permission, int resultCode)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) // Android M
        {
            boolean granted = context.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
            if(granted)
                return CHECK_PERMISSION_RESULT_GRANTED;
            if (false && !context.shouldShowRequestPermissionRationale(permission)) // do not ask
            {
                ContextUtility.OpenAppSetting(context);
                return CHECK_PERMISSION_RESULT_REJECT; // goto app detail settings activity
            }
            context.requestPermissions(new String[] { permission }, resultCode);
            return CHECK_PERMISSION_RESULT_REQUEST;
        }
        else
            return CHECK_PERMISSION_RESULT_GRANTED; // other think has granted
    }

    public static void SetScreenOrientation(Activity context, int o)
    {
        switch(o)
        {
            case SCREEN_ORIENTATION_LANDSCAPE:
                    context.setRequestedOrientation(Build.VERSION.SDK_INT >= 9 ? ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                    break;
            case SCREEN_ORIENTATION_PORTRAIT:
                    context.setRequestedOrientation(Build.VERSION.SDK_INT >= 9 ? ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT : ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                    break;
            case SCREEN_ORIENTATION_AUTOMATIC:
                default:
                    context.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER);
                    break;
        }
    }
    
    public static void Confirm(Context context, CharSequence title, CharSequence message, final Runnable yes, final Runnable no, final Object...opt)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(title);
        builder.setMessage(message);
        builder.setCancelable(true);
            builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int v)
                    {
                        if(yes != null)
                            yes.run();
                        dialog.dismiss();
                    }
                });   
            builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int v)
                    {
                        if(no != null)
                            no.run();
                        dialog.dismiss();
                    }
                });
        if(opt.length > 0 && null != opt[0])
        {
            final Object arg1 = opt[0];
            if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.N && arg1 instanceof Consumer)
                ((Consumer<AlertDialog.Builder>)arg1).accept(builder);
            else if(arg1 instanceof Runnable)
            {
                builder.setNeutralButton(R.string.other, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int v)
                        {
                            ((Runnable)arg1).run();
                            dialog.dismiss();
                        }
                    });
            }
            else
            {
                builder.setNeutralButton(arg1.toString(), new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int v)
                        {
                            if(null != opt[1])
                                ((Runnable)opt[1]).run();
                            dialog.dismiss();
                        }
                    });
            }
        }
        builder.create().show();
    }
    
    public static String NativeLibDir(Context context)
    {
        try
        {
            ApplicationInfo ainfo = context.getApplicationContext().getPackageManager().getApplicationInfo
            (
                context.getPackageName(),
                PackageManager.GET_SHARED_LIBRARY_FILES
            );
            return ainfo.nativeLibraryDir;
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return "";
        }
    }

    public static int CheckFilePermission(Activity context, int resultCode)
    {
        String permission = Manifest.permission.WRITE_EXTERNAL_STORAGE;
        // Android SDK > 28
/*        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) // Android 11
        {
            if(Environment.isExternalStorageManager())
                return CHECK_PERMISSION_RESULT_GRANTED;
            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
            intent.setData(Uri.parse("package:" + context.getPackageName()));
            context.startActivityForResult(intent, resultCode);
            return CHECK_PERMISSION_RESULT_REQUEST;
        }
        else*/ if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) // Android M - Q
        {
            boolean granted = context.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
            if(granted)
                return CHECK_PERMISSION_RESULT_GRANTED;
            if (false && !context.shouldShowRequestPermissionRationale(permission)) // do not ask
            {
                ContextUtility.OpenAppSetting(context);
                return CHECK_PERMISSION_RESULT_REJECT; // goto app detail settings activity
            }
            context.requestPermissions(new String[] { permission }, resultCode);
            return CHECK_PERMISSION_RESULT_REQUEST;
        }
        else
            return CHECK_PERMISSION_RESULT_GRANTED; // other think has granted
    }

    public static boolean ExtractAsset(Context context, String path, String outPath)
    {
        InputStream is = null;
        FileOutputStream os = null;
        try
        {
            is = context.getAssets().open(path);
            File out = new File(outPath);
            FileUtility.mkdir(out.getParent(), true);
            os = new FileOutputStream(out);

            long res = FileUtility.Copy(os, is);

            return res > 0;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
        finally {
            FileUtility.CloseStream(os);
            FileUtility.CloseStream(is);
        }
    }

    public static boolean ExtractAssetToDirectory(Context context, String path, String outPath)
    {
        InputStream is = null;
        FileOutputStream os = null;
        File f = new File(path);
        String name = f.getName();

        try
        {
            FileUtility.mkdir(outPath, true);
            is = context.getAssets().open(path);
            File out = new File(outPath + File.separator + name);
            os = new FileOutputStream(out);

            long res = FileUtility.Copy(os, is);

            return res > 0;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
        finally {
            FileUtility.CloseStream(os);
            FileUtility.CloseStream(is);
        }
    }

    public static void OpenUrlExternally(Context context, String url)
    {
        Uri uri = Uri.parse(url);
        Intent intent = new Intent(Intent.ACTION_VIEW, uri);
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
        context.startActivity(intent);
    }

    public static void RestartApp(Activity activity)
    {
        activity.finish();
        Intent intent = activity.getPackageManager().getLaunchIntentForPackage(activity.getPackageName());
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        activity.startActivity(intent);
    }

    public static boolean OpenApp(Activity activity, String packageName)
    {
        final Intent intent = activity.getPackageManager().getLaunchIntentForPackage(packageName);
        if(null == intent)
            return false;
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
        activity.startActivity(intent);
        return true;
    }

    public static Context GetContext(Fragment fragment)
    {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M)
            return fragment.getContext();
        else
            return fragment.getActivity();
    }
    
	private ContextUtility() {}
}
