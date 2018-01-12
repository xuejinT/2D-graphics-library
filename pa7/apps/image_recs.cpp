/**
 *  Copyright 2015 Mike Reed
 */

#include "image.h"
#include "GCanvas.h"
#include "GBitmap.h"
#include "GColor.h"
#include "GMatrix.h"
#include "GPoint.h"
#include "GRect.h"
#include "GShader.h"
#include <string>
#include <vector>

#define RAMP_W      1
#define RAMP_H     28

static void draw_solid_ramp(GCanvas* canvas) {
    const float c = 1.0 / 512;
    const float d = 1.0 / 256;

    const struct {
        GColor  fC0, fDC;
    } rec[] = {
        { GColor::MakeARGB(1,   c,   c,   c), GColor::MakeARGB(0,  d,  d,  d) },   // grey
        { GColor::MakeARGB(1, 1-c,   0,   0), GColor::MakeARGB(0, -d,  0,  0) },   // red
        { GColor::MakeARGB(1,   0,   c,   c), GColor::MakeARGB(0,  0,  d,  d) },   // cyan
        { GColor::MakeARGB(1,   0, 1-c,   0), GColor::MakeARGB(0,  0, -d,  0) },   // green
        { GColor::MakeARGB(1,   c,   0,   c), GColor::MakeARGB(0,  d,  0,  d) },   // magenta
        { GColor::MakeARGB(1,   0,   0, 1-c), GColor::MakeARGB(0,  0,  0, -d) },   // blue
        { GColor::MakeARGB(1,   c,   c,   0), GColor::MakeARGB(0,  d,  d,  0) },   // yellow
    };


    for (int y = 0; y < GARRAY_COUNT(rec); ++y) {
        GColor color = rec[y].fC0;
        GColor delta = rec[y].fDC;
        for (int x = 0; x < 256; x++) {
            const GRect rect = GRect::MakeXYWH(x * RAMP_W, y * RAMP_H, RAMP_W, RAMP_H);
            canvas->fillRect(rect, color);
            color.fA += delta.fA;
            color.fR += delta.fR;
            color.fG += delta.fG;
            color.fB += delta.fB;
        }
    }
}

static void offset(GRect* r, float dx, float dy) {
    r->fLeft += dx;
    r->fRight += dx;
    r->fTop += dy;
    r->fBottom += dy;
}

static void draw_blend_ramp(GCanvas* canvas, const GColor& bg) {
    canvas->clear(bg);

    GRect rect = GRect::MakeXYWH(-25, -25, 70, 70);

    int delta = 8;
    for (int i = 0; i < 200; i += delta) {
        float r = i / 200.0;
        float g = fabs(cos(i/40.0));
        float b = fabs(sin(i/50.0));
        GColor color = GColor::MakeARGB(0.3, r, g, b);
        canvas->fillRect(rect, color);
        offset(&rect, delta, delta);
    }
}

static void draw_blend_white(GCanvas* canvas) {
    draw_blend_ramp(canvas, GColor::MakeARGB(1, 1, 1, 1));
}

static void draw_blend_black(GCanvas* canvas) {
    draw_blend_ramp(canvas, GColor::MakeARGB(1, 0, 0, 0));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static void draw_spocks_quad_inner(GCanvas* canvas) {
    const int N = 300;

    GBitmap tex;
    tex.readFromFile("apps/oldwell.png");

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            canvas->fillBitmapRect(tex, GRect::MakeXYWH(x * N - N/2, y * N - N/2, N, N));
        }
    }

    float w = tex.width()*2/3;
    float h = tex.height()*2/3;
    GRect r = GRect::MakeXYWH(150 - w/2, 150 - h/2, w, h);
    canvas->fillRect(r, {1, 0, 0, 0});
    canvas->fillBitmapRect(tex, r);
}

static void draw_spocks_quad(GCanvas* canvas) {
    canvas->clear({1, 1, 1, 1});
    draw_spocks_quad_inner(canvas);
}

static void draw_spocks_zoom(GCanvas* canvas) {
    const int N = 300;

    GBitmap tex;
    tex.readFromFile("apps/spock.png");

    for (int i = 0; i < 9; ++i) {
        GRect r = GRect::MakeLTRB(i * 10, i * 10, N - i * 10, N - i * 10);
        canvas->fillBitmapRect(tex, r);
    }
}

// After scaling by this, the caller need just cast to (int)
static const float gScaleUnitToByte = 255.99999f;

static GPixel pin_and_premul_to_pixel(GColor c) {
    c = c.pinToUnit();

    float a = c.fA * gScaleUnitToByte;
    int ia = (int)a;
    int ir = (int)(a * c.fR);
    int ig = (int)(a * c.fG);
    int ib = (int)(a * c.fB);
    return GPixel_PackARGB(ia, ir, ig, ib);
}

static void make_circle(const GBitmap& bitmap, const GColor& color) {
    const GPixel px = pin_and_premul_to_pixel(color);

    const float cx = (float)bitmap.width() / 2;
    const float cy = (float)bitmap.height() / 2;
    const float radius = cx - 1;
    const float radius2 = radius * radius;

    GPixel* dst = bitmap.pixels();
    for (int y = 0; y < bitmap.height(); ++y) {
        const float dy = y - cy;
        for (int x = 0; x < bitmap.width(); ++x) {
            const float dx = x - cx;
            const float dist2 = dx*dx + dy*dy;
            if (dist2 <= radius2) {
                dst[x] = px;
            } else {
                dst[x] = 0; // transparent
            }
        }
        dst = (GPixel*)((char*)dst + bitmap.rowBytes());
    }
}

class AutoBitmap : public GBitmap {
public:
    AutoBitmap(int width, int height) {
        // just to exercise the ability to have a rowbytes > width
        const int slop = (height >> 1) * sizeof(GPixel);
        const size_t rb = width * sizeof(GPixel) + slop;
        this->reset(width, height, rb, (GPixel*)malloc(height * rb), kNo_IsOpaque);
    }

    ~AutoBitmap() {
        free(this->pixels());
    }
};

static void draw_bm_circles(GCanvas* canvas) {
    const int N = 300;

    AutoBitmap src(N, N);

    const struct {
        GRect   fRect;
        GColor  fColor;
    } recs[] = {
        { GRect::MakeXYWH(  0,   0,   N,   N), GColor::MakeARGB(1, 1, 1, 1) },

        { GRect::MakeXYWH(  0,   0, N/2, N/2), GColor::MakeARGB(0.8f, 0, 0, 1) },
        { GRect::MakeXYWH(N/2,   0, N/2, N/2), GColor::MakeARGB(0.6f, 0, 1, 0) },
        { GRect::MakeXYWH(  0, N/2, N/2, N/2), GColor::MakeARGB(0.4f, 1, 0, 0) },
        { GRect::MakeXYWH(N/2, N/2, N/2, N/2), GColor::MakeARGB(0.2f, 0, 0, 0) },

        { GRect::MakeXYWH(  0, N/3,   N, N/3), GColor::MakeARGB(0.5f, 1, 1, 0) },
        { GRect::MakeXYWH(N/3,   0, N/3,   N), GColor::MakeARGB(0.5f, 0, 1, 1) },
        { GRect::MakeXYWH(N/3, N/3, N/3, N/3), GColor::MakeARGB(0.5f, 1, 0, 1) },
    };

    for (int i = 0; i < GARRAY_COUNT(recs); ++i) {
        make_circle(src, recs[i].fColor);
        canvas->fillBitmapRect(src, recs[i].fRect);
    }
}

static void draw_circle_big(GCanvas* canvas) {
    const int N = 300;
    const float alpha = 0.4f;
    const GColor colors[] = {
        GColor::MakeARGB(alpha, 1, 0, 0),
        GColor::MakeARGB(alpha, 0, 1, 0),
        GColor::MakeARGB(alpha, 0, 0, 1),
    };

    int x = 0;
    int n = N;
    for (int i = 0; n > 4; ++i) {
        AutoBitmap src(n, n);
        make_circle(src, colors[i % GARRAY_COUNT(colors)]);
        canvas->fillBitmapRect(src, GRect::MakeXYWH(x, 0, N, N));
        x += N / 12;
        n >>= 1;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static void draw_tri(GCanvas* canvas) {
    GPoint pts[] = {
        { 10, 10 },
        { 200, 50 },
        { 100, 200 },
    };
    canvas->fillConvexPolygon(pts, GARRAY_COUNT(pts), GColor::MakeARGB(1, 0, 1, 0));
}

static void draw_tri_clipped(GCanvas* canvas) {
    GPoint pts[] = {
        { -10, -10 },
        { 300, 50 },
        { 100, 300 },
    };
    canvas->fillConvexPolygon(pts, GARRAY_COUNT(pts), GColor::MakeARGB(1, 1, 1, 0));
}

static void make_regular_poly(GPoint pts[], int count, float cx, float cy, float radius) {
    float angle = 0;
    const float deltaAngle = M_PI * 2 / count;

    for (int i = 0; i < count; ++i) {
        pts[i].set(cx + cos(angle) * radius, cy + sin(angle) * radius);
        angle += deltaAngle;
    }
}

static void dr_poly(GCanvas* canvas, float dx, float dy) {
    GPoint storage[12];
    for (int count = 12; count >= 3; --count) {
        make_regular_poly(storage, count, 256, 256, count * 10 + 120);
        for (int i = 0; i < count; ++i) {
            storage[i].fX += dx;
            storage[i].fY += dy;
        }
        GColor c = GColor::MakeARGB(0.8f,
                                    fabs(sin(count*7)),
                                    fabs(sin(count*11)),
                                    fabs(sin(count*17)));
        canvas->fillConvexPolygon(storage, count, c);
    }
}

static void draw_poly(GCanvas* canvas) {
    dr_poly(canvas, 0, 0);
}

static void draw_poly_center(GCanvas* canvas) {
    dr_poly(canvas, -128, -128);
}

static GPoint scale(GPoint vec, float size) {
    float scale = size / sqrt(vec.fX * vec.fX + vec.fY * vec.fY);
    return GPoint::Make(vec.fX * scale, vec.fY * scale);
}

static void draw_line(GCanvas* canvas, GPoint a, GPoint b, float width, const GColor& color) {
    GPoint norm = scale(GPoint::Make(b.fY - a.fY, a.fX - b.fX), width/2);

    GPoint pts[4];
    pts[0] = GPoint::Make(a.fX + norm.fX, a.fY + norm.fY);
    pts[1] = GPoint::Make(b.fX + norm.fX, b.fY + norm.fY);
    pts[2] = GPoint::Make(b.fX - norm.fX, b.fY - norm.fY);
    pts[3] = GPoint::Make(a.fX - norm.fX, a.fY - norm.fY);

    canvas->fillConvexPolygon(pts, 4, color);
}

static void draw_poly_rotate(GCanvas* canvas) {
    const GPoint start = GPoint::Make(20, 20);
    const float scale = 200;

    const int N = 10;
    GColor color = GColor::MakeARGB(1, 1, 0, 0);
    const float deltaR = -1.0 / N;
    const float deltaB = 1.0 / N;

    const float width = 10;

    for (float angle = 0; angle <= M_PI/2; angle += M_PI/2/N) {
        GPoint end = GPoint::Make(start.fX + cos(angle) * scale,
                                  start.fY + sin(angle) * scale);
        draw_line(canvas, start, end, width, color);

        color.fR += deltaR;
        color.fB += deltaB;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////

static void draw_bitmap_wheel(GCanvas* canvas) {
    GBitmap bitmap;
    bitmap.readFromFile("apps/spock.png");
    GRect rect = GRect::MakeWH(bitmap.width()/5, bitmap.height()/5);
    const float cx = rect.width()/2;
    const float cy = rect.height()/2;

    canvas->translate(150, 150);

    GMatrix matrix;
    for (float angle = 0; angle < M_PI * 2 - 0.00001; angle += M_PI/6) {
        matrix.setRotate(angle);
        matrix.preTranslate(0, -110);
        matrix.preTranslate(-cx, -cy);

        canvas->save();
        canvas->concat(matrix);
        canvas->fillBitmapRect(bitmap, rect);
        canvas->restore();
    }
}

static GMatrix make_scale(float scale) {
    GMatrix m;
    m.setScale(scale, scale);
    return m;
}

static GPixel scale(GPixel p, float alpha) {
    unsigned scale = (int)(alpha * 256 + 0.5);
    return GPixel_PackARGB(GPixel_GetA(p) * scale >> 8,
                           GPixel_GetR(p) * scale >> 8,
                           GPixel_GetG(p) * scale >> 8,
                           GPixel_GetB(p) * scale >> 8);
}

class CheckerShader : public GShader {
    const GPixel fP0, fP1;
    const GMatrix fLocalMatrix;

    GPixel fScaled[2];
    GMatrix fInverse;

public:
    CheckerShader(float scale, const GPixel& p0, const GPixel& p1)
    : fLocalMatrix(make_scale(scale))
    , fP0(p0)
    , fP1(p1)
    {}

    bool isOpaque() override {
        return GPixel_GetA(fScaled[0]) == 0xFF && GPixel_GetA(fScaled[1]) == 0xFF;
    }

    bool setContext(const GMatrix& ctm, float alpha) override {
        fScaled[0] = scale(fP0, alpha);
        fScaled[1] = scale(fP1, alpha);

        GMatrix m;
        m.setConcat(ctm, fLocalMatrix);
        return m.invert(&fInverse);
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
        const float dx = fInverse[GMatrix::SX];
        const float dy = fInverse[GMatrix::KY];
        GPoint loc = fInverse.mapXY(x + 0.5f, y + 0.5f);

        for (int i = 0; i < count; ++i) {
            row[i] = fScaled[((int)loc.fX + (int)loc.fY) & 1];
            loc.fX += dx;
            loc.fY += dy;
        }
    }
};

static void draw_checker(GCanvas* canvas) {
    CheckerShader shader(20,
                         GPixel_PackARGB(0xFF, 0, 0, 0),
                         GPixel_PackARGB(0xFF, 0xFF, 0xFF, 0xFF));
    GPaint paint(&shader);

    canvas->clear({ 1, 0.75, 0.75, 0.75 });
    canvas->drawRect(GRect::MakeXYWH(20, 20, 100, 100), paint);

    canvas->save();
    canvas->translate(130, 175);
    canvas->rotate(-M_PI/3);
    canvas->drawRect(GRect::MakeXYWH(20, 20, 100, 100), paint);
    canvas->restore();

    canvas->save();
    canvas->translate(10, 160);
    canvas->scale(0.5, 0.5);
    canvas->drawRect(GRect::MakeXYWH(20, 20, 200, 200), paint);
    canvas->restore();

    CheckerShader shader2(150,
                          GPixel_PackARGB(0xFF, 0xFF, 0, 0),
                          GPixel_PackARGB(0xFF, 0, 0, 0xFF));
    paint.setShader(&shader2);
    paint.setAlpha(0.25);
    canvas->drawRect(GRect::MakeXYWH(0, 0, 300, 300), paint);
}

static float signed_unit_to_unit(float x) {
    GASSERT(x >= -1 && x <= 1);
    return (x + 1) * 0.5;
}

static unsigned to_byte(float x) {
    GASSERT(x >= 0 && x <= 1);
    return (unsigned)(x * 255 + 0.5);
}

class TrigShader : public GShader {
    const GMatrix fLocalMatrix;

    float   fAlpha;
    GMatrix fInverse;

public:
    TrigShader(float scale) : fLocalMatrix(make_scale(scale)) {}

    bool isOpaque() override {
        return fAlpha >= 511.0f / 512;
    }

    bool setContext(const GMatrix& ctm, float alpha) override {
        fAlpha = alpha;

        GMatrix m;
        m.setConcat(ctm, fLocalMatrix);
        return m.invert(&fInverse);
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
        const float dx = fInverse[GMatrix::SX];
        const float dy = fInverse[GMatrix::KY];
        GPoint loc = fInverse.mapXY(x + 0.5f, y + 0.5f);

        unsigned a = (unsigned)(fAlpha * 255 + 0.5);
        for (int i = 0; i < count; ++i) {
            float r = fAlpha * signed_unit_to_unit(sinf(11*loc.fX) * cosf(7*loc.fY));
            float g = fAlpha * signed_unit_to_unit(sinf(7*loc.fX) * cosf(5*loc.fY));
            float b = fAlpha * signed_unit_to_unit(sinf(5*loc.fX) * cosf(11*loc.fY));
            row[i] = GPixel_PackARGB(a, to_byte(r), to_byte(g), to_byte(b));
            loc.fX += dx;
            loc.fY += dy;
        }
    }
};

class RegularPoly {
    GPoint* fPts;
    int     fCount;
public:
    RegularPoly() : fPts(nullptr), fCount(0) {}
    ~RegularPoly() { delete[] fPts; }

    RegularPoly& setPoly(int n, float cx, float cy, float radius) {
        delete[] fPts;
        fPts = new GPoint[n];
        fCount = n;
        make_regular_poly(fPts, fCount, cx, cy, radius);
        return *this;
    }

    void draw(GCanvas* canvas, const GPaint& paint) {
        canvas->drawConvexPolygon(fPts, fCount, paint);
    }
};

static void draw_trigger(GCanvas* canvas) {
    RegularPoly poly;

    TrigShader shader(200);
    GPaint paint(&shader);

    canvas->clear({ 1, 0.75, 0.75, 0.75 });
    poly.setPoly(5, 70, 70, 50).draw(canvas, paint);

    canvas->save();
    canvas->translate(130, 175);
    canvas->rotate(-M_PI/3);
    poly.setPoly(7, 70, 70, 50).draw(canvas, paint);
    canvas->restore();

    canvas->save();
    canvas->translate(10, 160);
    canvas->scale(0.5, 0.5);
    poly.setPoly(9, 130, 130, 100).draw(canvas, paint);
    canvas->restore();

    TrigShader shader2(150);
    paint.setShader(&shader2);
    paint.setAlpha(0.25);
    canvas->drawRect(GRect::MakeXYWH(0, 0, 300, 300), paint);
}

static void draw_composite_draws(GCanvas* canvas) {
    canvas->clear({ 1, 1, 1, 1 });

    canvas->save();
    canvas->translate(20, 20);
    canvas->scale(0.25, 0.25);
    draw_bm_circles(canvas);
    canvas->restore();

    canvas->save();
    canvas->translate(170, 50);
    canvas->scale(0.25, 0.25);
    draw_spocks_quad_inner(canvas);
    canvas->restore();

    canvas->save();
    canvas->translate(20, 180);
    canvas->scale(0.5, 0.5);
    draw_poly_rotate(canvas);
    canvas->restore();

    canvas->save();
    canvas->translate(169, 170);
    canvas->scale(0.25, 0.25);
    draw_poly(canvas);
    canvas->restore();
}

static void draw_poly_rotate_tex(GCanvas* canvas, const GBitmap* tex) {
    const GPoint pts[] {
        { 0, 0.1f }, { -1, 5 }, { 0, 6 }, { 1, 5 },
    };

    canvas->translate(150, 150);
    canvas->scale(25, 25);

    float steps = 12;
    float r = 0;
    float b = 1;
    float step = 1 / (steps - 1);

    GPaint paint;
    std::unique_ptr<GShader> shader;

    if (tex) {
        GMatrix m;
        m.setScale(1/50.f, 1/50.f);
        shader = GCreateBitmapShader(*tex, m);
        paint.setShader(shader.get());
    }

    for (float angle = 0; angle < 2*M_PI - 0.001f; angle += 2*M_PI/steps) {
        paint.setColor({ 1, r, 0, b });
        canvas->save();
        canvas->rotate(angle);
        canvas->drawConvexPolygon(pts, 4, paint);
        canvas->restore();
        r += step;
        b -= step;
    }
}

static void draw_poly_rotate2(GCanvas* canvas) {
    draw_poly_rotate_tex(canvas, nullptr);
}

static void draw_poly_rotate3(GCanvas* canvas) {
    GBitmap tex;
    tex.readFromFile("apps/spock.png");
    draw_poly_rotate_tex(canvas, &tex);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static void alpha_gradient(GCanvas* canvas) {
    const GColor white = { 1, 1, 1, 1 };
    const GColor black = { 1, 0, 0, 0 };
    const GColor colors[] = { black, white, black, white };
    canvas->save();
    for (int i = 0; i < 4; ++i) {
        canvas->fillRect(GRect::MakeWH(64, 256), colors[i]);
        canvas->translate(64, 0);
    }
    canvas->restore();

    GPaint paint;
    auto s = GCreateLinearGradient({0, 0}, {0, 256}, {0, 0, 1, 0}, {1, 0, 1, 0});
    paint.setShader(s.get());
    canvas->drawRect(GRect::MakeWH(128, 256), paint);

    s = GCreateLinearGradient({0, 0}, {0, 256}, {0, 0, 0, 0}, {1, 0, 1, 0});
    paint.setShader(s.get());
    canvas->drawRect(GRect::MakeXYWH(128, 0, 128, 256), paint);
}

static void bitmap_tilemode(GCanvas* canvas) {
    canvas->clear({ 1, 1, 1, 1 });
    
    GBitmap tex;
    tex.readFromFile("apps/spock.png");
    GMatrix localMatrix;
    localMatrix.setScale(1/3., 1/3.);
    
    const float d = 10;
    const float dd = d * 1.65;

    auto s = GCreateBitmapShader(tex, localMatrix, GShader::kRepeat);
    GPaint paint;
    paint.setShader(s.get());
    const GPoint p0[] = { { d, d }, { 512 - dd, d }, { d, 512 - dd } };
    canvas->drawConvexPolygon(p0, 3, paint);
    s = GCreateBitmapShader(tex, localMatrix, GShader::kMirror);
    paint.setShader(s.get());
    const GPoint p1[] = { { 512 - d, 512 - d }, { 512 - d, dd }, { dd, 512 - d } };
    canvas->drawConvexPolygon(p1, 3, paint);
}

static void gradient_tilemode(GCanvas* canvas) {
    canvas->save();
    canvas->translate(256, 256);
    canvas->rotate(M_PI/2);
    canvas->translate(-256, -256);

    canvas->clear({ 1, 1, 1, 1 });
    
    const GPoint pts[] = { { 90, 30 }, { 30, 120 } };
    const GColor colors[] = { { 1, 1, 0, 0 }, { 1, 0, 0, 1 } };

    const float d = 10;
    const float dd = d * 1.65;

    auto s = GCreateLinearGradient(pts[0], pts[1], colors[0], colors[1],
                                   GShader::kRepeat);
    GPaint paint;
    paint.setShader(s.get());
    const GPoint p0[] = { { d, d }, { 512 - dd, d }, { d, 512 - dd } };
    canvas->drawConvexPolygon(p0, 3, paint);

    s = GCreateLinearGradient(pts[0], pts[1], colors[0], colors[1],
                              GShader::kMirror);
    paint.setShader(s.get());
    const GPoint p1[] = { { 512 - d, 512 - d }, { 512 - d, dd }, { dd, 512 - d } };
    canvas->drawConvexPolygon(p1, 3, paint);

    canvas->restore();
}

class GContourStorage {
public:
    GContourStorage() {}
    ~GContourStorage() {
        this->reset();
    }
    
    void reset() {
        for (auto& ctr : fCtrs) {
            delete[] ctr.fPts;
        }
        fCtrs.clear();
    }

    GPoint* addContour(int count, bool isClosed) {
        fCtrs.emplace_back();
        GContour* ctr = &fCtrs.back();
        ctr->fCount = count;
        ctr->fPts = new GPoint[count];
        ctr->fClosed = isClosed;
        return const_cast<GPoint*>(ctr->fPts);
    }

    void transform(const GMatrix& matrix) {
        for (auto& ctr : fCtrs) {
            matrix.mapPoints(const_cast<GPoint*>(ctr.fPts), ctr.fPts, ctr.fCount);
        }
    }

    int countContours() const { return (int)fCtrs.size(); }
    const GContour* contours() const { return &fCtrs.front(); }

private:
    std::vector<GContour> fCtrs;
};

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

static void add_star(GContourStorage& storage, int count) {
    if (count & 1) {
        make_star(storage.addContour(count, true), count, 0);
    } else {
        count >>= 1;
        make_star(storage.addContour(count, true), count, 0);
        make_star(storage.addContour(count, true), count, M_PI);
    }
}

static void make_diamonds(GContourStorage& storage) {
    const GPoint pts[] {
        { 0, 0.1f }, { -1, 5 }, { 0, 6 }, { 1, 5 },
    };
    float steps = 12;
    float step = 1 / (steps - 1);
    
    GMatrix matrix;
    for (float angle = 0; angle < 2*M_PI - 0.001f; angle += 2*M_PI/steps) {
        GMatrix::MakeRotate(angle).mapPoints(storage.addContour(4, true), pts, 4);
    }
}

static void draw_stars(GCanvas* canvas, float width) {
    canvas->clear({1, 1, 1, 1});

    GPaint paint;
    paint.setStrokeWidth(width);

    GMatrix scale;
    scale.setScale(100, 100);

    paint.setColor({1,1,0,0});
    GContourStorage storage;
    add_star(storage, 5);
    storage.transform(scale);
    canvas->translate(120, 120);
    canvas->drawContours(storage.contours(), storage.countContours(), paint);
    
    paint.setColor({1,0,0,1});
    storage.reset();
    canvas->translate(250, 0);
    add_star(storage, 6);
    storage.transform(scale);
    canvas->drawContours(storage.contours(), storage.countContours(), paint);

    paint.setMiterLimit(0);
    paint.setColor({1,0,0,0});
    storage.reset();
    canvas->translate(-250, 250);
    GPoint* pts = storage.addContour(4, true);
    pts[0].set(-1, -1); pts[1].set(1, -1); pts[2].set(1, 1); pts[3].set(-1, 1);
    pts = storage.addContour(4, true);
    pts[0].set(0, -1); pts[1].set(-1, 0); pts[2].set(0, 1); pts[3].set(1, 0);
    pts = storage.addContour(4, true);
    pts[0].set(-0.5, -0.5); pts[1].set(0.5, -0.5); pts[2].set(0.5, 0.5); pts[3].set(-0.5, 0.5);
    pts = storage.addContour(4, true);
    pts[0].set(0, -0.5); pts[1].set(-0.5, 0); pts[2].set(0, 0.5); pts[3].set(0.5, 0);
    storage.transform(scale);
    canvas->drawContours(storage.contours(), storage.countContours(), paint);

    paint.setMiterLimit(4);
    paint.setColor({1,0,1,0});
    storage.reset();
    make_diamonds(storage);
    storage.transform(GMatrix::MakeScale(20));
    canvas->translate(250, 0);
    canvas->drawContours(storage.contours(), storage.countContours(), paint);
}

static void stars(GCanvas* canvas) {
    draw_stars(canvas, -1);
}

static void draw_external(GCanvas* canvas) {
    canvas->translate(130, 40);
    canvas->scale(1.2, 1.2);
#include "lion.inc"
}

static void draw_circle(GCanvas* canvas, float x, float y, float radius, const GPaint& paint) {
    const int N = 64;
    GPoint pts[N];
    const float da = 2 * M_PI / N;
    float angle = 0;
    for (int i = 0; i < N; ++i) {
        pts[i].fX = cosf(angle);
        pts[i].fY = sinf(angle);
        angle += da;
    }

    canvas->save();
    canvas->translate(x, y);
    canvas->scale(radius, radius);
    canvas->drawConvexPolygon(pts, N, paint);
    canvas->restore();
}

static void make_bitmap(const GBitmap& bm) {
    auto canvas = GCreateCanvas(bm);
    canvas->clear({0, 0, 0, 0});
    draw_circle(canvas.get(), bm.width()/2, bm.height()/2, bm.width()/2 - 1, GPaint({1, 0, 0, 0}));
}

static void draw_shader_alpha(GCanvas* canvas) {
    canvas->clear({ 1, 1, 1, 1 });

    const GPoint pts[] = {{ 0, 0 }, { 30, 30 }};
    const GColor c[] = {{ 1, 1, 0, 0 }, { 1, 0, 0, 1 }};

    GBitmap bm;
    bm.alloc(50, 50);
    make_bitmap(bm);
    auto shader0 = GCreateBitmapShader(bm, GMatrix(), GShader::kRepeat);
    auto shader1 = GCreateLinearGradient(pts[0], pts[1], c[0], c[1], GShader::kMirror);

    GPaint paint0, paint1;
    paint0.setShader(shader0.get());
    paint1.setShader(shader1.get());

    const GRect r1 = GRect::MakeXYWH(5, 105, 40, 80);

    canvas->translate(3, 3);
    for (int i = 1; i <= 10; ++i) {
        paint0.setAlpha(i * 1.0 / 10);
        canvas->drawRect(GRect::MakeWH(bm.width(), 2*bm.height()), paint0);
        paint1.setAlpha((11 - i) * 1.0 / 10);
        canvas->drawRect(r1, paint1);
        canvas->translate(bm.width(), 0);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static void stroke_stars(GCanvas* canvas) {
    draw_stars(canvas, 6);
}

static void draw_external2(GCanvas* canvas) {
    canvas->scale(.5, .5);
#include "tiger.inc"
}

#include "unc.inc"

static void draw_external3(GCanvas* canvas) {
    canvas->scale(.5, .5);
    draw_unc(canvas, nullptr, nullptr);
}

static void stroke_2(GCanvas* canvas, const GContour& ctr, const GColor& c0, const GColor& c1) {
    GPaint paint;
    paint.setColor(c0);
    paint.setStrokeWidth(50);
    canvas->drawContours(&ctr, 1, paint);
    paint.setColor(c1);
    paint.setStrokeWidth(20);
    paint.setMiterLimit(2);
    canvas->drawContours(&ctr, 1, paint);
}

static void stroke_tris(GCanvas* canvas) {
    GPoint pts0[] = { { 200, 20 }, { 128, 240 }, { 20, 100 } };
    stroke_2(canvas, {3, pts0, true}, {0.5, 0, 0, 0}, {1, 1, 0, 0});

    GPoint pts1[3];
    GMatrix m;
    m.setScale(-1, 1);
    m.postTranslate(128, -128);
    m.postRotate(M_PI/6);
    m.postTranslate(128, 128);
    m.mapPoints(pts1, pts0, 3);
    stroke_2(canvas, {3, pts1, true}, {0.5, 0, 1, 0}, {1, 0, 0, 1});
}

static void stroke_degen(GCanvas* canvas) {
    const float a = 40.1f,
                b = 256-a,
                c = (a + b)*0.5;
    
    GPoint pts0[] = { {a, a}, {b, b}, {a, a}, {b, b} };
    GPoint pts1[] = { {a, b}, {c, c}, {c, c}, {b, a} };

    GPaint p;
    GContour ctrs[] = {
        {GARRAY_COUNT(pts0), pts0, true},
        {GARRAY_COUNT(pts1), pts1, true},
    };

    p.setStrokeWidth(80);
    canvas->drawContours(ctrs, GARRAY_COUNT(ctrs), p);
    p.setStrokeWidth(40);
    p.setColor({0.85f, 1, 1, 1});
    canvas->drawContours(ctrs, GARRAY_COUNT(ctrs), p);
}

struct Spiro {
    float R, r, d;

    GPoint operator()(float t) const {
        return {
            (R - r)*cosf(t) + d*cosf((R - r)*t/r),
            (R - r)*sinf(t) - d*sinf((R - r)*t/r),
        };
    }
};

static void plot(GCanvas* canvas, const GPaint& paint, const Spiro& s) {
    const int N = 300;
    GPoint pts[N];
    for (int i = 0; i < N; ++i) {
        pts[i] = s(i * 2 * M_PI / N);
    }
    GContour ctr = {N, pts, true};
    canvas->drawContours(&ctr, 1, paint);
}

static void draw_spiro(GCanvas* canvas) {
    const float R = 5;
    GPaint p;
    p.setStrokeWidth(0.75f);
    canvas->translate(256, 256);
    canvas->scale(40, 40);
    plot(canvas, p, {5, 1, 2});
    p.setFill();
    p.setColor({1, 1, 0, 1});
    plot(canvas, p, {5, 1, 2});
}

///////////////////

const float twopi = (float)(2*M_PI);

static void circular_mesh(GCanvas* canvas, bool showColors, bool showTex) {
    auto shader = GCreateLinearGradient({0, 0}, {1,0}, {1, 0,0,0}, {1, 1,1,1});

    const int TRIS = 40;
    GPoint pts[TRIS + 1];
    GColor colors[TRIS + 1];
    GPoint tex[TRIS + 1];
    int indices[TRIS * 3];

    const float rad = 250;
    const float center = 256;
    pts[0] = { center, center };
    colors[0] = { 1, 0, 1, 0 };
    tex[0] = { 1, 1 };

    float angle = 0;
    float da = twopi / (TRIS - 1);
    int* idx = indices;
    for (int i = 1; i <= TRIS; ++i) {
        float x = cos(angle);
        float y = sin(angle);
        pts[i] = { x * rad + center, y * rad + center };
        colors[i] = { 1, angle / twopi, 0, (twopi - angle) / twopi };
        tex[i] = { angle / twopi, 0 };
        idx[0] = 0; idx[1] = i; idx[2] = i < TRIS ? i + 1 : 1;
        idx += 3;
        angle += da;
    }

    const GColor* colorPtr = showColors ? colors : nullptr;
    const GPoint* texPtr = showTex ? tex : nullptr;
    canvas->drawMesh(TRIS, pts, indices, colorPtr, texPtr, GPaint(shader.get()));
}

static void mesh_1(GCanvas* canvas) { circular_mesh(canvas, true, false); }
static void mesh_2(GCanvas* canvas) { circular_mesh(canvas, false, true); }
static void mesh_3(GCanvas* canvas) { circular_mesh(canvas, true, true); }

#include "GRandom.h"
static void perterb(GPoint pts[], int count) {
    GRandom rand;
    const float s = 5;

    for (int i = 0; i < count; ++i) {
        pts[i].fX += rand.nextRange(-s, s);
        pts[i].fY += rand.nextRange(-s, s);
    }
}

static void spock_quad(GCanvas* canvas) {
    GBitmap bitmap;
    bitmap.readFromFile("apps/spock.png");
    const float w = bitmap.width();
    const float h = bitmap.height();
    auto shader = GCreateBitmapShader(bitmap, GMatrix::MakeScale(1/w, 1/h), GShader::kMirror);

    GPoint outer[6], inner[6];
    for (int i = 0; i < 6; ++i) {
        float x = cosf(i * M_PI / 3);
        float y = sinf(i * M_PI / 3);
        outer[i] = { x * 250, y * 250 };
        inner[i] = { x *  50, y *  50 };
    }
    
    auto six = [](int index) { return index % 6; };
    
    canvas->translate(256, 256);
    for (int i = 0; i < 6; ++i) {
        GPoint pts[] = { outer[six(i)], outer[six(i+1)], inner[six(i+1)], inner[six(i)] };
        GPoint tex[] = {
            { i+0.0f, 0.0f }, { i+1.0f, 0.0f }, { i+1.0f, 1.0f }, { i+0.0f, 1.0f },
        };
        canvas->drawQuad(pts, nullptr, tex, 8, GPaint(shader.get()));
    }
}

static void color_quad(GCanvas* canvas) {
    const GColor r  = { 1, 1, 0, 0 };
    const GColor rg = { 1, 1, 1, 0 };
    const GColor g  = { 1, 0, 1, 0 };
    const GColor gb = { 1, 0, 1, 1 };
    const GColor b  = { 1, 0, 0, 1 };
    const GColor br = { 1, 1, 0, 1 };
    const GColor ring[] = { r, rg, g, gb, b, br };

    GPoint outer[6], inner[6];
    for (int i = 0; i < 6; ++i) {
        float x = cosf(i * M_PI / 3);
        float y = sinf(i * M_PI / 3);
        outer[i] = { x * 250, y * 250 };
        inner[i] = { x *  50, y *  50 };
    }

    auto six = [](int index) { return index % 6; };

    canvas->translate(256, 256);
    for (int i = 0; i < 6; ++i) {
        GPoint pts[] = { outer[six(i)], outer[six(i+1)], inner[six(i+1)], inner[six(i)] };
        GColor clr[] = { ring[six(i)], ring[six(i+1)], ring[six(i+4)], ring[six(i+3)] };
        canvas->drawQuad(pts, clr, nullptr, 8, GPaint());
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

const GDrawRec gDrawRecs[] = {
    { draw_solid_ramp,  256 * RAMP_W, 7*RAMP_H, "solid_ramp",       1 },
    { draw_blend_white, 200, 200,               "blend_white",      1 },
    { draw_blend_black, 200, 200,               "blend_black",      1 },

    { draw_spocks_quad, 300, 300,               "bitmap_quad",      2 },
    { draw_spocks_zoom, 300, 300,               "bitmap_zoom",      2 },
    { draw_bm_circles,  300, 300,               "circles_blend",    2 },
    { draw_circle_big,  400, 300,               "circles_fat",      2 },

    { draw_tri,         256, 256,               "tri",              3 },
    { draw_tri_clipped, 256, 256,               "tri_clipped",      3 },
    { draw_poly,        512, 512,               "poly",             3 },
    { draw_poly_center, 256, 256,               "poly_center",      3 },
    { draw_poly_rotate, 230, 230,               "poly_rotate",      3 },

    { draw_bitmap_wheel,    300, 300,           "spock_wheel",      4 },
    { draw_checker,         300, 300,           "checkerboard",     4 },
    { draw_trigger,         300, 300,           "trigshader",       4 },
    { draw_composite_draws, 300, 300,           "composite",        4 },
    { draw_poly_rotate2,    300, 300,           "color_clock",      4 },
    { draw_poly_rotate3,    300, 300,           "image_clock",      4 },

    { alpha_gradient,       256, 256,           "alpha_gradient",   5 },
    { bitmap_tilemode,      512, 512,           "bitmap_tiles",     5 },
    { gradient_tilemode,    512, 512,           "gradient_tiles",   5 },
    { stars,                512, 512,           "stars",            5 },
    { draw_external,        512, 512,           "lion",             5 },
    { draw_shader_alpha,    512, 200,           "shader_alpha",     5 },

    { stroke_stars,         512, 512,           "stroke_stars",     6 },
    { draw_external2,       512, 512,           "tiger",            6 },
    { draw_external3,       512, 512,           "unc",              6 },
    { stroke_tris,          256, 256,           "stroke_tris",      6 },
    { stroke_degen,         256, 256,           "stroke_degen",     6 },
    { draw_spiro,           512, 512,           "spiro",            6 },

    { mesh_1,               512, 512,           "sweep_mesh",       7 },
    { mesh_2,               512, 512,           "radial_mesh",      7 },
    { mesh_3,               512, 512,           "both_mesh",        7 },
    { spock_quad,           512, 512,           "spock_quad",       7 },
    { color_quad,           512, 512,           "color_quad",       7 },

    { NULL, 0, 0, NULL    },
};
