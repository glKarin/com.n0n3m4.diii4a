#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "BackgroundImage.h"
#include "../../ui/Window.h"
#include "../../ui/UserInterfaceLocal.h"

void hhBackground::Reset(void) {
    material = NULL;
    name = "";
}

void hhBackground::Setup(void)
{
    if (name.Length()) {
        material = declManager->FindMaterial(name);
        material->SetImageClassifications(1);	// just for resource tracking

        if (material && !material->TestMaterialFlag(MF_DEFAULTED)) {
            material->SetSort(SS_GUI);
        }

        name.SetMaterialPtr(&material);
    }
}

void hhBackground::Draw(idDeviceContext *dc, const idRectangle &drawRect, float matScalex, float matScaley, unsigned int flags, const idVec4 &color)
{
    if (material) {
        float scalex, scaley;

        if (flags & WIN_NATURALMAT) {
            scalex = drawRect.w / (float)material->GetImageWidth();
            scaley = drawRect.h / (float)material->GetImageHeight();
        } else {
            scalex = matScalex;
            scaley = matScaley;
        }

        //dc->DrawRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, 2, idVec4(1,0,0,1));
        dc->DrawMaterial(drawRect.x, drawRect.y, drawRect.w, drawRect.h, material, color, scalex, scaley);
    }
}



void hhBackgroundGroup::Reset(void) {
    left.Reset();
    middle.Reset();
    right.Reset();
    edge = -1.0f;
}

void hhBackgroundGroup::Setup(float ew) {
    left.Setup();
    middle.Setup();
    right.Setup();
    edge = ew;
}

void hhBackgroundGroup::Draw(idDeviceContext *dc, const idRectangle &total, bool vertical, float matScalex, float matScaley, unsigned int flags, const idVec4 &color)
{
    idRectangle rects[3];
    hhBackground *bgs[3] = {
        &left,
        &middle,
        &right,
    };
    int mask = CalcRects(total, bgs, rects, vertical, edge);
/*  f(!vertical)
    {
        printf("XXX %s\n", total.ToVec4().ToString());
        printf("111 %s\n", rects[0].ToVec4().ToString());
        printf("222 %s\n", rects[1].ToVec4().ToString());
        printf("333 %s\n", rects[2].ToVec4().ToString());
    }*/
    if(mask)
    {
        for(int i = 0; i < 3; i++)
        {
            if(bgs[i])
                bgs[i]->Draw(dc, rects[i], matScalex, matScaley, flags, color);
        }
    }
}

int hhBackgroundGroup::CalcRects(const idRectangle &total, hhBackground *bgs[3], idRectangle rects[3], bool vertical, float edgeWidth)
{
    const idMaterial *left = bgs[0]->material;
    const idMaterial *middle = bgs[1]->material;
    const idMaterial *right = bgs[2]->material;
    int res = BG_NONE;

    if(left)
        res |= BG_LEFT;
    if(middle)
        res |= BG_MIDDLE;
    if(right)
        res |= BG_RIGHT;
    if(res == 0)
        return BG_NONE;

    if(res & BG_MIDDLE)
    {
        if((res & BG_LEFT) && (res & BG_RIGHT) == 0)
            res |= BG_LEFT_MIRROR;
        if((res & BG_RIGHT) && (res & BG_LEFT) == 0)
            res |= BG_RIGHT_MIRROR;
    }
    int c = -1;
    if(res == BG_LEFT)
        c = 0;
    else if(res == BG_MIDDLE)
        c = 1;
    else if(res == BG_RIGHT)
        c = 2;

    if(c == -1)
    {
        if(res & BG_LEFT_MIRROR)
        {
            right = left;
            bgs[2] = bgs[0];
        }
        else if(res & BG_RIGHT_MIRROR)
        {
            left = right;
            bgs[0] = bgs[2];
        }
    }

    if(vertical)
    {
        rects[0].x = rects[1].x = rects[2].x = total.x;
        rects[0].w = rects[1].w = rects[2].w = total.w;

        // if full with one part
        if(c != -1)
        {
            for(int i = 0; i < 3; i++)
            {
                rects[i].y = i == c ? 0.0f : total.h;
                rects[i].h = i == c ? total.h : 0.0f;
                if(i != c)
                    bgs[i] = NULL;
            }
            return res;
        }

        float leftH = left ? (float)left->GetImageHeight() / (float)left->GetImageWidth() : 0.0f;
        float middleH = middle ? (float)middle->GetImageHeight() / (float)middle->GetImageWidth() : 0.0f;
        float rightH = right ? (float)right->GetImageHeight() / (float)right->GetImageWidth() : 0.0f;
        float totalH = leftH + middleH + rightH;
        rects[0].y = 0.0f;
        rects[0].h = edgeWidth > 0.0f ? edgeWidth : (leftH / totalH) * total.h;
        rects[2].h = edgeWidth > 0.0f ? edgeWidth : (rightH / totalH) * total.h;
        rects[2].y = total.h - rects[2].h;
        rects[1].y = rects[0].h;
        rects[1].h = total.h - rects[0].h - rects[2].h;
        rects[0].y += total.y;
        rects[1].y += total.y;
        rects[2].y += total.y;

        // if left or right missing one
        if(res & BG_LEFT_MIRROR)
        {
            rects[2].h = -rects[2].h;
        }
        else if(res & BG_RIGHT_MIRROR)
        {
            rects[0].h = -rects[0].h;
        }
    }
    else
    {
        rects[0].y = rects[1].y = rects[2].y = total.y;
        rects[0].h = rects[1].h = rects[2].h = total.h;

        // if full with one part
        if(c != -1)
        {
            for(int i = 0; i < 3; i++)
            {
                rects[i].x = i == c ? 0.0f : total.w;
                rects[i].w = i == c ? total.w : 0.0f;
                if(i != c)
                    bgs[i] = NULL;
            }
            return res;
        }

        float leftW = left ? (float)left->GetImageWidth() / (float)left->GetImageHeight() : 0.0f;
        float middleW = middle ? (float)middle->GetImageWidth() / (float)middle->GetImageHeight() : 0.0f;
        float rightW = right ? (float)right->GetImageWidth() / (float)right->GetImageHeight() : 0.0f;
        float totalW = leftW + middleW + rightW;
        rects[0].x = 0.0f;
        rects[0].w = edgeWidth > 0.0f ? edgeWidth : (leftW / totalW) * total.w;
        rects[2].w = edgeWidth > 0.0f ? edgeWidth : (rightW / totalW) * total.w;
        rects[2].x = total.w - rects[2].w;
        rects[1].x = rects[0].w;
        rects[1].w = total.w - rects[0].w - rects[2].w;
        rects[0].x += total.x;
        rects[1].x += total.x;
        rects[2].x += total.x;

        // if left or right missing one
        if(res & BG_LEFT_MIRROR)
        {
            rects[2].w = -rects[2].w;
        }
        else if(res & BG_RIGHT_MIRROR)
        {
            rects[0].w = -rects[0].w;
        }
    }
    return res;
}
