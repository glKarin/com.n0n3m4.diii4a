// for gui left-middle-right background widget
#ifndef _KARIN_BACKGROUNDIMAGE_H
#define _KARIN_BACKGROUNDIMAGE_H

#include "../../ui/DeviceContext.h"
#include "../../ui/Winvar.h"

/*
 * Background image struct
 *  Like idWindow::backGroundName and idWindow::background
 */
class hhBackground {
public:
    void Reset(void);
    void Draw(idDeviceContext *dc, const idRectangle &drawRect, float matScalex, float matScaley, unsigned int flags, const idVec4 &color = idVec4(1.0f, 1.0f, 1.0f, 1.0f));
    void Setup(void);

    idWinBackground           name;
    const idMaterial          *material;
};



/*
 * Complex background image
 *  Horizontal: left middle right
 *  Vertical: top middle bottom
 *  If left image is unset, using right mirror image
 *  If right image is unset, using left mirror image
 */
class hhBackgroundGroup {
public:
    void Reset(void);
    void Draw(idDeviceContext *dc, const idRectangle &total, bool vertical, float matScalex, float matScaley, unsigned int flags, const idVec4 &color = idVec4(1.0f, 1.0f, 1.0f, 1.0f));
    void Setup(float edge = -1.0f);

    static int CalcRects(const idRectangle &total, hhBackground *bgs[3], idRectangle rects[3], bool vertical, float edgeWidth = -1.0f);

    hhBackground           left, middle, right;
    float                  edge;
private:
    enum {
        BG_NONE = 0,
        BG_LEFT = BIT(0),
        BG_MIDDLE = BIT(1),
        BG_RIGHT = BIT(2),
        BG_LEFT_MIRROR = BIT(3), // when right image unset
        BG_RIGHT_MIRROR = BIT(4), // when left image unset
    };
};

#endif // _KARIN_BACKGROUNDIMAGE_H
