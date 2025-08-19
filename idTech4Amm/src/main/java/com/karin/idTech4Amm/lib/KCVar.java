package com.karin.idTech4Amm.lib;

import com.karin.idTech4Amm.misc.TextHelper;
import com.n0n3m4.q3e.karin.KStr;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

public final class KCVar
{
    public static final String TYPE_NONE    = "";
    public static final String TYPE_STRING  = "string";
    public static final String TYPE_BOOL    = "bool";
    public static final String TYPE_INTEGER = "integer";
    public static final String TYPE_FLOAT   = "float";
    public static final String TYPE_VECTOR3 = "vector3";

    public static final int CATEGORY_CVAR    = 1;
    public static final int CATEGORY_COMMAND = 2;
    public static final int CATEGORY_PARAM   = 3;

    public static final int FLAG_POSITIVE = 0x00000001; // number is positive
    public static final int FLAG_INIT     = 0x00000002; // only allow setup on command line, not save to file/load from from
    public static final int FLAG_AUTO     = 0x00000004; // auto setting when run game
    public static final int FLAG_READONLY = 0x00000100; // not allow modify
    public static final int FLAG_DISABLED = 0x00000200; // disabled
    public static final int FLAG_LAUNCHER = 0x00010000; // setting exists on launcher

    public final String  name;
    public final String  type;
    public final String  defaultValue;
    public final String  description;
    public final Value[] values;
    public final int     flags;
    public final int     category;

    private KCVar(String name, String type, String defaultValue, String description, int category, int flags, Value[] values)
    {
        this.name = name;
        this.type = type;
        this.defaultValue = defaultValue;
        this.description = description;
        this.values = values;
        this.category = category;
        this.flags = flags;
    }

    public boolean HasFlag(int flag)
    {
        return (flags & flag) == flag;
    }

    public boolean HasFlags(int flag)
    {
        return (flags & flag) != 0;
    }

    public String GenCVarString(String endl)
    {
        StringBuilder sb = new StringBuilder();
        if(category == KCVar.CATEGORY_COMMAND)
        {
            sb.append(TextHelper.FormatDialogMessageSpace("  *[Command] ")).append(name);
            sb.append(endl);
            if(!KCVar.TYPE_NONE.equals(type))
                sb.append(TextHelper.FormatDialogMessageSpace("    (")).append(type).append(")");
        }
        else
        {
            String typeName = category == KCVar.CATEGORY_PARAM ? "Param" : "CVar";
            sb.append(TextHelper.FormatDialogMessageSpace("  *[" + typeName + "] ")).append(name);
            sb.append(endl);
            sb.append(TextHelper.FormatDialogMessageSpace("    - ")).append(KStr.ucfirst(type)).append(TextHelper.FormatDialogMessageSpace("  default: ")).append(defaultValue);
            if(HasFlags(Integer.MAX_VALUE & ~(Integer.MAX_VALUE & KCVar.FLAG_LAUNCHER)))
            {
                sb.append(TextHelper.FormatDialogMessageSpace("  ("));
                if(HasFlag(KCVar.FLAG_POSITIVE))
                    sb.append(" Positive");
                if(HasFlag(KCVar.FLAG_INIT))
                    sb.append(" CommandLine-Only");
                if(HasFlag(KCVar.FLAG_AUTO))
                    sb.append(" Auto-Setup");
                if(HasFlag(KCVar.FLAG_READONLY))
                    sb.append(" Readonly");
                if(HasFlag(KCVar.FLAG_DISABLED))
                    sb.append(" Disabled");
                sb.append(" )");
            }
        }
        sb.append(endl);
        sb.append(TextHelper.FormatDialogMessageSpace("    ")).append(description);
        sb.append(endl);
        if(null != values)
        {
            for(KCVar.Value str : values)
            {
                sb.append(TextHelper.FormatDialogMessageSpace("      "));
                sb.append(str.value).append(" - ").append(str.desc);
                sb.append(endl);
            }
        }
        sb.append(endl);
        return sb.toString();
    }

    public static class Value {
        public final String value;
        public final String desc;

        public Value(String value, String desc)
        {
            this.value = value;
            this.desc = desc;
        }
    }

    public static class Group {
        public final String      name;
        public final boolean     engine;
        public final List<KCVar> list;

        public Group(String name, boolean engine)
        {
            this.name = name;
            this.engine = engine;
            list = new ArrayList<>();
        }

        public Group AddCVar(KCVar...cvars)
        {
            if(null != cvars)
            {
                list.addAll(Arrays.asList(cvars));
            }
            return this;
        }

        public String GetCvarText(String endl)
        {
            StringBuilder sb = new StringBuilder();
            sb.append(endl);
            for(KCVar cvar : list)
                sb.append(cvar.GenCVarString(endl));
            return sb.toString();
        }
    }

    public static KCVar CreateCVar(String name, String type, String def, String desc, int flag, String...args)
    {
        List<Value> values = null;
        if(null != args && args.length >= 2)
        {
            values = new ArrayList<>();
            for(int i = 0; i < args.length - 1; i += 2)
                values.add(new Value(args[i], args[i + 1]));
        }
        return new KCVar(name, type, def, desc, CATEGORY_CVAR, flag, null != values ? values.toArray(new Value[0]) : null);
    }

    public static KCVar CreateCommand(String name, String type, String desc, int flag, String...args)
    {
        List<Value> values = null;
        if(null != args && args.length >= 2)
        {
            values = new ArrayList<>();
            for(int i = 0; i < args.length - 1; i += 2)
                values.add(new Value(args[i], args[i + 1]));
        }
        return new KCVar(name, type, "", desc, CATEGORY_COMMAND, flag, null != values ? values.toArray(new Value[0]) : null);
    }

    public static KCVar CreateParam(String name, String type, String def, String desc, int flag, String...args)
    {
        List<Value> values = null;
        if(null != args && args.length >= 2)
        {
            values = new ArrayList<>();
            for(int i = 0; i < args.length - 1; i += 2)
                values.add(new Value(args[i], args[i + 1]));
        }
        return new KCVar(name, type, def, desc, CATEGORY_PARAM, flag, null != values ? values.toArray(new Value[0]) : null);
    }
}
