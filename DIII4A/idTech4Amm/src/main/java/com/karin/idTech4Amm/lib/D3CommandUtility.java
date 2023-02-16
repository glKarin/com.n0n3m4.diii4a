package com.karin.idTech4Amm.lib;

/**
 * idTech4 command line utility
 */
public final class D3CommandUtility
{
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
        name = " +set " + name + " ";
		String insertCmd = val.toString().trim();
        if (str.contains(name))
        {
            int start = str.indexOf(name) + name.length();
            int end = str.indexOf(" +", start);
            if(end != -1)
            {
                str = str.substring(0, start) + insertCmd + str.substring(end);
            }
            else
                str = str.substring(0, start) + insertCmd;
        }
        else
            str += name + insertCmd;
		return str;
	}

    public static String GetProp(final String str, String name, String...def)
    {
        name = " +set " + name + " ";
		String defVal = def.length > 0 ? def[0] : null;
        String val = defVal;
        if (str.contains(name))
        {
            int start = str.indexOf(name) + name.length();
            int end = str.indexOf(" +", start);
            if(end != -1)
            {
                val = str.substring(start, end).trim();
                if(val.isEmpty()) // ""
                    return defVal;
            }
            else
                val = str.substring(start).trim();
        }
        return val;
	}

    public static String RemoveProp(String str, String name, boolean[]...b)
    {
        name = " +set " + name + " ";
        boolean res = false;
        if (str.contains(name))
        {
            int start = str.indexOf(name);
            int len = start + name.length();
            int end = str.indexOf(" +", len);
            if(end != -1)
            {
                str = str.substring(0, start) + str.substring(end);
            }
            else
                str = str.substring(0, start);
            res = true;
        }
		if(b.length > 0 && null != b[0] && b[0].length > 0)
			b[0][0] = res;
        return str;
	}

    public static boolean IsProp(final String str, String name)
    {
        name=" +set "+name+" ";
        return(str.contains(name));
	}
    
    public static Boolean GetBoolProp(final String str, String name, Boolean...def)
	{
		Boolean defVal = def.length > 0 ? def[0] : null;
		String defStr = null != defVal ? (defVal ? "1" : "0") : null;
		String val = GetProp(str, name, defStr);
		return null != val ? strtob(val) : null;
	}

    public static String SetBoolProp(String str, String name, boolean val)
	{
		return SetProp(str, name, val ? "1" : "0");
	}



    public static String RemoveParam(String str, String name, boolean[]...b)
    {
        name = " +" + name + " ";
        boolean res = false;
        if (str.contains(name))
        {
            int start = str.indexOf(name);
            int len = start + name.length();
            int end = str.indexOf(" +", len);
            if(end != -1)
            {
                str = str.substring(0, start) + str.substring(end);
            }
            else
                str = str.substring(0, start);
            res = true;
        }
		if(b.length > 0 && null != b[0] && b[0].length > 0)
			b[0][0] = res;
        return str;
	}

    public static String SetParam(String str, String name, Object val)
    {
        name = " +" + name + " ";
        String insertCmd = val.toString().trim();
        if (str.contains(name))
        {
            int start = str.indexOf(name) + name.length();
            int end = str.indexOf(" +", start);
            if(end != -1)
            {
                str = str.substring(0, start) + insertCmd + str.substring(end);
            }
            else
                str = str.substring(0, start) + insertCmd;
        }
        else
            str += name + insertCmd;
		return str;
	}

    public static String GetParam(final String str, String name, String...def)
    {
        name = " +" + name + " ";
		String defVal = def.length > 0 ? def[0] : null;
        String val = defVal;
        if (str.contains(name))
        {
            int start = str.indexOf(name) + name.length();
            int end = str.indexOf(" +", start);
            if(end != -1)
            {
                val = str.substring(start, end).trim();
                if(val.isEmpty()) // ""
                    return defVal;
            }
            else
                val = str.substring(start).trim();
        }
        return val;
	}
    
	private D3CommandUtility() {}
}
