package com.n0n3m4.DIII4A.launcher;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.Display;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;

import com.karin.idTech4Amm.sys.GameManager;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.MyHorizontalScrollView;
import com.n0n3m4.q3e.Q3EGameConstants;
import com.n0n3m4.q3e.Q3EUtils;

import java.util.ArrayList;
import java.util.Objects;

public class GameAd extends GameLauncherFunc
{
    private Runnable adrunnable = null;
    private boolean start = false;

    public GameAd(GameLauncher gameLauncher)
    {
        super(gameLauncher);
    }

    private Bitmap CreateBanner(int width, String gameId)
    {
        final int WIDTH  = 640;
        final int HEIGHT = 100;

        Resources resources = m_gameLauncher.getResources();
        Bitmap bmp = Bitmap.createBitmap(WIDTH, HEIGHT, Bitmap.Config.ARGB_8888);

        int iconId = GameManager.GetGameIcon(gameId);
        Bitmap orig = BitmapFactory.decodeResource(resources, iconId);
        int colorId = GameManager.GetGameThemeColor(gameId);
        int color = resources.getColor(colorId);
        int r = (color >> 16) & 0xFF;
        int g = (color >> 8) & 0xFF;
        int b = color & 0xFF;
        //int a = (color >> 24) & 0xFF;
        int nameId = GameManager.GetGameNameRS(gameId);
        String name = resources.getString(nameId);
        int border = 4;
        int spacing = 10;
        int vspacing = 20;

        Rect srcImgRect = new Rect(0, 0, orig.getWidth(), orig.getHeight());
        Rect dstImgRect = new Rect(border, border, HEIGHT - border, HEIGHT - border);

        Canvas c = new Canvas(bmp);
        Paint p = new Paint();
        p.setTextSize(1);
        p.setAntiAlias(true);

        /*while (p.measureText(name) < WIDTH)
            p.setTextSize(p.getTextSize() + 1);*/
        Rect bnd = new Rect();
        int startX = HEIGHT + spacing;
        int startY = vspacing + border;
        int remainWidth = WIDTH - spacing - startX;
        int remainHeight = HEIGHT - border * 2 - vspacing * 2;
        while(true)
        {
            p.getTextBounds(name, 0, name.length(), bnd);
            if(bnd.width() < remainWidth && bnd.height() < remainHeight)
                p.setTextSize(p.getTextSize() + 1);
            else
                break;
        }
        p.setTextSize(p.getTextSize() - 1);
        p.getTextBounds(name, 0, name.length(), bnd);

        p.setStyle(Paint.Style.STROKE);
        c.drawARGB(255, 255, 255, 255);
        p.setStrokeWidth(border);
        c.drawRect(new Rect(0, 0, WIDTH, HEIGHT), p);

        p.setARGB(196, r, g, b);
        p.setStyle(Paint.Style.FILL);
        p.setStrokeWidth(1);
        c.drawRect(new Rect(border, border, WIDTH - border, HEIGHT - border), p);
        p.setARGB(255, 255, 255, 255);

        c.drawBitmap(orig, srcImgRect, dstImgRect, p);

        c.drawText(name, startX + (int) ((float) (remainWidth - bnd.width()) / 2.0f), startY + (int) ((float) (remainHeight - bnd.height()) / 2.0f + bnd.height()), p);

        Bitmap bitmap = Bitmap.createScaledBitmap(bmp, width, (int) ((float) width * (float) HEIGHT / (float) WIDTH), true);
        bmp.recycle();
        return bitmap;
    }

    private int ItemWidth()
    {
        return Q3EUtils.dip2px(m_gameLauncher, 320);//Magic number
    }

    private ImageView createiwbyid(int width, String game)
    {
        ImageView iw = new ImageView(m_gameLauncher);
        Bitmap bm = CreateBanner(width, game);
        iw.setImageBitmap(bm);
        iw.setTag(game);
        iw.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v)
            {
                final int width = ItemWidth();
                final MyHorizontalScrollView mhsw = m_gameLauncher.findViewById(com.n0n3m4.q3e.R.id.adlayout_hsw);
                final LinearLayout ll = m_gameLauncher.findViewById(com.n0n3m4.q3e.R.id.adlayout_id);
                for(int i = 0; i < ll.getChildCount(); i++)
                {
                    if(ll.getChildAt(i) == iw)
                    {
                        mhsw.smoothScrollTo(i * width, 0);
                        break;
                    }
                }
                Callback(game);
            }
        });
        return iw;
    }

    private void LoadAds()
    {
        final LinearLayout ll = m_gameLauncher.findViewById(com.n0n3m4.q3e.R.id.adlayout_id);
        ll.removeAllViews();

        String[] games = GameManager.Games(false);

        //carousel
        ArrayList<Integer> ids = new ArrayList<>(0);
        ArrayList<View> vws = new ArrayList<>(0);
        Display display = m_gameLauncher.getWindowManager().getDefaultDisplay();
        final int width = ItemWidth();
        final int dspwidth = display.getWidth();
        for(int i = 0; i < games.length; i++)
        {
            vws.add(createiwbyid(width, games[i]));
            ids.add(i);
        }
        int carouselcount = dspwidth / width + ((dspwidth % width == 0) ? 0 : 1);

        if(carouselcount <= ids.size())
        {
            for(int i = 0; i < carouselcount; i++)
            {
                int index = ids.get(i);
                vws.add(createiwbyid(width, games[index]));
            }
            for(int i = 0; i < carouselcount; i++)
            {
                int index = ids.get(ids.size() - 1 - i);
                vws.add(0, createiwbyid(width, games[index]));
            }
            MyHorizontalScrollView.maxscrollx = width * (ids.size()) + carouselcount * width / 2;
            MyHorizontalScrollView.minscrollx = width * carouselcount / 2;
            MyHorizontalScrollView.deltascrollx = width * ids.size();
        }
        else
        {
            MyHorizontalScrollView.maxscrollx = 0;
            MyHorizontalScrollView.minscrollx = 0;
            MyHorizontalScrollView.deltascrollx = 0;
        }
        for(View v : vws)
        {
            ll.addView(v);
        }
        final MyHorizontalScrollView mhsw = m_gameLauncher.findViewById(com.n0n3m4.q3e.R.id.adlayout_hsw);

        if(adrunnable != null)
        {
            ll.removeCallbacks(adrunnable);
        }
        adrunnable = new Runnable() {
            @Override
            public void run()
            {
                if(!m_gameLauncher.isFinishing()/* && (mhsw.getScrollX() % width == 0)*/)
                {
                    int d = mhsw.getScrollX() % width;
                    //KLog.E("%d %b : %d != 0 ? %d : %d", d, (mhsw.getScrollX() % width == 0), width, mhsw.getScrollX(), d != 0 ? width - d : width);

                    //mhsw.scrollBy(d != 0 ? width - d : width, 0);
                    mhsw.smoothScrollBy(d != 0 ? width - d : width, 0);
                    ll.postDelayed(this, 10000);
                }
            }
        };

        ll.postDelayed(new Runnable() {
            @Override
            public void run()
            {
                Init();
            }
        }, 1000);
    }

    private void Init()
    {
        if(!start)
            return;

        final int width = ItemWidth();
        final MyHorizontalScrollView mhsw = m_gameLauncher.findViewById(com.n0n3m4.q3e.R.id.adlayout_hsw);
        final LinearLayout ll = m_gameLauncher.findViewById(com.n0n3m4.q3e.R.id.adlayout_id);
        String m_game = GetData().getString("game", Q3EGameConstants.GAME_DOOM3);
        for(int i = 0; i < ll.getChildCount(); i++)
        {
            if(Objects.equals(ll.getChildAt(i).getTag(), m_game))
            {
                mhsw.scrollTo(i * width, 0);
                break;
            }
        }

        ll.postDelayed(adrunnable, 10000);
    }

    @Override
    public void run()
    {
        if(start)
            return;
        start = true;
        LinearLayout ll = m_gameLauncher.findViewById(com.n0n3m4.q3e.R.id.adlayout_id);
        if(ll.getChildCount() > 0)
            return;
        LoadAds();
    }

    @Override
    public void Start(Bundle data)
    {
        super.Start(data);
        if(!start)
            run();
    }
}
