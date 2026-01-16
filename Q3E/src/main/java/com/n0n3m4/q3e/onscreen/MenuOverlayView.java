package com.n0n3m4.q3e.onscreen;

import static android.util.TypedValue.COMPLEX_UNIT_SP;
import static android.view.View.TEXT_ALIGNMENT_CENTER;

import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.RectShape;
import android.graphics.drawable.shapes.Shape;
import android.os.Handler;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.n0n3m4.q3e.Q3EEditButtonHandler;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUiView;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.R;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

// MenuOverlay with Android View
public class MenuOverlayView implements UiViewOverlay
{
    private int           x;
    private int           y;
    private int           width;
    private int           height;
    private float         alpha;
    private int           touchType = Q3EGlobals.TYPE_BUTTON;

    private final Context      context;
    private       TextView     positionTextView;
    private       TextView     sizeTextView;
    private       TextView     alphaTextView;
    private       LinearLayout mainLayout;
    private final Q3EEditButtonHandler    uiView;
    private       int          posX;
    private       int          posY;
    private final int          layoutWidth;
    private final ViewGroup    layout;
    private       FingerUi     fngr;

    public MenuOverlayView(int width, ViewGroup layout, Q3EEditButtonHandler uiView)
    {
        this.uiView = uiView;
        this.context = uiView.getContext();
        layoutWidth = width;
        this.layout = layout;
    }

    private Drawable CreateBorder()
    {
        GradientDrawable border = new GradientDrawable();
        border.setColor(Color.TRANSPARENT);
        border.setStroke(1, Color.WHITE);
        return border;
    }

    private void AddDivider()
    {
        View view = new View(context);
        view.setBackgroundColor(Color.WHITE);
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 1);
        mainLayout.addView(view, layoutParams);
    }

    private TextView AddLabel(int resid)
    {
        TextView text = new TextView(context);
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        text.setGravity(Gravity.CENTER);
        text.setTextColor(Color.WHITE);
        if(resid > 0)
            text.setText(Q3ELang.tr(context, resid));
        mainLayout.addView(text, layoutParams);
        return text;
    }

    private TextView AddButton(Object label, LinearLayout sublayout, int sp, int w, int h, float weight)
    {
        TextView button = new TextView(context);
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(w, h);
        button.setGravity(Gravity.CENTER);
        if(w == 0)
            layoutParams.weight = weight;
        button.setBackground(CreateBorder());
        button.setPadding(0, 0, 0, 0);
        if(sp > 0)
            button.setTextSize(COMPLEX_UNIT_SP, sp);
        button.setTextColor(Color.WHITE);
        button.setText(Q3ELang.Str(context, label));
        sublayout.addView(button, layoutParams);
        return button;
    }

    private LinearLayout AddSubLayout()
    {
        LinearLayout sublayout = new LinearLayout(context);
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 96);
        mainLayout.addView(sublayout, layoutParams);
        sublayout.setOrientation(LinearLayout.HORIZONTAL);
        return sublayout;
    }

    private void CreateInternal(ViewGroup layout)
    {
        LinearLayout.LayoutParams layoutParams;
        TextView button;
        LinearLayout sublayout;

        mainLayout = new LinearLayout(context);
        layoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        mainLayout.setLayoutParams(layoutParams);
        mainLayout.setOrientation(LinearLayout.VERTICAL);
        mainLayout.setBackground(CreateBorder());

        // position
        AddLabel(R.string.position_);
        positionTextView = AddLabel(0);

        button = AddButton(R.string.reset, mainLayout, 0, LinearLayout.LayoutParams.MATCH_PARENT, 64, 0.0f);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v)
            {
                uiView.ResetOnScreenButtonPosition(fngr.target);
            }
        });

        AddDivider();

        // size
        AddLabel(R.string.size);
        sizeTextView = AddLabel(0);
        sublayout = AddSubLayout();

        button = AddButton(" - ", sublayout, 32, 0, LinearLayout.LayoutParams.MATCH_PARENT, 0.35f);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v)
            {
                tgtresize(false);
            }
        });

        button = AddButton(R.string.reset, sublayout, 0, 0, LinearLayout.LayoutParams.MATCH_PARENT, 0.3f);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v)
            {
                uiView.ResetOnScreenButtonSize(fngr.target);
            }
        });

        button = AddButton(" + ", sublayout, 32, 0, LinearLayout.LayoutParams.MATCH_PARENT, 0.35f);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v)
            {
                tgtresize(true);
            }
        });

        AddDivider();

        // opacity
        AddLabel(R.string.opacity);
        alphaTextView = AddLabel(0);
        sublayout = AddSubLayout();

        button = AddButton(" - ", sublayout, 32, 0, LinearLayout.LayoutParams.MATCH_PARENT, 0.5f);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v)
            {
                tgtalpha(false);
            }
        });

        button = AddButton(" + ", sublayout, 32, 0, LinearLayout.LayoutParams.MATCH_PARENT, 0.5f);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v)
            {
                tgtalpha(true);
            }
        });

        if(null != layout)
        {
            ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(layoutWidth, ViewGroup.LayoutParams.WRAP_CONTENT);
            layout.addView(mainLayout, params);
        }

        mainLayout.setVisibility(View.GONE);
    }

    public void loadtex(GL10 gl)
    {
        if(null != mainLayout)
            return;

        Q3EUtils.RunOnUiThread(context, new Runnable() {
                @Override
                public void run()
                {
                    CreateInternal(layout);
                }
            });
    }

    private MenuOverlayView SetPosition(int x, int y)
    {
        this.x = x;
        this.y = y;
        return this;
    }

    private MenuOverlayView SetSize(int w, int h)
    {
        this.width = w;
        this.height = h;
        return this;
    }

    private MenuOverlayView SetRadius(int r)
    {
        SetSize(r, -1);
        return this;
    }

    private MenuOverlayView SetAlpha(float a)
    {
        this.alpha = a;
        return this;
    }

    private void Reset()
    {
        SetPosition(-1, -1);
        SetSize(-1, -1);
        SetAlpha(-1.0f);
        touchType = Q3EGlobals.TYPE_BUTTON;
    }

    private void SetDisc(Disc btn)
    {
        SetPosition(btn.cx, btn.cy);
        SetRadius(btn.size);
        SetAlpha(btn.alpha);
        touchType = Q3EGlobals.TYPE_DISC;
    }

    private void SetJoystick(Joystick btn)
    {
        SetPosition(btn.cx, btn.cy);
        SetRadius(btn.size);
        SetAlpha(btn.alpha);
        touchType = Q3EGlobals.TYPE_JOYSTICK;
    }

    private void SetButton(Button btn)
    {
        SetPosition(btn.cx, btn.cy);
        SetSize(btn.width, btn.height);
        SetAlpha(btn.alpha);
        touchType = Q3EGlobals.TYPE_BUTTON;
    }

    private void SetSlider(Slider btn)
    {
        SetPosition(btn.cx, btn.cy);
        SetSize(btn.width, btn.height);
        SetAlpha(btn.alpha);
        touchType = Q3EGlobals.TYPE_SLIDER;
    }

    private void SetTouchItem(TouchListener touch)
    {
        fngr = new FingerUi(touch, 9000);

        UpdateTouchItem();
    }

    private void UpdateTouchItem()
    {
        TouchListener touchItem = fngr.target;

        if (touchItem instanceof Slider)
        {
            SetSlider((Slider) touchItem);
        }
        else if (touchItem instanceof Button)
        {
            SetButton((Button) touchItem);
        }
        else if (touchItem instanceof Joystick)
        {
            SetJoystick((Joystick) touchItem);
        }
        else if (touchItem instanceof Disc)
        {
            SetDisc((Disc) touchItem);
        }
        else
        {
            Reset();
        }
    }

    public void Show()
    {
        Q3EUtils.RunOnUiThread(context, new Runnable() {
                @Override
                public void run()
                {
                    mainLayout.setVisibility(View.VISIBLE);
                }
            });
    }

    public void hide()
    {
        Q3EUtils.RunOnUiThread(context, new Runnable() {
                @Override
                public void run()
                {
                    mainLayout.setVisibility(View.GONE);
                }
            });
    }

    public void Paint(GL11 gl)
    {
        Q3EUtils.RunOnUiThread(context, new Runnable() {
                @Override
                public void run()
                {
                    positionTextView.setText(x + " , " + y);
                    if(touchType == Q3EGlobals.TYPE_JOYSTICK)
                        sizeTextView.setText(Q3ELang.tr(context, R.string.radius_) + " " + width);
                    else if(touchType == Q3EGlobals.TYPE_DISC)
                        sizeTextView.setText(Q3ELang.tr(context, R.string.center_radius_) + " " + width);
                    else
                        sizeTextView.setText(Q3ELang.tr(context, R.string.size_) + " " + width + " x " + height);
                    alphaTextView.setText(String.format("%.1f", alpha));
                }
            });
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act)
    {
        return false;
    }

    public boolean isInside(int x, int y)
    {
        return ((mainLayout.getVisibility() == View.VISIBLE) && (2 * Math.abs(posX - x) < mainLayout.getWidth()) && (2 * Math.abs(posX - y) < mainLayout.getHeight()));
    }

    public void show(int x, int y, FingerUi fn)
    {
        SetTouchItem(fn.target);

        Q3EUtils.RunOnUiThread(context, new Runnable() {
            @Override
            public void run()
            {
                Show();

                Q3EUtils.Post(context, new Runnable() {
                    @Override
                    public void run()
                    {
                        int w = mainLayout.getWidth();
                        int h = mainLayout.getHeight();
                        int fullW = uiView.width;
                        int fullH = uiView.height;
                        posX = Math.min(Math.max(0, x - w / 2), fullW - w);
                        posY = Math.min(Math.max(0, y - (/*h / 2 +*/ uiView.yoffset)), fullH + uiView.yoffset - h);
                        mainLayout.setX(posX);
                        mainLayout.setY(posY);

                        Paint(null);

                        layout.requestLayout();
                    }
                });
            }
        });
    }

    public boolean tgtresize(boolean dir)
    {
        boolean res = UiViewOverlay.Resize(dir, uiView.step, fngr, uiView);
        if(res)
        {
            UpdateTouchItem();
            Paint(null);
        }
        return res;
    }

    public boolean tgtalpha(boolean dir)
    {
        boolean res = UiViewOverlay.SetupAlpha(dir, fngr, uiView);
        if(res)
        {
            UpdateTouchItem();
            Paint(null);
        }
        return res;
    }

    public int Type()
    {
        return -1;
    }
}
