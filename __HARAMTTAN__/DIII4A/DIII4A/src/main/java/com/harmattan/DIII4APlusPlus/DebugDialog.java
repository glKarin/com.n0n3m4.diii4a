package com.harmattan.DIII4APlusPlus;
import android.content.Context;
import android.content.DialogInterface;
import android.app.AlertDialog;
import android.os.Bundle;
import android.widget.CheckBox;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import com.n0n3m4.q3e.Q3EUtils;
import android.widget.CompoundButton;
import android.app.Activity;
import android.widget.Button;
import android.view.View.OnClickListener;
import android.view.View;
import android.widget.RadioGroup.LayoutParams;
import android.view.ViewGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.app.DialogFragment;
import android.view.LayoutInflater;
import android.app.FragmentManager;
import android.app.Dialog;
import com.n0n3m4.DIII4A.R;

public class DebugDialog extends DialogFragment
{

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
    {
        View view = inflater.inflate(R.layout.launcher_settings_dialog, container);

        FragmentManager manager = getChildFragmentManager();
        manager.beginTransaction()
            .add(R.id.launcher_settings_dialog_main_layout, new DebugPreference(), "_Debug_preference")
            .commit()
            ;
        return view;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState)
    {
        Dialog dialog = super.onCreateDialog(savedInstanceState);
        dialog.setTitle("Debug");
        return dialog;
    }

    public static DebugDialog newInstance()
    {
        DebugDialog dialog = new DebugDialog();
        return dialog;
    }
}
