package com.karin.idTech4Amm.ui;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.app.DialogFragment;
import android.view.LayoutInflater;
import android.app.FragmentManager;
import android.app.Dialog;

import com.karin.idTech4Amm.R;

/**
 * Experimental dialog
 */
public class ExperimentalDialog extends DialogFragment
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

        FragmentManager manager;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.JELLY_BEAN_MR1)
            manager = getChildFragmentManager();
        else
            manager = getFragmentManager();
        manager.beginTransaction()
            .add(R.id.launcher_settings_dialog_main_layout, new ExperimentalPreference(), "_Experimental_preference")
            .commit()
            ;
        return view;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState)
    {
        Dialog dialog = super.onCreateDialog(savedInstanceState);
        dialog.setTitle(R.string.experimental);
        return dialog;
    }

    public static ExperimentalDialog newInstance()
    {
        ExperimentalDialog dialog = new ExperimentalDialog();
        return dialog;
    }
}
