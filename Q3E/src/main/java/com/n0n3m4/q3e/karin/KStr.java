package com.n0n3m4.q3e.karin;

import java.io.File;
import java.util.Collection;
import java.util.Objects;

public final class KStr
{

    public static boolean IsEmpty(String str)
    {
        return null == str || str.isEmpty();
    }

    public static boolean NotEmpty(String str)
    {
        return null != str && !str.isEmpty();
    }

    public static boolean IsBlank(String str)
    {
        return null == str || str.trim().isEmpty();
    }

    public static boolean NotBlank(String str)
    {
        return null != str && !str.trim().isEmpty();
    }

    public static String TrimRight(String str, char ch)
    {
        if(null == str)
            return null;
        if(str.isEmpty())
            return "";
        int i = str.length();
        while (i > 0)
        {
            if(str.charAt(i - 1) != ch)
                break;
            i--;
        }
        if(i <= 0)
            return "";
        else if(i >= str.length())
            return str;
        else
            return str.substring(0, i);
    }

    public static String TrimLeft(String str, char ch)
    {
        if(null == str)
            return null;
        if(str.isEmpty())
            return "";
        int i = 0;
        int length = str.length();
        while (i < length)
        {
            if(str.charAt(i) != ch)
                break;
            i++;
        }
        if(i <= 0)
            return str;
        else if(i >= length)
            return "";
        else
            return str.substring(i);
    }

    public static void TrimRight(StringBuilder str, char ch)
    {
        if(null == str || str.length() == 0)
            return;
        while (str.length() > 0)
        {
            if(str.charAt(str.length() - 1) != ch)
                break;
            str.deleteCharAt(str.length() - 1);
        }
    }

    public static void TrimLeft(StringBuilder str, char ch)
    {
        if(null == str || str.length() == 0)
            return;
        while (str.length() > 0)
        {
            if(str.charAt(0) != ch)
                break;
            str.deleteCharAt(0);
        }
    }

    // /1/2/3
    public static String AppendPath(String...args)
    {
        if(null == args)
            return null;
        if(args.length == 0)
            return "";

        StringBuilder sb = new StringBuilder();
        for (String arg : args)
        {
            if(IsBlank(arg))
                continue;
            if(sb.length() == 0)
            {
                sb.append(arg);
            }
            else
            {
                TrimRight(sb, File.separatorChar);
                sb.append(File.separatorChar);
                sb.append(TrimLeft(arg, File.separatorChar));
            }
        }
        return sb.toString();
    }

    public static String Join(Object[] args, String sep)
    {
        StringBuilder sb = new StringBuilder();
        for(int i = 0; i < args.length; i++)
        {
            Object o = args[i];
            if(null == o)
                continue;
            sb.append(o);
            if(i < args.length - 1)
                sb.append(null != sep ? sep : "");
        }
        return sb.toString();
    }

    public static String Join(Collection<?> args, String sep)
    {
        return Join(args.toArray(), sep);
    }

    public static String Join(String sep, String...args)
    {
        return Join(args, sep);
    }

    public static String CmdStr(String str)
    {
        if(null == str)
            return "";
        if(str.contains(" ") || str.contains("\""))
            return KidTechCommand.EscapeQuotes(str);
        else
            return str;
    }

    public static String ucfirst(String str)
    {
        if(IsBlank(str))
            return str;
        return Character.toUpperCase(str.charAt(0)) + str.substring(1);
    }

    public static String Str(Object str)
    {
        return Objects.toString(str, "");
    }

    private KStr() {}
}
