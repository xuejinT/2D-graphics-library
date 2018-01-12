#include <math.h>
#include "GPixel.h"
#include "GBitmap.h"
#include "GCanvas.h"
#include "GColor.h"
#include "GRect.h"
#include "GPoint.h"
#include "GMatrix.h"
#include "GShader.h"
#include "myColorShader.cpp"
#include "myProxyShader.cpp"
#include <stdio.h>


class myComposeShader : public GShader {
    
public:
    //GShader* fS0, *fS1;
    myColorShader* fS0;
    myProxyShader* fS1;
    
    myComposeShader(myColorShader* s0, myProxyShader* s1) : fS0(s0), fS1(s1) {}
    
    bool setContext(const GMatrix& ctm, float alpha) override {
        return fS0->setContext(ctm, alpha) &&
        fS1->setContext(ctm, alpha);
    }
    bool isOpaque(){
            return fS0->isOpaque() && fS1->isOpaque();
    }
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        // call shadeRow on fS0 and fS1, but into separate buffers
        // and then “compose” them together into row[].
        // A classic composition rule is just multiplication:
        // a0 * a1, r0 * r1, g0 * g1, b0 * b1
        GPixel row1[count];
        GPixel row2[count];
        fS0 -> shadeRow(x, y, count, row1);
        fS1 -> shadeRow(x, y, count, row2);

        for (int i = 0; i < count; ++i){
            float a = (GPixel_GetA(row1[i])/255.0) * (GPixel_GetA(row2[i])/255.0);
            float r = (GPixel_GetR(row1[i])/255.0) * (GPixel_GetR(row2[i])/255.0);
            float g = (GPixel_GetG(row1[i])/255.0) * (GPixel_GetG(row2[i])/255.0);
            float b = (GPixel_GetB(row1[i])/255.0) * (GPixel_GetB(row2[i])/255.0);

            row[i] = blendWithAlpha(a,r,g,b);
        }
    }

    GPixel blendWithAlpha(float a, float r, float g, float b){
        int newA = GPinToUnit(a)*255;
        int newR = GPinToUnit(r)*255;
        int newG = GPinToUnit(g)*255;
        int newB = GPinToUnit(b)*255;
        
        GPixel thePixel = GPixel_PackARGB(newA, newR, newG, newB);
        return thePixel;
    }



};
