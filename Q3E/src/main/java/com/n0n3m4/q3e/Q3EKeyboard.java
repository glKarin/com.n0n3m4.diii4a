package com.n0n3m4.q3e;

import android.content.Context;
import android.preference.PreferenceManager;
import android.view.View;
import android.view.ViewGroup;

import com.n0n3m4.q3e.karin.KKeyToolBar;
import com.n0n3m4.q3e.karin.KVKBView;

import java.util.Arrays;

public class Q3EKeyboard
{
    private Q3EMain m_context;

    private boolean     m_toolbarActive = true;
    private KKeyToolBar m_keyToolbar    = null;
    private boolean     vkbActive       = false;
    private KVKBView    vkb;

    public Q3EKeyboard(Q3EMain m_context)
    {
        this.m_context = m_context;
    }

    public void onDetachedFromWindow()
    {
        m_keyToolbar = null;
        vkb = null;
        m_toolbarActive = true;
        vkbActive = false;
    }

    public void OnPause()
    {
        ToggleToolbar(false);
        CloseBuiltInVKB();
    }

    public View CreateToolbar()
    {
        if(Q3EUtils.q3ei.function_key_toolbar)
        {
            Context context = m_context;
            m_keyToolbar = new KKeyToolBar(context);
            m_keyToolbar.Close();
            try
            {
                String str = PreferenceManager.getDefaultSharedPreferences(context).getString(Q3EPreference.pref_harm_function_key_toolbar_y, "0");
                if(null == str)
                    str = "0";
                int y = Integer.parseInt(str);
                if(y > 0)
                    m_keyToolbar.setY(y);
            }
            catch(Exception e)
            {
                e.printStackTrace();
            }
        }
        return m_keyToolbar;
    }

    public View Toolbar()
    {
        return m_keyToolbar;
    }

    public void ToggleToolbar(boolean b)
    {
        if(null != m_keyToolbar && Q3EUtils.q3ei.function_key_toolbar)
        {
            m_toolbarActive = b;
            m_keyToolbar.SetVisible(m_toolbarActive);
        }
    }

    public View CreateBuiltInVKB()
    {
        if(null == vkb)
        {
            vkb = new KVKBView(m_context);
            vkb.Init(Q3EUtils.asset_get_contents(m_context, "keyboard/generic.json"));
            vkb.Close();
        }
        return vkb;
    }

    public void OpenBuiltInVKB()
    {
        if(null != vkb && Q3EUtils.q3ei.builtin_virtual_keyboard)
        {
            vkbActive = true;
            vkb.Open();
        }
    }

    public void CloseBuiltInVKB()
    {
        if(null != vkb && Q3EUtils.q3ei.builtin_virtual_keyboard)
        {
            vkbActive = false;
            vkb.Close();
        }
    }

    public void ToggleBuiltInVKB()
    {
        if(null != vkb && Q3EUtils.q3ei.builtin_virtual_keyboard)
        {
            vkbActive = !vkbActive;
            vkb.SetVisible(vkbActive);
        }
    }

    public boolean IsBuiltInVKBVisible()
    {
        return vkbActive;
    }

    public void onAttachedToWindow()
    {
        if(m_keyToolbar != null)
        {
            if(m_context.IsCoverEdges())
            {
                int x = Q3EUtils.GetEdgeHeight(m_context, true);
                if(x != 0)
                    m_keyToolbar.setX(x);
            }
            int[] size = Q3EUtils.GetNormalScreenSize(m_context);
            ViewGroup.LayoutParams layoutParams = m_keyToolbar.getLayoutParams();
            layoutParams.width = size[0];
            m_keyToolbar.setLayoutParams(layoutParams);
            //mainLayout.requestLayout();
        }

        if(vkb != null)
        {
            if(m_context.IsCoverEdges())
            {
                int x = Q3EUtils.GetEdgeHeight(m_context, true);
                if(x != 0)
                    vkb.setX(x);
            }
            int[] size = Q3EUtils.GetNormalScreenSize(m_context);
            ViewGroup.LayoutParams layoutParams = vkb.getLayoutParams();
            layoutParams.width = size[0];
            vkb.setLayoutParams(layoutParams);

            vkb.setY(size[1] - vkb.GetCalcHeight());

            vkb.Resize(size[0]);
            //mainLayout.requestLayout();
        }
    }

    public void onAttachedToWindow(int offsetY)
    {
        if(m_keyToolbar != null)
        {
            int[] size = Q3EUtils.GetNormalScreenSize(m_context);
            ViewGroup.LayoutParams layoutParams = m_keyToolbar.getLayoutParams();
            layoutParams.width = size[0];
            m_keyToolbar.setLayoutParams(layoutParams);
            //mainLayout.requestLayout();
        }

        if(vkb != null)
        {
            int[] size = Q3EUtils.GetNormalScreenSize(m_context);
            ViewGroup.LayoutParams layoutParams = vkb.getLayoutParams();
            layoutParams.width = size[0];
            vkb.setLayoutParams(layoutParams);

            vkb.setY(size[1] - vkb.GetCalcHeight() + offsetY);

            vkb.Resize(size[0]);
            //mainLayout.requestLayout();
        }
    }
}
