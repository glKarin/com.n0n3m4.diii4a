package com.n0n3m4.q3e.karin;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.text.Html;
import android.text.method.ArrowKeyMovementMethod;
import android.text.method.LinkMovementMethod;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.R;

import java.util.ArrayList;
import java.util.List;

public class KVKBView extends RelativeLayout
{
    private int keyboard_background;

    private int keyboard_text_released;
    private int keyboard_text_pressed;
    private int keyboard_bg_released;
    private int keyboard_bg_pressed;

    private int keyboard_enter_text_released;
    private int keyboard_enter_text_pressed;
    private int keyboard_enter_bg_released;
    private int keyboard_enter_bg_pressed;

    private int keyboard_secondary_text_released;
    private int keyboard_secondary_text_pressed;
    private int keyboard_secondary_bg_released;
    private int keyboard_secondary_bg_pressed;

    private int width;
    private int height;

    public KVKBView(Context context)
    {
        super(context);
        Setup();
    }

    public KVKBView(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        Setup();
    }

    public KVKBView(Context context, AttributeSet attrs, int defStyle)
    {
        super(context, attrs, defStyle);
        Setup();
    }

    private void ExecAction(String action)
    {
        switch(action)
        {
            case "close":
                Q3E.keyboard.CloseBuiltInVKB();
                break;
            default:
                break;
        }
    }

    public int GetCalcHeight()
    {
        return height;
    }

    private static int CalcWeight(int total, int percent, int num, int spacing)
    {
        return (int) Math.floor((float) (total - ((float) num * 2.0f) * (float) spacing) * ((float) percent / 100.0f));
    }

    private final OnTouchListener m_keyListener = new OnTouchListener()
    {
        @Override
        public boolean onTouch(View v, MotionEvent event)
        {
            switch(event.getActionMasked())
            {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_POINTER_DOWN:
                    ((Button) v).Touch(true);
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_POINTER_UP:
                case MotionEvent.ACTION_CANCEL:
                    ((Button) v).Touch(false);
                    break;
                default:
                    return false;
            }
            return true;
        }
    };

    void Setup()
    {
        Resources resources = getResources();
        keyboard_background = resources.getColor(R.color.keyboard_background);

        keyboard_text_released = resources.getColor(R.color.keyboard_text_released);
        keyboard_text_pressed = resources.getColor(R.color.keyboard_text_pressed);
        keyboard_bg_released = resources.getColor(R.color.keyboard_bg_released);
        keyboard_bg_pressed = resources.getColor(R.color.keyboard_bg_pressed);

        keyboard_enter_text_released = resources.getColor(R.color.keyboard_enter_text_released);
        keyboard_enter_text_pressed = resources.getColor(R.color.keyboard_enter_text_pressed);
        keyboard_enter_bg_released = resources.getColor(R.color.keyboard_enter_bg_released);
        keyboard_enter_bg_pressed = resources.getColor(R.color.keyboard_enter_bg_pressed);

        keyboard_secondary_text_released = resources.getColor(R.color.keyboard_secondary_text_released);
        keyboard_secondary_text_pressed = resources.getColor(R.color.keyboard_secondary_text_pressed);
        keyboard_secondary_bg_released = resources.getColor(R.color.keyboard_secondary_bg_released);
        keyboard_secondary_bg_pressed = resources.getColor(R.color.keyboard_secondary_bg_pressed);

        setBackgroundColor(keyboard_background);
        setFocusable(false);
        setFocusableInTouchMode(false);
    }

    public void Open()
    {
        Reset();
        setVisibility(View.VISIBLE);
    }

    public void Close()
    {
        setVisibility(View.GONE);
    }

    public void SetVisible(boolean on)
    {
        if(on)
            Open();
        else
            Close();
    }

    public boolean IsVisible()
    {
        return getVisibility() == View.VISIBLE;
    }

    public void Resize(int w)
    {
        KLog.I("Resize built-in keyboard width = " + w);
        width = w;
        Relayout();
    }

    public void Relayout()
    {
        for(int i = 0; i < getChildCount(); i++)
        {
            Layout layout = (Layout) getChildAt(i);
            layout.Relayout();
        }
        requestLayout();
    }

    private void Reset()
    {
        for(int i = 0; i < getChildCount(); i++)
        {
            Layout layout = (Layout) getChildAt(i);
            layout.Reset();
        }
    }

    private void ShowLayout(String name)
    {
        for(int i = 0; i < getChildCount(); i++)
        {
            Layout layout = (Layout) getChildAt(i);
            //layout.Reset();
            layout.SetVisible(name.equals(layout.name));
        }
    }

    public void Init(String config)
    {
        KVKB vkb = new KVKB();
        if(!vkb.Parse(config))
            return;

        removeAllViews();
        height = 0;

        for(KVKB.KVKBLayout layout : vkb.layouts)
        {
            Layout l = new Layout(getContext());
            l.Create(layout);
            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            addView(l, params);
            height = Math.max(height, l.height);
        }
        KLog.I("Built-in keyboard calc height = " + height);
    }

    private class Layout extends LinearLayout
    {
        public boolean main   = false;
        public String  name;
        public int     group  = 0;
        public int     height = 0;

        public Layout(Context context)
        {
            super(context);
            setOrientation(VERTICAL);
        }

        public void Create(KVKB.KVKBLayout layout)
        {
            main = layout.main;
            height = 0;
            Resources resources = getResources();
            int keyboardVerticalSpacing = resources.getDimensionPixelSize(R.dimen.keyboardVerticalSpacing);
            for(KVKB.KVKBRow r : layout.rows)
            {
                Row row = new Row(getContext());
                row.Create(r);
                LinearLayout.LayoutParams params = new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
                params.gravity = Gravity.CENTER_HORIZONTAL;
                params.setMargins(0, keyboardVerticalSpacing, 0, keyboardVerticalSpacing);
                addView(row, params);
                row.layout = this;
                height += resources.getDimensionPixelSize(R.dimen.keyboardRowHeight);
                height += keyboardVerticalSpacing * 2;
            }
            setFocusable(false);
            setFocusableInTouchMode(false);
            setBackgroundColor(keyboard_background);
            name = layout.name;
            setTag(layout.name);
            SetVisible(main);
        }

        public void Show()
        {
            setVisibility(View.VISIBLE);
        }

        public void Hide()
        {
            setVisibility(View.GONE);
        }

        public void SetVisible(boolean on)
        {
            setVisibility(on ? View.VISIBLE : View.GONE);
        }

        public void SetGroup(int g)
        {
            group = g;
            for(int i = 0; i < getChildCount(); i++)
            {
                Row row = (Row) getChildAt(i);
                row.SetGroup(g);
            }
        }

        public void Reset()
        {
            for(int i = 0; i < getChildCount(); i++)
            {
                Row row = (Row) getChildAt(i);
                row.Reset();
            }
            SetVisible(main);
        }

        public void ToggleGroup()
        {
            group = group == 1 ? 0 : 1;
            for(int i = 0; i < getChildCount(); i++)
            {
                Row row = (Row) getChildAt(i);
                row.SetGroup(group);
            }
        }

        public void Relayout()
        {
            for(int i = 0; i < getChildCount(); i++)
            {
                Row row = (Row) getChildAt(i);
                row.Relayout();
            }
            requestLayout();
        }
    }

    private class Row extends LinearLayout
    {
        public Layout layout;
        public int    group = 0;

        public Row(Context context)
        {
            super(context);
            setOrientation(HORIZONTAL);
        }

        public void Create(KVKB.KVKBRow row)
        {
            Resources resources = getResources();
            int keyboardHorizontalSpacing = resources.getDimensionPixelSize(R.dimen.keyboardHorizontalSpacing);
            for(KVKB.KVKBButton button : row.buttons)
            {
                Button btn = new Button(getContext());
                btn.Create(button);
                LinearLayout.LayoutParams params = new LayoutParams(CalcWeight(KVKBView.this.width, btn.weight, row.buttons.size(), keyboardHorizontalSpacing), resources.getDimensionPixelSize(R.dimen.keyboardRowHeight));
                params.gravity = Gravity.CENTER_HORIZONTAL;
                params.setMarginStart(keyboardHorizontalSpacing);
                params.setMarginEnd(keyboardHorizontalSpacing);
                addView(btn, params);
                btn.row = this;
            }
            setGravity(Gravity.CENTER);
            setBackgroundColor(keyboard_background);
            setFocusable(false);
            setFocusableInTouchMode(false);
        }

        public void SetGroup(int g)
        {
            group = g;
            for(int i = 0; i < getChildCount(); i++)
            {
                Button btn = (Button) getChildAt(i);
                btn.SetGroup(g);
            }
        }

        public void Reset()
        {
            for(int i = 0; i < getChildCount(); i++)
            {
                Button btn = (Button) getChildAt(i);
                btn.Reset();
            }
        }

        public void Relayout()
        {
            for(int i = 0; i < getChildCount(); i++)
            {
                Button btn = (Button) getChildAt(i);
                LinearLayout.LayoutParams params = (LinearLayout.LayoutParams) btn.getLayoutParams();
                params.gravity = Gravity.CENTER_HORIZONTAL;
                params.width = CalcWeight(KVKBView.this.width, btn.weight, getChildCount(), getResources().getDimensionPixelSize(R.dimen.keyboardHorizontalSpacing));
                btn.setLayoutParams(params);
            }
            requestLayout();
        }
    }

    private class Button extends TextView
    {
        public static final int STATE_RELEASED = 0;
        public static final int STATE_PRESSED  = 1;
        public static final int STATE_TOGGLED  = 2;

        public static final int TYPE_BUTTON = 1;
        public static final int TYPE_TOGGLE = 2;
        public static final int TYPE_SWITCH = 3;
        public static final int TYPE_LAYOUT = 4;
        public static final int TYPE_ACTION = 5;

        public int    type;
        public int    weight;
        public String name;
        public int    keyCode;
        public String data;
        public String name2;
        public int    keyCode2;
        public String data2;

        public int state = STATE_RELEASED;
        public int group = 0;
        public Row row;

        public int textPressedColor;
        public int textReleasedColor;
        public int backgroundPressedColor;
        public int backgroundReleasedColor;
        public int textPressedColor2;
        public int textReleasedColor2;
        public int backgroundPressedColor2;
        public int backgroundReleasedColor2;

        public Button(Context context)
        {
            super(context);
        }

        public void Create(KVKB.KVKBButton button)
        {
            switch(button.type)
            {
                case KVKB.TYPE_SWITCH:
                    type = TYPE_SWITCH;
                    break;
                case KVKB.TYPE_TOGGLE:
                    type = TYPE_TOGGLE;
                    break;
                case KVKB.TYPE_LAYOUT:
                    type = TYPE_LAYOUT;
                    break;
                case KVKB.TYPE_ACTION:
                    type = TYPE_ACTION;
                    break;
                case KVKB.TYPE_BUTTON:
                default:
                    type = TYPE_BUTTON;
                    break;
            }
            weight = button.weight;

            name = button.name;
            name2 = button.secondaryName;
            data = button.data;
            data2 = button.secondaryData;
            if(null == name2)
                name2 = name;
            if(null == data2)
                data2 = data;
            if(type == TYPE_TOGGLE || type == TYPE_BUTTON)
            {
                try
                {
                    keyCode = KeyEvent.class.getDeclaredField("KEYCODE_" + button.code.toUpperCase()).getInt(null);
                    if(KStr.IsEmpty(button.secondaryCode))
                        keyCode2 = keyCode;
                    else
                        keyCode2 = KeyEvent.class.getDeclaredField("KEYCODE_" + button.secondaryCode.toUpperCase()).getInt(null);
                }
                catch(Exception e)
                {
                    e.printStackTrace();
                }
            }

            if(keyCode == KeyEvent.KEYCODE_ENTER || keyCode == KeyEvent.KEYCODE_NUMPAD_ENTER)
            {
                textPressedColor = keyboard_enter_text_pressed;
                textReleasedColor = keyboard_enter_text_released;
                backgroundPressedColor = keyboard_enter_bg_pressed;
                backgroundReleasedColor = keyboard_enter_bg_released;
            }
            else if(keyCode == KeyEvent.KEYCODE_SPACE || keyCode == KeyEvent.KEYCODE_DEL)
            {
                textPressedColor = keyboard_secondary_text_pressed;
                textReleasedColor = keyboard_secondary_text_released;
                backgroundPressedColor = keyboard_secondary_bg_pressed;
                backgroundReleasedColor = keyboard_secondary_bg_released;
            }
            else if(type == TYPE_TOGGLE)
            {
                textPressedColor = keyboard_enter_bg_pressed;
                textReleasedColor = keyboard_text_released;
                backgroundPressedColor = keyboard_bg_pressed;
                backgroundReleasedColor = keyboard_bg_released;
            }
            else if(type == TYPE_SWITCH || type == TYPE_LAYOUT || type == TYPE_ACTION)
            {
                textPressedColor = keyboard_enter_bg_pressed;
                textReleasedColor = keyboard_secondary_text_released;
                backgroundPressedColor = keyboard_secondary_bg_pressed;
                backgroundReleasedColor = keyboard_secondary_bg_released;
            }
            else
            {
                textPressedColor = keyboard_text_pressed;
                textReleasedColor = keyboard_text_released;
                backgroundPressedColor = keyboard_bg_pressed;
                backgroundReleasedColor = keyboard_bg_released;
            }

            if(keyCode2 == KeyEvent.KEYCODE_ENTER || keyCode2 == KeyEvent.KEYCODE_NUMPAD_ENTER)
            {
                textPressedColor2 = keyboard_enter_text_pressed;
                textReleasedColor2 = keyboard_enter_text_released;
                backgroundPressedColor2 = keyboard_enter_bg_pressed;
                backgroundReleasedColor2 = keyboard_enter_bg_released;
            }
            else if(keyCode2 == KeyEvent.KEYCODE_SPACE || keyCode2 == KeyEvent.KEYCODE_DEL || type == TYPE_SWITCH || type == TYPE_LAYOUT || type == TYPE_ACTION)
            {
                textPressedColor2 = keyboard_secondary_text_pressed;
                textReleasedColor2 = keyboard_secondary_text_released;
                backgroundPressedColor2 = keyboard_secondary_bg_pressed;
                backgroundReleasedColor2 = keyboard_secondary_bg_released;
            }
            else
            {
                textPressedColor2 = keyboard_text_pressed;
                textReleasedColor2 = keyboard_text_released;
                backgroundPressedColor2 = keyboard_bg_pressed;
                backgroundReleasedColor2 = keyboard_bg_released;
            }

            SetText(name);
            setGravity(Gravity.CENTER);
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
            SetupColor();

            setOnTouchListener(m_keyListener);
            setFocusable(false);
            setFocusableInTouchMode(false);
        }

        public void SetupColor()
        {
            if(group == 1)
            {
                switch(state)
                {
                    case STATE_RELEASED:
                        setTextColor(textReleasedColor2);
                        setBackgroundColor(backgroundReleasedColor2);
                        break;
                /*case Key.STATE_TOGGLED:
                    break;*/
                    case STATE_PRESSED:
                    default:
                        setTextColor(textPressedColor2);
                        setBackgroundColor(backgroundPressedColor2);
                        break;
                }
            }
            else
            {
                switch(state)
                {
                    case STATE_RELEASED:
                        setTextColor(textReleasedColor);
                        setBackgroundColor(backgroundReleasedColor);
                        break;
                /*case Key.STATE_TOGGLED:
                    break;*/
                    case STATE_PRESSED:
                    default:
                        setTextColor(textPressedColor);
                        setBackgroundColor(backgroundPressedColor);
                        break;
                }
            }
        }

        private void SetText(String text)
        {
            setMovementMethod(null);
            if(KStr.IsEmpty(text))
                setText("");
            else if(text.startsWith("@+id/")) // image
            {
                String html = "<img src=\"" + text.substring(5) + "\">";
                Context context = getContext();
                setMovementMethod(LinkMovementMethod.getInstance());
                setText(Html.fromHtml(html, new Html.ImageGetter()
                {
                    @Override
                    public Drawable getDrawable(String source)
                    {
                        int drawableId = context.getResources().getIdentifier(source, "drawable", context.getPackageName());
                        if(drawableId == 0)
                            return null;
                        Drawable drawable = context.getResources().getDrawable(drawableId);
                        drawable.setBounds(0, 0, drawable.getIntrinsicWidth(), drawable.getIntrinsicHeight());
                        return drawable;
                    }
                }, null));
                setMovementMethod(LinkMovementMethod.getInstance());
            }
            else
                setText(text);
        }

        public void SetGroup(int g)
        {
            group = g;
            SetText(g == 1 ? name2 : name);
            //state = Button.STATE_RELEASED;
            SetupColor();
        }

        public void Reset()
        {
            group = 0;
            SetText(name);
            state = Button.STATE_RELEASED;
            SetupColor();
        }

        public void Touch(boolean pressed)
        {
            boolean down;
            boolean send;
            if(pressed)
            {
                switch(type)
                {
                    case Button.TYPE_TOGGLE:
                        down = false;
                        send = false;
                        break;
                    case Button.TYPE_SWITCH:
                    case Button.TYPE_ACTION:
                    case Button.TYPE_LAYOUT:
                        state = Button.STATE_PRESSED;
                        down = true;
                        send = false;
                        break;
                    case Button.TYPE_BUTTON:
                    default:
                        state = Button.STATE_PRESSED;
                        down = true;
                        send = true;
                        break;
                }
            }
            else
            {
                send = true;
                switch(type)
                {
                    case Button.TYPE_TOGGLE:
                        if(state == Button.STATE_RELEASED)
                        {
                            state = Button.STATE_PRESSED;
                            down = true;
                        }
                        else
                        {
                            state = Button.STATE_RELEASED;
                            down = false;
                        }
                        break;
                    case Button.TYPE_SWITCH:
                    case Button.TYPE_ACTION:
                    case Button.TYPE_LAYOUT:
                        state = Button.STATE_RELEASED;
                        down = false;
                        break;
                    case Button.TYPE_BUTTON:
                    default:
                        state = Button.STATE_RELEASED;
                        down = false;
                        break;
                }
            }

            if(send)
            {
                switch(type)
                {
                    case TYPE_BUTTON:
                    case TYPE_TOGGLE:
                        int code = group == 1 ? keyCode2 : keyCode;
                        if(code > 0)
                        {
                            String d = group == 1 ? data2 : data;
                            int uchar = KStr.NotEmpty(d) ? d.charAt(0) : 0;
                            code = Q3EKeyCodes.convertKeyCode(code, uchar, null);
                            Q3E.sendKeyEvent(down, code, uchar);
                        }
                        break;
                    case TYPE_SWITCH:
                        row.layout.SetGroup(Integer.parseInt(group == 1 ? data2 : data));
                        break;
                    case TYPE_ACTION:
                        ExecAction(group == 1 ? data2 : data);
                        break;
                    case TYPE_LAYOUT:
                        ShowLayout(data);
                        break;
                    default:
                        break;
                }
            }
            SetupColor();
        }
    }
}
