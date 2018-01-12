#include <math.h>
#include "GPixel.h"
#include "GBitmap.h"
#include "GCanvas.h"
#include "GColor.h"
#include "GRect.h"
#include "GPoint.h"
#include "GMatrix.h"
#include "GShader.h"
#include <stdio.h>

class myColorShader : public GShader{
public :
    GPoint p0;
    GPoint p1;
    GPoint p2;
    GColor c0;
    GColor c1;
    GColor c2;
    
    float shaderAlpha;
    GMatrix pts;
    GMatrix finalM;
    
    float deltaXa = c1.fA - c0.fA;     //C1-C0
    float deltaXr = c1.fR - c0.fR;
    float deltaXg = c1.fG - c0.fG;
    float deltaXb = c1.fB - c0.fB;
    
    float deltaYa = c2.fA - c0.fA;      //C2-C0
    float deltaYr = c2.fR - c0.fR;
    float deltaYg = c2.fG - c0.fG;
    float deltaYb = c2.fB - c0.fB;
    // float shaderA;

    myColorShader(const GPoint& newp0, const GPoint& newp1, const GPoint& newp2,
                   const GColor& newc0, const GColor& newc1, const GColor& newc2)
    : p0(newp0), p1(newp1), p2(newp2), c0(newc0), c1(newc1), c2(newc2){
        float p1x = p1.fX - p0.fX;
        float p2x = p2.fX - p0.fX;
        float p1y = p1.fY - p0.fY;
        float p2y = p2.fY - p0.fY;
        pts.set6(p1x, p2x, p0.fX, p1y, p2y, p0.fY);
    }
    bool isOpaque(){
        if(shaderAlpha!=1){
            return false;
        }
        return true;
    }
    
    bool setContext(const GMatrix& newCTM, float newAlpha){
        shaderAlpha = newAlpha;
        
        GMatrix tmpM;
        tmpM.setConcat(newCTM, pts);
        return tmpM.invert(&finalM);
        
    }
    

    void shadeRow(int x, int y, int count, GPixel row[]){
        GPoint start = finalM.mapXY(x+0.5, y+0.5);
        float currX = start.fX;       
        float currY = start.fY;
        
        float dx = finalM[GMatrix::SX];
        float dy = finalM[GMatrix::KY];
        
        for (int i = 0; i < count; ++i){
            float a = currX * deltaXa + currY * deltaYa + c0.fA;
            float r = currX * deltaXr + currY * deltaYr + c0.fR;
            float g = currX * deltaXg + currY * deltaYg + c0.fG;
            float b = currX * deltaXb + currY * deltaYb + c0.fB;
            
            row[i] = blendWithAlpha(a,r,g,b);
            
            currX += dx;
            currY += dy;
            
        }
    }
    
    
    GPixel blendWithAlpha(float a, float r, float g, float b){
        int newA = GPinToUnit(a * shaderAlpha)*255;    //*255
        int newR = GPinToUnit(r * a * shaderAlpha)*255;
        int newG = GPinToUnit(g * a * shaderAlpha)*255;
        int newB = GPinToUnit(b * a * shaderAlpha)*255;
        
        GPixel thePixel = GPixel_PackARGB(newA, newR, newG, newB);
        return thePixel;
    }

};
