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

class myProxyShader : public GShader{
public:
    GShader* fShader;
    GMatrix  fMatrix;


    myProxyShader(GShader* shader, const GMatrix& matrix)
    : fShader(shader), fMatrix(matrix) {}
    bool isOpaque(){
        return fShader->isOpaque();
    }
    bool setContext(const GMatrix& ctm, float alpha) override {
        GMatrix tmpM;
        tmpM.setConcat(ctm, fMatrix);
        return fShader->setContext(tmpM, alpha);
    }
    
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        fShader->shadeRow(x, y, count, row);
    }
};
