package su.xash.hlsdk;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        String pkg = "su.xash.engine.test";

        try {
            getPackageManager().getPackageInfo(pkg, 0);
        } catch (PackageManager.NameNotFoundException e) {
            try {
                pkg = "su.xash.engine";
                getPackageManager().getPackageInfo(pkg, 0);
            } catch (PackageManager.NameNotFoundException ex) {
                startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/FWGS/xash3d-fwgs/releases/tag/continuous")).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK));
                finish();
                return;
            }
        }

        startActivity(new Intent().setComponent(new ComponentName(pkg, "su.xash.engine.XashActivity"))
                .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK)
                .putExtra("gamedir", "valve")
                .putExtra("gamelibdir", getApplicationInfo().nativeLibraryDir)
                .putExtra("package", getPackageName()));
        finish();
    }
}
