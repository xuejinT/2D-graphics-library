#include "GMatrix.h"
#include "cmath"
#include <stdio.h>
    /**
     *  Set this matrix to identity.
     */
    void GMatrix::setIdentity(){
        fMat[GMatrix::SX]=1.0;
        fMat[GMatrix::KX]=0.0;
        fMat[GMatrix::TX]=0.0;
        fMat[GMatrix::KY]=0.0;
        fMat[GMatrix::SY]=1.0;
        fMat[GMatrix::TY]=0.0;
    }

    /**
     *  Set this matrix to translate by the specified amounts.
     */
    void GMatrix::setTranslate(float tx, float ty){
        fMat[GMatrix::SX]=1.0;
        fMat[GMatrix::KX]=0.0;
        fMat[GMatrix::TX]=tx;
        fMat[GMatrix::KY]=0.0;
        fMat[GMatrix::SY]=1.0;
        fMat[GMatrix::TY]=ty;
    }

    /**
     *  Set this matrix to scale by the specified amounts.
     */
    void GMatrix::setScale(float sx, float sy){
        fMat[GMatrix::SX]=sx;
        fMat[GMatrix::KX]=0;
        fMat[GMatrix::TX]=0;
        fMat[GMatrix::KY]=0;
        fMat[GMatrix::SY]=sy;
        fMat[GMatrix::TY]=0;
    }

    /**
     *  Set this matrix to rotate by the specified radians.
     *
     *  Note: since positive-Y goes down, a small angle of rotation will increase Y.
     */
    void GMatrix::setRotate(float radians){
        fMat[GMatrix::SX]=cos(radians);
        fMat[GMatrix::KX]=-sin(radians);
        fMat[GMatrix::TX]=0;
        fMat[GMatrix::KY]=sin(radians);
        fMat[GMatrix::SY]=cos(radians);
        fMat[GMatrix::TY]=0;
    }

    /**
     *  Set this matrix to the concatenation of the two specified matrices, such that the resulting
     *  matrix, when applied to points will have the same effect as first applying the primo matrix
     *  to the points, and then applying the secundo matrix to the resulting points.
     *
     *  Pts' = Secundo * Primo * Pts
     */
    void GMatrix::setConcat(const GMatrix& secundo, const GMatrix& primo){
    	float sx,kx,tx,ky,sy,ty;
		sx=secundo[GMatrix::SX]*primo[GMatrix::SX]+secundo[GMatrix::KX]*primo[GMatrix::KY];
		kx=secundo[GMatrix::SX]*primo[GMatrix::KX]+secundo[GMatrix::KX]*primo[GMatrix::SY];
		tx=secundo[GMatrix::SX]*primo[GMatrix::TX]+secundo[GMatrix::KX]*primo[GMatrix::TY]+secundo[GMatrix::TX];
		ky=secundo[GMatrix::KY]*primo[GMatrix::SX]+secundo[GMatrix::SY]*primo[GMatrix::KY];
		sy=secundo[GMatrix::KY]*primo[GMatrix::KX]+secundo[GMatrix::SY]*primo[GMatrix::SY];
		ty=secundo[GMatrix::KY]*primo[GMatrix::TX]+secundo[GMatrix::SY]*primo[GMatrix::TY]+secundo[GMatrix::TY];

    	fMat[GMatrix::SX]=sx;
    	fMat[GMatrix::KX]=kx;
    	fMat[GMatrix::TX]=tx;
    	fMat[GMatrix::KY]=ky;
    	fMat[GMatrix::SY]=sy;
    	fMat[GMatrix::TY]=ty;
    }

    /*
     *  If this matrix is invertible, return true and (if not null) set the inverse parameter.
     *  If this matrix is not invertible, return false and ignore the inverse parameter.
     */
    bool GMatrix::invert(GMatrix* inverse) const{
    	 // float det = fMat[GMatrix::SX ] * fMat[GMatrix::SY ] - fMat[GMatrix::KX ] * fMat[GMatrix::KY ] ;//AE-BD
    	float det = (fMat[GMatrix::SX])*(fMat[GMatrix::SY]) -\
        (fMat[GMatrix::KX])*(fMat[GMatrix::KY]);
    	 
    	 if (det==0){ 
    	     return false;
    	 } else{
            inverse -> fMat[SX] = fMat[GMatrix::SY] * 1/det;
            inverse -> fMat[KX] = -fMat[GMatrix::KX] * 1/det;
            inverse -> fMat[TX] = fMat[GMatrix::KX] * fMat[GMatrix::TY] - fMat[GMatrix::SY] * fMat[GMatrix::TX];
            inverse -> fMat[TX] *= 1/det;
            inverse -> fMat[KY] = -fMat[GMatrix::KY] * 1/det;   //-D
            inverse -> fMat[SY] = fMat[GMatrix::SX] * 1/det;  //A
            inverse -> fMat[TY] = fMat[GMatrix::KY] * fMat[GMatrix::TX] - fMat[GMatrix::SX] * fMat[GMatrix::TY];
            inverse -> fMat[TY] *= 1/det;
    	     return true;
    	 }
    }
    
    /**
     *  Transform the set of points in src, storing the resulting points in dst, by applying this
     *  matrix. It is the caller's responsibility to allocate dst to be at least as large as src.
     *
     *  Note: It is legal for src and dst to point to the same memory (however, they may not
     *  partially overlap). Thus the following is supported.
     *
     *  GPoint pts[] = { ... };
     *  matrix.mapPoints(pts, pts, count);
     */
    void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const{
    	  for (int i = 0; i < count; ++i){
            GPoint newPoint = GPoint::Make(src[i].x()*fMat[GMatrix::SX]+\
            src[i].y()*fMat[GMatrix::KX] + fMat[GMatrix::TX] ,\
            src[i].x()*fMat[GMatrix::KY] + src[i].y()*fMat[GMatrix::SY] +\
            fMat[GMatrix::TY]);
            dst[i] = newPoint;
        }
    }