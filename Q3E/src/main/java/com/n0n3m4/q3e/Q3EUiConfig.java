/*
	Copyright (C) 2012 n0n3m4
	
    This file is part of Q3E.

    Q3E is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Q3E is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Q3E.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.n0n3m4.q3e;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import com.n0n3m4.q3e.onscreen.Q3EControls;

@SuppressLint("NonConstantResourceId")
public class Q3EUiConfig extends Activity
{
    private static int m_onScreenButtonGlobalOpacity = Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_OPACITY;
    private static float m_onScreenButtonGlobalSizeScale = Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE;
    private static boolean m_onScreenButtonFriendlyEdge = Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_FRIENDLY_EDGE;

    Q3EUiView vw;
    //k
    private boolean m_hideNav = true;
    @SuppressLint("InlinedApi")
    private final int m_uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_FULLSCREEN;
    @SuppressLint("InlinedApi")
    private final int m_uiOptions_def = View.SYSTEM_UI_FLAG_FULLSCREEN
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && preferences.getBoolean(Q3EPreference.COVER_EDGES, true))
        {
            WindowManager.LayoutParams lp = getWindow().getAttributes();
            lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(lp);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) // 9
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);

        //k
        m_hideNav = preferences.getBoolean(Q3EPreference.HIDE_NAVIGATION_BAR, true);
        SetupUIFlags();

        super.onCreate(savedInstanceState);

        RelativeLayout mainLayout = new RelativeLayout(this);
        RelativeLayout.LayoutParams params;

        params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        vw = new Q3EUiView(this);
		//vw.setZOrderOnTop();
		vw.setZOrderMediaOverlay(true);
        mainLayout.addView(vw, params);

        int px = Q3EUtils.dip2px(this, 48);
		params = new RelativeLayout.LayoutParams(px, px);
        params.addRule(RelativeLayout.ALIGN_PARENT_TOP | RelativeLayout.CENTER_HORIZONTAL);
		ImageView btn = new ImageView(this);
		btn.setAlpha(0.75f);
		btn.setFocusable(false);
		btn.setFocusableInTouchMode(false);
		btn.setImageDrawable(getResources().getDrawable(R.drawable.icon_m_settings));
		mainLayout.addView(btn, params);

        setContentView(mainLayout);
        btn.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                openOptionsMenu();
            }
        });
    }

    @Override
    protected void onPause()
    {
        vw.SaveAll();
        super.onPause();
    }

    @Override
    public void onBackPressed()
    {
        vw.SaveAll();
        super.onBackPressed();
    }

    //k
    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        SetupUIFlags();
    }

    private void SetupUIFlags()
    {
        final View decorView = getWindow().getDecorView();
        if (m_hideNav)
            decorView.setSystemUiVisibility(m_uiOptions);
        else
            decorView.setSystemUiVisibility(m_uiOptions_def);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.uiconfig_menu, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        int itemId = item.getItemId();
        if (itemId == R.id.uiconfig_menu_opacity)
        {
            OpenOnScreenButtonOpacitySetting();
            return true;
        }
        else if (itemId == R.id.uiconfig_menu_size)
        {
            OpenOnScreenButtonSizeSetting();
            return true;
        }
        else if (itemId == R.id.reset_onscreen_controls_btn)
        {
            resetcontrols(null);
            return true;
        }
        else if (itemId == R.id.uiconfig_save)
        {
            SaveAll();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void OpenOnScreenButtonSizeSetting()
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Setup all on-screen button size");
        View widget = getLayoutInflater().inflate(R.layout.edit_line, null, false);

        EditText editText = widget.findViewById(R.id.edit_line_text);
        editText.setText("" + m_onScreenButtonGlobalSizeScale);
        editText.setEms(10);
        editText.setInputType(EditorInfo.TYPE_CLASS_NUMBER | EditorInfo.TYPE_TEXT_FLAG_NO_SUGGESTIONS | EditorInfo.TYPE_NUMBER_FLAG_DECIMAL);
        editText.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI);
        editText.setHint("Size's scale factor");

        TextView textView = widget.findViewById(R.id.edit_line_label);
        textView.setText("Scale factor");

        builder.setView(widget);
        builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                m_onScreenButtonGlobalSizeScale = Q3EUtils.parseFloat_s(editText.getText().toString(), Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE);
                SetupOnScreenButtonSize(m_onScreenButtonGlobalSizeScale);
            }
        })
                .setNeutralButton("Reset", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        SetupOnScreenButtonSize(Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE);
                    }
                })
                .setNegativeButton("Cancel", null);
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private void SetupOnScreenButtonSize(float scale)
    {
        //Q3EControls.SetupAllSize(this, scale, false);
        vw.UpdateOnScreenButtonsSize(scale);
        Toast.makeText(this, "Setup all on-screen buttons size done.", Toast.LENGTH_SHORT).show();
    }

    private void SetupOnScreenButtonOpacity(int alpha)
    {
        //Q3EControls.SetupAllOpacity(this, alpha, false);
        vw.UpdateOnScreenButtonsOpacity((float)alpha / 100.0f);
        Toast.makeText(this, "Setup all on-screen buttons opacity done.", Toast.LENGTH_SHORT).show();
    }

    private void OpenOnScreenButtonOpacitySetting()
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Setup all on-screen button opacity");
        View seekBarWidget = getLayoutInflater().inflate(R.layout.seek_bar_dialog_preference, null, false);
        builder.setView(seekBarWidget);
        final SeekBar seekBar = seekBarWidget.findViewById(R.id.seek_bar_dialog_preference_layout_seekbar);
        final TextView min = seekBarWidget.findViewById(R.id.seek_bar_dialog_preference_layout_min);
        final TextView max = seekBarWidget.findViewById(R.id.seek_bar_dialog_preference_layout_max);
        final TextView progress = seekBarWidget.findViewById(R.id.seek_bar_dialog_preference_layout_progress);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            seekBar.setMin(0);
        }
        seekBar.setMax(100);
        seekBar.setProgress(m_onScreenButtonGlobalOpacity);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
            min.setText("" + seekBar.getMin());
        else
            min.setText("0");
        max.setText("" + seekBar.getMax());
        progress.setText("" + seekBar.getProgress());
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int p, boolean fromUser)
            {
                progress.setText("" + p);
            }
            public void onStartTrackingTouch(SeekBar seekBar)
            {
                progress.setTextColor(Color.RED);
            }
            public void onStopTrackingTouch(SeekBar seekBar)
            {
                progress.setTextColor(Color.BLACK);
            }
        });
        builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                m_onScreenButtonGlobalOpacity = seekBar.getProgress();
                SetupOnScreenButtonOpacity(m_onScreenButtonGlobalOpacity);
            }
        })
                .setNeutralButton("Reset", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        SetupOnScreenButtonOpacity(Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_OPACITY);
                    }
                })
                .setNegativeButton("Cancel", null);
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    public void resetcontrols(View vw)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Reset on-screen controls");
        View widget = getLayoutInflater().inflate(R.layout.onscreen_button_reset_dialog, null, false);

        EditText sizeEdit = widget.findViewById(R.id.onscreen_button_reset_dialog_size);
        sizeEdit.setText("" + m_onScreenButtonGlobalSizeScale);
        sizeEdit.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                m_onScreenButtonGlobalSizeScale = Q3EUtils.parseFloat_s(s.toString(), Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE);
            }
        });

        CheckBox friendly = widget.findViewById(R.id.onscreen_button_reset_dialog_friendly);
        friendly.setChecked(m_onScreenButtonFriendlyEdge);
        friendly.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                m_onScreenButtonFriendlyEdge = isChecked;
            }
        });

        SeekBar opacity = widget.findViewById(R.id.onscreen_button_reset_dialog_opacity);
        TextView opacityLabel = widget.findViewById(R.id.onscreen_button_reset_dialog_opacity_label);
        opacity.setProgress(m_onScreenButtonGlobalOpacity);
        opacityLabel.setText(String.format("Opacity(%3d)", opacity.getProgress()));
        opacity.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if(fromUser)
                    m_onScreenButtonGlobalOpacity = opacity.getProgress();
                opacityLabel.setText(String.format("Opacity(%3d)", progress));
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                opacityLabel.setTextColor(Color.RED);
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                opacityLabel.setTextColor(Color.BLACK);
            }
        });

        builder.setView(widget);
        builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                ResetControlsLayout(m_onScreenButtonFriendlyEdge, m_onScreenButtonGlobalSizeScale, m_onScreenButtonGlobalOpacity);
            }
        })
                .setNeutralButton("Default", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        ResetControlsLayout(m_onScreenButtonFriendlyEdge, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_OPACITY);
                    }
                })
                .setNegativeButton("Cancel", null);
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private void ResetControlsLayout(boolean friendly, float scale, int opacity)
    {
        if(scale <= 0.0f)
            scale = 1.0f;
        vw.UpdateOnScreenButtonsPosition(friendly);
        vw.UpdateOnScreenButtonsOpacity((float)opacity / 100.0f);
        vw.UpdateOnScreenButtonsSize(scale);
        Toast.makeText(Q3EUiConfig.this, "On-screen controls has reset.", Toast.LENGTH_SHORT).show();
    }

    private void SaveAll()
    {
        vw.SaveAll();
        Toast.makeText(this, "All on-screen buttons configs saved.", Toast.LENGTH_SHORT).show();
    }
}
