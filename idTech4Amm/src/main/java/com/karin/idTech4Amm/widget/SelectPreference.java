package com.karin.idTech4Amm.widget;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.os.Build;
import android.os.Parcel;
import android.os.Parcelable;
import android.preference.DialogPreference;
import android.preference.ListPreference;
import android.support.annotation.ArrayRes;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

import com.karin.idTech4Amm.R;
import com.n0n3m4.q3e.Q3EUtils;

/**
 * Select preference widget
 */
public class SelectPreference extends DialogPreference
{
    private CharSequence[] mEntries;
    private CharSequence[] mEntryValues;
    private String mValue;
    private String mSummary;
    private int mClickedDialogEntryIndex;
    private boolean mValueSet;

    private boolean mIsInt;
    private boolean mPositiveManually;
    private boolean mResetDefault;
    private String mDefValue;

    @SuppressLint("NewApi")
    public SelectPreference(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes)
    {
        super(context, attrs, defStyleAttr, defStyleRes);
        CreateUI(attrs, defStyleAttr, defStyleRes);
    }

    public SelectPreference(Context context, AttributeSet attrs, int defStyleAttr)
    {
        this(context, attrs, defStyleAttr, 0);
    }

    public SelectPreference(Context context, AttributeSet attrs)
    {
        this(context, attrs, android.R.attr.dialogPreferenceStyle);
    }

    public SelectPreference(Context context)
    {
        this(context, null);
    }

    private void CreateUI(AttributeSet attrs, int defStyleAttr, int defStyleRes)
    {
        Context context = getContext();
        Resources resources = context.getResources();
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.SelectDialogPreference, defStyleAttr, defStyleRes);
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
            int resourceId = a.getResourceId(R.styleable.SelectDialogPreference_entryValues, 0);
            if(resourceId > 0)
            {
                TypedArray ta = resources.obtainTypedArray(resourceId);
                int type = ta.getType(0);
                if (type == TypedValue.TYPE_INT_DEC/* || type == TypedValue.TYPE_INT_HEX*/)
                {
                    int[] arr = resources.getIntArray(resourceId);
                    mIsInt = true;
                    mEntryValues = new CharSequence[arr.length];
                    for (int i = 0; i < arr.length; i++) {
                        mEntryValues[i] = Integer.toString(arr[i]);
                    }
                }
                else
                    mEntryValues = a.getTextArray(R.styleable.SelectDialogPreference_entryValues);
                ta.recycle();
            }
        }
        else
        {
            mEntryValues = a.getTextArray(R.styleable.SelectDialogPreference_entryValues);
        }
        mEntries = a.getTextArray(R.styleable.SelectDialogPreference_entries);
        mPositiveManually = a.getBoolean(R.styleable.SelectDialogPreference_positiveManually, false);
        mResetDefault = a.getBoolean(R.styleable.SelectDialogPreference_resetDefault, false);

        a.recycle();

        mSummary = "";
    }

    public void setEntries(CharSequence[] entries) {
        mEntries = entries;
    }

    public void setEntries(@ArrayRes int entriesResId) {
        setEntries(getContext().getResources().getTextArray(entriesResId));
    }

    public CharSequence[] getEntries() {
        return mEntries;
    }

    public void setEntryValues(CharSequence[] entryValues) {
        mEntryValues = entryValues;
        mIsInt = false;
    }

    public void setEntryValues(@ArrayRes int entryValuesResId) {
        Resources resources = getContext().getResources();


        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
            TypedArray ta = resources.obtainTypedArray(entryValuesResId);
            int type = ta.getType(0);
            if (type == TypedValue.TYPE_INT_DEC/* || type == TypedValue.TYPE_INT_HEX*/)
            {
                int[] arr = resources.getIntArray(entryValuesResId);
                CharSequence[] carr = new CharSequence[arr.length];
                for (int i = 0; i < arr.length; i++) {
                    carr[i] = Integer.toString(arr[i]);
                }
                setEntryValues(carr);
                mIsInt = true;
            }
            else
                setEntryValues(resources.getTextArray(entryValuesResId));
            ta.recycle();
        }
        else
        {
            setEntryValues(resources.getTextArray(entryValuesResId));
        }
    }

    public CharSequence[] getEntryValues() {
        return mEntryValues;
    }

    public void setValue(Object obj) {
        String value = null != obj ? obj.toString() : null;
        // Always persist/notify the first time.
        final boolean changed = !TextUtils.equals(mValue, value);
        if (changed || !mValueSet) {
            mValue = value;
            mValueSet = true;
            if(mIsInt)
                persistInt(Q3EUtils.parseInt_s(value));
            else
                persistString(value);
            if (changed) {
                notifyChanged();
            }
        }
        int indexOfValue = findIndexOfValue(mValue);
        setSummary( indexOfValue != -1 ? mEntries[indexOfValue] : "");
    }

    @Override
    public CharSequence getSummary() {
        final CharSequence entry = getEntry();
        if (mSummary == null) {
            return super.getSummary();
        } else {
            return String.format(mSummary, entry == null ? "" : entry);
        }
    }

    public void setSummary(CharSequence summary) {
        super.setSummary(summary);
        if (summary == null && mSummary != null) {
            mSummary = null;
        } else if (summary != null && !summary.equals(mSummary)) {
            mSummary = summary.toString();
        }
    }

    public void setValueIndex(int index) {
        if (mEntryValues != null) {
            setValue(mEntryValues[index].toString());
        }
    }

    public String getValue() {
        return mValue;
    }

    public CharSequence getEntry() {
        int index = getValueIndex();
        return index >= 0 && mEntries != null ? mEntries[index] : null;
    }

    public int findIndexOfValue(String value) {
        if (value != null && mEntryValues != null) {
            for (int i = mEntryValues.length - 1; i >= 0; i--) {
                if (mEntryValues[i].equals(value)) {
                    return i;
                }
            }
        }
        return -1;
    }

    private int getValueIndex() {
        return findIndexOfValue(mValue);
    }



    @Override
    protected void onPrepareDialogBuilder(AlertDialog.Builder builder) {
        super.onPrepareDialogBuilder(builder);

        if (mEntries == null || mEntryValues == null) {
            throw new IllegalStateException(
                    "ListPreference requires an entries array and an entryValues array.");
        }

        mClickedDialogEntryIndex = getValueIndex();
        builder.setSingleChoiceItems(mEntries, mClickedDialogEntryIndex,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        mClickedDialogEntryIndex = which;
                        if(mPositiveManually)
                            return;

                        /*
                         * Clicking on an item simulates the positive button
                         * click, and dismisses the dialog.
                         */
                        SelectPreference.this.onClick(dialog, DialogInterface.BUTTON_POSITIVE);
                        postDismiss();
                    }
                });

        /*
         * The typical interaction for list-based dialogs is to have
         * click-on-an-item dismiss the dialog instead of the user having to
         * press 'Ok'.
         */
        if(mPositiveManually)
        {
            builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        /*
                         * Clicking on an item simulates the positive button
                         * click, and dismisses the dialog.
                         */
                        SelectPreference.this.onClick(dialog, DialogInterface.BUTTON_POSITIVE);
                        postDismiss();
                    }
                });
        }
        else
        builder.setPositiveButton(null, null);

        if(mResetDefault)
        {
            builder.setNeutralButton(R.string.reset, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    int index = findIndexOfValue(mDefValue);
                    mClickedDialogEntryIndex = index;
                    /*
                     * Clicking on an item simulates the positive button
                     * click, and dismisses the dialog.
                     */
                    SelectPreference.this.onClick(dialog, DialogInterface.BUTTON_POSITIVE);
                    if(!mPositiveManually)
                        postDismiss();
                }
            });
        }
    }
    
    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);

        if (positiveResult && mClickedDialogEntryIndex >= 0 && mEntryValues != null) {
            String value = mEntryValues[mClickedDialogEntryIndex].toString();
            if (callChangeListener(value)) {
                setValue(value);
            }
        }
    }

    @Override
    protected Object onGetDefaultValue(TypedArray a, int index) {
        return a.getString(index);
    }

    @Override
    protected void onSetInitialValue(boolean restoreValue, Object defaultValue) {
        if(null != defaultValue)
            defaultValue = defaultValue.toString();
        setValue(restoreValue ? getPersistedString(mValue) : (String) defaultValue);
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        final Parcelable superState = super.onSaveInstanceState();
        if (isPersistent()) {
            // No need to save instance state since it's persistent
            return superState;
        }

        final SavedState myState = new SavedState(superState);
        myState.value = getValue();
        return myState;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        if (state == null || !state.getClass().equals(SavedState.class)) {
            // Didn't save state for us in onSaveInstanceState
            super.onRestoreInstanceState(state);
            return;
        }

        SavedState myState = (SavedState) state;
        super.onRestoreInstanceState(myState.getSuperState());
        setValue(myState.value);
    }

    private static class SavedState extends BaseSavedState {
        String value;

        public SavedState(Parcel source) {
            super(source);
            value = source.readString();
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            super.writeToParcel(dest, flags);
            dest.writeString(value);
        }

        public SavedState(Parcelable superState) {
            super(superState);
        }

        public static final Parcelable.Creator<SavedState> CREATOR =
                new Parcelable.Creator<SavedState>() {
                    public SavedState createFromParcel(Parcel in) {
                        return new SavedState(in);
                    }

                    public SavedState[] newArray(int size) {
                        return new SavedState[size];
                    }
                };
    }

    private final Runnable mDismissRunnable = new Runnable() {
        @Override
        public void run() {
            Dialog mDialog = getDialog();
            //if(null != mDialog)
            mDialog.dismiss();
        }
    };

    private View getDecorView() {
        Dialog mDialog = getDialog();
        if (mDialog != null && mDialog.getWindow() != null) {
            return mDialog.getWindow().getDecorView();
        }
        return null;
    }

    private void removeDismissCallbacks() {
        View decorView = getDecorView();
        if (decorView != null) {
            decorView.removeCallbacks(mDismissRunnable);
        }
    }

    void postDismiss() {
        removeDismissCallbacks();
        View decorView = getDecorView();
        if (decorView != null) {
            // If decorView is null, dialog was already dismissed
            decorView.post(mDismissRunnable);
        }
    }

    public void setDefaultValue(Object defaultValue) {
        super.setDefaultValue(defaultValue);
        mDefValue = null != defaultValue ? defaultValue.toString() : null;
    }
}
