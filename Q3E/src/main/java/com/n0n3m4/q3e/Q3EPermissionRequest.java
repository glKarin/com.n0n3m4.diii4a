package com.n0n3m4.q3e;

import android.content.pm.PackageManager;

public final class Q3EPermissionRequest
{
    public String permission;
    public int requestCode;
    public int result;

    public void Request(String perm, int code)
    {
        permission = perm;
        requestCode = code;
        result = PackageManager.PERMISSION_DENIED;
    }

    public void Result(int res)
    {
        result = res;
    }

    public void Result(boolean res)
    {
        result = res ? PackageManager.PERMISSION_GRANTED : PackageManager.PERMISSION_DENIED;
    }

    public void Reset()
    {
        permission = null;
        requestCode = 0;
        result = PackageManager.PERMISSION_DENIED;
    }

    public boolean IsGranted()
    {
        return result == PackageManager.PERMISSION_GRANTED;
    }
}
