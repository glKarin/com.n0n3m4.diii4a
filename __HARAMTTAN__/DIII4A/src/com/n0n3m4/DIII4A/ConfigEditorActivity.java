package com.n0n3m4.DIII4A;
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

public class ConfigEditorActivity extends Activity
{
    private TextView m_titleText;
    private EditText m_editText;
    private Button m_saveBtn;
    private Button m_reloadBtn;
    private String m_filePath;
    private File m_file;
    private boolean m_edited = false;

    private View.OnClickListener m_buttonClickListener = new View.OnClickListener(){
        @Override
        public void onClick(View p1)
        {
            switch (p1.getId())
            {
                case R.id.editor_page_save_btn:
                    if (IsValid())
                    {
                        if (SaveFile())  
                        {
                            m_edited = false;
                            m_titleText.setText(m_filePath);
                            m_titleText.setTextColor(Color.BLACK);
                            m_saveBtn.setEnabled(false);
                            m_reloadBtn.setEnabled(false);  
                            Toast.makeText(ConfigEditorActivity.this, "Save file successful.", Toast.LENGTH_LONG).show();
                        }
                        else
                            Toast.makeText(ConfigEditorActivity.this, "Save file failed!", Toast.LENGTH_LONG).show(); 

                    }
                    else
                        Toast.makeText(ConfigEditorActivity.this, "No file!", Toast.LENGTH_LONG).show();
                    break;
                case R.id.editor_page_reload_btn:
                    if (IsValid())
                        LoadFile(m_filePath);
                    else
                        Toast.makeText(ConfigEditorActivity.this, "No file!", Toast.LENGTH_LONG).show();
                    break;
                default:
                    break;
            }
        }
    };
    private TextWatcher m_textWatcher = new TextWatcher() {           
    public void onTextChanged(CharSequence s, int start, int before, int count) {
        if(m_edited)
            return;
        m_edited = true; 
        m_titleText.setText(m_filePath + "*");
        m_titleText.setTextColor(Color.RED);
        m_saveBtn.setEnabled(true);
        m_reloadBtn.setEnabled(true);  
    }           
    public void beforeTextChanged(CharSequence s, int start, int count,int after) {}            
    public void afterTextChanged(Editable s) {}
    };

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.editor_page);

        m_titleText = (TextView)findViewById(R.id.editor_page_title);
        m_editText = (EditText)findViewById(R.id.editor_page_editor);
        m_saveBtn = (Button)findViewById(R.id.editor_page_save_btn);
        m_reloadBtn = (Button)findViewById(R.id.editor_page_reload_btn);

        SetupUI();
    }

    private void SetupUI()
    {
        m_saveBtn.setOnClickListener(m_buttonClickListener);
        m_reloadBtn.setOnClickListener(m_buttonClickListener);
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

        m_saveBtn.setEnabled(false);
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
        if (file.isFile())
        {
            FileReader reader = null;
            try
            {
                reader = new FileReader(file);
                int BUF_SIZE = 1024;
                char chars[] = new char[BUF_SIZE];
                int len;
                StringBuffer sb = new StringBuffer();
                while ((len = reader.read(chars)) > 0)
                    sb.append(chars, 0, len);
                m_editText.setText(sb.toString());
                //m_saveBtn.setClickable(true);
                //m_reloadBtn.setClickable(true);
                m_editText.setFocusableInTouchMode(true);
                m_editText.addTextChangedListener(m_textWatcher);
                m_titleText.setText(path);
                m_file = file;
                m_filePath = path;
                return true;
            }
            catch (IOException e)
            {
                e.printStackTrace();
            }
            finally
            {
                try
                {
                    if (reader != null)
                        reader.close();
                }
                catch (IOException e)
                {
                    e.printStackTrace();
                }
            }
        }
        //else
        {
            Reset();
            return false;
        }
    }

    private boolean SaveFile()
    {
        if (m_file == null)
            return false;

        FileWriter writer = null;
        try
        {
            writer = new FileWriter(m_file);
            writer.append(m_editText.getText());
            writer.flush();
            return true;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            try
            {
                if (writer != null)
                    writer.close();
            }
            catch (IOException e)
            {
                e.printStackTrace();
            }
        }
    }

    private boolean IsValid()
    {
        return m_file != null;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        switch(keyCode)
        {
            case KeyEvent.KEYCODE_BACK:
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
                        return true;
                    }
                }
                break;
            default:
                break;
        }
        return super.onKeyDown(keyCode, event);
	}
}
