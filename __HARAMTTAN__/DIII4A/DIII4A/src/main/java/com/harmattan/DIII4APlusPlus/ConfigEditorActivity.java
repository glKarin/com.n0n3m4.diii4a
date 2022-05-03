package com.harmattan.DIII4APlusPlus;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.Button;
import android.view.View;
import android.view.View.OnClickListener;
import android.content.Intent;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import android.widget.Toast;
import java.io.FileWriter;
import android.view.KeyEvent;
import android.text.TextWatcher;
import android.text.Editable;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.Build;
import android.content.pm.ActivityInfo;
import android.preference.PreferenceManager;
import com.n0n3m4.q3e.Q3EUtils;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import com.harmattan.DIII4APlusPlus.FileUtility;
import com.harmattan.DIII4APlusPlus.Constants;
import com.harmattan.DIII4APlusPlus.ContextUtility;
import com.n0n3m4.DIII4A.R;

public class ConfigEditorActivity extends Activity
{
    private TextView m_titleText;
    private EditText m_editText;
    private MenuItem m_saveBtn;
    private MenuItem m_reloadBtn;
    private String m_filePath;
    private File m_file;
    private boolean m_edited = false;

    private TextWatcher m_textWatcher = new TextWatcher() {           
    public void onTextChanged(CharSequence s, int start, int before, int count) {
        if(m_edited)
            return;
        m_edited = true; 
        m_titleText.setText(m_filePath + "*");
        m_titleText.setTextColor(Color.RED);
        if(m_saveBtn != null)
            m_saveBtn.setEnabled(true);
        if(m_reloadBtn != null)
            m_reloadBtn.setEnabled(true);  
    }           
    public void beforeTextChanged(CharSequence s, int start, int count,int after) {}            
    public void afterTextChanged(Editable s) {}
    };

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        boolean o = PreferenceManager.getDefaultSharedPreferences(this).getBoolean(Constants.PreferenceKey.LAUNCHER_ORIENTATION, false);
        ContextUtility.SetScreenOrientation(this, o ? 0 : 1);

        setContentView(R.layout.editor_page);

        m_titleText = (TextView)findViewById(R.id.editor_page_title);
        m_editText = (EditText)findViewById(R.id.editor_page_editor);

        SetupUI();
    }

    private void SetupUI()
    {
        Intent intent = getIntent();
        String path = intent.getStringExtra("CONST_FILE_PATH");
        if (path != null)
        {
            LoadFile(path);
        }
    }

    private void Reset()
    {
        m_editText.removeTextChangedListener(m_textWatcher);
        m_edited = false;
        m_filePath = null;
        m_file = null;

        if(m_saveBtn != null)
            m_saveBtn.setEnabled(false);
        if(m_reloadBtn != null)
            m_reloadBtn.setEnabled(false);
        m_editText.setText("");
        m_titleText.setText("");
        m_editText.setFocusableInTouchMode(false);
        m_titleText.setTextColor(Color.BLACK);
    }

    private boolean LoadFile(String path)
    {
        Reset();

        File file = new File(path);
        String text = FileUtility.file_get_contents(file);
        if (text != null)
        {
                m_editText.setText(text);
                //m_saveBtn.setClickable(true);
                //m_reloadBtn.setClickable(true);
                m_editText.setFocusableInTouchMode(true);
                m_editText.addTextChangedListener(m_textWatcher);
                m_titleText.setText(path);
                m_file = file;
                m_filePath = path;
                return true;
        }
        else
        {
            Reset();
            return false;
        }
    }

    private boolean SaveFile()
    {
        if (m_file == null)
            return false;
        return FileUtility.file_put_contents(m_file, m_editText.getText().toString());
    }

    private boolean IsValid()
    {
        return m_file != null;
    }

    @Override
    public void onBackPressed()
    {
        if(IsValid() && m_edited)
        {
            DialogInterface.OnClickListener listener = new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which)
                {
                    switch(which)
                    {
                        case DialogInterface.BUTTON_POSITIVE:
                            SaveFile();
                            dialog.dismiss();
                            finish();
                            break;
                        case DialogInterface.BUTTON_NEGATIVE:
                            dialog.dismiss();
                            finish();
                            break;
                        case DialogInterface.BUTTON_NEUTRAL:
                        default:
                            dialog.dismiss();
                            break;
                    }
                }
            };
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle("Warning");
            builder.setMessage("The text is modified since open, save changes to file?");
            builder.setPositiveButton("Yes", listener);
            builder.setNegativeButton("No", listener);
            builder.setNeutralButton("Cancel", listener);
            AlertDialog dialog = builder.create();
            dialog.show();
        }
        else
            super.onBackPressed();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.config_editor_menu, menu);
        m_saveBtn = (MenuItem)menu.findItem(R.id.config_editor_menu_save);
        m_reloadBtn = (MenuItem)menu.findItem(R.id.config_editor_menu_reload);
        m_saveBtn.setEnabled(false);
        m_reloadBtn.setEnabled(false);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        switch(item.getItemId())
        {
            case R.id.config_editor_menu_save:
                if (IsValid())
                {
                    if (SaveFile())  
                    {
                        m_edited = false;
                        m_titleText.setText(m_filePath);
                        m_titleText.setTextColor(Color.BLACK);
                        if(m_saveBtn != null)
                            m_saveBtn.setEnabled(false);
                        if(m_reloadBtn != null)
                            m_reloadBtn.setEnabled(false);  
                        Toast.makeText(ConfigEditorActivity.this, "Save file successful.", Toast.LENGTH_LONG).show();
                    }
                    else
                        Toast.makeText(ConfigEditorActivity.this, "Save file failed!", Toast.LENGTH_LONG).show(); 

                }
                else
                    Toast.makeText(ConfigEditorActivity.this, "No file!", Toast.LENGTH_LONG).show();
                break;
                case R.id.config_editor_menu_reload:
                if (IsValid())
                    LoadFile(m_filePath);
                else
                    Toast.makeText(ConfigEditorActivity.this, "No file!", Toast.LENGTH_LONG).show();
                    break;
                default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }
}
