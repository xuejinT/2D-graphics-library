/*
 *  Copyright 2015 Mike Reed
 */

#ifndef GCanvas_DEFINED
#define GCanvas_DEFINED

#include "GContour.h"
#include "GPaint.h"

class GBitmap;
class GColor;
class GMatrix;
class GPoint;
class GRect;
class GShader;

class GCanvas {
public:
    virtual ~GCanvas() {}

    /**
     *  Saves a copy of the CTM, allowing subsequent modifications (by calling concat()) to be
     *  undone when restore() is called.
     *
     *  e.g.
     *  // CTM is in state A
     *  canvas->save();
     *  canvas->conact(...);
     *  canvas->conact(...);
     *  // Now the CTM has been modified by the calls to concat()
     *  canvas->restore();
     *  // Now the CTM is again in state A
     */
    virtual void save() = 0;
    
    /**
     *  Balances calls to save(), returning the CTM to the state it was in when the corresponding
     *  call to save() was made. These calls can be nested.
     *
     *  e.g.
     *  save()
     *      concat()
     *      concat()
     *      save()
     *          concat()
     *          ...
     *      restore()
     *      ...
     *  restore()
     */
    virtual void restore() = 0;
    
    /**
     *  Modifies the CTM (current transformation matrix) by pre-concatenating it with the specfied
     *  matrix.
     *
     *  CTM' = CTM * matrix
     *
     *  The result is that any drawing that uses the new CTM' will be affected AS-IF it were
     *  first transformed by matrix, and then transformed by the previous CTM.
     */
    virtual void concat(const GMatrix&) = 0;
    
    /**
     *  Pretranslates the CTM by the specified tx, ty
     */
    void translate(float tx, float ty);
    
    /**
     *  Prescales the CTM by the specified sx, sy
     */
    void scale(float sx, float sy);
    
    /**
     *  Prerotates the CTM by the specified angle in radians.
     */
    void rotate(float radians);

    //////////

    /**
     *  Fill the entire canvas with the specified color.
     *
     *  This completely overwrites any previous colors, it does not blend.
     */
    virtual void clear(const GColor&) = 0;
    
    /**
     *  Scale and translate the bitmap such that it fills the specific rectangle.
     *
     *  Any area in the rectangle that is outside of the bounds of the canvas is ignored.
     *
     *  Draws using SRCOVER blend mode.
     */
    virtual void fillBitmapRect(const GBitmap& src, const GRect& dst) = 0;

    /**
     *  Fill the rectangle with the color.
     *
     *  The affected pixels are those whose centers are "contained" inside the rectangle:
     *      e.g. contained == center > min_edge && center <= max_edge
     *
     *  Any area in the rectangle that is outside of the bounds of the canvas is ignored.
     *
     *  Draws using SRCOVER blend mode.
     */
    virtual void drawRect(const GRect&, const GPaint&) = 0;
    
    /**
     *  Fill the convex polygon with the color, following the same "containment" rule as
     *  rectangles.
     *
     *  Any area in the polygon that is outside of the bounds of the canvas is ignored.
     *
     *  Draws using SRCOVER blend mode.
     */
    virtual void drawConvexPolygon(const GPoint[], int count, const GPaint&) = 0;

    void fillRect(const GRect& r, const GColor& c) {
        this->drawRect(r, GPaint(c));
    }

    void fillConvexPolygon(const GPoint pts[], int count, const GColor& c) {
        this->drawConvexPolygon(pts, count, GPaint(c));
    }

    /**
     *  Each contour need not be convex.
     */
    virtual void drawContours(const GContour ctrs[], int count, const GPaint&) = 0;

    void drawPolygon(const GPoint pts[], int count, const GPaint& paint) {
        const GContour ctr { count, pts };
        this->drawContours(&ctr, 1, paint);
    }

    /**
     *  Draw a mesh of triangles, each with optional colors and/or text-coordinates at each
     *  vertex.
     *
     *  The triangles are specified in one of two ways:
     *  - If indices is null, then each triangle is 3 consecutive points from pts[].
     *      { pts[0], pts[1], pts[2] }
     *  - Else each triangle is formed by references points from 3 consecutive indices...
     *      { pts[indices[0]], pts[indices[1]], pts[indices[2]] }
     *
     *  If colors is not null, then each vertex has an associated color, to be interpolated
     *  across the triangle. The colors are referenced in the same way as the pts, either
     *  sequentially or via indices[].
     *
     *  If tex is not null, then each vertex has an associated texture coordinate, to be used
     *  to specify a coordinate in the paint's shader's space. If there is no shader on the
     *  paint, then tex[] should be ignored. It is referenced in the same way as pts and colors,
     *  either sequentially or via indices[].
     *
     *  If both colors and tex[] are specified, then at each pixel their values are multiplied
     *  together, component by component.
     */
    virtual void drawMesh(int triCount, const GPoint pts[], const int indices[],
                          const GColor colors[], const GPoint tex[], const GPaint&) = 0;

    /**
     *  Draw the quad, with optional color and/or texture coordinate at each corner. Tesselate
     *  the quad based on "level":
     *      level == 0 --> 1 quad  -->  2 triangles
     *      level == 1 --> 4 quads -->  8 triangles
     *      level == 2 --> 9 quads --> 18 triangles
     *      ...
     *  The 4 corners of the quad are specified in this order:
     *      top-left --> top-right --> bottom-right --> bottom-left
     *  Each quad is triangulated on the diagonal top-right --> bottom-left
     *      0---1
     *      |  /|
     *      | / |
     *      |/  |
     *      3---2
     *
     *  If colors is null, then this just uses the paint's color at each corner.
     *  If tex is null, then the paint's shader is ignored
     *  If both are specified, then their color values are multiplied together.
     */
    virtual void drawQuad(const GPoint pts[4], const GColor colors[4], const GPoint tex[4],
                          int level, const GPaint&) = 0;
};

/**
 *  If the bitmap is valid for drawing into, this returns a subclass that can perform the
 *  drawing. If bitmap is invalid, this returns NULL.
 */
std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& bitmap);

#endif
