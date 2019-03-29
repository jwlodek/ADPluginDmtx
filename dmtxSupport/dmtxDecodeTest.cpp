/**
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

    // libdmtx structures
    DmtxImage* dmtxImage;
    DmtxDecode* dmtxDecode;
    DmtxRegion* dmtxRegion;
    size_t bytesPerPixel = 24;
    DmtxMessage* message;

    // read in test image with opencv
    Mat im = imread("DMTXTest.png");

    // initialize the image for dmtx
    dmtxImage = dmtxImageCreate(im.data, im.size().width, im.size().height, DmtxPack24bppRGB);
    assert(dmtxImage != NULL);

    // initialize the decoder
    dmtxDecode = dmtxDecodeCreate(dmtxImage, 1);
    assert(dmtxDecode != NULL);

    // find the region in which the data matrix code is found
    dmtxRegion = dmtxRegionFindNext(dmtxDecode, NULL);
    if(dmtxRegion != NULL){
        // if a region is found, decode it
        message = dmtxDecodeMatrixRegion(dmtxDecode, dmtxRegion, DmtxUndefined);
        if(message != NULL){
            // print to stdout
            fputs("output: \"", stdout);
            fwrite(message->output, sizeof(unsigned  char),  message->outputIdx, stdout);
            fputs("\"\n", stdout);
            dmtxMessageDestroy(&message);
        }
        dmtxRegionDestroy(&dmtxRegion);
    }

    // memory cleanup
    dmtxDecodeDestroy(&dmtxDecode);
    dmtxImageDestroy(&dmtxImage);
    im.release();

    exit(0);

}