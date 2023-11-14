package com.karin.idTech4Amm.widget;

import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.content.Context;
import android.view.View;
import android.widget.SeekBar;
import java.util.Map;
import java.util.HashMap;
import android.widget.TextView;
import android.graphics.Color;
import android.content.res.TypedArray;
import android.annotation.SuppressLint;
import android.os.Build;

import com.karin.idTech4Amm.R;

/**
 * SeekBar preference widget
 */
public class SeekBarPreference extends DialogPreference
{
    private Map<String, Object> m_initValues = null;
    private final ViewHolder V = new ViewHolder();
    
    @SuppressLint("NewApi")
    public SeekBarPreference(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
    {
        super(context, attrs, defStyleAttr, defStyleRes);
        CreateUI(attrs);
    }
    
    public SeekBarPreference(Context context, AttributeSet attrs, int defStyleAttr)
    {
        super(context, attrs, defStyleAttr); 
        CreateUI(attrs);  
    }
    
    public SeekBarPreference(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        CreateUI(attrs);
    }
    
    public SeekBarPreference(Context context)
    {
        this(context, null);
    }

    private void CreateUI(AttributeSet attrs)
    {
        setDialogLayoutResource(R.layout.seek_bar_dialog_preference);
        if(attrs != null)
        {
            TypedArray ta = getContext().obtainStyledAttributes(attrs, R.styleable.SeekBarDialogPreference);
            m_initValues = new HashMap<>();
            m_initValues.put("min", ta.getInt(R.styleable.SeekBarDialogPreference_min, 0));
            m_initValues.put("max", ta.getInt(R.styleable.SeekBarDialogPreference_max, 100));  
            m_initValues.put("suffix", ta.getString(R.styleable.SeekBarDialogPreference_suffix));
            m_initValues.put("editable", ta.getBoolean(R.styleable.SeekBarDialogPreference_editable, true));   
            ta.recycle();
        }
    }
    
    private void SetupUI(View view)
    {
        V.Setup(view);
        if(m_initValues != null)
        {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                Object min = m_initValues.get("min");
                V.seek_bar.setMin(min != null ? (int)min : V.seek_bar.getMin());
            }
            Object max = m_initValues.get("max");
            if(max != null)
                V.seek_bar.setMax((int)max);
            Object suffix = m_initValues.get("suffix");
            if(suffix != null)
                V.suffix.setText(suffix.toString());
            Object editable = m_initValues.get("editable");
            if(editable != null)
                V.seek_bar.setEnabled((boolean)editable);
        }
        V.seek_bar.setProgress(getPersistedInt(0));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            V.min.setText("" + V.seek_bar.getMin());
        }
        V.max.setText("" + V.seek_bar.getMax());
        V.progress.setText("" + V.seek_bar.getProgress());
        V.seek_bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
                {
                    V.progress.setText("" + progress);
                }
                public void onStartTrackingTouch(SeekBar seekBar)
                {
                    V.progress.setTextColor(Color.RED);
                }
                public void onStopTrackingTouch(SeekBar seekBar)
                {
                    V.progress.setTextColor(Color.BLACK);
                }
        });
    }
    
    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);
        if(positiveResult)
            persistInt(V.seek_bar.getProgress());
    }

    @Override
    protected View onCreateDialogView()
    {
        View view = super.onCreateDialogView();
        SetupUI(view);
        return view;
    }
    
    private class ViewHolder
    {
        private SeekBar seek_bar;
        private TextView min;
        private TextView max;
        private TextView progress;
        private TextView suffix;
        
        public void Setup(View view)
        {
            seek_bar = view.findViewById(R.id.seek_bar_dialog_preference_layout_seekbar);
            min = view.findViewById(R.id.seek_bar_dialog_preference_layout_min);
            max = view.findViewById(R.id.seek_bar_dialog_preference_layout_max);
            progress = view.findViewById(R.id.seek_bar_dialog_preference_layout_progress);
            suffix = view.findViewById(R.id.seek_bar_dialog_preference_layout_suffix);
        }
    }
}
