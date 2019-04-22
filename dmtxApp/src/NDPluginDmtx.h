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
#define DMTX_REVISION 1
#define DMTX_MODIFICATION 0

#define NDPluginDmtxCodeFoundString "CODE_FOUND"     //asynParamInt32
#define NDPluginDmtxCodeMessageString "CODE_MESSAGE" //asynParamOctet
#define NDPluginDmtxNumberCodesString "NUMBER_CODES" //asynParamInt32

// define all necessary structs and enums here


typedef struct NDDmtxJobProcessor {
    thread processing_thread;
    NDArray* pScratch;
    bool processing;
} NDDmtxJobProcessor_t;


/* Plugin class, extends plugin driver */
class NDPluginDmtx : public NDPluginDriver
{
  public:
    NDPluginDmtx(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr, int maxBuffers,
                 size_t maxMemory, int priority, int stackSize);

    ~NDPluginDmtx();

    void processCallbacks(NDArray *pArray);
    void process_incoming_frame(NDArray *pArray);
    void process_thread_loop();

    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

  protected:
    //in this section i define the coords of database vals

    //Place PV indexes here, define first and last as appropriate, replace PLUGINNAME with name,

    int NDPluginDmtxNumberCodes;

    #define ND_DMTX_FIRST_PARAM NDPluginDmtxNumberCodes

    int NDPluginDmtxCodeMessage;

    int NDPluginDmtxCodeFound;

    #define ND_DMTX_LAST_PARAM NDPluginDmtxCodeFound

  private:
    // init all global variables here
    DmtxImage *dmtxImage;
    DmtxDecode *dmtxDecode;
    DmtxRegion *dmtxRegion;
    DmtxMessage *message;

    // variables for processing thread
    bool run_processing_thread;

    NDDmtxJobProcessor_t dmtxProcessor;

    // init all plugin additional functions here

    asynStatus init_dmtx_structs(NDArray *pArray, size_t width, size_t height);
    asynStatus decode_dmtx_image();
};

// Replace PLUGINNAME with plugin name ex. BAR
#define NUM_DMTX_PARAMS ((int)(&ND_DMTX_LAST_PARAM - &ND_DMTX_FIRST_PARAM + 1))

#endif