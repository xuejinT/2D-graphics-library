/**
 *  Copyright 2017 Mike Reed
 */

#ifndef draw_shape_DEFINED
#define draw_shape_DEFINED

#include "GColor.h"
#include "GCanvas.h"
#include "GShader.h"

class Shape {
    float fRotation = 0;
public:
    virtual ~Shape() {}

    void preRotate(float rad) {
        fRotation += rad;
    }

    GMatrix computeMatrix();
    GMatrix computeInverse();
    void draw(GCanvas* canvas);

    virtual void drawHilite(GCanvas* canvas);

    void setGradient(const GPoint pts[2], const GColor colors[]);
    void toggleTile() {
        fTile = (fTile + 1) % 3;
        this->updateGradient();
    }

    virtual GRect getRect() = 0;
    virtual void setRect(const GRect&) {}
    virtual GColor getColor() = 0;
    virtual void setColor(const GColor&) {}

    virtual bool hasLocalTranslate() { return false; }
    virtual void moveLocalTranslate(GVector) {}

    virtual GClick* findClick(GPoint p, GWindow* wind);
    virtual bool doSym(uint32_t) { return false; }

    virtual void toggleGeo() {}

protected:
    virtual void onDraw(GCanvas* canvas) {}

    void updateGradient() {
        fGradient = GCreateLinearGradient(fGradPts[0], fGradPts[1],
                                          fGradColors[0], fGradColors[1],
                                          GShader::TileMode(fTile));
    }

    GPaint makePaint() {
        GPaint p(this->getColor());
        p.setShader(fGradient.get());
        return p;
    }

    GPoint fGradPts[2];
    GColor fGradColors[2] = { {1, 0, 0, 1}, { 0, 0, 1, 1 } };
    int fTile = 0;
    std::unique_ptr<GShader> fGradient;
};

#endif

