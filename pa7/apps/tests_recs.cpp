/**
 *  Copyright 2015 Mike Reed
 */

#include "GCanvas.h"
#include "GBitmap.h"
#include "GColor.h"
#include "GMatrix.h"
#include "GPoint.h"
#include "GRect.h"
#include "tests.h"

static void setup_bitmap(GBitmap* bitmap, int w, int h) {
    size_t rb = w << 2;
    bitmap->reset(w, h, rb, (GPixel*)calloc(h, rb), GBitmap::kNo_IsOpaque);
}

static void clear(const GBitmap& bitmap) {
    memset(bitmap.pixels(), 0, bitmap.rowBytes() * bitmap.height());
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static bool mem_eq(const void* ptr, int value, size_t size) {
    const char* cptr = (const char*)ptr;
    for (int i = 0; i < size; ++i) {
        if (cptr[i] != value) {
            return false;
        }
    }
    return true;
}

static bool bitmap_pix_eq(const GBitmap& bitmap, GPixel inside, GPixel outside) {
    const int lastX = bitmap.rowBytes() >> 2;
    const GPixel* row = bitmap.pixels();

    for (int y = 0; y < bitmap.height(); ++y) {
        for (int x = 0; x < bitmap.width(); ++x) {
            if (row[x] != inside) {
                return false;
            }
        }
        for (int x = bitmap.width(); x < lastX; ++x) {
            if (row[x] != outside) {
                return false;
            }
        }
        row += lastX;
    }
    return true;
}

static void test_clear(GTestStats* stats) {
    int w = 10, h = 10;
    size_t rb = (w + 11) * sizeof(GPixel);
    size_t size = rb * h;
    GBitmap bitmap(w, h, rb, (GPixel*)malloc(size), GBitmap::kNo_IsOpaque);

    const int wacky_component = 123;

    memset(bitmap.pixels(), wacky_component, size);
    auto canvas = GCreateCanvas(bitmap);

    // ensure that creating the canvas didn't change any pixels
    stats->expectTrue(mem_eq(bitmap.pixels(), wacky_component, size), "clear 0");

    const GPixel wacky_pixel = GPixel_PackARGB(wacky_component, wacky_component,
                                               wacky_component, wacky_component);

    canvas->clear(GColor::MakeARGB(0, 1, 1, 1));
    stats->expectTrue(bitmap_pix_eq(bitmap, 0, wacky_pixel), "clear 1");

    canvas->clear(GColor::MakeARGB(1, 1, 1, 1));
    stats->expectTrue(bitmap_pix_eq(bitmap, 0xFFFFFFFF, wacky_pixel), "clear 2");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static bool check9(const GBitmap& bitmap, const GPixel expected[9]) {
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            if (*bitmap.getAddr(x, y) != expected[y * 3 + x]) {
                return false;
            }
        }
    }
    return true;
}

static void test_rect_colors(GTestStats* stats) {
    const GPixel pred = GPixel_PackARGB(0xFF, 0xFF, 0, 0);
    const GColor cred = GColor::MakeARGB(1, 1, 0, 0);

    GBitmap bitmap;
    setup_bitmap(&bitmap, 3, 3);
    auto canvas = GCreateCanvas(bitmap);

    GPixel nine[9] = { 0, 0, 0, 0, pred, 0, 0, 0, 0 };
    canvas->fillRect(GRect::MakeLTRB(1, 1, 2, 2), cred);
    stats->expectTrue(check9(bitmap, nine), "rect 0");

    nine[4] = 0;
    clear(bitmap);
    // don't expect these to draw anything
    const GRect rects[] = {
        GRect::MakeLTRB(-10, 0, 0.25f, 10),
        GRect::MakeLTRB(0, -10, 10, 0.25f),
        GRect::MakeLTRB(2.51f, 0, 10, 10),
        GRect::MakeLTRB(0, 2.51, 10, 10),

        GRect::MakeLTRB(1, 1, 1, 1),
        GRect::MakeLTRB(1.51f, 0, 2.49f, 3),
    };
    for (int i = 0; i < GARRAY_COUNT(rects); ++i) {
        canvas->fillRect(rects[i], cred);
        stats->expectTrue(check9(bitmap, nine), "rect 1");
    }

    // vertical stripe down center
    nine[1] = nine[4] = nine[7] = pred;
    canvas->fillRect(GRect::MakeLTRB(0.6f, -3, 2.3f, 2.6f), cred);
    stats->expectTrue(check9(bitmap, nine), "rect 2");

    clear(bitmap);
    memset(nine, 0, sizeof(nine));
    // don't expect anything to draw
    const GColor colors[] = {
        GColor::MakeARGB(0, 1, 0, 0),
        GColor::MakeARGB(-1, 1, 0, 0),
        GColor::MakeARGB(0.00001f, 1, 0, 0),
    };
    for (int i = 0; i < GARRAY_COUNT(colors); ++i) {
        canvas->fillRect(GRect::MakeWH(3, 3), colors[i]);
        stats->expectTrue(check9(bitmap, nine), "rect 3");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static void test_hori_bitmap(GTestStats* stats) {
    GPixel srcStorage[4] = {
        GPixel_PackARGB(0xFF, 0xFF, 0, 0),
        GPixel_PackARGB(0xFF, 0, 0xFF, 0),
        GPixel_PackARGB(0xFF, 0, 0, 0xFF),
        GPixel_PackARGB(0xFF, 0xFF, 0xFF, 0xFF),
    };
    GBitmap src(4, 1, 4 * sizeof(GPixel), srcStorage, GBitmap::kYes_IsOpaque);

    GPixel dstStorage[16];
    GBitmap dst(4, 4, 4 * sizeof(GPixel), dstStorage, GBitmap::kNo_IsOpaque);

    GCreateCanvas(dst)->fillBitmapRect(src, GRect::MakeWH(4, 4));
    
    for (int y = 0; y < dst.height(); ++y) {
        stats->expectEQ(memcmp(srcStorage, dst.getAddr(0, y), src.rowBytes()), 0, "hori_bitmap");
    }
}

static void test_vert_bitmap(GTestStats* stats) {
    GPixel srcStorage[4] = {
        GPixel_PackARGB(0xFF, 0xFF, 0, 0),
        GPixel_PackARGB(0xFF, 0, 0xFF, 0),
        GPixel_PackARGB(0xFF, 0, 0, 0xFF),
        GPixel_PackARGB(0xFF, 0xFF, 0xFF, 0xFF),
    };
    GBitmap src(1, 4, sizeof(GPixel), srcStorage, GBitmap::kYes_IsOpaque);

    GPixel dstStorage[16];
    GBitmap dst(4, 4, 4 * sizeof(GPixel), dstStorage, GBitmap::kNo_IsOpaque);

    GCreateCanvas(dst)->fillBitmapRect(src, GRect::MakeWH(4, 4));
    
    for (int y = 0; y < dst.height(); ++y) {
        for (int x = 0; x < dst.width(); ++x) {
            stats->expectEQ(*dst.getAddr(x, y), *src.getAddr(0, y), "vert_bitmap");
        }
    }
}

static void test_shrink_bitmap(GTestStats* stats) {
    GPixel srcStorage[9];
    for (int i = 0; i < 9; ++i) {
        srcStorage[i] = GPixel_PackARGB(0xFF, 0xFF, 0, 0);  // red
    }
    srcStorage[4] = GPixel_PackARGB(0xFF, 0, 0xFF, 0);  // green center
    
    GBitmap src(3, 3, 3 * sizeof(GPixel), srcStorage, GBitmap::kYes_IsOpaque);

    GPixel dstStorage[9];
    GBitmap dst(3, 3, 3 * sizeof(GPixel), dstStorage, GBitmap::kNo_IsOpaque);
    memset(dst.pixels(), 0, sizeof(dstStorage));
    
    GCreateCanvas(dst)->fillBitmapRect(src, GRect::MakeXYWH(1, 1, 1, 1));
    // expect dst to still be zeros, exept its center, which should be green since we shrank down
    // src from 3x3 to 1x1 and placed it at dst's center.
    
    for (int y = 0; y < dst.height(); ++y) {
        for (int x = 0; x < dst.width(); ++x) {
            GPixel expected = 0;
            if (1 == x && 1 == y) {
                expected = GPixel_PackARGB(0xFF, 0, 0xFF, 0);
            }
            stats->expectEQ(*dst.getAddr(x, y), expected, "shrink_bitmap");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static bool is_filled_with(const GBitmap& bitmap, GPixel expected) {
    for (int y = 0; y < bitmap.height(); ++y) {
        for (int x = 0; x < bitmap.width(); ++x) {
            if (*bitmap.getAddr(x, y) != expected) {
                return false;
            }
        }
    }
    return true;
}

class GSurface {
public:
    GSurface(int width, int height) {
        fBitmap.alloc(width, height);
        fCanvas = GCreateCanvas(fBitmap);
    }
    ~GSurface() {
        free(fBitmap.pixels());
    }

    GCanvas* canvas() const { return fCanvas.get(); }
    const GBitmap& bitmap() const { return fBitmap; }

private:
    std::unique_ptr<GCanvas>    fCanvas;
    GBitmap                     fBitmap;
};

static void test_bad_input_poly(GTestStats* stats) {
    GSurface surface(10, 10);
    GCanvas* canvas = surface.canvas();

    canvas->clear(GColor::MakeARGB(1, 1, 1, 1));
    const GPixel white = GPixel_PackARGB(0xFF, 0xFF, 0xFF, 0xFF);
    stats->expectTrue(is_filled_with(surface.bitmap(), white), "poly_invalid_clear");

    const GColor color = GColor::MakeARGB(1, 0, 0, 0);  // black
    const GPixel black = GPixel_PackARGB(0xFF, 0, 0, 0);

    // inside the top/left corner
    const GPoint pts[] = {
        GPoint::Make(0, 0), GPoint::Make(5, 10), GPoint::Make(10, 5)
    };

    // Now draw some polygons that shouldn't actually draw any pixels
    const struct {
        int fCount;
        const char* fMsg;
    } recs[] = {
        { -1, "bad_ploly_count_-1" },
        {  0, "bad_ploly_count_0" },
        {  1, "bad_ploly_count_1" },
        {  2, "bad_ploly_count_2" },
    };
    for (int i = 0; i < GARRAY_COUNT(recs); ++i) {
        canvas->fillConvexPolygon(pts, recs[i].fCount, color);
        stats->expectTrue(is_filled_with(surface.bitmap(), white), recs[i].fMsg);
    }
}

static void test_offscreen_poly(GTestStats* stats) {
    GSurface surface(10, 10);
    GCanvas* canvas = surface.canvas();

    canvas->clear(GColor::MakeARGB(1, 1, 1, 1));
    const GPixel white = GPixel_PackARGB(0xFF, 0xFF, 0xFF, 0xFF);
    stats->expectTrue(is_filled_with(surface.bitmap(), white), "poly_offscreen_clear");

    const GColor color = GColor::MakeARGB(1, 0, 0, 0);  // black
    const GPixel black = GPixel_PackARGB(0xFF, 0, 0, 0);

    // draw a valid polygon, but it is "offscreen", so nothing should get drawn

    // triange up and to the left of the top/left corner
    const GPoint pts[] = {
        GPoint::Make(-10, -10), GPoint::Make(5, -10), GPoint::Make(-10, 5)
    };
    canvas->fillConvexPolygon(pts, 3, color);
    stats->expectTrue(is_filled_with(surface.bitmap(), white), "poly_offscreen");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static bool ie_eq(float a, float b) {
    return fabs(a - b) <= 0.00001f;
}

static bool is_eq(const GMatrix& m, float a, float b, float c, float d, float e, float f) {
    //    printf("%g %g %g    %g %g %g\n", m[0], m[1], m[2], m[3], m[4], m[5]);
    return ie_eq(m[0], a) && ie_eq(m[1], b) && ie_eq(m[2], c) &&
    ie_eq(m[3], d) && ie_eq(m[4], e) && ie_eq(m[5], f);
}

static void test_matrix(GTestStats* stats) {
    GMatrix m;
    stats->expectTrue(is_eq(m, 1, 0, 0, 0, 1, 0), "matrix_identity");
    m.setTranslate(2.5, -4);
    stats->expectTrue(is_eq(m, 1, 0, 2.5, 0, 1, -4), "matrix_setTranslate");
    m.setScale(2.5, -4);
    stats->expectTrue(is_eq(m, 2.5, 0, 0, 0, -4, 0), "matrix_setScale");
    m.setRotate(M_PI);
    stats->expectTrue(is_eq(m, -1, 0, 0, 0, -1, 0), "matrix_setRotate");

    GMatrix m2, m3;
    m.setScale(2, 3);
    m2.setScale(-1, -2);
    m3.setConcat(m, m2);
    stats->expectTrue(is_eq(m3, -2, 0, 0, 0, -6, 0), "matrix_setConcat0");
    m2.setTranslate(5, 6);
    m3.setConcat(m2, m);
    stats->expectTrue(is_eq(m3, 2, 0, 5, 0, 3, 6), "matrix_setConcat1");

    m2.setScale(2, 3);
    m3.setRotate(M_PI/6);
    m.setConcat(m2, m3);
    m2.preConcat(m3);
    stats->expectTrue(m2 == m, "matrix_concat_check_for_aliasing");

    GPoint p0[] = { {0, 0}, {2, 3}, {10, 20}, {-20, -10} };
    GPoint p1[4];
    m3.mapPoints(p1, p0, 4);
    m3.mapPoints(p0, p0, 4);
    stats->expectTrue(!memcmp(p0, p1, sizeof(p0)), "matrix_mapPoints_check_for_aliasing");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#include "GShader.h"

static void expect_opaque(GTestStats* stats, std::unique_ptr<GShader> shader, bool isOpaque) {
    const GMatrix ctm;
    // set an opaque context
    stats->expectTrue(shader->setContext(ctm, 1), "shader_opaque0");
    stats->expectTrue(shader->isOpaque() == isOpaque, "shader_opaque1");
    // now set a non-opaque context
    stats->expectTrue(shader->setContext(ctm, 0.5), "shader_opaque2");
    stats->expectTrue(shader->isOpaque() == false, "shader_opaque3");
}

static void test_shaders(GTestStats* stats) {
    GPixel p0[] = { GPixel_PackARGB(0xFF, 0, 0, 0), GPixel_PackARGB(0xFF, 0xFF, 0xFF, 0xFF) };
    GPixel p1[] = { GPixel_PackARGB(0   , 0, 0, 0), GPixel_PackARGB(0xFF, 0xFF, 0xFF, 0xFF) };

    GBitmap bm;
    
    bm.reset(2, 1, 2 * sizeof(GPixel), p0, GBitmap::kYes_IsOpaque);
    expect_opaque(stats, GCreateBitmapShader(bm, GMatrix()), true);

    bm.reset(2, 1, 2 * sizeof(GPixel), p1, GBitmap::kNo_IsOpaque);
    expect_opaque(stats, GCreateBitmapShader(bm, GMatrix()), false);

    expect_opaque(stats, GCreateLinearGradient({0,0},{1,1}, {1,1,1,1}, {1,0,0,0}), true);
    expect_opaque(stats, GCreateLinearGradient({0,0},{1,1}, {1,1,1,1}, {0,0,0,0}), false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

const GTestRec gTestRecs[] = {
    { test_clear,       "clear"         },
    { test_rect_colors, "rect_colors"   },

    { test_hori_bitmap, "hori_bitmap" },
    { test_vert_bitmap, "vert_bitmap" },
    { test_shrink_bitmap, "shrink_bitmap" },

    { test_bad_input_poly, "poly_bad_input" },
    { test_offscreen_poly, "poly_offscreen" },

    { test_matrix,      "matrix",   },

    { test_shaders,     "shaders",  },

    { nullptr, nullptr },
};

bool gTestSuite_Verbose;
bool gTestSuite_CrashOnFailure;
