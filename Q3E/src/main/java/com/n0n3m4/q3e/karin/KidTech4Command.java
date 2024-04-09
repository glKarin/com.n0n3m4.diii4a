package com.n0n3m4.q3e.karin;

import com.n0n3m4.q3e.Q3EGlobals;

/**
 * idTech4 command line utility
 */
public final class KidTech4Command
{
    private static String PreCmd(String cmd)
    {
        if(null == cmd)
        {
            return "";
        }
        if(cmd.startsWith("+"))
            cmd = " " + cmd;
        return cmd;
    }

    private static String PostCmd(String cmd)
    {
        if(null == cmd)
        {
            return "";
        }
        int i = 0;
        int length = cmd.length();
        while(i < length)
        {
            if(!Character.isSpaceChar(cmd.charAt(i)))
                break;
            i++;
        }
        return i > 0 ? (i < length ? cmd.substring(i) : "") : cmd;
    }

    public static String btostr(boolean b)
    {
        return b ? "1" : "0";
    }

    public static boolean strtob(String str)
    {
        return "1".equals(str);
    }

    public static String SetProp(String str, String name, Object val)
    {
        str = PreCmd(str);
        String nname = " +set " + name;
        String insertCmd = val.toString().trim();
        if (str.contains(nname))
        {
            int start = str.indexOf(nname) + nname.length();
            if(start == str.length()) // at last
            {
                str = str + " " + insertCmd;
            }
            else
            {
                nname += " ";
                start = str.indexOf(nname);
                if(start != -1)
                {
                    start += nname.length();
                    int end = str.indexOf("+", start);
                    if (end != -1)
                        str = str.substring(0, start) + insertCmd + " " + str.substring(end);
                    else
                        str = str.substring(0, start) + insertCmd;
                }
            }
        }
        else
        {
            str += nname + " " + insertCmd;
        }
        return PostCmd(str);
    }

    public static String GetProp(String str, String name, String...def)
    {
        str = PreCmd(str);
        String nname = " +set " + name + " ";
        String defVal = null != def && def.length > 0 ? def[0] : null;
        String val = defVal;
        if (str.contains(nname))
        {
            int start = str.indexOf(nname) + nname.length();
            int end = str.indexOf("+", start);
            if (end != -1)
                val = str.substring(start, end).trim();
            else
                val = str.substring(start).trim();
            if (val.isEmpty()) // ""
                return defVal;
        }
        return val;
    }

    public static String RemoveProp(String str, String name, boolean[]...b)
    {
        str = PreCmd(str);
        String nname = " +set " + name;
        boolean res = false;
        if (str.contains(nname))
        {
            int start = str.indexOf(nname) + nname.length();
            if(start == str.length()) // at last
            {
                str = str.substring(0, start);
            }
            else
            {
                nname += " ";
                start = str.indexOf(nname);
                if(start != -1)
                {
                    int end = str.indexOf("+", start + nname.length());
                    if (end != -1)
                        str = str.substring(0, start) + " " + str.substring(end);
                    else
                        str = str.substring(0, start);
                    res = true;
                }
            }
        }
        if (null != b && b.length > 0 && null != b[0] && b[0].length > 0)
            b[0][0] = res;
        return PostCmd(str);
    }

    public static boolean IsProp(String str, String name)
    {
        str = PreCmd(str);
        String nname = " +set " + name;
        int start = str.indexOf(nname) + nname.length();
        if(start == str.length()) // at last
            return true;
        nname += " ";
        return (str.contains(nname));
    }

    public static Boolean GetBoolProp(final String str, String name, Boolean...def)
    {
        Boolean defVal = null != def && def.length > 0 ? def[0] : null;
        String defStr = null != defVal ? (defVal ? "1" : "0") : null;
        String val = GetProp(str, name, defStr);
        return null != val ? strtob(val) : null;
    }

    public static String SetBoolProp(String str, String name, boolean val)
    {
        return SetProp(str, name, btostr(val));
    }

    public static String RemoveParam(String str, String name, boolean[]...b)
    {
        str = PreCmd(str);
        String nname = " +" + name;
        boolean res = false;
        if (str.contains(nname))
        {
            int start = str.indexOf(nname) + nname.length();
            if(start == str.length()) // at last
            {
                str = str.substring(0, start);
                res = true;
            }
            else
            {
                nname += " ";
                start = str.indexOf(nname);
                if(start != -1)
                {
                    int end = str.indexOf("+", start + nname.length());
                    if (end != -1)
                        str = str.substring(0, start) + " " + str.substring(end);
                    else
                        str = str.substring(0, start);
                    res = true;
                }
            }
        }
        if (b.length > 0 && null != b[0] && b[0].length > 0)
            b[0][0] = res;
        return PostCmd(str);
    }

    public static String SetParam(String str, String name, Object val)
    {
        str = PreCmd(str);
        String nname = " +" + name;
        String insertCmd = val.toString().trim();
        if (str.contains(nname))
        {
            int start = str.indexOf(nname) + nname.length();
            if(start == str.length()) // at last
            {
                str = str + " " + insertCmd;
            }
            else
            {
                nname += " ";
                start = str.indexOf(nname);
                if(start != -1)
                {
                    start += nname.length();
                    int end = str.indexOf("+", start);
                    if (end != -1)
                        str = str.substring(0, start) + insertCmd + " " + str.substring(end);
                    else
                        str = str.substring(0, start) + insertCmd;
                }
            }
        }
        else
        {
            str += nname + " " + insertCmd;
        }
        return PostCmd(str);
    }

    public static String SetCommand(String str, String name, boolean...prepend)
    {
        str = PreCmd(str);
        String nname = " +" + name;
        boolean pp = null != prepend && prepend.length > 0 && prepend[0];
        if (!str.contains(nname))
        {
            if(pp)
                str = nname + str;
            else
                str += nname;
        }
        return PostCmd(str);
    }

    public static String RemoveCommand(String str, String name, boolean[]...b)
    {
        str = PreCmd(str);
        String nname = " +" + name;
        boolean res = false;
        int index = str.indexOf(nname);
        if (index != -1)
        {
            int start = index + nname.length();
            if(start == str.length()) // at last
            {
                str = str.substring(0, index);
                res = true;
            }
            else
            {
                if(str.charAt(start) == ' ')
                {
                    str = str.substring(0, index) + str.substring(start);
                    res = true;
                }
            }
        }
        if (b.length > 0 && null != b[0] && b[0].length > 0)
            b[0][0] = res;
        return PostCmd(str);
    }

    public static String GetParam(String str, String name, String...def)
    {
        str = PreCmd(str);
        String nname = " +" + name;
        String defVal = null != def && def.length > 0 ? def[0] : null;
        String val = defVal;
        if (str.contains(nname))
        {
            int start = str.indexOf(nname) + nname.length();
            int end = str.indexOf("+", start);
            if (end != -1)
                val = str.substring(start, end).trim();
            else
                val = str.substring(start).trim();
            if (val.isEmpty()) // ""
                return defVal;
        }
        return val;
    }

    public static boolean HasParam(String str, String name)
    {
        str = PreCmd(str);
        String nname = " +" + name;
        int start = str.indexOf(nname) + nname.length();
        if(start == str.length()) // at last
            return true;
        nname += " ";
        return (str.contains(nname));
    }



    private String m_cmd = Q3EGlobals.GAME_EXECUABLE;

    public KidTech4Command(String str)
    {
        m_cmd = str;
    }

    @Override
    public String toString()
    {
        return m_cmd;
    }

    public KidTech4Command SetProp(String name, Object val)
    {
        m_cmd = KidTech4Command.SetProp(m_cmd, name, val);
        return this;
    }

    public String Prop(String name, String...def)
    {
        return KidTech4Command.GetProp(m_cmd, name, def);
    }

    public KidTech4Command RemoveProp(String name, boolean[]...b)
    {
        m_cmd = KidTech4Command.RemoveProp(m_cmd, name, b);
        return this;
    }

    public boolean IsProp(String name)
    {
        return KidTech4Command.IsProp(m_cmd, name);
    }

    public Boolean GetBoolProp(String name, Boolean...def)
    {
        return KidTech4Command.GetBoolProp(m_cmd, name, def);
    }

    public KidTech4Command SetBoolProp(String name, boolean val)
    {
        return SetProp(name, btostr(val));
    }

    public KidTech4Command RemoveParam(String name, boolean[]...b)
    {
        m_cmd = KidTech4Command.RemoveParam(m_cmd, name, b);
        return this;
    }

    public KidTech4Command SetParam(String name, Object val)
    {
        m_cmd = KidTech4Command.SetParam(m_cmd, name, val);
        return this;
    }

    public KidTech4Command SetCommand(String name, boolean...prepend)
    {
        m_cmd = KidTech4Command.SetCommand(m_cmd, name, prepend);
        return this;
    }

    public KidTech4Command RemoveCommand(String name, boolean[]...b)
    {
        m_cmd = KidTech4Command.RemoveCommand(m_cmd, name, b);
        return this;
    }

    public String Param(String name, String...def)
    {
        return GetParam(m_cmd, name, def);
    }
}
