/**
 * 
 * Test program for decoding sample image with dmtx code
 * 
 * Author: Jakub Wlodek
 * Created on: March 29, 2019
 */


#include <stdio.h>
#include <dmtx.h>
#include <opencv/highgui.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace cv;
using namespace std;

int main(int argc, char** argv){

    DmtxImage* dmtxImage;
    DmtxDecode* dmtxDecode;
    DmtxRegion* dmtxRegion;
    size_t bytesPerPixel = 24;
    DmtxMessage* message;

    Mat im = imread("DMTXTest.png");

    dmtxImage = dmtxImageCreate(im.data, im.size().width, im.size().height, DmtxPack24bppRGB);
    assert(dmtxImage != NULL);

    dmtxDecode = dmtxDecodeCreate(dmtxImage, 1);
    assert(dmtxDecode != NULL);

    dmtxRegion = dmtxRegionFindNext(dmtxDecode, NULL);
    if(dmtxRegion != NULL){
        message = dmtxDecodeMatrixRegion(dmtxDecode, dmtxRegion, DmtxUndefined);
        if(message != NULL){
            fputs("output: \"", stdout);
            fwrite(message->output, sizeof(unsigned  char),  message->outputIdx, stdout);
            fputs("\"\n", stdout);
            dmtxMessageDestroy(&message);
        }
        dmtxRegionDestroy(&dmtxRegion);
    }

    dmtxDecodeDestroy(&dmtxDecode);
    dmtxImageDestroy(&dmtxImage);
    im.release();

    exit(0);

}