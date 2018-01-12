#include "GMatrix.h"
#include "GBitmap.h"
#include "GShader.h"
#include "GPixel.h"
#include "GCanvas.h"
#include "GColor.h"
#include <math.h>
#include "GPoint.h"
#include "GRect.h"
#include <stdio.h>
#include <cstdint>

class LinearGradientShader: public GShader{
public:
    const GPoint p0;
    const GPoint p1;
    const GColor c0;
    const GColor c1;//
    const GShader::TileMode tMode;
    float dcA;
    float dcR;
    float dcG;
    float dcB;
    GMatrix localM;
    GMatrix finalM;
    float shaderA;

    LinearGradientShader(const GPoint& new_p0, const GPoint& new_p1
    ,const GColor& new_c0, const GColor& new_c1,GShader::TileMode tMode)
    :p0(new_p0),p1(new_p1),c0(new_c0),c1(new_c1),tMode(tMode){
        float dx = p1.x() - p0.x();
        float dy = p1.y() - p0.y();
        localM = GMatrix(dx,-dy,p0.x(),dy,dx,p0.y());
    }


    bool isOpaque(){
        if(shaderA!=1){
            return false;
        }
        return true;
    }

    bool setContext(const GMatrix& ctm, float alpha){
        shaderA= alpha;
        GMatrix tmp;
        tmp.setConcat(ctm,localM);
        return tmp.invert(&finalM);
    }

    GPixel alphaBlend(float a,float r,float g,float b){

        int newA= (uint8_t)(GPinToUnit(a*shaderA)*255);
        int newR= (uint8_t)(GPinToUnit(r*a*shaderA)*255);
        int newG= (uint8_t)(GPinToUnit(g*a*shaderA)*255);
        int newB= (uint8_t)(GPinToUnit(b*a*shaderA)*255);
        return GPixel_PackARGB(newA, newR, newG, newB);
    }

    void shadeRow(int x, int y, int count, GPixel row[]){
        if(tMode == GShader::TileMode::kClamp){
            clamp(x,y,count,row);
        }else if(tMode == GShader::TileMode::kRepeat){
            repeat(x,y,count,row);
        }else{
            mirror(x,y,count,row);
        }
    }

    void clamp(int x, int y, int count, GPixel row[]){
        GPoint startPt = finalM.mapXY(x+0.5,y+0.5);
        float w = startPt.x();
        dcA=(c1.fA-c0.fA)*finalM[GMatrix::SX];
        dcR=(c1.fR-c0.fR)*finalM[GMatrix::SX];
        dcG=(c1.fG-c0.fG)*finalM[GMatrix::SX];
        dcB=(c1.fB-c0.fB)*finalM[GMatrix::SX];
        float a = w * c1.fA + (1-w)*c0.fA;
        float r = w * c1.fR + (1-w)*c0.fR;
        float g = w * c1.fG + (1-w)*c0.fG;
        float b = w * c1.fB + (1-w)*c0.fB;
        if(w<0){
            row[0] = alphaBlend(c0.fA,c0.fR,c0.fG,c0.fB);
            w += finalM[GMatrix::SX];
            }
        else if(w>0&&w<1){
            row[0] = alphaBlend(a,r,g,b);
        }
        else{
            row[0] = alphaBlend(c1.fA,c1.fR,c1.fG,c1.fB);
            w += finalM[GMatrix::SX];
        }
        for (int i = 1; i < count; i++){
            if(w<0){
                row[i] = alphaBlend(c0.fA,c0.fR,c0.fG,c0.fB);
                w += finalM[GMatrix::SX];
            }
            else if(w>0&&w<1){
                a += dcA;
                r += dcR;
                g += dcG;
                b += dcB;
                row[i] = alphaBlend(a,r,g,b);
                w+=finalM[GMatrix::SX];
                
            }
            else{
                row[i] = alphaBlend(c1.fA,c1.fR,c1.fG,c1.fB);
                w+=finalM[GMatrix::SX];
            }
        }
    }

    void repeat(int x, int y, int count, GPixel row[]){
        GPoint startPt = finalM.mapXY(x+0.5,y+0.5);
        float w = startPt.x();
        for(int i = 0; i<count;i++){
            w = w - floor(w);
            float newA= w*c1.fA+ c0.fA*(1-w);
            float newR= w*c1.fR+ c0.fR*(1-w);
            float newG= w*c1.fG+ c0.fG*(1-w);
            float newB= w*c1.fB+ c0.fB*(1-w);
            row[i]= alphaBlend(newA,newR,newG,newB);
            w += finalM[GMatrix::SX];
        }
    }

     void mirror(int x, int y, int count, GPixel row[]){
        GPoint startPt = finalM.mapXY(x+0.5,y+0.5);
        float w = startPt.x();
        for(int i = 0; i<count;i++){
            float curW = w*0.5;
            curW -= floorf(curW);
            curW *=2;
            if(curW>=1){
                curW = 2-curW;
            }
            float newA= curW*c1.fA+ (1-curW)*c0.fA;
            float newR= curW*c1.fR+ (1-curW)*c0.fR;
            float newG= curW*c1.fG+ (1-curW)*c0.fG;
            float newB= curW*c1.fB+ (1-curW)*c0.fB;
            row[i] = alphaBlend(newA,newR,newG,newB);
            w += finalM[GMatrix::SX];
        }
    }

    
};

std::unique_ptr<GShader> GCreateLinearGradient(const GPoint& p0, const GPoint& p1,
                                                   const GColor& c0, const GColor& c1,
                                                   GShader::TileMode newTM){
    return std::unique_ptr<GShader>(new LinearGradientShader(p0, p1, c0, c1, newTM));
};