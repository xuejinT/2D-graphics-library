#include <iostream>
#include "GCanvas.h"
#include "GBitmap.h"
#include "GColor.h"
#include <math.h>
#include "GRect.h"
#include "GPoint.h"
#include "GPixel.h"
#include "GMatrix.h"
#include "GShader.h"
#include "GColor.h"
#include "GContour.h"
#include "GMath.h"
#include <stdio.h>
#include <stack>
#include <list>
#include <vector>

// #include "myColorShader.cpp"
// #include "myProxyShader.cpp"
#include "myComposeShader.cpp"
struct edge{
	int eYmin;
	int eY;
	float eSlope;
	float eCurX;
	int winding;
};

// bool operator< (const edge& a) const{
//     if(eYmin==a.eYmin){  
//         return eCurX<a.eCurX;
//     }

//     return eYmin<a.eYmin;
// }

bool operator< (const edge a, const edge b){
    if(a.eYmin == b.eYmin){   
        return a.eCurX < b.eCurX;
    }
    return a.eYmin < b.eYmin;
};

struct matrix{
    float a;
    float b;
    float c;
    float d;
    float e;
    float f;
};

bool compare_x(const edge& a,const edge& b){
        return a.eCurX<b.eCurX;
    }

bool compare_y(const edge& a,const edge& b){
        return a.eY<b.eY;
    }


class EmptyCanvas : public GCanvas {
public:
	int h,w;
	size_t rowBytes;
	GPixel* start;
	GPixel* end;
    GBitmap canvasBit;
    GMatrix ctm;
    std::stack<GMatrix> mStack;

    EmptyCanvas(const GBitmap& device) : fDevice(device) {
    	 w= device.width();
    	 h=device.height();
    	 rowBytes=device.rowBytes();
         canvasBit=device;
    	 start= device.pixels();
    	 end=start+rowBytes*h/4+w;
    }


    void clear(const GColor& color)  {
    	GPixel px= colToPix(color);
    	for (int i = 0; i < h; i++){
    		for (int j = 0; j < w; j++){
    			GPixel* current=get_pixel_addr(start,j,i);
    			*(current)=px;
    		}
    	}
    }
    
    // void fillRect(const GRect& rect, const GColor& color)  {
    //     this->drawRect(rect, GPaint(color));
  //   	int rectW,rectH,x,y;
  //   	GPixel* rectStart;

  //   	GIRect iRect=rect.round();
  //   	rectW=iRect.width();
  //   	rectH=iRect.height();
  //   	x=iRect.x();
  //   	y=iRect.y();

  //   	// std::cout<<"X: "<<x<<" "<<"Y: "<<y<<" \n"<<"W: "<<rectW<<" "<<"H: "<<rectH<<" \n"<<"w: "<<w<<" "<<"h: "<<h<<" \n";
  //   	(y+rectH<=h )?(y>=0?rectH=iRect.height():rectH=rectH+y):rectH=h-y;
  //   	(x+rectW<=w )?(x>=0?rectW=iRect.width():rectW=rectW+x):rectW=w-x;

		// // std::cout<<"H: "<<rectH<<" \n"<<"W: "<<rectW<<" \n";
  //   	(x>=0&&y>=0)?rectStart=start+rowBytes*y/4+x:rectStart=start;

    	
  //   	for(int i = 0; i < rectH; i++){
    		
  //   		for (int j = 0; j < rectW; j++){
  //   			GPixel* current=get_pixel_addr(rectStart,j,i);
  //   			GPixel colPx= blendPixel(color,*(current));
    			
  //   			*current=colPx;
  //   		}
  //   	}
   
    // }

    void translate(float tx, float ty){
        ctm.preTranslate(tx,ty);
    }

    void scale(float sx, float sy){
        ctm.preTranslate(sx,sy);
    }

    void rotate(float radians){
        ctm.preRotate(radians);
    }

    void setCTM(const GMatrix& new_matrix){
        ctm= new_matrix;
    }

    void fillBitmapRect(const GBitmap& src, const GRect& dst){
        GMatrix tm;
        float top = dst.top();
        float left = dst.left();
        float xScle = dst.width()/src.width();
        float yScle = dst.height()/src.height();
        GPoint pts[4];
        pts[0] = GPoint::Make(dst.left(),dst.top());
        pts[1] = GPoint::Make(dst.right(),dst.top());
        pts[2] = GPoint::Make(dst.right(),dst.bottom());
        pts[3] = GPoint::Make(dst.left(),dst.bottom());
        tm.setTranslate(left, top);
        tm.preScale(xScle,yScle);
        // GShader* shader =GCreateBitmapShader(src,tm).get();
        // GPaint paint = GPaint(shader);
        std::unique_ptr<GShader> shader = GCreateBitmapShader(src,tm);
        GPaint paint = GPaint(shader.get());
        drawConvexPolygon(pts,4,paint);
    }

    // void fillConvexPolygon(const GPoint GPoint[], int count, const GColor& color){
    //     this->drawConvexPolygon(GPoint, count, GPaint(color));
        // if(count>=2){
        //     edge edges[count];
        //     //Build edges
        //     for(int i=0;i<count;i++){
        //         edges[i]=makeEdge(GPoint[i],GPoint[i+1]);
        //         if(i==count-1){
        //             edges[i]=makeEdge(GPoint[i],GPoint[0]);
        //         }
        //     }

        //     std::sort(&edges[0],&edges[count]);
        //     edge RE=edges[1];
        //     edge LE=edges[0];
        //     for(int i=2;i<=count;i++){
        //         int yStart=fmax(RE.eTop,LE.eTop);
        //         int yEnd=fmin(RE.eBottom,LE.eBottom);
        //         // std::cout<<"1"<<"\n";
        //         for(int y=yStart;y<yEnd;y++){
        //             int xMin = floor(LE.eCurX + 0.5);
        //             int xMax = floor(RE.eCurX + 0.5);
        //             int xStart = fmax(0,fmin(xMin,xMax));
        //             int xEnd = fmin(w,fmax(xMin,xMax));
        //             // std::cout<<"2: "<<y<<"\n";
        //             for(int x=xStart;x<xEnd;x++){
        //                 GPixel srcPix = colToPix(color);
        //                 GPixel *src = &srcPix;                
        //                 GPixel *dst = canvasBit.getAddr(x,y); 
        //                 if (GPixel_GetA(*src) < 255){
        //                     GPixel newCol = blend(*src, *dst);
        //                     *dst = newCol;
        //                 } else {
        //                     *dst = GPixel_PackARGB(255, GPixel_GetR(*src), GPixel_GetG(*src), GPixel_GetB(*src));
        //                 }
        //             }
        //             LE.eCurX=LE.eCurX+LE.eSlope;
        //             RE.eCurX=RE.eCurX+RE.eSlope;   
        //         }
        //         yEnd==LE.eBottom?LE=edges[i]:RE=edges[i];
        //     }
        // }
    // }

    edge makeEdge(GPoint p,GPoint q ){  
        edge anEdge;

        if(q.y()>p.y()){
            anEdge.winding=1;
        }else if(q.y()<p.y()){
            anEdge.winding=-1;
        }

        float pointMax=fmax(p.y(),q.y());
        float pointMin=fmin(p.y(),q.y());

        anEdge.eYmin=fmin(fmax(0,(int)floor(pointMin+0.5)), h);
        anEdge.eY=fmax(fmin((int)floor(pointMax+0.5),h),0);

        if(p.y()==q.y()){
            anEdge.eSlope=0;
        }else{
            anEdge.eSlope=(p.x()-q.x())/(p.y()-q.y());
        }

        float b=p.x()-anEdge.eSlope*p.y();
        anEdge.eCurX=b+anEdge.eSlope*(anEdge.eYmin+0.5);

        return anEdge;

    }

    GRect clip(const GRect& rec){
        int left=fmax((int)(rec.left()+0.5),0);
        int top=fmax((int)(rec.top()+0.5),0);
        int right=fmin((int)(rec.right()+0.5),w);
        int bottom=fmin((int)(rec.bottom()+0.5),h);
        GRect recNew;
        recNew.setLTRB(left,top,right,bottom);
        return recNew;
    }   

    GPixel colToPix(GColor color) {
        GColor srcC = color.pinToUnit();
        int a =  GRoundToInt(255 * srcC.fA);
        int r =  GRoundToInt(255 * srcC.fA * srcC.fR);
        int g =  GRoundToInt(255 * srcC.fA * srcC.fG);
        int b =  GRoundToInt(255 * srcC.fA * srcC.fB);
        return GPixel_PackARGB(a, r, g, b);
    }

    // GPixel blendPixel(const GColor& color,const GPixel& px) {
    //     GColor srcC = color.pinToUnit();
    //     GPixel destPx =  px;
    //     //int k=(1<<16)|(1<<8)|1;
    //     int da= GPixel_GetA(px);
    //     int dr= GPixel_GetR(px);
    //     int dg= GPixel_GetG(px);
    //     int db= GPixel_GetB(px);

    //     int sa = GRoundToInt(255 * srcC.fA);
    //     int sr =  GRoundToInt(255 * srcC.fA * srcC.fR);
    //     int sg =  GRoundToInt(255 * srcC.fA * srcC.fG);
    //     int sb =  GRoundToInt(255 * srcC.fA * srcC.fB);

    //     int a =  sa+((255-sa)*da+127)/255;
    //     int r =  sr+((255-sa)*dr+127)/255;
    //     int g =  sg+((255-sa)*dg+127)/255;
    //     int b =  sb+((255-sa)*db+127)/255;
        
    //     return GPixel_PackARGB(a, r, g, b);
    // }

    GPixel blend(const GPixel& src,const GPixel& dst) {
        int srcA=GPixel_GetA(src);
        int newA=(uint8_t)(srcA + (((255-srcA)*GPixel_GetA(dst)*65793+(1<<23))>>24));
        int newR=(uint8_t)(GPixel_GetR(src)+ (((255-srcA) *GPixel_GetR(dst)*65793+(1<<23))>>24));
        int newG=(uint8_t)(GPixel_GetG(src)+ (((255-srcA) *GPixel_GetG(dst)*65793+(1<<23))>>24));
        int newB=(uint8_t)(GPixel_GetB(src)+ (((255-srcA) *GPixel_GetB(dst)*65793+(1<<23))>>24));
        return GPixel_PackARGB(newA, newR, newG, newB);
    }

    GPixel* get_pixel_addr(GPixel* startPt, int x, int y) {
    	char* row = (char*)startPt + y * rowBytes;
    	return (GPixel*)row + x;
	}

    // void setCTM(const GMatrix& matrix){
    //     ctm = matrix;
    // }

    void save(){
		mStack.push(ctm);
	}

	void restore(){
		ctm = mStack.top();
		mStack.pop();
	}

	void concat(const GMatrix& new_matrix){
		ctm.preConcat(new_matrix);
	}

    void drawRect(const GRect& rect, const GPaint& paint){
        GPoint pts[4];
        pts[0]=GPoint::Make(rect.left(),rect.top());
        pts[1]=GPoint::Make(rect.right(),rect.top());
        pts[2]=GPoint::Make(rect.right(),rect.bottom());
        pts[3]=GPoint::Make(rect.left(),rect.bottom());
        drawConvexPolygon(pts,4,paint);
    }
    void drawConvexPolygon(const GPoint new_points[], int count, const GPaint& paint){
        GPoint mappedPts[count];
        ctm.mapPoints(mappedPts, new_points, count);
        //check if number of edges is valid
        if(count>=2){
            edge edges[count];
            //Build edges
            for(int i=0;i<count;i++){
                edges[i]=makeEdge(mappedPts[i],mappedPts[i+1]);
                if(i==count-1){
                    edges[i]=makeEdge(mappedPts[i],mappedPts[0]);
                }
            }

            std::sort(&edges[0],&edges[count]);
            edge RE=edges[1];
            edge LE=edges[0];
            for(int i=2;i<=count;i++){
                int yStart=fmax(RE.eYmin,LE.eYmin);
                int yEnd=fmin(RE.eY,LE.eY);
                // std::cout<<"1"<<"\n";
                for(int y=yStart;y<yEnd;y++){
                    int xMin = floor(LE.eCurX + 0.5);
                    int xMax = floor(RE.eCurX + 0.5);
                    int xStart = fmax(0,fmin(xMin,xMax));
                    int xEnd = fmin(w,fmax(xMin,xMax));
                    // std::cout<<"2: "<<y<<"\n";
                    int j=0;
                    for(int x=xStart;x<xEnd;x++){
                        // std::cout<<"w: "<<w<<"h: "<<h<<"---";
                        // std::cout<<"x: "<<x<<"y: "<<y<<"---";
                        GPixel srcPix = colToPix(paint.getColor());
                        GPixel *src = &srcPix;                
                        GPixel *dst = canvasBit.getAddr(x,y); 
                        if(paint.getShader()==nullptr){
                            // std::cout<<"no shader"<<"\n";
                            if (GPixel_GetA(*src) < 255){
                                GPixel newCol = blend(*src, *dst);
                                *dst = newCol;
                            } else {
                                *dst = GPixel_PackARGB(255, GPixel_GetR(*src), GPixel_GetG(*src), GPixel_GetB(*src));
                            }
                        }
                        else{
                            // printf("shader\n");
                            // printf("xEnd:%i,xStart%i---",xEnd,xStart);
                            int xCount=xEnd-xStart;
                            // printf("xCount: %i,j=%i\n",xCount,j);
                            GPixel row[xCount];
                            paint.getShader()->setContext(ctm,paint.getAlpha());
                            // printf("setContext---\n");
                            paint.getShader()->shadeRow(xStart, y, xCount, row);
                            // printf("shadeRow\n");
                            // std::cout<<"row j"<<row[j]<<"\n";
                            *dst = blend(row[j], *dst);
                            // printf("blend\n");
                            j++;
                        }
                    }
                    LE.eCurX=LE.eCurX+LE.eSlope;
                    RE.eCurX=RE.eCurX+RE.eSlope;   
                }
                yEnd==LE.eY?LE=edges[i]:RE=edges[i];
            }
        } 
    }

void drawContours(const GContour ctrs[], int count, const GPaint& paint){
    std::vector<GContour> ctrsVec;
    std::vector<edge> edges;      //keep a vector of all the edges
    int width = paint.getStrokeWidth();
    int ctrsNum = 0;
    int edgeIndex = 0;
    int edgeNum = 0;

    if (width > 0){
        for (int i = 0; i < count; ++i){
            GPoint points[ctrs[i].fCount];
            GMatrix().mapPoints(points, ctrs[i].fPts, ctrs[i].fCount);
            if (ctrs[i].fClosed == true){
                int j = 0;
                while(j < ctrs[i].fCount - 2){
                    GPoint a = points[j];
                    GPoint b = points[j+1];
                    GPoint c = points[j+2];
                    //quad
                    GContour tempContour = saveQuad(a, b, width);
                    ctrsVec.push_back(tempContour);
                    ctrsNum++;
                    //miter or bevel
                    GContour tempContour2 = saveMiterBevel(a, b, c, width, paint);
                    ctrsVec.push_back(tempContour2);
                    ctrsNum++;
                    j++;
                }
                
                ctrsVec.push_back(saveQuad(points[j], points[j+1], width));
                ctrsVec.push_back(saveMiterBevel(points[j], points[j+1], points[0], width, paint));
                ctrsNum += 2;
                
                ctrsVec.push_back(saveQuad(points[j+1], points[0], width));
                ctrsVec.push_back(saveMiterBevel(points[j+1], points[0], points[1], width, paint));
                ctrsNum += 2;

            
            } else if (ctrs[i].fClosed == false){
                int j = 0;
                while(j < ctrs[i].fCount - 2){
                    GPoint a = points[j];
                    GPoint b = points[j+1];
                    GPoint c = points[j+2];
                    //quad
                    GContour tempContour = saveQuad(a, b, width);
                    if (tempContour.fCount > 4) {tempContour.fCount = 4;}
                    ctrsVec.push_back(tempContour);
                    ctrsNum++;
                    //miter or bevel
                    GContour tempContour2 = saveMiterBevel(a, b, c, width, paint);
                    if (tempContour2.fCount > 4) {tempContour2.fCount = 4;}
                    ctrsVec.push_back(tempContour2);
                    ctrsNum++;
                    ++j;
                }
                ctrsVec.push_back(saveQuad(points[j], points[j+1], width));
                ctrsNum++;
            }
        }
        GContour* tmpCtrs = &ctrsVec[0];
        GPaint tmpPaint = paint;
        tmpPaint.setStrokeWidth(-1);
        drawContours(tmpCtrs, ctrsNum, tmpPaint);
    } 
    else {
        for (int i = 0; i < count;i++){
            GPoint mappedPts[ctrs[i].fCount];
            ctm.mapPoints(mappedPts,ctrs[i].fPts, ctrs[i].fCount);
            for(int j = 0; j<ctrs[i].fCount-1; j++){
                if(GRoundToInt(mappedPts[j].y())!=GRoundToInt(mappedPts[j+1].y())){
                    edge newE = makeEdge(mappedPts[j],mappedPts[j+1]);
                    edges.push_back(newE);
                    edgeNum++;
                }
            }
            if(GRoundToInt(mappedPts[ctrs[i].fCount-1].y())!=GRoundToInt(mappedPts[0].y())){
                edge newE = makeEdge(mappedPts[ctrs[i].fCount-1],mappedPts[0]);
                edges.push_back(newE);
                edgeNum++;
            }
        }
        if (edgeNum > 0){
            std::sort(edges.begin(), edges.end(), compare_y);    //first max then min
            int yEnd = GRoundToInt(edges[edgeNum-1].eY);
            std::sort(edges.begin(), edges.end());
            int yStart = GRoundToInt(edges[0].eYmin);  //groundtoint***
            std::list<edge> goodEdges;   //linked list for edges that need to be painted
            for (int curY = yStart; curY < yEnd; ++curY){
                for(std::list<edge>::iterator it=goodEdges.begin(); it!= goodEdges.end(); it++){
                    if((*it).eY<=curY){
                        goodEdges.erase(it);
                        it--;
                    }
                }
                for(int i = edgeIndex; i < edgeNum; i++){
                    if (GRoundToInt(edges[i].eYmin)==curY){
                        goodEdges.push_back(edges[i]);
                        edgeIndex++;
                    }
                }
                goodEdges.sort(compare_x);
                int curWinding = 0;
                for(std::list<edge>::iterator it=goodEdges.begin(); it != goodEdges.end(); it++){
                    curWinding += (*it).winding;
                    if (curWinding!=0){
                        int xL =(*it).eCurX;
                        int xR = (*(std::next(it,1))).eCurX;

                        if(paint.getShader() == nullptr){
                            xL=fmax(0,xL);
                            xR=fmax(0,xR);
                            xL=fmin(w-1,xL);
                            xR=fmin(w-1,xR);//crop

                            GPixel srcPx = colToPix(paint.getColor());
                            for(int x = xL; x < xR; x++){
                                GPixel *dstPx = fDevice.getAddr(x,curY);
                                paint.getColor().fA==1?*dstPx = srcPx:*dstPx = blend(srcPx, * dstPx);
                            }
                        }else{
                            int pxNum = xR - xL;
                            GPixel row[w];
                            paint.getShader()->setContext(ctm,paint.getAlpha());
                            paint.getShader()->shadeRow(xL,curY,pxNum,row);
                            int j = 0;
                            for(int x = xL; x < xR; x++){
                                GPixel *dstPx = fDevice.getAddr(x,curY);
                                *dstPx = blend(row[j], *dstPx);
                                j++;
                            }
                        }
                    }
                }
                for(std::list<edge>::iterator it=goodEdges.begin(); it != goodEdges.end();it++){
                    (*it).eCurX += (*it).eSlope;
                }//eCurX+=slope
            }
        }
    
    }    
}


GContour saveQuad(GPoint a, GPoint b, int width){
    GPoint ab = GPoint::Make(b.fX - a.fX, b.fY - a.fY);
    float abLen = sqrt (pow(ab.fX, 2.0) + pow(ab.fY, 2.0) );
    GPoint abUnit = GPoint::Make(ab.fX/abLen * (width/2), ab.fY/abLen * (width/2));
    GPoint abUnitCCW90 = GPoint::Make(-abUnit.fY, abUnit.fX);
    GPoint abUnitCW90 = GPoint::Make(abUnit.fY, -abUnit.fX);
    GPoint* ptsArr = new GPoint[4];
    ptsArr[0]= GPoint::Make(a.fX + abUnitCW90.fX, a.fY + abUnitCW90.fY);
    ptsArr[1]= GPoint::Make(a.fX + abUnitCCW90.fX, a.fY + abUnitCCW90.fY);
    ptsArr[2]= GPoint::Make(b.fX + abUnitCCW90.fX, b.fY + abUnitCCW90.fY);
    ptsArr[3]= GPoint::Make(b.fX + abUnitCW90.fX, b.fY + abUnitCW90.fY);
    GContour tempContour;
    tempContour.fCount = 4;
    tempContour.fPts = ptsArr;
    tempContour.fClosed = false;
    return tempContour;
}

GContour saveMiterBevel(GPoint a, GPoint b, GPoint c, int width, const GPaint& paint){
    GPoint ba = GPoint::Make(a.fX - b.fX, a.fY - b.fY);   
    GPoint bc = GPoint::Make(c.fX - b.fX, c.fY - b.fY);
    GPoint ab = GPoint::Make(b.fX - a.fX, b.fY - a.fY);
    float baLen = sqrt ( pow(ba.fX, 2.0) + pow(ba.fY, 2.0) );
    float bcLen = sqrt ( pow(bc.fX, 2.0) + pow(bc.fY, 2.0) );
    float abLen = sqrt ( pow(ab.fX, 2.0) + pow(ab.fY, 2.0) );
    GPoint u = GPoint::Make(ba.fX/baLen, ba.fY/baLen);  
    GPoint v = GPoint::Make(bc.fX/bcLen, bc.fY/bcLen);   
    GPoint w = GPoint::Make(ab.fX/abLen, ab.fY/abLen);

    float sintheta = u.fX * v.fY - u.fY * v.fX;
    
    if (sintheta > 0){  //angle < 180
        GPoint baUnitCW90 = GPoint::Make(u.fY, -u.fX);
        GPoint bcUnitCCW90 = GPoint::Make(-v.fY, v.fX);
        GPoint Q = GPoint::Make(b.fX + baUnitCW90.fX, b.fY + baUnitCW90.fY);
        GPoint R = GPoint::Make(b.fX + bcUnitCCW90.fX, b.fY + bcUnitCCW90.fY);
        GPoint tmpQ = GPoint::Make(Q.fX - b.fX, Q.fY - b.fY);
        GPoint tmpR = GPoint::Make(R.fX - b.fX, R.fY - b.fY);
        GPoint wrongbp = GPoint::Make(tmpQ.fX + tmpR.fX, tmpQ.fY + tmpR.fY); 
        float wrongbpLen = sqrt ( pow(wrongbp.fX, 2.0) + pow(wrongbp.fY, 2.0) );
        GPoint wrongbpUnit = GPoint::Make(wrongbp.fX/wrongbpLen, wrongbp.fY/wrongbpLen);
        float crossProduct = u.fX * v.fX + u.fY * v.fY;
         float oldLen = sqrt( 2/(1 - crossProduct));
        float bpLen = width/2 * sqrt( 2/(1 - crossProduct) );
        GPoint bp = GPoint::Make(wrongbpUnit.fX * bpLen, wrongbpUnit.fY * bpLen);
        if (oldLen <= paint.getMiterLimit() * (width/2)){  //miter
            GPoint p = GPoint::Make(b.fX + bp.fX, b.fY + bp.fY);
            return makeMiter(b,p,Q,R,true);
        } else {  //bevel
            return makeBevel(b,Q,R,true);
        }
    } else {    //angle > 180
        GPoint tmpQ = GPoint::Make(-u.fY, u.fX);
        GPoint tmpR = GPoint::Make(v.fY, -v.fX);
        GPoint Q = GPoint::Make(b.fX + tmpQ.fX, b.fY + tmpQ.fY);
        GPoint R = GPoint::Make(b.fX + tmpR.fX, b.fY + tmpR.fY);
        GPoint wrongbp = GPoint::Make(tmpQ.fX + tmpR.fX, tmpQ.fY + tmpR.fY);    //tmpQ+tmpR
        float wrongbpLen = sqrt ( pow(wrongbp.fX, 2.0) + pow(wrongbp.fY, 2.0) );
        GPoint wrongbpUnit = GPoint::Make(wrongbp.fX/wrongbpLen, wrongbp.fY/wrongbpLen);
        float crossProduct = u.fX * v.fX + u.fY * v.fY;
        float oldLen = sqrt( 2/(1 - crossProduct));
        float bpLen = width/2 * sqrt( 2/(1 - crossProduct) );
        GPoint bp = GPoint::Make(wrongbpUnit.fX * bpLen, wrongbpUnit.fY * bpLen);
        if (oldLen < paint.getMiterLimit() * (width/2)){  //miter
            GPoint p = GPoint::Make(b.fX + bp.fX, b.fY + bp.fY);
            return makeMiter(b,p,Q,R,true);
        } else {  //bevel
            return makeBevel(b,Q,R,true);
        }
    }
}


GContour makeMiter(GPoint b, GPoint p, GPoint q, GPoint r,bool close){
    GPoint* ptsArr = new GPoint[4];
    ptsArr[0] = r; 
    ptsArr[1] = b; 
    ptsArr[2] = q; 
    ptsArr[3] = p;
    GContour tempContour;
    tempContour.fCount = 4;
    tempContour.fPts = ptsArr;
    tempContour.fClosed = close;
    return tempContour;
}

GContour makeBevel(GPoint b, GPoint q, GPoint r,bool close){
    GPoint* ptsArr = new GPoint[3];
    ptsArr[0] = r; 
    ptsArr[1] = b; 
    ptsArr[2] = q;
    GContour tempContour;
    tempContour.fCount = 3;
    tempContour.fPts = ptsArr;
    tempContour.fClosed = close;
    return tempContour;
}


void drawMesh(int triCount, const GPoint pts[], const int indices[],
                         const GColor colors[], const GPoint tex[], const GPaint& paint){

    if (indices == nullptr){         
        for (int i = 0; i < triCount*3; i+=3){
            GPoint vertex[3];
            vertex[0] = pts[i];vertex[1] = pts[i+1];vertex[2] = pts[i+2];
            if (tex != nullptr && colors == nullptr){
                GPoint vertexTex[3];
                vertexTex[0] = tex[i];vertexTex[1] = tex[i+1];vertexTex[2] = tex[i+2];
                GMatrix P;
                GMatrix M;
                float p1x = vertex[1].fX - vertex[0].fX;
                float p2x = vertex[2].fX - vertex[0].fX;
                float p1y = vertex[1].fY - vertex[0].fY;
                float p2y = vertex[2].fY - vertex[0].fY;
                P.set6(p1x, p2x, vertex[0].fX, p1y, p2y, vertex[0].fY);
                float M1x = vertexTex[1].fX - vertexTex[0].fX;
                float M2x = vertexTex[2].fX - vertexTex[0].fX;
                float M1y = vertexTex[1].fY - vertexTex[0].fY;
                float M2y = vertexTex[2].fY - vertexTex[0].fY;
                M.set6(M1x, M2x, vertexTex[0].fX, M1y, M2y, vertexTex[0].fY);
                GMatrix invM;
                GMatrix finalMatrix;
                M.invert(&invM);
                finalMatrix.setConcat(P, invM);
                myProxyShader tmpShader(paint.getShader(), finalMatrix);
                GPaint newPaint = GPaint(&tmpShader);
                drawConvexPolygon(vertex, 3, newPaint);
            }
            else if (colors != nullptr && tex == nullptr)  { 
                GColor vertexCol[3];     
                vertexCol[0] = colors[i];
                vertexCol[1] = colors[i+1];
                vertexCol[2] = colors[i+2];
                myColorShader tmpShader(vertex[0], vertex[1], vertex[2], vertexCol[0], vertexCol[1], vertexCol[2]);
                GPaint newPaint = GPaint(&tmpShader);
                drawConvexPolygon(vertex, 3, newPaint);
            }else if (colors != nullptr && tex != nullptr){
                GColor vertexCol[3];
                vertexCol[0] = colors[i];
                vertexCol[1] = colors[i+1];
                vertexCol[2] = colors[i+2];
                GPoint vertexTex[3];
                vertexTex[0] = tex[i];
                vertexTex[1] = tex[i+1];
                vertexTex[2] = tex[i+2];
                myColorShader s0(vertex[0], vertex[1], vertex[2], vertexCol[0], vertexCol[1], vertexCol[2]);      
                GMatrix P;
                float p1x = vertex[1].fX - vertex[0].fX;
                float p2x = vertex[2].fX - vertex[0].fX;
                float p1y = vertex[1].fY - vertex[0].fY;
                float p2y = vertex[2].fY - vertex[0].fY;
                P.set6(p1x, p2x, vertex[0].fX, p1y, p2y, vertex[0].fY);
                
                GMatrix M;
                float M1x = vertexTex[1].fX - vertexTex[0].fX;
                float M2x = vertexTex[2].fX - vertexTex[0].fX;
                float M1y = vertexTex[1].fY - vertexTex[0].fY;
                float M2y = vertexTex[2].fY - vertexTex[0].fY;
                M.set6(M1x, M2x, vertexTex[0].fX, M1y, M2y, vertexTex[0].fY);
                
                GMatrix invM;
                GMatrix finalMatrix;
                M.invert(&invM);
                finalMatrix.setConcat(P, invM);
                
                myProxyShader s1(paint.getShader(), finalMatrix);       
                
                myComposeShader tmpShader(&s0, &s1);
                GPaint newPaint = GPaint(&tmpShader);
                
                drawConvexPolygon(vertex, 3, newPaint);
            }
        }
        
    } else {
        for (int i = 0; i < triCount*3; i+=3){
            GPoint vertex[3];
            vertex[0] = pts[indices[i]];
            vertex[1] = pts[indices[i+1]];
            vertex[2] = pts[indices[i+2]];
            
            
            if (colors != nullptr && tex == nullptr)  {   
                GColor vertexCol[3];     
                vertexCol[0] = colors[indices[i]];
                vertexCol[1] = colors[indices[i+1]];
                vertexCol[2] = colors[indices[i+2]];
                
                myColorShader tmpShader(vertex[0], vertex[1], vertex[2], vertexCol[0], vertexCol[1], vertexCol[2]);
                GPaint newPaint = GPaint(&tmpShader);

                drawConvexPolygon(vertex, 3, newPaint);
           
            } else if (tex != nullptr && colors == nullptr){
                GPoint vertexTex[3];
                vertexTex[0] = tex[indices[i]];
                vertexTex[1] = tex[indices[i+1]];
                vertexTex[2] = tex[indices[i+2]];
                
                GMatrix P;
                float p1x = vertex[1].fX - vertex[0].fX;
                float p2x = vertex[2].fX - vertex[0].fX;
                float p1y = vertex[1].fY - vertex[0].fY;
                float p2y = vertex[2].fY - vertex[0].fY;
                P.set6(p1x, p2x, vertex[0].fX, p1y, p2y, vertex[0].fY);
                
                GMatrix M;
                float M1x = vertexTex[1].fX - vertexTex[0].fX;
                float M2x = vertexTex[2].fX - vertexTex[0].fX;
                float M1y = vertexTex[1].fY - vertexTex[0].fY;
                float M2y = vertexTex[2].fY - vertexTex[0].fY;
                M.set6(M1x, M2x, vertexTex[0].fX, M1y, M2y, vertexTex[0].fY);
                
                GMatrix invM;
                GMatrix finalMatrix;
                M.invert(&invM);
                finalMatrix.setConcat(P, invM);
                
                myProxyShader tmpShader(paint.getShader(), finalMatrix);
                GPaint newPaint = GPaint(&tmpShader);
                
                drawConvexPolygon(vertex, 3, newPaint);
                
            } else if (colors != nullptr && tex != nullptr){
                GColor vertexCol[3];
                vertexCol[0] = colors[indices[i]];
                vertexCol[1] = colors[indices[i+1]];
                vertexCol[2] = colors[indices[i+2]];

                GPoint vertexTex[3];
                vertexTex[0] = tex[indices[i]];
                vertexTex[1] = tex[indices[i+1]];
                vertexTex[2] = tex[indices[i+2]];
                
                myColorShader s0(vertex[0], vertex[1], vertex[2], vertexCol[0], vertexCol[1], vertexCol[2]);      

                GMatrix P;
                float p1x = vertex[1].fX - vertex[0].fX;
                float p2x = vertex[2].fX - vertex[0].fX;
                float p1y = vertex[1].fY - vertex[0].fY;
                float p2y = vertex[2].fY - vertex[0].fY;
                P.set6(p1x, p2x, vertex[0].fX, p1y, p2y, vertex[0].fY);
                
                GMatrix M;
                float M1x = vertexTex[1].fX - vertexTex[0].fX;
                float M2x = vertexTex[2].fX - vertexTex[0].fX;
                float M1y = vertexTex[1].fY - vertexTex[0].fY;
                float M2y = vertexTex[2].fY - vertexTex[0].fY;
                M.set6(M1x, M2x, vertexTex[0].fX, M1y, M2y, vertexTex[0].fY);
                
                GMatrix invM;
                GMatrix finalMatrix;
                M.invert(&invM);
                finalMatrix.setConcat(P, invM);
                myProxyShader s1(paint.getShader(), finalMatrix);         
                myComposeShader tmpShader(&s0, &s1);
                GPaint newPaint = GPaint(&tmpShader);
                drawConvexPolygon(vertex, 3, newPaint);
            }
        }

    }
}

void drawQuad(const GPoint pts[4], const GColor colors[4], const GPoint tex[4],int level, const GPaint& paint){
    int n=level+1;    
    GPoint vertex[n*n*2*3];

    GPoint A=pts[0];
    GPoint B=pts[1];
    GPoint C=pts[2];
    GPoint D=pts[3];
    
    int i=0;
    //compute vertices
    for(float u=0;u<n;u++){
        for(float v=0;v<n;v++){

            //R=(1-v)*[(1-u)A+uB]+v[(1-u)D+uC]
            GPoint a=(1-v/n)*((1-u/n)*A+u/n*B)+v/n*((1-u/n)*D+u/n*C);
            GPoint b=(1-v/n)*((1-(u+1)/n)*A+(u+1)/n*B)+v/n*((1-(u+1)/n)*D+(u+1)/n*C);
            GPoint c=(1-(v+1)/n)*((1-(u+1)/n)*A+(u+1)/n*B)+(v+1)/n*((1-(u+1)/n)*D+(u+1)/n*C);
            GPoint d=(1-(v+1)/n)*((1-u/n)*A+u/n*B)+(v+1)/n*((1-u/n)*D+u/n*C);

            vertex[i]=a;
            vertex[i+1]=b;
            vertex[i+2]=d;
            vertex[i+3]=b;
            vertex[i+4]=c;
            vertex[i+5]=d;
            i+=6;
        }
    }
    //compute colors 
    if(colors != nullptr && tex == nullptr){//have color, texure is null
        GColor vertexCol[n*n*2*3];
        GColor ACol=colors[0];
        GColor BCol=colors[1];
        GColor CCol=colors[2];
        GColor DCol=colors[3];
        int A_A=GPixel_GetA(colToPix(ACol));
        int A_R=GPixel_GetR(colToPix(ACol));
        int A_G=GPixel_GetG(colToPix(ACol));
        int A_B=GPixel_GetB(colToPix(ACol));

        int B_A=GPixel_GetA(colToPix(BCol));
        int B_R=GPixel_GetR(colToPix(BCol));
        int B_G=GPixel_GetG(colToPix(BCol));
        int B_B=GPixel_GetB(colToPix(BCol));

        int C_A=GPixel_GetA(colToPix(CCol));
        int C_R=GPixel_GetR(colToPix(CCol));
        int C_G=GPixel_GetG(colToPix(CCol));
        int C_B=GPixel_GetB(colToPix(CCol));

        int D_A=GPixel_GetA(colToPix(DCol));
        int D_R=GPixel_GetR(colToPix(DCol));
        int D_G=GPixel_GetG(colToPix(DCol));
        int D_B=GPixel_GetB(colToPix(DCol));

        i=0;
    for(float u=0;u<n;u++){
        for(float v=0;v<n;v++){
                float a_A=((1-v/n)*((1-u/n)*A_A+u/n*B_A)+v/n*((1-u/n)*D_A+u/n*C_A))/255;
                float a_R=((1-v/n)*((1-u/n)*A_R+u/n*B_R)+v/n*((1-u/n)*D_R+u/n*C_R))/255;
                float a_G=((1-v/n)*((1-u/n)*A_G+u/n*B_G)+v/n*((1-u/n)*D_G+u/n*C_G))/255;
                float a_B=((1-v/n)*((1-u/n)*A_B+u/n*B_B)+v/n*((1-u/n)*D_B+u/n*C_B))/255;

                float b_A=((1-v/n)*((1-(u+1)/n)*A_A+(u+1)/n*B_A)+v/n*((1-(u+1)/n)*D_A+(u+1)/n*C_A))/255;
                float b_R=((1-v/n)*((1-(u+1)/n)*A_R+(u+1)/n*B_R)+v/n*((1-(u+1)/n)*D_R+(u+1)/n*C_R))/255;
                float b_G=((1-v/n)*((1-(u+1)/n)*A_G+(u+1)/n*B_G)+v/n*((1-(u+1)/n)*D_G+(u+1)/n*C_G))/255;
                float b_B=((1-v/n)*((1-(u+1)/n)*A_B+(u+1)/n*B_B)+v/n*((1-(u+1)/n)*D_B+(u+1)/n*C_B))/255;

                float c_A=((1-(v+1)/n)*((1-(u+1)/n)*A_A+(u+1)/n*B_A)+(v+1)/n*((1-(u+1)/n)*D_A+(u+1)/n*C_A))/255;
                float c_R=((1-(v+1)/n)*((1-(u+1)/n)*A_R+(u+1)/n*B_R)+(v+1)/n*((1-(u+1)/n)*D_R+(u+1)/n*C_R))/255;
                float c_G=((1-(v+1)/n)*((1-(u+1)/n)*A_G+(u+1)/n*B_G)+(v+1)/n*((1-(u+1)/n)*D_G+(u+1)/n*C_G))/255;
                float c_B=((1-(v+1)/n)*((1-(u+1)/n)*A_B+(u+1)/n*B_B)+(v+1)/n*((1-(u+1)/n)*D_B+(u+1)/n*C_B))/255;

                float d_A=((1-(v+1)/n)*((1-u/n)*A_A+u/n*B_A)+(v+1)/n*((1-u/n)*D_A+u/n*C_A))/255;
                float d_R=((1-(v+1)/n)*((1-u/n)*A_R+u/n*B_R)+(v+1)/n*((1-u/n)*D_R+u/n*C_R))/255;
                float d_G=((1-(v+1)/n)*((1-u/n)*A_G+u/n*B_G)+(v+1)/n*((1-u/n)*D_G+u/n*C_G))/255;
                float d_B=((1-(v+1)/n)*((1-u/n)*A_B+u/n*B_B)+(v+1)/n*((1-u/n)*D_B+u/n*C_B))/255;

                // GPixel a_Pix= GPixel_PackARGB(a_A, a_R, a_G, a_B);
                // GPixel b_Pix= GPixel_PackARGB(b_A, b_R, b_G, b_B);
                // GPixel c_Pix= GPixel_PackARGB(c_A, c_R, c_G, c_B);
                // GPixel d_Pix= GPixel_PackARGB(d_A, d_R, d_G, d_B);
                GColor a_Col=GColor::MakeARGB(a_A, a_R, a_G, a_B);
                GColor b_Col=GColor::MakeARGB(b_A, b_R, b_G, b_B);
                GColor c_Col=GColor::MakeARGB(c_A, c_R, c_G, c_B);
                GColor d_Col=GColor::MakeARGB(d_A, d_R, d_G, d_B);

                vertexCol[i]=a_Col;
                vertexCol[i+1]=b_Col;
                vertexCol[i+2]=d_Col;
                vertexCol[i+3]=b_Col;
                vertexCol[i+4]=c_Col;
                vertexCol[i+5]=d_Col;

                i+=6;
            }
        }
        drawMesh(n*n*2, vertex, nullptr,vertexCol, nullptr, paint);
    }
    else if (colors == nullptr && tex != nullptr){
        GPoint vertexTex[n*n*2*3];
        GPoint ATex=tex[0];
        GPoint BTex=tex[1];
        GPoint CTex=tex[2];
        GPoint DTex=tex[3];
        i=0;
        for(float u=0;u<n;u++){
            for(float v=0;v<n;v++){
                GPoint aTex=(1-v/n)*((1-u/n)*ATex+u/n*BTex)+v/n*((1-u/n)*DTex+u/n*CTex);
                GPoint bTex=(1-v/n)*((1-(u+1)/n)*ATex+(u+1)/n*BTex)+v/n*((1-(u+1)/n)*DTex+(u+1)/n*CTex);
                GPoint cTex=(1-(v+1)/n)*((1-(u+1)/n)*ATex+(u+1)/n*BTex)+(v+1)/n*((1-(u+1)/n)*DTex+(u+1)/n*CTex);
                GPoint dTex=(1-(v+1)/n)*((1-u/n)*ATex+u/n*BTex)+(v+1)/n*((1-u/n)*DTex+u/n*CTex);

                vertexTex[i]=aTex;
                vertexTex[i+1]=bTex;
                vertexTex[i+2]=dTex;
                vertexTex[i+3]=bTex;
                vertexTex[i+4]=cTex;
                vertexTex[i+5]=dTex;
                i+=6;
            }
        }
        drawMesh(n*n*2, vertex, nullptr,nullptr, vertexTex, paint);
    }else if(colors != nullptr && tex != nullptr){
        GPoint vertexTex[n*n*2*3];
        GPoint ATex=tex[0];
        GPoint BTex=tex[1];
        GPoint CTex=tex[2];
        GPoint DTex=tex[3];

        GColor vertexCol[n*n*2*3];
        GColor ACol=colors[0];
        GColor BCol=colors[1];
        GColor CCol=colors[2];
        GColor DCol=colors[3];
        int A_A=GPixel_GetA(colToPix(ACol));
        int A_R=GPixel_GetR(colToPix(ACol));
        int A_G=GPixel_GetG(colToPix(ACol));
        int A_B=GPixel_GetB(colToPix(ACol));

        int B_A=GPixel_GetA(colToPix(BCol));
        int B_R=GPixel_GetR(colToPix(BCol));
        int B_G=GPixel_GetG(colToPix(BCol));
        int B_B=GPixel_GetB(colToPix(BCol));

        int C_A=GPixel_GetA(colToPix(CCol));
        int C_R=GPixel_GetR(colToPix(CCol));
        int C_G=GPixel_GetG(colToPix(CCol));
        int C_B=GPixel_GetB(colToPix(CCol));

        int D_A=GPixel_GetA(colToPix(DCol));
        int D_R=GPixel_GetR(colToPix(DCol));
        int D_G=GPixel_GetG(colToPix(DCol));
        int D_B=GPixel_GetB(colToPix(DCol));

        i=0;
        for(float u=0;u<n;u++){
            for(float v=0;v<n;v++){
                GPoint aTex=(1-v/n)*((1-u/n)*ATex+u/n*BTex)+v/n*((1-u/n)*DTex+u/n*CTex);
                GPoint bTex=(1-v/n)*((1-(u+1)/n)*ATex+(u+1)/n*BTex)+v/n*((1-(u+1)/n)*DTex+(u+1)/n*CTex);
                GPoint cTex=(1-(v+1)/n)*((1-(u+1)/n)*ATex+(u+1)/n*BTex)+(v+1)/n*((1-(u+1)/n)*DTex+(u+1)/n*CTex);
                GPoint dTex=(1-(v+1)/n)*((1-u/n)*ATex+u/n*BTex)+(v+1)/n*((1-u/n)*DTex+u/n*CTex);

                vertexTex[i]=aTex;
                vertexTex[i+1]=bTex;
                vertexTex[i+2]=dTex;
                vertexTex[i+3]=bTex;
                vertexTex[i+4]=cTex;
                vertexTex[i+5]=dTex;
                float a_A=((1-v/n)*((1-u/n)*A_A+u/n*B_A)+v/n*((1-u/n)*D_A+u/n*C_A))/255;
                float a_R=((1-v/n)*((1-u/n)*A_R+u/n*B_R)+v/n*((1-u/n)*D_R+u/n*C_R))/255;
                float a_G=((1-v/n)*((1-u/n)*A_G+u/n*B_G)+v/n*((1-u/n)*D_G+u/n*C_G))/255;
                float a_B=((1-v/n)*((1-u/n)*A_B+u/n*B_B)+v/n*((1-u/n)*D_B+u/n*C_B))/255;

                float b_A=((1-v/n)*((1-(u+1)/n)*A_A+(u+1)/n*B_A)+v/n*((1-(u+1)/n)*D_A+(u+1)/n*C_A))/255;
                float b_R=((1-v/n)*((1-(u+1)/n)*A_R+(u+1)/n*B_R)+v/n*((1-(u+1)/n)*D_R+(u+1)/n*C_R))/255;
                float b_G=((1-v/n)*((1-(u+1)/n)*A_G+(u+1)/n*B_G)+v/n*((1-(u+1)/n)*D_G+(u+1)/n*C_G))/255;
                float b_B=((1-v/n)*((1-(u+1)/n)*A_B+(u+1)/n*B_B)+v/n*((1-(u+1)/n)*D_B+(u+1)/n*C_B))/255;

                float c_A=((1-(v+1)/n)*((1-(u+1)/n)*A_A+(u+1)/n*B_A)+(v+1)/n*((1-(u+1)/n)*D_A+(u+1)/n*C_A))/255;
                float c_R=((1-(v+1)/n)*((1-(u+1)/n)*A_R+(u+1)/n*B_R)+(v+1)/n*((1-(u+1)/n)*D_R+(u+1)/n*C_R))/255;
                float c_G=((1-(v+1)/n)*((1-(u+1)/n)*A_G+(u+1)/n*B_G)+(v+1)/n*((1-(u+1)/n)*D_G+(u+1)/n*C_G))/255;
                float c_B=((1-(v+1)/n)*((1-(u+1)/n)*A_B+(u+1)/n*B_B)+(v+1)/n*((1-(u+1)/n)*D_B+(u+1)/n*C_B))/255;

                float d_A=((1-(v+1)/n)*((1-u/n)*A_A+u/n*B_A)+(v+1)/n*((1-u/n)*D_A+u/n*C_A))/255;
                float d_R=((1-(v+1)/n)*((1-u/n)*A_R+u/n*B_R)+(v+1)/n*((1-u/n)*D_R+u/n*C_R))/255;
                float d_G=((1-(v+1)/n)*((1-u/n)*A_G+u/n*B_G)+(v+1)/n*((1-u/n)*D_G+u/n*C_G))/255;
                float d_B=((1-(v+1)/n)*((1-u/n)*A_B+u/n*B_B)+(v+1)/n*((1-u/n)*D_B+u/n*C_B))/255;

                // GPixel a_Pix= GPixel_PackARGB(a_A, a_R, a_G, a_B);
                // GPixel b_Pix= GPixel_PackARGB(b_A, b_R, b_G, b_B);
                // GPixel c_Pix= GPixel_PackARGB(c_A, c_R, c_G, c_B);
                // GPixel d_Pix= GPixel_PackARGB(d_A, d_R, d_G, d_B);
                GColor a_Col=GColor::MakeARGB(a_A, a_R, a_G, a_B);
                GColor b_Col=GColor::MakeARGB(b_A, b_R, b_G, b_B);
                GColor c_Col=GColor::MakeARGB(c_A, c_R, c_G, c_B);
                GColor d_Col=GColor::MakeARGB(d_A, d_R, d_G, d_B);

                vertexCol[i]=a_Col;
                vertexCol[i+1]=b_Col;
                vertexCol[i+2]=d_Col;
                vertexCol[i+3]=b_Col;
                vertexCol[i+4]=c_Col;
                vertexCol[i+5]=d_Col;

                i+=6;
            }
        }

        drawMesh(n*n*2, vertex, nullptr,vertexCol, vertexTex, paint);



    }


}
private:
    const GBitmap fDevice;
};




std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    if (!device.pixels()) {
        return nullptr;
    }
    return std::unique_ptr<GCanvas>(new EmptyCanvas(device));
}