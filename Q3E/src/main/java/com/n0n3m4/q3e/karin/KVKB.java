package com.n0n3m4.q3e.karin;

import android.content.Context;
import android.preference.PreferenceManager;
import android.view.View;

import com.n0n3m4.q3e.Q3EMain;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class KVKB
{
    public static final String TYPE_BUTTON = "button";
    public static final String TYPE_SWITCH = "switch";
    public static final String TYPE_TOGGLE = "toggle";
    public static final String TYPE_LAYOUT = "layout";
    public static final String TYPE_ACTION = "action";

    public static final String ALIGN_FILL   = "fill";
    public static final String ALIGN_CENTER = "center";

    public String           name;
    public List<KVKBLayout> layouts;
    public int              rowHeight;

    public static class KVKBButton
    {
        public String type = TYPE_BUTTON;
        public String name;
        public String code;
        public String data = "";
        public String secondaryName;
        public String secondaryCode;
        public String secondaryData;
        public int    weight = 10;

        private boolean Parse(JSONObject root)
        {
            try
            {
                if(root.has("type"))
                    type = root.getString("type");
                name = root.getString("name");
                code = root.getString("code");
                if(root.has("data"))
                    data = root.getString("data");
                if(root.has("secondaryName"))
                    secondaryName = root.getString("secondaryName");
                if(root.has("secondaryCode"))
                    secondaryCode = root.getString("secondaryCode");
                if(root.has("secondaryData"))
                    secondaryData = root.getString("secondaryData");
                weight = root.getInt("weight");
                return true;
            }
            catch(Exception e)
            {
                e.printStackTrace();
                return false;
            }
        }
    }

    public static class KVKBRow
    {
        public String           align = ALIGN_FILL;
        public List<KVKBButton> buttons;

        private boolean Parse(JSONObject root)
        {
            try
            {
                if(root.has("align"))
                    align = root.getString("align");
                List<KVKBButton> list = new ArrayList<>();
                JSONArray btnList = root.getJSONArray("buttons");
                for(int i = 0; i < btnList.length(); i++)
                {
                    KVKBButton btn = new KVKBButton();
                    if(!btn.Parse(btnList.getJSONObject(i)))
                        return false;
                    list.add(btn);
                }
                buttons = list;
                return true;
            }
            catch(Exception e)
            {
                e.printStackTrace();
                return false;
            }
        }
    }

    public static class KVKBLayout
    {
        public String        name;
        public List<KVKBRow> rows;
        public Boolean       main = false;

        private boolean Parse(JSONObject root)
        {
            try
            {
                name = root.getString("name");
                if(root.has("main"))
                    main = root.getBoolean("main");
                List<KVKBRow> list = new ArrayList<>();
                JSONArray rowList = root.getJSONArray("rows");
                for(int i = 0; i < rowList.length(); i++)
                {
                    KVKBRow row = new KVKBRow();
                    if(!row.Parse(rowList.getJSONObject(i)))
                        return false;
                    list.add(row);
                }
                rows = list;
                return true;
            }
            catch(Exception e)
            {
                e.printStackTrace();
                return false;
            }
        }
    }

    private boolean Parse(JSONObject root)
    {
        try
        {
            name = root.getString("name");
            List<KVKBLayout> list = new ArrayList<>();
            JSONArray layoutList = root.getJSONArray("layouts");
            for(int i = 0; i < layoutList.length(); i++)
            {
                KVKBLayout layout = new KVKBLayout();
                if(!layout.Parse(layoutList.getJSONObject(i)))
                    return false;
                list.add(layout);
            }
            layouts = list;
            return true;
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    public boolean Parse(String json)
    {
        try
        {
            return Parse(new JSONObject(json));
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }
}