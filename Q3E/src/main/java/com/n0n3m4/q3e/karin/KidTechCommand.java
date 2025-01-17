package com.n0n3m4.q3e.karin;

import com.n0n3m4.q3e.Q3EGlobals;

import java.util.ArrayList;
import java.util.List;

/**
 * idTech command line utility
 * prop: +/- set name value
 * param: +/-name value
 * bool: 1=true, 0=false
 * +: idTech2 3 4 TDM BFG
 * -: QuakeTech
 */
public class KidTechCommand
{
    public static final String ARG_PREFIX_IDTECH = "+";
    public static final String ARG_PREFIX_QUAKETECH = "-";
    public static final String ARG_PREFIX_ALL = ARG_PREFIX_IDTECH + ARG_PREFIX_QUAKETECH;

    private String m_cmd = Q3EGlobals.GAME_EXECUABLE;
    private String argPrefix;

    public KidTechCommand(String str)
    {
        this(ARG_PREFIX_IDTECH + ARG_PREFIX_QUAKETECH, str);
    }

    public KidTechCommand(String PLUS, String str)
    {
        argPrefix = PLUS;
        if(null == str)
            str = "";
        m_cmd = str;

        Parse();
    }

    public static String btostr(boolean b)
    {
        return b ? "1" : "0";
    }

    public static boolean strtob(String str)
    {
        return "1".equals(str);
    }

    public static String SetProp(String PLUS, String str, String name, Object val)
    {
        return new KidTechCommand(PLUS, str).SetProp(name, val).toString();
    }

    public static String GetProp(String PLUS, String str, String name, String...def)
    {
        return new KidTechCommand(PLUS, str).Prop(name, def);
    }

    public static String RemoveProp(String PLUS, String str, String name)
    {
        return new KidTechCommand(PLUS, str).RemoveProp(name).toString();
    }

    public static boolean IsProp(String PLUS, String str, String name)
    {
        return new KidTechCommand(PLUS, str).IsProp(name);
    }

    public static Boolean GetBoolProp(String PLUS, final String str, String name, Boolean...def)
    {
        return new KidTechCommand(PLUS, str).GetBoolProp(name, def);
    }

    public static String SetBoolProp(String PLUS, String str, String name, boolean val)
    {
        return new KidTechCommand(PLUS, str).SetBoolProp(name, val).toString();
    }

    public static String RemoveParam(String PLUS, String str, String name, String...val)
    {
        return new KidTechCommand(PLUS, str).RemoveParam(name, val).toString();
    }

    public static String SetParam(String PLUS, String str, String name, Object val)
    {
        return new KidTechCommand(PLUS, str).SetParam(name, val).toString();
    }

    public static String AddParam(String PLUS, String str, String name, Object val)
    {
        return new KidTechCommand(PLUS, str).AddParam(name, val).toString();
    }

    public static String SetCommand(String PLUS, String str, String name, boolean...prepend)
    {
        return new KidTechCommand(PLUS, str).SetCommand(name, prepend).toString();
    }

    public static String RemoveCommand(String PLUS, String str, String name)
    {
        return new KidTechCommand(PLUS, str).RemoveCommand(name).toString();
    }

    public static String GetParam(String PLUS, String str, String name, String...def)
    {
        return new KidTechCommand(PLUS, str).Param(name, def);
    }

    public static boolean HasParam(String PLUS, String str, String name, String...val)
    {
        return new KidTechCommand(PLUS, str).HasParam(name, val);
    }

    public KidTechCommand SetProp(String name, Object val)
    {
        int i = 0;
        while(!IsEnd(i = FindNext(i, CMD_PART_SET)))
        {
            i++;
            int blank1I = i;
            if(!RequireType(blank1I, CMD_PART_BLANK))
                continue;

            if(!RequireNextType(blank1I, CMD_PART_NAME))
                continue;

            int nameI = blank1I + 1;
            CmdPart part = cmdParts.get(nameI);
            if(!name.equals(part.str))
            {
                i = nameI + 1;
                continue;
            }

            int blank2I = nameI + 1;
            if(!RequireType(blank2I, CMD_PART_BLANK))
            {
                InsertBlankPart(blank2I);
                InsertPart(blank2I + 1, CMD_PART_VALUE, val);
                return this;
            }

            int valueI = blank2I + 1;
            if(!RequireType(valueI, CMD_PART_VALUE))
            {
                CmdPart lastBlank = GetPart(blank2I);
                if(null != lastBlank && lastBlank.str.length() >= 2)
                {
                    lastBlank.str = " ";
                    InsertPart(valueI, CMD_PART_VALUE, val);
                    InsertBlankPart(valueI + 1);
                }
                else
                {
                    InsertPart(valueI, CMD_PART_VALUE, val);
                    int nextBlankI = valueI + 1;
                    if(!IsEnd(nextBlankI) && !RequireType(nextBlankI, CMD_PART_BLANK))
                        InsertBlankPart(nextBlankI);
                }
                return this;
            }

            cmdParts.get(valueI).str = ValueToString(val);
            return this;
        }

        if(!EndsWithBlank())
            AddBlankPart();

        AddPart(CMD_PART_SET, GetArgPrefixChar() + "set");
        AddBlankPart();
        AddPart(CMD_PART_NAME, name);
        AddBlankPart();
        AddPart(CMD_PART_VALUE, val);

        return this;
    }

    public String Prop(String name, String...def)
    {
        int i = 0;
        String defVal = null != def && def.length > 0 ? def[0] : null;
        while(!IsEnd(i = FindNext(i, CMD_PART_SET)))
        {
            i++;

            int blank1I = i;
            if(!RequireType(blank1I, CMD_PART_BLANK))
                continue;

            if(!RequireNextType(blank1I, CMD_PART_NAME))
                continue;

            int nameI = blank1I + 1;
            CmdPart part = cmdParts.get(nameI);
            if(!name.equals(part.str))
            {
                i = nameI + 1;
                continue;
            }

            int blank2I = nameI + 1;
            if(!RequireType(blank2I, CMD_PART_BLANK))
                break;

            int valueI = blank2I + 1;
            if(!RequireType(valueI, CMD_PART_VALUE))
                break;

            return cmdParts.get(valueI).str;
        }

        return defVal;
    }

    public KidTechCommand RemoveProp(String name)
    {
        int i = 0;
        while(!IsEnd(i = FindNext(i, CMD_PART_SET)))
        {
            int start = i;
            i++;

            int blank1I = i;
            if(!RequireType(blank1I, CMD_PART_BLANK))
                continue;

            if(!RequireNextType(blank1I, CMD_PART_NAME))
                continue;

            int nameI = blank1I + 1;
            CmdPart part = cmdParts.get(nameI);
            if(!name.equals(part.str))
            {
                i = nameI + 1;
                continue;
            }

            int blank2I = nameI + 1;
            int valueI = blank2I + 1;
            int blank3I = valueI + 1;
            boolean blank2 = RequireType(blank2I, CMD_PART_BLANK);
            boolean value = RequireType(valueI, CMD_PART_VALUE);
            boolean blank3 = RequireType(blank3I, CMD_PART_BLANK);

            int end = nameI;
            if(blank2)
            {
                end = blank2I;
                if(value)
                {
                    end = valueI;
                    if(blank3)
                    {
                        end = blank3I;
                    }
                }
            }

            RemoveParts(start, end + 1);
            return this;
        }

        return this;
    }

    public boolean IsProp(String name)
    {
        int i = 0;
        while(!IsEnd(i = FindNext(i, CMD_PART_SET)))
        {
            i++;

            int blank1I = i;
            if(!RequireType(blank1I, CMD_PART_BLANK))
                continue;

            if(!RequireNextType(blank1I, CMD_PART_NAME))
                continue;

            int nameI = blank1I + 1;
            CmdPart part = cmdParts.get(nameI);
            if(name.equals(part.str))
                return true;

            i = nameI + 1;
        }
        return false;
    }

    public boolean HasParam(String name, String...val)
    {
        int i = 0;
        while(!IsEnd(i = FindNext(i, CMD_PART_PARAM)))
        {
            CmdPart part = cmdParts.get(i);

            if(EqualsParam(part.str, name))
            {
                if(null != val && val.length > 0 && null != val[0])
                {
                    int valueI = i + 2;
                    boolean value = RequireType(valueI, CMD_PART_VALUE);

                    if(value && val[0].equals(cmdParts.get(valueI).str))
                        return true;
                }
                else
                    return true;
            }
            i++;
        }
        return false;
    }

    public Boolean GetBoolProp(String name, Boolean...def)
    {
        Boolean defVal = null != def && def.length > 0 ? def[0] : null;
        String defStr = null != defVal ? (defVal ? "1" : "0") : null;
        String val = Prop(name, defStr);
        return null != val ? strtob(val) : null;
    }

    public KidTechCommand SetBoolProp(String name, boolean val)
    {
        return SetProp(name, btostr(val));
    }

    public KidTechCommand RemoveParam(String name, String...val)
    {
        int i = 0;
        while(!IsEnd(i = FindNext(i, CMD_PART_PARAM)))
        {
            int start = i;
            CmdPart part = cmdParts.get(i);
            i++;
            if(!EqualsParam(part.str, name))
                continue;

            int blank1I = i;
            int valueI = blank1I + 1;
            int blank2I = valueI + 1;
            boolean blank1 = RequireType(blank1I, CMD_PART_BLANK);
            boolean value = RequireType(valueI, CMD_PART_VALUE);
            boolean blank2 = RequireType(blank2I, CMD_PART_BLANK);

            if(null != val && val.length > 0 && null != val[0])
            {
                if(!value)
                    continue;
                if(!val[0].equals(cmdParts.get(valueI).str))
                    continue;
            }

            int end = start;
            if(blank1)
            {
                end = blank1I;
                if(value)
                {
                    end = valueI;
                    if(blank2)
                    {
                        end = blank2I;
                    }
                }
            }

            RemoveParts(start, end + 1);
            return this;
        }

        return this;
    }

    public KidTechCommand SetParam(String name, Object val)
    {
        int i = 0;
        while(!IsEnd(i = FindNext(i, CMD_PART_PARAM)))
        {
            CmdPart part = cmdParts.get(i);

            i++;
            if(!EqualsParam(part.str, name))
                continue;

            int blank1I = i;
            if(!RequireType(blank1I, CMD_PART_BLANK))
            {
                InsertBlankPart(blank1I);
                InsertPart(blank1I + 1, CMD_PART_VALUE, val);
                return this;
            }

            int valueI = blank1I + 1;
            if(!RequireType(valueI, CMD_PART_VALUE))
            {
                CmdPart lastBlank = GetPart(blank1I);
                if(null != lastBlank && lastBlank.str.length() >= 2)
                {
                    lastBlank.str = " ";
                    InsertPart(valueI, CMD_PART_VALUE, val);
                    InsertBlankPart(valueI + 1);
                }
                else
                {
                    InsertPart(valueI, CMD_PART_VALUE, val);
                    int nextBlankI = valueI + 1;
                    if(!IsEnd(nextBlankI) && !RequireType(nextBlankI, CMD_PART_BLANK))
                        InsertBlankPart(nextBlankI);
                }
                return this;
            }

            cmdParts.get(valueI).str = ValueToString(val);
            return this;
        }

        if(!EndsWithBlank())
            AddBlankPart();

        AddPart(CMD_PART_PARAM, GetArgPrefixChar() + name);
        AddBlankPart();
        AddPart(CMD_PART_VALUE, val);

        return this;
    }

    public KidTechCommand AddParam(String name, Object val)
    {
        if(HasParam(name, ValueToString(val)))
            return this;

        if(!EndsWithBlank())
            AddBlankPart();

        AddPart(CMD_PART_PARAM, GetArgPrefixChar() + name);
        AddBlankPart();
        AddPart(CMD_PART_VALUE, val);

        return this;
    }

    public KidTechCommand SetCommand(String name, boolean...prepend)
    {
        int i = 0;
        while(!IsEnd(i = FindNext(i, CMD_PART_PARAM)))
        {
            CmdPart part = cmdParts.get(i);
            if(!EqualsParam(part.str, name))
                return this;
            i++;
        }

        boolean pp = null != prepend && prepend.length > 0 && prepend[0];
        if(pp)
        {
            int start = FirstBlank();
            InsertBlankPart(start);
            InsertPart(start, CMD_PART_PARAM, GetArgPrefixChar() + name);
        }
        else
        {
            if(!EndsWithBlank())
                AddBlankPart();

            AddPart(CMD_PART_PARAM, GetArgPrefixChar() + name);
        }

        return this;
    }

    public KidTechCommand RemoveCommand(String name)
    {
        int i = 0;
        while(!IsEnd(i = FindNext(i, CMD_PART_PARAM)))
        {
            int start = i;
            CmdPart part = cmdParts.get(i);
            i++;
            if(!EqualsParam(part.str, name))
                continue;

            int blank1I = i;
            boolean blank1 = RequireType(blank1I, CMD_PART_BLANK);

            int end = start;
            if(blank1)
            {
                end = blank1I;
            }

            RemoveParts(start, end + 1);
            return this;
        }

        return this;
    }

    public String Param(String name, String...def)
    {
        int i = 0;
        String defVal = null != def && def.length > 0 ? def[0] : null;
        while(!IsEnd(i = FindNext(i, CMD_PART_PARAM)))
        {
            CmdPart part = cmdParts.get(i);

            i++;
            if(!EqualsParam(part.str, name))
                continue;

            int blank1I = i;
            if(!RequireType(blank1I, CMD_PART_BLANK))
                break;

            int valueI = blank1I + 1;
            if(!RequireType(valueI, CMD_PART_VALUE))
                break;

            return cmdParts.get(valueI).str;
        }

        return defVal;
    }

    private static final int CMD_PART_EXECUTION = 0; // game.arm
    private static final int CMD_PART_BLANK = 1; // space
    private static final int CMD_PART_PARAM = 2; // +param/-param
    private static final int CMD_PART_SET   = 3; // +set
    private static final int CMD_PART_NAME = 4; // r_shadows
    private static final int CMD_PART_VALUE = 5; // 1
    public static class CmdPart
    {
        public int type;
        public String str;

        @Override
        public String toString()
        {
            String tname;
            switch(type)
            {
                case CMD_PART_EXECUTION: tname = "命令"; break;
                case CMD_PART_BLANK: tname = "空格"; break;
                case CMD_PART_PARAM: tname = "参数"; break;
                case CMD_PART_SET: tname = "设置"; break;
                case CMD_PART_NAME: tname = "名称"; break;
                default: tname = "值"; break;
            }
            return String.format("{%s|%s}", tname, str);
        }
    }
    public final List<CmdPart> cmdParts = new ArrayList<>();

    private void Parse()
    {
        cmdParts.clear();
        int i = 0;
        boolean hasArg0 = false;
        boolean readingSet = false;

        while(i < m_cmd.length())
        {
            char c = m_cmd.charAt(i);
            if (c == '"')
            {
                if(readingSet)
                {
                    readingSet = false;
                    String val = ReadWordWithQuotes(m_cmd, i);
                    AddPart(CMD_PART_NAME, val);
                    i += val.length();
                }
                else
                {
                    String val = ReadUtil(m_cmd, i, argPrefix);
                    if(!hasArg0)
                        AddPart(CMD_PART_EXECUTION, val);
                    else
                        AddPart(CMD_PART_VALUE, val);
                    i += val.length();
                }
                hasArg0 = true;
            }
            else if(c == '+' || c == '-')
            {
                readingSet = false;
                String cmd = ReadWord(m_cmd, i + 1);
                boolean isSet = cmd.equals("set") || cmd.equals("\"set\"");
                cmd = c + cmd;
                if(isSet)
                    AddPart(CMD_PART_SET, cmd);
                else
                    AddPart(CMD_PART_PARAM, cmd);
                i += cmd.length();
                hasArg0 = true;
                if(isSet)
                    readingSet = true;
            }
            else if(Character.isSpaceChar(c))
            {
                String blank = SkipBlank(m_cmd, i);
                if(!blank.isEmpty())
                {
                    AddPart(CMD_PART_BLANK, blank);
                    i += blank.length();
                }
            }
            else
            {
                if(readingSet)
                {
                    readingSet = false;
                    String val = ReadWord(m_cmd, i);
                    AddPart(CMD_PART_NAME, val);
                    i += val.length();
                }
                else
                {
                    String val = ReadUtil(m_cmd, i, argPrefix);
                    if(!hasArg0)
                        AddPart(CMD_PART_EXECUTION, val);
                    else
                        AddPart(CMD_PART_VALUE, val);
                    i += val.length();
                }
                hasArg0 = true;
            }
        }
    }

    private boolean EndsWithBlank()
    {
        return(!cmdParts.isEmpty() && cmdParts.get(cmdParts.size() - 1).type == CMD_PART_BLANK);
    }

    private void AddPart(int type, Object value)
    {
        CmdPart part = new CmdPart();
        part.type = type;
        part.str = ValueToString(value);
        cmdParts.add(part);
    }

    private void InsertPart(int i, int type, Object value)
    {
        CmdPart part = new CmdPart();
        part.type = type;
        part.str = ValueToString(value);
        cmdParts.add(i, part);
    }

    private static String SkipBlank(String str, int start)
    {
        StringBuilder ret = new StringBuilder();
        int i = start;
        while(i < str.length())
        {
            char c = str.charAt(i);
            if(!Character.isSpaceChar(c))
                break;
            ret.append(c);
            i++;
        }
        return ret.toString();
    }

    private static String ReadWord(String str, int start)
    {
        if(start < str.length() && str.charAt(start) == '\"')
        {
            return ReadWordWithQuotes(str, start);
        }

        StringBuilder ret = new StringBuilder();
        int i = start;
        while(i < str.length())
        {
            char c = str.charAt(i);
            if(Character.isSpaceChar(c))
                break;
            ret.append(c);
            i++;
        }
        return ret.toString();
    }

    private static String ReadWordWithQuotes(String str, int start)
    {
        char quoteStart = str.charAt(start);
        StringBuilder ret = new StringBuilder();
        ret.append(quoteStart);
        int i = start + 1;
        char lastp = 0;
        while(i < str.length() && ( str.charAt(i) != quoteStart || lastp == '\\' ))
        {
            lastp = str.charAt(i);
            ret.append(lastp);
            i++;
        }
        ret.append(quoteStart);
        return ret.toString(); // with quotes
    }

    private static String ReadUtil(String str, int start, String chars)
    {
        StringBuilder ret = new StringBuilder();
        int i = start;
        while(i < str.length())
        {
            char c = str.charAt(i);
            if(c == '"')
            {
                String val = ReadWordWithQuotes(str, i);
                ret.append(val);
                i += val.length();
                continue;
            }

            if(Character.isSpaceChar(c))
            {
                if(i + 1 < str.length())
                {
                    char c2 = str.charAt(i + 1);
                    if(chars.contains("" + c2))
                        break;
                }
                else
                    break;
            }
            ret.append(c);
            i++;
        }
        return ret.toString();
    }

    private int FindNext(int start, int type)
    {
        int i = start;
        for(; i < cmdParts.size(); i++)
        {
            if(cmdParts.get(i).type == type)
                break;
        }
        return i;
    }

    private boolean RequireNextType(int start, int type)
    {
        return RequireType(start + 1, type);
    }

    private boolean RequireType(int i, int type)
    {
        if(IsEnd(i))
            return false;
        return cmdParts.get(i).type == type;
    }

    private char GetArgPrefixChar()
    {
        return argPrefix.charAt(0);
    }

    private void AddBlankPart()
    {
        AddPart(CMD_PART_BLANK, " ");
    }

    private void InsertBlankPart(int pos)
    {
        InsertPart(pos, CMD_PART_BLANK, " ");
    }

    private void RemoveParts(int start, int end)
    {
        int i = 0;
        int num = end - start;
        while(i < num)
        {
            cmdParts.remove(start);
            i++;
        }
    }

    private boolean EqualsParam(String part, String name)
    {
        return name.equals(part.substring(1));
    }

    private int FirstBlank()
    {
        int start = 0;
        start = FindNext(start, CMD_PART_EXECUTION);
        if(IsEnd(start))
            return 0;
        else
            return start + 1;
    }

    private static String ValueToString(Object o)
    {
        if(null == o)
            return "";
        return o.toString();
    }

    public static List<String> SplitValue(String value, boolean unescape)
    {
        value = value.trim();
        List<String> ret = new ArrayList<>();
        int i = 0;
        while(i < value.length())
        {
            char c = value.charAt(i);
            if(Character.isSpaceChar(c))
            {
                i += SkipBlank(value, i).length();
            }
            else
            {
                String val = ReadWord(value, i);
                i += val.length();
                if(unescape && val.startsWith("\""))
                    val = UnEscapeQuotes(val);
                ret.add(val);
            }
        }
        return ret;
    }

    public static String UnEscapeQuotes( String arg )
    {
        char[] arr = arg.toCharArray();
        char first = '"';
        char last = 0;
        int _i = 0;
        int last_i = 0;
        while( _i < arr.length ) {
            char ch = arr[_i];
            if( ch == first && last == '\\' ) {
                int c_curr = _i;
                int c_last = last_i;
                while( c_curr < arr.length ) {
                    arr[c_last] = arr[c_curr];
                    c_last = c_curr;
                    c_curr++;
                }
                arr[c_last] = '\0';
            }
            last_i = _i;
            last = ch;
            _i++;
        }

        StringBuilder buf = new StringBuilder();
        for(_i = 1; _i < arr.length; _i++)
        {
            if(arr[_i] == 0)
                break;
            buf.append(arr[_i]);
        }
        if(buf.length() == 0)
            return "";
        buf.delete(buf.length() - 1, buf.length());
        return buf.toString();
    }

    public static String JoinValue(List<String> list, boolean escape)
    {
        String split = " ";
        StringBuilder buf = new StringBuilder();
        for(int i = 0; i < list.size(); i++)
        {
            String str = list.get(i);
            if(null == str)
                str = "";
            else if(escape && (str.contains(split) || str.contains("\"")))
                str = EscapeQuotes(str);
            buf.append(str);
            if(i < list.size() - 1)
                buf.append(split);
        }
        return buf.toString();
    }

    public static String EscapeQuotes( String arg )
    {
        return "\"" + arg.replaceAll("\"", "\\\\\"") + "\"";
    }

    private CmdPart GetPart(int index)
    {
        if(index < 0 || index >= cmdParts.size())
            return null;
        return cmdParts.get(index);
    }

    private boolean IsEnd(int i)
    {
        return i >= cmdParts.size();
    }

    @Override
    public String toString()
    {
        StringBuilder sb = new StringBuilder();
        for(CmdPart cmdPart : cmdParts)
        {
            sb.append(cmdPart.str);
        }
        return sb.toString();
    }
}
