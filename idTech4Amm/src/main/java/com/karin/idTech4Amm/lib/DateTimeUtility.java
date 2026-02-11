package com.karin.idTech4Amm.lib;

import com.n0n3m4.q3e.Q3EUtils;

import java.text.SimpleDateFormat;
import java.util.Date;

public final class DateTimeUtility
{
    public static String Format(long ts, String format)
    {
        return Q3EUtils.date_format(format, new Date(ts));
    }

    private DateTimeUtility() {}
}
