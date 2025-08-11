package com.n0n3m4.q3e.karin;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public final class KControlsTheme
{
    public String name;
    public String path;
    public String author;
    public String desc;

    public KControlsTheme(String path, String name)
    {
        this.path = path;
        this.name = name;
    }

    public KControlsTheme()
    {
    }

    public void Parse(String text)
    {
        if(KStr.IsBlank(text))
            return;

        try
        {
            JSONObject json = new JSONObject(text);
            String author = json.optString("author");
            String name = json.optString("name");
            String desc = json.optString("desc");
            if(KStr.NotBlank(name))
                this.name = name;
            if(KStr.NotBlank(author))
                this.author = author;
            if(KStr.NotBlank(desc))
                this.desc = desc;
        }
        catch(Exception e)
        {
            this.name = text;
        }
    }

    public static List<String> MakePathList(List<KControlsTheme> list)
    {
        List<String> res = new ArrayList<>();
        for(KControlsTheme item : list)
        {
            res.add(item.path);
        }
        return res;
    }

    public static List<String> MakeLabelList(List<KControlsTheme> list)
    {
        List<String> res = new ArrayList<>();
        for(KControlsTheme item : list)
        {
            res.add(item.name);
        }
        return res;
    }
}
