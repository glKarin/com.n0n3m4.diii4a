package com.karin.idTech4Amm;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.InputDevice;
import android.view.InputEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.sys.PreferenceKey;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;
import com.n0n3m4.q3e.karin.Theme;

import java.util.ArrayList;
import java.util.List;

/**
 * test event
 */
public class EventTestActivity extends Activity
{
    private final ViewHolder V = new ViewHolder();
    private final StringBuilder buf = new StringBuilder();

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Q3ELang.Locale(this);

        boolean o = PreferenceManager.getDefaultSharedPreferences(this).getBoolean(PreferenceKey.LAUNCHER_ORIENTATION, false);
        ContextUtility.SetScreenOrientation(this, o ? 0 : 1);

        Theme.SetTheme(this, false);
        setContentView(R.layout.event_test_page);

        V.SetupUI();

        SetupUI();
    }

    @Override
    protected void onPause()
    {
        super.onPause();
        Clear();
    }

    private void Clear()
    {
        buf.setLength(0);
        V.eventResult.setText("");
        V.eventDeviceResult.setText("");
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        StartHandleEvent("KeyDown");
        HandleKeyEvent(event);
        HandleInputDevice(event.getDevice());
        return keyCode != KeyEvent.KEYCODE_BACK;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        StartHandleEvent("KeyUp");
        HandleKeyEvent(event);
        HandleInputDevice(event.getDevice());
        return keyCode != KeyEvent.KEYCODE_BACK;
    }

    private void SetupUI()
    {
        //V.logtext.setTextColor(Theme.BlackColor(this));
        V.view = new View(this) {
            @Override
            public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event)
            {
                StartHandleEvent("KeyMultiple");
                HandleKeyEvent(event);
                HandleInputDevice(event.getDevice());
                return true;
            }

            @Override
            public boolean onTouchEvent(MotionEvent event)
            {
                StartHandleEvent("Touch");
                HandleMotionEvent(event);
                HandleInputDevice(event.getDevice());
                return true;
            }

            @Override
            public boolean onCapturedPointerEvent(MotionEvent event)
            {
                StartHandleEvent("CapturedPointer");
                HandleMotionEvent(event);
                HandleInputDevice(event.getDevice());
                return true;
            }

            @Override
            public boolean onGenericMotionEvent(MotionEvent event)
            {
                StartHandleEvent("GenericMotion");
                HandleMotionEvent(event);
                HandleInputDevice(event.getDevice());
                return true;
            }

            @Override
            public boolean onTrackballEvent(MotionEvent event)
            {
                StartHandleEvent("Trackball");
                HandleMotionEvent(event);
                HandleInputDevice(event.getDevice());
                return true;
            }
        };

        RelativeLayout root = findViewById(R.id.eventTestRoot);
        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        V.view.setBackgroundColor(Color.TRANSPARENT);
        root.addView(V.view, params);
        Q3EUtils.SetViewZ(V.view, 5);

        Q3EUtils.SetViewZ(findViewById(R.id.eventTestButtons1), 10);
        Q3EUtils.SetViewZ(findViewById(R.id.eventTestButtons2), 10);
    }

    private void StartHandleEvent(String type)
    {
        Clear();
        AppendLine("Type: " + type);
    }

    private void HandleDeviceKeyboard(InputDevice device)
    {
        Append("Description: " + device.getKeyboardType() + " -> ");
        switch(device.getKeyboardType())
        {
            case InputDevice.KEYBOARD_TYPE_ALPHABETIC: AppendLine("Alphabetic"); break;
            case InputDevice.KEYBOARD_TYPE_NON_ALPHABETIC: AppendLine("Non-Alphabetic"); break;
            case InputDevice.KEYBOARD_TYPE_NONE: AppendLine("None"); break;
        }
    }

    private void HandleDeviceProduct(InputDevice device)
    {
        AppendLine("Product: " + device.getProductId());
    }

    private void HandleDeviceVendor(InputDevice device)
    {
        AppendLine("Vendor: " + device.getVendorId());
    }

    private void HandleSource(int source)
    {
        AppendLine("Source: source=" + HexString(source) + " class=" + HexString(source & InputDevice.SOURCE_CLASS_MASK));

        int SOURCE_BLUETOOTH_STYLUS;
        int SOURCE_MOUSE_RELATIVE;
        int SOURCE_ROTARY_ENCODER;
        int SOURCE_HDMI;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            SOURCE_MOUSE_RELATIVE = InputDevice.SOURCE_MOUSE_RELATIVE;
            SOURCE_ROTARY_ENCODER = InputDevice.SOURCE_ROTARY_ENCODER;
        } else {
            SOURCE_MOUSE_RELATIVE = 0x00020000 | InputDevice.SOURCE_CLASS_TRACKBALL;
            SOURCE_ROTARY_ENCODER = 0x00400000 | InputDevice.SOURCE_CLASS_NONE;
        }
        if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            SOURCE_HDMI = InputDevice.SOURCE_HDMI;
        } else {
            SOURCE_HDMI = 0x02000000 | InputDevice.SOURCE_CLASS_BUTTON;
        }
        if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            SOURCE_BLUETOOTH_STYLUS = InputDevice.SOURCE_BLUETOOTH_STYLUS;
        } else {
            SOURCE_BLUETOOTH_STYLUS = 0x00008000 | InputDevice.SOURCE_STYLUS;
        }

        final Object[] Sources = {
            InputDevice.SOURCE_KEYBOARD, "keyboard",
            InputDevice.SOURCE_DPAD, "DPad",
            InputDevice.SOURCE_GAMEPAD, "game pad",
            InputDevice.SOURCE_TOUCHSCREEN, "touch screen",
            InputDevice.SOURCE_MOUSE, "mouse",
            InputDevice.SOURCE_STYLUS, "stylus",
            SOURCE_BLUETOOTH_STYLUS, "Bluetooth stylus",
            InputDevice.SOURCE_TRACKBALL, "trackball",
            SOURCE_MOUSE_RELATIVE, "relative mouse",
            InputDevice.SOURCE_TOUCHPAD, "touch pad",
            InputDevice.SOURCE_TOUCH_NAVIGATION, "touch navigation",
            SOURCE_ROTARY_ENCODER, "rotary encoder",
            InputDevice.SOURCE_JOYSTICK, "joystick",
            SOURCE_HDMI, "HDMI",
            InputDevice.SOURCE_ANY, "any",
        };
        List<String> sourceList = new ArrayList<>();
        for(int i = 0; i < Sources.length; i+=2)
        {
            if(Q3EUtils.IncludeBit(source, (Integer) Sources[i])) sourceList.add((String)Sources[i+1]);
        }

        final Object[] Classes = {
                InputDevice.SOURCE_CLASS_BUTTON, "button",
                InputDevice.SOURCE_CLASS_POINTER, "pointer",
                InputDevice.SOURCE_CLASS_TRACKBALL, "trackball",
                InputDevice.SOURCE_CLASS_POSITION, "position",
                InputDevice.SOURCE_CLASS_JOYSTICK, "joystick",
        };
        List<String> classList = new ArrayList<>();
        for(int i = 0; i < Classes.length; i+=2)
        {
            if(Q3EUtils.IncludeBit(source, (Integer) Classes[i])) classList.add((String)Classes[i+1]);
        }
        if(classList.isEmpty())
            classList.add("none");

        AppendLine("  Source: " + String.join(", ", sourceList));
        AppendLine("  Class: " + String.join(", ", classList));
    }

    private void HandleInputEvent(InputEvent event)
    {
        int source = event.getSource();
        int deviceId = event.getDeviceId();
        HandleSource(source);
        AppendLine("DeviceId: " + deviceId);
    }

    private void HandleDeviceMotionRange(InputDevice device, int axis, String name)
    {
        InputDevice.MotionRange range = device.getMotionRange(axis);
        if(null != range)
        {
            AppendLine(name + ": %f - %f", range.getMin(), range.getMax());
            AppendLine("  Range = %f", range.getRange());
            AppendLine("  Flat = %f", range.getFlat());
        }
    }

    private void HandleDeviceMotionRange(InputDevice device)
    {
        HandleDeviceMotionRange(device, MotionEvent.AXIS_Z, "Z-Axis");
        HandleDeviceMotionRange(device, MotionEvent.AXIS_RZ, "RZ-Axis");
    }

    private void HandleInputDevice(InputDevice device)
    {
        Finish(false);
        if(null != device)
        {
            AppendLine("Name: " + device.getName());
            AppendLine("Id: " + device.getId());
            HandleSource(device.getSources());
            HandleDeviceKeyboard(device);
            HandleDeviceProduct(device);
            HandleDeviceVendor(device);
            AppendLine("ControllerNumber: " + device.getControllerNumber());
            HandleDeviceMotionRange(device);
        }
        Finish(true);
    }

    private void HandleKeyModifier(int modifiers)
    {
        final Object[] Sources = {
                KeyEvent.META_SHIFT_ON, "shift",
                KeyEvent.META_SHIFT_LEFT_ON, "left-shift",
                KeyEvent.META_SHIFT_RIGHT_ON, "right-shift",
                KeyEvent.META_ALT_ON, "alt",
                KeyEvent.META_ALT_LEFT_ON, "left-alt",
                KeyEvent.META_ALT_RIGHT_ON, "right-alt",
                KeyEvent.META_CTRL_ON, "ctrl",
                KeyEvent.META_CTRL_LEFT_ON, "left-ctrl",
                KeyEvent.META_CTRL_RIGHT_ON, "right-ctrl",
                KeyEvent.META_META_ON, "meta",
                KeyEvent.META_META_LEFT_ON, "left-meta",
                KeyEvent.META_META_RIGHT_ON, "right-meta",
                KeyEvent.META_SYM_ON, "sym",
                KeyEvent.META_FUNCTION_ON, "function",
        };
        List<String> sourceList = new ArrayList<>();
        for(int i = 0; i < Sources.length; i+=2)
        {
            if(Q3EUtils.IncludeBit(modifiers, (Integer) Sources[i])) sourceList.add((String)Sources[i+1]);
        }
        AppendLine("Modifiers: " + HexString(modifiers) + " -> " + String.join(", ", sourceList));
    }

    private void HandleKeyEvent(KeyEvent event)
    {
        HandleInputEvent(event);

        Append("Action: ");
        switch(event.getAction())
        {
            case KeyEvent.ACTION_DOWN: AppendLine("Down"); break;
            case KeyEvent.ACTION_UP: AppendLine("Up"); break;
            case KeyEvent.ACTION_MULTIPLE: AppendLine("Multiple"); break;
        }
        AppendLine("KeyCode: " + event.getKeyCode());
        AppendLine("UnicodeChar: " + event.getUnicodeChar() + " -> " + (event.getUnicodeChar() > 0 ? (char)event.getUnicodeChar() : ""));
        AppendLine("ScanCode: " + event.getScanCode());
        AppendLine("RepeatCount: " + event.getRepeatCount());
        AppendLine("Characters: " + event.getCharacters());
        HandleKeyModifier(event.getModifiers());
    }

    private static String HexString(int i)
    {
        return "0x" + Integer.toHexString(i).toUpperCase();
    }

    private void HandleButtonState(int state)
    {
        int BUTTON_STYLUS_PRIMARY;
        int BUTTON_STYLUS_SECONDARY;
        if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            BUTTON_STYLUS_PRIMARY = MotionEvent.BUTTON_STYLUS_PRIMARY;
            BUTTON_STYLUS_SECONDARY = MotionEvent.BUTTON_STYLUS_SECONDARY;
        } else {
            BUTTON_STYLUS_PRIMARY = 1 << 5;
            BUTTON_STYLUS_SECONDARY = 1 << 6;
        }
        final Object[] Sources = {
                MotionEvent.BUTTON_PRIMARY, "primary",
                MotionEvent.BUTTON_SECONDARY, "secondary",
                MotionEvent.BUTTON_TERTIARY, "tertiary",
                MotionEvent.BUTTON_BACK, "back",
                MotionEvent.BUTTON_FORWARD, "forward",
                BUTTON_STYLUS_PRIMARY, "stylus primary",
                BUTTON_STYLUS_SECONDARY, "stylus secondary",
        };
        List<String> sourceList = new ArrayList<>();
        for(int i = 0; i < Sources.length; i+=2)
        {
            if(Q3EUtils.IncludeBit(state, (Integer) Sources[i])) sourceList.add((String)Sources[i+1]);
        }
        AppendLine("ButtonState: " + HexString(state) + " -> " + String.join(", ", sourceList));
    }

    private void HandleMotionEvent(MotionEvent event)
    {
        HandleInputEvent(event);

        Append("Action: ");
        switch(event.getAction())
        {
            case MotionEvent.ACTION_DOWN: AppendLine("Down"); break;
            case MotionEvent.ACTION_UP: AppendLine("Up"); break;
            case MotionEvent.ACTION_MOVE: AppendLine("Move"); break;
            case MotionEvent.ACTION_CANCEL: AppendLine("Cancel"); break;
            case MotionEvent.ACTION_OUTSIDE: AppendLine("Outside"); break;
            case MotionEvent.ACTION_POINTER_DOWN: AppendLine("Pointer Down"); break;
            case MotionEvent.ACTION_POINTER_UP: AppendLine("Pointer Up"); break;
            case MotionEvent.ACTION_HOVER_MOVE: AppendLine("Hover Move"); break;
            case MotionEvent.ACTION_SCROLL: AppendLine("Scroll"); break;
            case MotionEvent.ACTION_HOVER_ENTER: AppendLine("Hover Enter"); break;
            case MotionEvent.ACTION_HOVER_EXIT: AppendLine("Hover Exit"); break;
            case MotionEvent.ACTION_BUTTON_PRESS: AppendLine("Button Press"); break;
            case MotionEvent.ACTION_BUTTON_RELEASE: AppendLine("Button Release"); break;
        }
        AppendLine("Position: " + event.getX() + "," + event.getY());
        AppendLine("ActionIndex: " + event.getActionIndex());
        AppendLine("PointerId: " + event.getPointerId(event.getActionIndex()));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            AppendLine("ActionButton: " + HexString(event.getActionButton()));
        }
        AppendLine("ActionMasked: " + HexString(event.getActionMasked()));
        HandleButtonState(event.getButtonState());

        AppendLine("X-Axis: " + event.getAxisValue(MotionEvent.AXIS_X));
        AppendLine("Y-Axis: " + event.getAxisValue(MotionEvent.AXIS_Y));
        AppendLine("X-Hat-Axis: " + event.getAxisValue(MotionEvent.AXIS_HAT_X));
        AppendLine("Y-Hat-Axis: " + event.getAxisValue(MotionEvent.AXIS_HAT_Y));
        AppendLine("Z-Axis: " + event.getAxisValue(MotionEvent.AXIS_Z));
        AppendLine("RZ-Axis: " + event.getAxisValue(MotionEvent.AXIS_RZ));
        AppendLine("Left-Trigger-Axis: " + event.getAxisValue(MotionEvent.AXIS_LTRIGGER));
        AppendLine("Left-Trigger-Axis: " + event.getAxisValue(MotionEvent.AXIS_RTRIGGER));
        AppendLine("Brake-Axis: " + event.getAxisValue(MotionEvent.AXIS_BRAKE));
        AppendLine("Gas-Axis: " + event.getAxisValue(MotionEvent.AXIS_GAS));
    }

    private void AppendLine(Object fmt, Object...args)
    {
        KStr.Append(buf, fmt, args);
        buf.append("\n");
    }

    private void Append(Object fmt, Object...args)
    {
        KStr.Append(buf, fmt, args);
    }

    private void Finish(boolean all)
    {
        if(all)
            V.eventDeviceResult.setText(buf.toString());
        else
            V.eventResult.setText(buf.toString());
        buf.setLength(0);
    }

    public void ShowSoftInputMethod(View vw)
    {
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        if(!imm.hideSoftInputFromWindow(vw.getWindowToken(), 0))
            imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
    }

    public void HideSoftInputMethod(View vw)
    {
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(vw.getWindowToken(), 0);
    }

    public void CapturePointer(View vw)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            V.view.requestPointerCapture();
        }
    }

    public void ReleasePointer(View vw)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            V.view.releasePointerCapture();
        }
    }

    private class ViewHolder
    {
        private TextView eventResult;
        private TextView eventDeviceResult;
        private View view;

        public void SetupUI()
        {
            eventResult = findViewById(R.id.eventResult);
            eventDeviceResult = findViewById(R.id.eventDeviceResult);
        }
    }
}
