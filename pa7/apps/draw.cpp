/**
 *  Copyright 2015 Mike Reed
 */

#include "GWindow.h"
#include "GBitmap.h"
#include "GCanvas.h"
#include "GColor.h"
#include "GMatrix.h"
#include "GRandom.h"
#include "GRect.h"
#include "GShader.h"

#include "draw_shape.h"

#include <vector>

extern Shape* MeshShape_Factory();

static const float CORNER_SIZE = 9;

template <typename T> int find_index(const std::vector<T*>& list, T* target) {
    for (int i = 0; i < list.size(); ++i) {
        if (list[i] == target) {
            return i;
        }
    }
    return -1;
}

static GRandom gRand;

static GColor rand_color(float alpha = 0.5) {
    return GColor::MakeARGB(alpha, gRand.nextF(), gRand.nextF(), gRand.nextF());
}

static GRect make_from_pts(const GPoint& p0, const GPoint& p1) {
    return GRect::MakeLTRB(std::min(p0.fX, p1.fX), std::min(p0.fY, p1.fY),
                           std::max(p0.fX, p1.fX), std::max(p0.fY, p1.fY));
}

static bool contains(const GRect& rect, float x, float y) {
    return rect.left() < x && x < rect.right() && rect.top() < y && y < rect.bottom();
}

static GRect offset(const GRect& rect, float dx, float dy) {
    return GRect::MakeLTRB(rect.left() + dx, rect.top() + dy,
                           rect.right() + dx, rect.bottom() + dy);
}

static bool hit_test(float x0, float y0, float x1, float y1) {
    const float dx = fabs(x1 - x0);
    const float dy = fabs(y1 - y0);
    return std::max(dx, dy) <= CORNER_SIZE;
}

static bool in_resize_corner(const GRect& r, float x, float y, GPoint* anchor) {
    if (hit_test(r.left(), r.top(), x, y)) {
        anchor->set(r.right(), r.bottom());
        return true;
    } else if (hit_test(r.right(), r.top(), x, y)) {
        anchor->set(r.left(), r.bottom());
        return true;
    } else if (hit_test(r.right(), r.bottom(), x, y)) {
        anchor->set(r.left(), r.top());
        return true;
    } else if (hit_test(r.left(), r.bottom(), x, y)) {
        anchor->set(r.right(), r.top());
        return true;
    }
    return false;
}

static void draw_corner(GCanvas* canvas, const GColor& c, float x, float y, float dx, float dy) {
    canvas->fillRect(make_from_pts(GPoint::Make(x, y - 1), GPoint::Make(x + dx, y + 1)), c);
    canvas->fillRect(make_from_pts(GPoint::Make(x - 1, y), GPoint::Make(x + 1, y + dy)), c);
}

static void draw_hilite(GCanvas* canvas, const GRect& r) {
    const float size = CORNER_SIZE;
    GColor c = GColor::MakeARGB(1, 0, 0, 0);
    draw_corner(canvas, c, r.fLeft, r.fTop, size, size);
    draw_corner(canvas, c, r.fLeft, r.fBottom, size, -size);
    draw_corner(canvas, c, r.fRight, r.fTop, -size, size);
    draw_corner(canvas, c, r.fRight, r.fBottom, -size, -size);
}

static void draw_line(GCanvas* canvas, GPoint p0, GPoint p1, GColor c, float width) {
    GVector norm = { p1.y() - p0.y(), p0.x() - p1.x() };
    float scale = width / 2 / norm.length();
    norm.fX *= scale;
    norm.fY *= scale;

    GPoint quad[4] = { p0 + norm, p1 + norm, p1 - norm, p0 - norm };
    canvas->fillConvexPolygon(quad, 4, c);
}

static void draw_arrow(GCanvas* canvas, GPoint p0, GPoint p1, GColor color) {
    GVector v = p1 - p0;
    GVector uv = v * (1 / v.length());
    GVector un = { uv.y(), -uv.x() };

    GPoint c = p0 + v * 0.5;
    GPoint pts[3] = { c + un * 4, c - un * 4, c + uv * 12 };
    canvas->drawConvexPolygon(pts, 3, color);
}

static void constrain_color(GColor* c) {
    c->fA = std::max(std::min(c->fA, 1.f), 0.1f);
    c->fR = std::max(std::min(c->fR, 1.f), 0.f);
    c->fG = std::max(std::min(c->fG, 1.f), 0.f);
    c->fB = std::max(std::min(c->fB, 1.f), 0.f);
}

GMatrix Shape::computeMatrix() {
    const GRect r = this->getRect();
    GMatrix m;
    m.preTranslate(r.centerX(), r.centerY());
    m.preRotate(fRotation);
    m.preTranslate(-r.centerX(), -r.centerY());
    return m;
}
    
GMatrix Shape::computeInverse() {
    GMatrix inverse;
    this->computeMatrix().invert(&inverse);
    return inverse;
}
    
void Shape::draw(GCanvas* canvas) {
    canvas->save();
    canvas->concat(this->computeMatrix());
    this->onDraw(canvas);
    canvas->restore();
}

void Shape::drawHilite(GCanvas* canvas) {
    draw_hilite(canvas, this->getRect());
    if (fGradient) {
        draw_line(canvas, fGradPts[0], fGradPts[1], {1,0,0,0}, 1.6f);
    }
}

void Shape::setGradient(const GPoint pts[2], const GColor colors[]) {
    if (fGradient) {
        fGradient = nullptr;
    } else {
        memcpy(fGradPts, pts, sizeof(fGradPts));
        memcpy(fGradColors, colors, sizeof(fGradColors));
        this->updateGradient();
    }
}

GClick* Shape::findClick(GPoint p, GWindow* wind) {
    if (fGradient) {
        int index = -1;
        for (int i = 0; i < 2; ++i) {
            if (hit_test(p.x(), p.y(), fGradPts[i].x(), fGradPts[i].y())) {
                index = i;
            }
        }
        if (index >= 0) {
            return new GClick(p, [this, wind, index](GClick* click) {
                fGradPts[index] = click->curr();
                this->updateGradient();
                wind->requestDraw();
            });
        }
    }
    return nullptr;
}

class RectShape : public Shape {
public:
    RectShape(GColor c) : fColor(c) {
        fRect = GRect::MakeXYWH(0, 0, 0, 0);
    }

    void onDraw(GCanvas* canvas) override {
        if (fIsRect) {
            canvas->drawRect(fRect, this->makePaint());
        } else {
            float inset = fRect.height() / 4;
            GPoint pts[6] = {
                { fRect.centerX(), fRect.top() },
                { fRect.right(),   fRect.bottom() - inset },
                { fRect.left(),    fRect.bottom() - inset },
                ////
                { fRect.centerX(), fRect.bottom() },
                { fRect.left(),    fRect.top() + inset },
                { fRect.right(),   fRect.top() + inset },
            };
            GContour ctrs[2] = {
                { 3, &pts[0] },
                { 3, &pts[3] },
            };
            canvas->drawContours(ctrs, 2, this->makePaint());
        }
    }

    GRect getRect() override { return fRect; }
    void setRect(const GRect& r) override { fRect = r; }
    GColor getColor() override { return fColor; }
    void setColor(const GColor& c) override { fColor = c; }

    void toggleGeo() override { fIsRect = !fIsRect; }

private:
    GRect   fRect;
    GColor  fColor;
    bool    fIsRect = true;
};

static float compute_cross(GPoint a, GPoint b, GPoint c) {
    GPoint ac = { c.x() - a.x(), c.y() - a.y() };
    GPoint ab = { b.x() - a.x(), b.y() - a.y() };
    return ac.x() * ab.y() - ac.y() * ab.x();
}

static bool inside(const GPoint p[], int count, GPoint target) {
    if (count <= 2) {
        return false;
    }
    float cross = compute_cross(p[count-1], p[0], target);
    for (int i = 0; i < count - 1; ++i) {
        float c = compute_cross(p[i], p[i+1], target);
        if ((cross < 0) != (c < 0)) {
            return false;
        }
    }
    return true;
}

static void offset(GPoint p[], int count, GVector d) {
    for (int i = 0; i < count; ++i) {
        p[i] += d;
    }
}

static void set_rect_pts(GPoint pts[4], const GRect& r) {
    pts[0] = { r.left(),  r.top() };
    pts[1] = { r.right(), r.top() };
    pts[2] = { r.right(), r.bottom() };
    pts[3] = { r.left(),  r.bottom() };
}

static void make_star(GPoint pts[], int count, float anglePhase) {
    GASSERT(count & 1);
    float da = 2 * M_PI * (count >> 1) / count;
    float angle = anglePhase;
    for (int i = 0; i < count; ++i) {
        pts[i].fX = cosf(angle);
        pts[i].fY = sinf(angle);
        angle += da;
    }
}

class PolyShape : public Shape {
    GPoint fPts[10];
    GContour fCtrs[2];
    int fPtCount;
    int fCtrCount;
public:
    PolyShape(GColor c, int ctrCount) : fColor(c) {
        if (ctrCount == 2) {
            fPtCount = 8;
            fCtrCount = 2;
            set_rect_pts(fPts, GRect::MakeXYWH(100, 100, 200, 150));
            set_rect_pts(fPts + 4, GRect::MakeXYWH(150, 150, 100, 50));
            fCtrs[0] = { 4, &fPts[0] };
            fCtrs[1] = { 4, &fPts[4] };
        } else if (ctrCount == 1) {
            fPtCount = 7;
            fCtrCount = 1;
            make_star(fPts, 7, 0);
            GMatrix m;
            m.setScale(100, 100); m.postTranslate(200, 200); m.mapPoints(fPts, fPts, 7);
            fCtrs[0] = { 7, &fPts[0] };
        }
    }
    
    void onDraw(GCanvas* canvas) override {
        canvas->drawContours(fCtrs, fCtrCount, this->makePaint());
    }
    void drawHilite(GCanvas* canvas) override {
        GPaint p;
        for (int i = 0; i < fPtCount; ++i) {
            canvas->drawRect(GRect::MakeXYWH(fPts[i].x() - 2, fPts[i].y() - 2, 5, 5), p);
        }
        int base = 0;
        for (int j = 0; j < fCtrCount; ++j) {
            for (int i = 0; i < fCtrs[j].fCount; ++i) {
                GPoint p0 = fPts[base + i],
                       p1 = fPts[base + ((i + 1) % fCtrs[j].fCount)];
                draw_line(canvas, p0, p1, {1,0,0,0}, 1);
                draw_arrow(canvas, p0, p1, {1,0,0,0});
            }
            base += fCtrs[j].fCount;
        }
    }

    GRect getRect() override {
        float L, T, R, B;
        L = R = fPts[0].x();
        T = B = fPts[0].y();
        for (int i = 1; i < fPtCount; ++i) {
            L = std::min(L, fPts[i].x());
            T = std::min(T, fPts[i].y());
            R = std::max(R, fPts[i].x());
            B = std::max(B, fPts[i].y());
        }
        return GRect::MakeLTRB(L, T, R, B);
    }
    void setRect(const GRect& r) override {}
    GColor getColor() override { return fColor; }
    void setColor(const GColor& c) override { fColor = c; }
    
    GClick* findClick(GPoint p, GWindow* wind) override {
        int index = -1;
        for (int i = 0; i < fPtCount; ++i) {
            if (hit_test(p.x(), p.y(), fPts[i].x(), fPts[i].y())) {
                index = i;
            }
        }
        if (index >= 0) {
            return new GClick(p, [this, wind, index](GClick* click) {
                fPts[index] = click->curr();
                wind->requestDraw();
            });
        }
        index = -1;
        for (int i = 0; i < fCtrCount; ++i) {
            if (inside(fCtrs[i].fPts, fCtrs[i].fCount, p)) {
                index = i;
                break;
            }
        }
        if (index >= 0) {
            return new GClick(p, [this, wind, index](GClick* click) {
                offset(const_cast<GPoint*>(fCtrs[index].fPts), fCtrs[index].fCount,
                       click->curr() - click->prev());
                wind->requestDraw();
            });
        }
        return Shape::findClick(p, wind);
    }
private:
    GColor  fColor;
};

static GMatrix rect2rect(const GRect& src, const GRect& dst) {
    GMatrix m;
    m.setTranslate(dst.left(), dst.top());
    m.preScale(dst.width() / src.width(), dst.height() / src.height());
    m.preTranslate(-src.left(), -src.top());
    return m;
}

class BitmapShape : public Shape {
public:
    BitmapShape(const GBitmap& bm) : fBM(bm) {
        const int w = std::max(bm.width(), 100);
        const int h = std::max(bm.height(), 100);
        fRect = GRect::MakeXYWH(100, 100, w, h);
    }
    
    void onDraw(GCanvas* canvas) override {
        GMatrix m = rect2rect(GRect::MakeWH(fBM.width(), fBM.height()), fRect);
        m.postConcat(fLM);
        auto s = GCreateBitmapShader(fBM, m, GShader::kRepeat);
        GPaint paint;
        paint.setShader(s.get());
        paint.setColor(fColor);
        canvas->drawRect(fRect, paint);
    }
    
    GRect getRect() override { return fRect; }
    void setRect(const GRect& r) override { fRect = r; }
    GColor getColor() override { return fColor; }
    void setColor(const GColor& c) override { fColor = c; }

    bool hasLocalTranslate() override { return true; }
    void moveLocalTranslate(GVector v) override {
        fLM.postTranslate(v.x(), v.y());
    }

private:
    GRect   fRect;
    GBitmap fBM;
    GColor  fColor = { 1, 0, 0, 0 };
    GMatrix fLM;
};

static Shape* cons_up_shape(unsigned index) {
    const char* names[] = {
        "apps/spock.png",
        "apps/oldwell.png",
    };
    GBitmap bm;
    if (index < GARRAY_COUNT(names) && bm.readFromFile(names[index])) {
        return new BitmapShape(bm);
    }
    switch (index) {
        case 2: return new PolyShape({1, 0, 0.75, 1}, 2);
        case 3: return new PolyShape({0.5, 1, 0, 0}, 1);
        case 4: return MeshShape_Factory();
    }
    return NULL;
}

static bool ctrl_mod() {
    const uint8_t* state = SDL_GetKeyboardState(nullptr);
    return state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL];
}

class TestWindow : public GWindow {
    std::vector<Shape*> fList;
    Shape* fShape;
    GColor fBGColor;

public:
    TestWindow(int w, int h) : GWindow(w, h) {
        fBGColor = GColor::MakeARGB(1, 1, 1, 1);
        fShape = NULL;
    }

    virtual ~TestWindow() {}
    
protected:
    void onDraw(GCanvas* canvas) override {
        canvas->clear(fBGColor);

        for (int i = 0; i < fList.size(); ++i) {
            fList[i]->draw(canvas);
        }
        if (fShape) {
            canvas->save();
            canvas->concat(fShape->computeMatrix());
            fShape->drawHilite(canvas);
            canvas->restore();
        }
    }

    bool onKeyPress(uint32_t sym) override {
        if (fShape && fShape->doSym(sym)) {
            this->requestDraw();
            return true;
        }

        {
            Shape* s = cons_up_shape(sym - '1');
            if (s) {
                fList.push_back(fShape = s);
                this->updateTitle();
                this->requestDraw();
                return true;
            }
        }

        if (fShape) {
            switch (sym) {
                case SDLK_UP: {
                    int index = find_index(fList, fShape);
                    if (index < fList.size() - 1) {
                        std::swap(fList[index], fList[index + 1]);
                        this->requestDraw();
                        return true;
                    }
                    return false;
                }
                case SDLK_DOWN: {
                    int index = find_index(fList, fShape);
                    if (index > 0) {
                        std::swap(fList[index], fList[index - 1]);
                        this->requestDraw();
                        return true;
                    }
                    return false;
                }
                case SDLK_LEFT:
                case SDLK_RIGHT: {
                    const float rad = M_PI * 2 / 180;
                    fShape->preRotate(rad * (sym == SDLK_LEFT ? -1 : 1));
                    this->requestDraw();
                    return true;
                }
                case SDLK_DELETE:
                case SDLK_BACKSPACE:
                    this->removeShape(fShape);
                    fShape = NULL;
                    this->updateTitle();
                    this->requestDraw();
                    return true;

                case 'l': {
                    GPoint pts[] = { { 100, 100 }, {200, 200}};
                    GColor clr[] = { rand_color(1), rand_color(1) };
                    fShape->setGradient(pts, clr);
                    this->requestDraw();
                    return true;
                } break;
                case 'o':
                    fShape->toggleGeo();
                    this->requestDraw();
                    return true;
                case 't':
                    fShape->toggleTile();
                    this->requestDraw();
                    return true;
                default:
                    break;
            }
        }

        GColor c = fShape ? fShape->getColor() : fBGColor;
        const float delta = 0.1f;
        switch (sym) {
            case 'a': c.fA -= delta; break;
            case 'A': c.fA += delta; break;
            case 'r': c.fR -= delta; break;
            case 'R': c.fR += delta; break;
            case 'g': c.fG -= delta; break;
            case 'G': c.fG += delta; break;
            case 'b': c.fB -= delta; break;
            case 'B': c.fB += delta; break;
            default:
                return false;
        }
        constrain_color(&c);
        if (fShape) {
            fShape->setColor(c);
        } else {
            c.fA = 1;   // need the bg to stay opaque
            fBGColor = c;
        }
        this->updateTitle();
        this->requestDraw();
        return true;
    }

    GClick* onFindClickHandler(GPoint loc) override {
        if (fShape) {
            if (GClick* click = fShape->findClick(loc, this)) {
                return click;
            }
            GPoint anchor;
            if (in_resize_corner(fShape->getRect(), loc.x(), loc.y(), &anchor)) {
                return new GClick(loc, [this, anchor](GClick* click) {
                    fShape->setRect(make_from_pts(click->curr(), anchor));
                    this->updateTitle();
                    this->requestDraw();
                });
            }
        }

        for (int i = fList.size() - 1; i >= 0; --i) {
            if (contains(fList[i]->getRect(), loc.x(), loc.y())) {
                fShape = fList[i];
                this->updateTitle();
                return new GClick(loc, [this](GClick* click) {
                    const GPoint curr = click->curr();
                    const GPoint prev = click->prev();
                    fShape->setRect(offset(fShape->getRect(), curr.x() - prev.x(), curr.y() - prev.y()));
                    this->updateTitle();
                    this->requestDraw();
                });
            }
        }

        // try dragging its shader's matrix
        if (fShape && fShape->hasLocalTranslate() && ctrl_mod()) {
            return new GClick(loc, [this](GClick* click) {
                fShape->moveLocalTranslate(click->curr() - click->prev());
                this->requestDraw();
            });
        }

        // else create a new shape
        fShape = new RectShape(rand_color());
        fList.push_back(fShape);
        this->updateTitle();
        return new GClick(loc, [this](GClick* click) {
            if (fShape && GClick::kUp_State == click->state()) {
                if (fShape->getRect().isEmpty()) {
                    this->removeShape(fShape);
                    fShape = NULL;
                    return;
                }
            }
            fShape->setRect(make_from_pts(click->orig(), click->curr()));
            this->updateTitle();
            this->requestDraw();
        });
    }

private:
    void removeShape(Shape* target) {
        GASSERT(target);

        std::vector<Shape*>::iterator it = std::find(fList.begin(), fList.end(), target);
        if (it != fList.end()) {
            fList.erase(it);
        } else {
            GASSERT(!"shape not found?");
        }
    }

    void updateTitle() {
        char buffer[100];
        buffer[0] = ' ';
        buffer[1] = 0;

        GColor c = fBGColor;
        if (fShape) {
            c = fShape->getColor();
        }

        sprintf(buffer, "%02X %02X %02X %02X",
                int(c.fA * 255), int(c.fR * 255), int(c.fG * 255), int(c.fB * 255));
        this->setTitle(buffer);
    }

    typedef GWindow INHERITED;
};

int main(int argc, char const* const* argv) {
    GWindow* wind = new TestWindow(640, 480);

    return wind->run();
}

