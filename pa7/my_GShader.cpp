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


class BitmapShader: public GShader{
public:
    float shaderA;
    GBitmap shaderBM;
    GMatrix localM;
    //GMatrix theCTM;
    GMatrix shaderM;
    const GMatrix internalM;
    GMatrix finalM;
    GMatrix invertSM;
    const int srcW;
    const int srcH;
    const GShader::TileMode tMode;


    
    BitmapShader(const GBitmap& newBM, const GMatrix& internalMatrix, GShader::TileMode newTMode)
    :internalM(internalMatrix),shaderBM(newBM),tMode(newTMode)
    ,srcW(newBM.width()),srcH(newBM.height()){
        localM = GMatrix(srcW,0,0,0,srcH,0);;
        //shaderBM = newBM;
    }

    bool isOpaque(){
        if(shaderA!=1){
        	return false;
        }
        return true;
    }
    
    bool setContext(const GMatrix& ctm, float alpha){
    	GMatrix tmp;

        // theCTM = ctm;
        // // printf("IN:theCTM---" );
         shaderA = alpha;
        // // printf("%f\n",shaderA );
         shaderM.setConcat(ctm, internalM);
         tmp.setConcat(shaderM,localM);
         shaderM.invert(&invertSM);
         return tmp.invert(&finalM);
        
        // return true;
    }

    GPixel alphaBlend(GPixel& p){
        int newA= (uint8_t)(GPixel_GetA(p)*shaderA);
        int newR= (uint8_t)(GPixel_GetR(p)*shaderA);
        int newG= (uint8_t)(GPixel_GetG(p)*shaderA);
        int newB= (uint8_t)(GPixel_GetB(p)*shaderA);
        return GPixel_PackARGB(newA, newR, newG, newB);
    }

    void shadeRow(int x, int y, int count, GPixel row[]){        
        if(tMode== GShader::TileMode::kClamp){
        	 clamp(x, y, count, row);
        }else if(tMode== GShader::TileMode::kRepeat){
        	 repeat(x, y, count, row);
        }else{
        	 mirror(x, y, count, row);
        }
    }   

    void clamp(int x, int y, int count, GPixel row[]){
    	GPoint mappedPt = invertSM.mapXY(x+0.5,y+0.5);
        float dx= invertSM[GMatrix::SX];
        float dy= invertSM[GMatrix::KY];
        for(int i= 0; i<count; i++){
            int currX= fmin(shaderBM.width()-1,fmax(0,(int)(mappedPt.fX)));
            int currY= fmin(shaderBM.height()-1,fmax(0,(int)(mappedPt.fY)));
            GPixel srcPx= *shaderBM.getAddr(currX, currY);
            row[i]= alphaBlend(srcPx);
            mappedPt.fX+= dx;
            mappedPt.fY+= dy;
        }
    }

    void repeat(int x, int y, int count, GPixel row[]){
    	//GPoint mappedP= finalM.mapXY(x+0.5,y+0.5);
        float mappedX= finalM.mapXY(x+0.5,y+0.5).x();
        float mappedY= finalM.mapXY(x+0.5,y+0.5).y();
        float dx= finalM[GMatrix::SX];
        float dy= finalM[GMatrix::KY];
        for(int i= 0; i<count ; i++){
            mappedX= mappedX - floor(mappedX);
            mappedY= mappedY - floor(mappedY);
            int currX= fmin(shaderBM.width()-1, fmax(0,(int)(mappedX * srcW)));
            int currY= fmin(shaderBM.height()-1, fmax(0,(int)(mappedY * srcH)));
            GPixel srcPx= *shaderBM.getAddr(currX, currY);
            row[i]= alphaBlend(srcPx);
            mappedX+= dx;
            mappedY+= dy;
        }     
    }

    void mirror(int x, int y, int count, GPixel row[]){
    	//GPoint loc = final_matrix.mapXY(x+0.5,y+0.5);
        float mappedX= finalM.mapXY(x+0.5,y+0.5).x();
        float mappedY= finalM.mapXY(x+0.5,y+0.5).y();
        float dx= finalM[GMatrix::SX];
        float dy= finalM[GMatrix::KY];
        float currMX;
        float currMY;
        for(int i= 0; i<count ; i++){
            currMX= mappedX;
            currMX*= 0.5;
            currMX-= floorf(currMX);
            currMX*= 2.0;
            if(currMX>=1){
                currMX= 2.0-currMX;
            }
            currMY= mappedY;
            currMY*= 0.5;
            currMY-= floorf(currMY);
            currMY*= 2.0;
            if(currMY>=1){
                currMY= 2.0-currMY;
            }
            int currX =  fmin(shaderBM.width()-1, fmax(0,(int)(currMX * srcW)));
            int currY =  fmin(shaderBM.height()-1, fmax(0,(int)(currMY * srcH)));
            GPixel srcPx = *shaderBM.getAddr(currX, currY);
            row[i] = alphaBlend(srcPx);
            mappedX+= dx;
            mappedY+= dy;
        }     
    }



};

std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& newBM, const GMatrix& localMatrix,GShader::TileMode newTM){
	return std::unique_ptr<GShader>(new BitmapShader(newBM, localMatrix, newTM));
};

