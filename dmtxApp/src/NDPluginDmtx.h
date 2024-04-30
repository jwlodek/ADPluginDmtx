/**
 * ADPluginDmtx header file
 * 
 * Author: Jakub Wlodek
 * Created on: March 29, 2019
 * 
 */

#ifndef NDPluginDmtxH
#define NDPluginDmtxH

//Define necessary includes here

#include <dmtx.h>

using namespace std;

//include base plugin driver
#include "NDPluginDriver.h"

//version numbers
#define DMTX_VERSION 0
#define DMTX_REVISION 3
#define DMTX_MODIFICATION 0

#define NDPluginDmtxCodeFoundString "CODE_FOUND"     //asynParamInt32
#define NDPluginDmtxCodeMessageString "CODE_MESSAGE" //asynParamOctet
#define NDPluginDmtxNumberCodesString "NUMBER_CODES" //asynParamInt32


/* Struct representing all data required for dmtx decoding by ADPluginDmtx */
typedef struct AD_Dmtx_Data {
    // init all plugin-specific variables here
    DmtxImage *dmtxImage;
    DmtxDecode *dmtxDecode;
    DmtxRegion *dmtxRegion;
    DmtxMessage *message;
}ADDmtxData_t;


// define all necessary structs and enums here

/* Plugin class, extends plugin driver */
class NDPluginDmtx : public NDPluginDriver
{
  public:
    NDPluginDmtx(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr, int maxBuffers,
                 size_t maxMemory, int priority, int stackSize, int maxThreads);

    //~NDPlugin___();

    void processCallbacks(NDArray *pArray);
    void process_incoming_frame(NDArray *pArray);

    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

  protected:
    //in this section I define the coords of database vals

    int NDPluginDmtxNumberCodes;

    #define ND_DMTX_FIRST_PARAM NDPluginDmtxNumberCodes

    int NDPluginDmtxCodeMessage;

    int NDPluginDmtxCodeFound;

    #define ND_DMTX_LAST_PARAM NDPluginDmtxCodeFound

  private:

    // init all plugin additional functions here
    asynStatus init_dmtx_structs(NDArray *pArray, size_t width, size_t height, ADDmtxData_t* dmtxData);
    void clear_dmtx_structs(ADDmtxData_t* dmtxData);
    asynStatus decode_dmtx_image(ADDmtxData_t* dmtxData);
};

#define NUM_DMTX_PARAMS ((int)(&ND_DMTX_LAST_PARAM - &ND_DMTX_FIRST_PARAM + 1))

#endif