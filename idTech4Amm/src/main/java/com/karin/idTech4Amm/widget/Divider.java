package com.karin.idTech4Amm.widget;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.karin.idTech4Amm.R;

import java.util.HashMap;
import java.util.Map;

/**
 * Divider widget
 */
public class Divider extends LinearLayout
{
    private Map<String, Object> m_initValues = null;
    private final ViewHolder V = new ViewHolder();

    @SuppressLint("NewApi")
    public Divider(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
    {
        super(context, attrs, defStyleAttr, defStyleRes);
        CreateUI(attrs);
    }

    public Divider(Context context, AttributeSet attrs, int defStyleAttr)
    {
        super(context, attrs, defStyleAttr);
        CreateUI(attrs);
    }

    public Divider(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        CreateUI(attrs);
    }

    public Divider(Context context)
    {
        this(context, null);
    }

    private void CreateUI(AttributeSet attrs)
    {
        View inflate = LayoutInflater.from(getContext()).inflate(R.layout.divider, this, false);
        addView(inflate);
        if(attrs != null)
        {
            TypedArray ta = getContext().obtainStyledAttributes(attrs, R.styleable.Divider);
            m_initValues = new HashMap<>();
            m_initValues.put("lineColor", ta.getColor(R.styleable.Divider_lineColor, Color.argb(255, 0xCC, 0xCC, 0xCC)));
            m_initValues.put("label", ta.getString(R.styleable.Divider_label));
            ta.recycle();
        }
        SetupUI(inflate);
    }
    
    private void SetupUI(View view)
    {
        V.Setup(view);
        if(m_initValues != null)
        {
            Object text = m_initValues.get("label");
            String t = null != text ? (String)text : "";
            V.text_label.setText(t);

            Object lineColor = m_initValues.get("lineColor");
            int c = null != lineColor ? (Integer) lineColor : Color.argb(255, 0xCC, 0xCC, 0xCC);
            V.left_line.setBackgroundColor(c);
            V.right_line.setBackgroundColor(c);
        }
    }
    
    private class ViewHolder
    {
        private TextView text_label;
        private View left_line;
        private View right_line;
        
        public void Setup(View view)
        {
            text_label = view.findViewById(R.id.divider_text_label);
            left_line = view.findViewById(R.id.divider_left_line);
            right_line = view.findViewById(R.id.divider_right_line);
        }
    }
}
