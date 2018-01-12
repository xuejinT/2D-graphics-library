/*
 *  Copyright 2017 Mike Reed
 */

#ifndef GProxyCanvas_DEFINED
#define GProxyCanvas_DEFINED

#include "GCanvas.h"

class GProxyCanvas : public GCanvas {
public:
    GProxyCanvas(GCanvas* proxy) : fProxy(proxy) {}

    bool virtual allowDraw() { return true; }

    void save() override {
        if (this->allowDraw()) {
            fProxy->save();
        }
    }
    void restore() override {
        if (this->allowDraw()) {
            fProxy->restore();
        }
    }
    void concat(const GMatrix& m) override {
        if (this->allowDraw()) {
            fProxy->concat(m);
        }
    }

    void clear(const GColor& c) override {
        if (this->allowDraw()) {
            fProxy->clear(c);
        }
    }

    void drawRect(const GRect& r, const GPaint& p) override {
        if (this->allowDraw()) {
            fProxy->drawRect(r, p);
        }
    }

    void fillBitmapRect(const GBitmap& b, const GRect& r) override {
        if (this->allowDraw()) {
            fProxy->fillBitmapRect(b, r);
        }
    }

    void drawConvexPolygon(const GPoint pts[], int count, const GPaint& p) override {
        if (this->allowDraw()) {
            fProxy->drawConvexPolygon(pts, count, p);
        }
    }

    void drawContours(const GContour ctrs[], int count, const GPaint& p) override {
        if (this->allowDraw()) {
            fProxy->drawContours(ctrs, count, p);
        }
    }

    void drawMesh(int triCount, const GPoint pts[], const int indices[], const GColor colors[],
                  const GPoint tex[], const GPaint& paint) override {
        if (this->allowDraw()) {
            fProxy->drawMesh(triCount, pts, indices, colors, tex, paint);
        }
    }

    void drawQuad(const GPoint pts[4], const GColor colors[4], const GPoint tex[4],
                  int level, const GPaint& paint) override {
        if (this->allowDraw()) {
            fProxy->drawQuad(pts, colors, tex, level, paint);
        }
    }

private:
    GCanvas* fProxy;
};

#endif
