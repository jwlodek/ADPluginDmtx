/**
 * This file is a basic template for implementing areaDetector plugins.
 * You must implement all of the functions already listed here along with any 
 * additional plugin specific functions you require.
 * 
 * Author: Jakub Wlodek
 * Corresponding Author: Kazimierz Gofron
 * Copyright (c): Brookhaven Science Associates 2019
 * Created on: 27-March-2019
 * 
 */


//include some standard libraries
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <thread>


//include epics/area detector libraries
#include <epicsMutex.h>
#include <epicsString.h>
#include <iocsh.h>
#include "NDArray.h"


// Include your plugin's header file here
#include <epicsExport.h>
#include "NDPluginDmtx.h"


// include your external dependency libraries here
// Not needed - all included in NDPluginDmtx.h

//some basic namespaces
using namespace std;

// Name your plugin
static const char *pluginName = "NDPluginDmtx";



/**
 * Override of NDPluginDriver function. Must be implemented by your plugin
 * 
 * @params[in]: pasynUser	-> pointer to asyn User that initiated the transaction
 * @params[in]: value		-> value PV was set to
 * @return: success if PV was updated correctly, otherwise error
 */
asynStatus NDPluginDmtx::writeInt32(asynUser *pasynUser, epicsInt32 value) {
    const char *functionName = "writeInt32";
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    status = setIntegerParam(function, value);
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s function = %d value=%d\n", pluginName, functionName, function, value);

    // replace PLUGINNAME with your plugin (ex. BAR)
    if (function < ND_DMTX_FIRST_PARAM) {
        status = NDPluginDriver::writeInt32(pasynUser, value);
    }
    callParamCallbacks();
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error writing Int32 val to PV\n", pluginName, functionName);
    }
    return status;
}


/**
 * Function that initializes libdmtx structs used for decoding thedata matrix barcodes found in
 * the image. Structures initialized include DmtxImage and DmtxDecode
 * 
 * @params[in]: pArray  -> Pointer to input array
 * @params[in]: width   -> width of the input array
 * @params[in]: height  -> height of the input array
 * @params[out]: dmtxData -> A structure containing initialized dmtx structs
 * @return: asynError if couldn't initialize structs, otherwise success
 */
asynStatus NDPluginDmtx::init_dmtx_structs(NDArray *pArray, size_t width, size_t height, ADDmtxData_t* dmtxData) {
    const char *functionName = "init_dmtx_structs";
    int dataType, colorMode;
    getIntegerParam(NDDataType, &dataType);
    getIntegerParam(NDColorMode, &colorMode);
    NDDataType_t ndDataType = (NDDataType_t)dataType;
    NDColorMode_t ndColorMode = (NDColorMode_t)colorMode;
    DmtxPackOrder packOrder;
    if (ndColorMode == NDColorModeRGB1) {
        switch (ndDataType) {
            case NDUInt8:
                packOrder = DmtxPack24bppRGB;
                break;
            default:
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error unsupported data type + color mode combo\n", pluginName, functionName);
                return asynError;
        }
    } else {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error unsupported data type + color mode combo\n", pluginName, functionName);
        return asynError;
    }
    dmtxData->dmtxImage = dmtxImageCreate((unsigned char *)pArray->pData, width, height, packOrder);
    if (dmtxData->dmtxImage == NULL) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error unable to allocate image\n", pluginName, functionName);
        return asynError;
    }
    dmtxData->dmtxDecode = dmtxDecodeCreate(dmtxData->dmtxImage, 1);
    if (dmtxData->dmtxDecode == NULL) {
        dmtxImageDestroy(&(dmtxData->dmtxImage));
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error unable to allocate decoder\n", pluginName, functionName);
        return asynError;
    }
    return asynSuccess;
}


/**
 * Function that frees memory allocated to the
 * libdmtx structs used in the decoding process
 * 
 * @params[in]: dmtxData -> structure containing all dmtx data
 * @return: void
 */
void NDPluginDmtx::clear_dmtx_structs(ADDmtxData_t* dmtxData) {
    dmtxMessageDestroy(&(dmtxData->message));
    dmtxRegionDestroy(&(dmtxData->dmtxRegion));
    dmtxDecodeDestroy(&(dmtxData->dmtxDecode));
    dmtxImageDestroy(&(dmtxData->dmtxImage));
}


/** 
 * Main function used for decoding the dmtx image.
 * Uses previously initialized structs to identify the possible next code location,
 * decode it, and update the appropriate PV values for code found, message, and
 * number codes.
 * 
 * @return: asynDisabled if no region found, asynError if found but cannot decode message, and success otherwise
 */
asynStatus NDPluginDmtx::decode_dmtx_image(ADDmtxData_t* dmtxData) {
    dmtxData->dmtxRegion = dmtxRegionFindNext(dmtxData->dmtxDecode, NULL);
    if (dmtxData->dmtxRegion != NULL) {
        dmtxData->message = dmtxDecodeMatrixRegion(dmtxData->dmtxDecode, dmtxData->dmtxRegion, DmtxUndefined);
        if (dmtxData->message != NULL) {
            int number_codes;
            char decoded_message[256];
            char current_message[256];
            epicsSnprintf(decoded_message, sizeof(decoded_message), "%s", dmtxData->message->output);
            getStringParam(NDPluginDmtxCodeMessage, 256, current_message);
            setStringParam(NDPluginDmtxCodeMessage, decoded_message);
            if(strcmp(decoded_message, current_message) != 0){
                getIntegerParam(NDPluginDmtxNumberCodes, &number_codes);
                setIntegerParam(NDPluginDmtxNumberCodes, number_codes + 1);
            }
            setIntegerParam(NDPluginDmtxCodeFound, 1);
        } else {
            setIntegerParam(NDPluginDmtxCodeFound, 0);
            return asynError;
        }
    } else {
        setIntegerParam(NDPluginDmtxCodeFound, 0);
        return asynDisabled;
    }
    return asynSuccess;
}


/**
 * Function that processes each incoming frame (in the processing thread)
 * First, gets input array info, then inits dmtx structs. Then decodes the image, 
 * finally clears the structs
 * 
 * @params[in]: pArray -> pointer to the input array
 * @return: void
 */
void NDPluginDmtx::process_incoming_frame(NDArray *pArray) {
    const char *functionName = "process_incoming_frame";
    NDArrayInfo arrayInfo;
    pArray->getInfo(&arrayInfo);
    ADDmtxData_t* dmtxData = (ADDmtxData_t*) malloc(sizeof(ADDmtxData_t));
    asynStatus status = init_dmtx_structs(pArray, arrayInfo.xSize, arrayInfo.ySize, dmtxData);
    if (status == asynError)
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error, unable to initialize decoder\n", pluginName, functionName);
    else {
        status = decode_dmtx_image(dmtxData);
        clear_dmtx_structs(dmtxData);
        if (status == asynDisabled)
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s No code found in image\n", pluginName, functionName);
        else if(status == asynError)
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Unable to decode discovered code\n", pluginName, functionName);
    }
    free(dmtxData);
}


/* Process callbacks function inherited from NDPluginDriver.
 * You must implement this function for your plugin to accept NDArrays
 *
 * @params[in]: pArray -> NDArray recieved by the plugin from the camera
 * @return: void
*/
void NDPluginDmtx::processCallbacks(NDArray *pArray) {
    //static const char *functionName = "processCallbacks";

    //call base class and get information about frame
    NDPluginDriver::beginProcessCallbacks(pArray);

    //unlock the mutex for the processing portion
    this->unlock();

    process_incoming_frame(pArray);

    this->lock(); 

    NDPluginDriver::endProcessCallbacks(pArray, true, true);
    
    // Update PV values
    callParamCallbacks();
}


/**
 * Constructor from base class. All parameters are simply passed to the 
 * NDPluginDriver superclass constructor. Then Dmtx PVs are initialized, the version
 * number is set, and the plugin connects to its array port.
 */
NDPluginDmtx::NDPluginDmtx(const char *portName, int queueSize, int blockingCallbacks,
                           const char *NDArrayPort, int NDArrayAddr,
                           int maxBuffers, size_t maxMemory,
                           int priority, int stackSize, int maxThreads)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                     NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,
                     asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                     asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                     ASYN_MULTIDEVICE, 1, priority, stackSize, maxThreads) {

    // Initialize the PVs
    createParam(NDPluginDmtxCodeFoundString,    asynParamInt32,     &NDPluginDmtxCodeFound);
    createParam(NDPluginDmtxCodeMessageString,  asynParamOctet,     &NDPluginDmtxCodeMessage);
    createParam(NDPluginDmtxNumberCodesString,  asynParamInt32,     &NDPluginDmtxNumberCodes);

    // Set the version number
    char versionString[25];
    setStringParam(NDPluginDriverPluginType, "NDPluginDmtx");
    epicsSnprintf(versionString, sizeof(versionString), "%d.%d.%d", DMTX_VERSION, DMTX_REVISION, DMTX_MODIFICATION);
    setStringParam(NDDriverVersion, versionString);

    // Connect to the array port
    connectToArrayPort();
}


/**
 * External configure function. This will be called from the IOC shell of the
 * detector the plugin is attached to, and will create an instance of the plugin and start it
 * 
 * @params[in]	-> all passed to constructor
 */
extern "C" int NDDmtxConfigure(const char *portName, int queueSize, int blockingCallbacks,
                               const char *NDArrayPort, int NDArrayAddr,
                               int maxBuffers, size_t maxMemory,
                               int priority, int stackSize, int maxThreads) {

    // Create new plugin instance
    NDPluginDmtx *pPlugin = new NDPluginDmtx(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                                             maxBuffers, maxMemory, priority, stackSize, maxThreads);
    // try to start the plugin
    return pPlugin->start();
}


/* IOC shell arguments passed to the plugin configure function */
static const iocshArg initArg0 = {"portName", iocshArgString};
static const iocshArg initArg1 = {"frame queue size", iocshArgInt};
static const iocshArg initArg2 = {"blocking callbacks", iocshArgInt};
static const iocshArg initArg3 = {"NDArrayPort", iocshArgString};
static const iocshArg initArg4 = {"NDArrayAddr", iocshArgInt};
static const iocshArg initArg5 = {"maxBuffers", iocshArgInt};
static const iocshArg initArg6 = {"maxMemory", iocshArgInt};
static const iocshArg initArg7 = {"priority", iocshArgInt};
static const iocshArg initArg8 = {"stackSize", iocshArgInt};
static const iocshArg initArg9 = {"maxThreads", iocshArgInt};
static const iocshArg *const initArgs[] = {&initArg0,
                                           &initArg1,
                                           &initArg2,
                                           &initArg3,
                                           &initArg4,
                                           &initArg5,
                                           &initArg6,
                                           &initArg7,
                                           &initArg8,
                                           &initArg9};


// Define the path to your plugin's extern configure function above
static const iocshFuncDef initFuncDef = {"NDDmtxConfigure", 10, initArgs};


/* link the configure function with the passed args, and call it from the IOC shell */
static void initCallFunc(const iocshArgBuf *args) {
    NDDmtxConfigure(args[0].sval, args[1].ival, args[2].ival,
                    args[3].sval, args[4].ival, args[5].ival,
                    args[6].ival, args[7].ival, args[8].ival, args[9].ival);
}


/* function to register the configure function in the IOC shell */
extern "C" void NDDmtxRegister(void) {
    iocshRegister(&initFuncDef, initCallFunc);
}


/* Exports plugin registration */
extern "C" {
    epicsExportRegistrar(NDDmtxRegister);
}
