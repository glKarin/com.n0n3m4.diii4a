package com.karin.idTech4Amm;

import android.app.Activity;
import android.app.AlertDialog;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.misc.Function;
import com.karin.idTech4Amm.sys.Constants;
import com.karin.idTech4Amm.sys.PreferenceKey;
import com.n0n3m4.q3e.karin.Theme;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;
import com.karin.idTech4Amm.misc.KLogcat;
import com.n0n3m4.q3e.karin.KStr;

/**
 * logcat viewer
 */
public class LogcatActivity extends Activity
{
    private final ViewHolder V = new ViewHolder();
    private KLogcat m_logcat;
    private String m_filterText = "";
    private final KLogcat.KLogcatCallback m_callback = new KLogcat.KLogcatCallback() {
        @Override
        public void Output(String str)
        {
            runOnUiThread(new Runnable()
            {
                @Override
                public void run()
                {
                    V.logtext.append(str + "\n");
                    if(null != V.scrollCheckBox && V.scrollCheckBox.isChecked())
                        V.logscroll.smoothScrollTo(0, V.logtext.getHeight());
                }
            });
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Q3ELang.Locale(this);

        boolean o = PreferenceManager.getDefaultSharedPreferences(this).getBoolean(PreferenceKey.LAUNCHER_ORIENTATION, false);
        ContextUtility.SetScreenOrientation(this, o ? 0 : 1);

        Theme.SetTheme(this, false);
        setContentView(R.layout.logcat_page);

        m_logcat = new KLogcat();
        SetupCommand("idTech4Amm", false, false, -2);

        V.SetupUI();

        SetupUI();

        Start(false);
    }

    private void SetupCommand(String str, boolean run, boolean clear, int scroll)
    {
        if(m_filterText.equals(str))
            return;

        m_filterText = str;
        String cmd = "logcat";

        if(KStr.NotBlank(m_filterText))
            cmd += " | grep " + m_filterText;

        Stop();
        m_logcat.SetCommand(cmd);

        if(run)
        {
            Start(clear);
            if(scroll >= -1)
                ScrollToBottom(scroll);
        }
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        if(null == m_logcat)
            Start(true);
        else
            Resume();
    }

    @Override
    protected void onPause()
    {
        super.onPause();
        Pause();
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        Stop();
    }

    private void Start(boolean clear)
    {
        if(clear)
            Clear();
        m_logcat.Start(m_callback);
        if(null != V.runBtn)
            V.runBtn.setTitle(R.string.stop);
    }

    private void Stop()
    {
        if(null != V.runBtn)
            V.runBtn.setTitle(R.string.start);
        if(null != m_logcat)
            m_logcat.Stop();
    }

    private void Pause()
    {
        if(null != V.runBtn)
            V.runBtn.setTitle(R.string.start);
        m_logcat.Pause();
    }

    private void Resume()
    {
        m_logcat.Resume();
        if(null != V.runBtn)
            V.runBtn.setTitle(R.string.stop);
    }

    private void SetupUI()
    {
        //V.logtext.setTextColor(Theme.BlackColor(this));
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.logcat_menu, menu);
        V.SetupMenu(menu);
        return true;
    }

    private void Clear()
    {
        if(null != V.logtext)
            V.logtext.setText("");
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        int itemId = item.getItemId();
        if (itemId == R.id.logcat_menu_clear)
        {
            Clear();
        }
        else if (itemId == R.id.logcat_menu_dump)
        {
            String path = Q3EUtils.GetAppStoragePath(this, "/logcat");
            if(Q3EUtils.mkdir(path, true))
            {
                String filePath = path + "/" + Constants.CONST_APP_NAME + "_logcat.log";
                Q3EUtils.file_put_contents(filePath, V.logtext.getText().toString());
                Toast.makeText(LogcatActivity.this, "Save logcat to " + filePath, Toast.LENGTH_LONG).show();
            }
            else
                Toast.makeText(LogcatActivity.this, R.string.fail, Toast.LENGTH_LONG).show();
        }
        else if (itemId == R.id.logcat_menu_run)
        {
            if(null != m_logcat)
            {
                if(m_logcat.IsRunning())
                {
                    if(m_logcat.IsPaused())
                        m_logcat.Resume();
                    else
                        m_logcat.Pause();
                }
                else
                {
                    Stop();
                    Start(true);
                }
            }
            else
                Start(true);
        }
        else if (itemId == R.id.logcat_menu_up)
        {
            V.logscroll.scrollTo(0, 0);
        }
        else if (itemId == R.id.logcat_menu_down)
        {
            ScrollToBottom();
        }
        else if (itemId == R.id.logcat_menu_scroll)
        {
            V.scrollCheckBox.setChecked(!V.scrollCheckBox.isChecked());
            boolean checked = V.scrollCheckBox.isChecked();
            if(checked)
                V.logscroll.scrollTo(0, V.logtext.getHeight());
            V.scrollCheckBox.setTitle(checked ? R.string.pause : R.string.scroll);
        }
        else if (itemId == R.id.logcat_menu_filter)
        {
            OpenFilterDialog();
        }
        return super.onOptionsItemSelected(item);
    }

    private void ScrollToBottom()
    {
        V.logscroll.scrollTo(0, V.logtext.getHeight());
    }

    private void ScrollToBottom(int delay)
    {
        if(null != V.logscroll && null != V.logtext)
        {
            if(delay == 0)
                V.logtext.post(this::ScrollToBottom);
            else if(delay > 0)
                V.logtext.postDelayed(this::ScrollToBottom, delay);
            else
                ScrollToBottom();
        }
    }

    private void OpenFilterDialog()
    {
        String[] args = { m_filterText };
        AlertDialog input = ContextUtility.Input(this, Q3ELang.tr(this, R.string.filter), Q3ELang.tr(this, R.string.filter), args, new Runnable() {
            @Override
            public void run()
            {
                String arg = args[0];
                SetupCommand(arg, true, true, 1000);
            }
        }, null, Q3ELang.tr(this, R.string._default), new Runnable() {
            @Override
            public void run()
            {
                SetupCommand("idTech4Amm", true, true, 1000);
            }
        }, new Function() {
            @Override
            public Object Invoke(Object... args)
            {
                //EditText editText = (EditText)args[0];
                return null;
            }
        });
    }

    private class ViewHolder
    {
        private TextView logtext;
        private ScrollView logscroll;
        private MenuItem runBtn;
        private MenuItem scrollCheckBox;

        public void SetupUI()
        {
            logtext = findViewById(R.id.logtext);
            logscroll = findViewById(R.id.logscroll);
        }

        public void SetupMenu(Menu menu)
        {
            runBtn = menu.findItem(R.id.logcat_menu_run);
            runBtn.setTitle(R.string.stop);
            scrollCheckBox = menu.findItem(R.id.logcat_menu_scroll);
            scrollCheckBox.setTitle(R.string.scroll);
        }
    }
}
