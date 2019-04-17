/**
 * This file is a basic template for implementing areaDetector plugins.
 * You must implement all of the functions already listed here along with any 
 * additional plugin specific functions you require.
 * 
 * Author:
 * Created on:
 * 
 */



//include some standard libraries
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <thread>


//include epics/area detector libraries
#include <epicsMutex.h>
#include <epicsString.h>
#include <iocsh.h>
#include "NDArray.h"
// Include your plugin's header file here
#include "NDPluginDmtx.h"
#include <epicsExport.h>

// include your external dependency libraries here


//some basic namespaces
using namespace std;


// Name your plugin
static const char *pluginName="NDPluginDmtx";



/**
 * Override of NDPluginDriver function. Must be implemented by your plugin
 * 
 * @params[in]: pasynUser	-> pointer to asyn User that initiated the transaction
 * @params[in]: value		-> value PV was set to
 * @return: success if PV was updated correctly, otherwise error
 */
asynStatus NDPluginDmtx::writeInt32(asynUser* pasynUser, epicsInt32 value){
	const char* functionName = "writeInt32";
	int function = pasynUser->reason;
	asynStatus status = asynSuccess;

	status = setIntegerParam(function, value);
	asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s function = %d value=%d\n", pluginName, functionName, function, value);

    // replace PLUGINNAME with your plugin (ex. BAR)
	if(function < ND_DMTX_FIRST_PARAM){
		status = NDPluginDriver::writeInt32(pasynUser, value);
	}
	callParamCallbacks();
	if(status){
		asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error writing Int32 val to PV\n", pluginName, functionName);
	}
	return status;
}


asynStatus NDPluginDmtx::init_dmtx_structs(NDArray* pArray, size_t width, size_t height){
	const char* functionName = "init_dmtx_structs";
	int dataType, colorMode;
	getIntegerParam(NDDataType, &dataType);
	getIntegerParam(NDColorMode, &colorMode);
	NDDataType_t ndDataType = (NDDataType_t) dataType;
	NDColorMode_t ndColorMode = (NDColorMode_t) colorMode;
	DmtxPackOrder packOrder;
	if(ndColorMode == NDColorModeRGB1){
		switch(ndDataType){
			case NDUInt8:
				packOrder = DmtxPack24bppRGB;
				break;
			default:
				asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error unsupported data type + color mode combo\n", pluginName, functionName);
				return asynError;
		}
	}
	else{
		asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error unsupported data type + color mode combo\n", pluginName, functionName);
		return asynError;
	}
	this->dmtxImage = dmtxImageCreate((unsigned char*) pArray->pData, width, height, packOrder);
	if(this->dmtxImage == NULL){
		asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error unable to allocate image\n", pluginName, functionName);
		return asynError;
	}
	this->dmtxDecode = dmtxDecodeCreate(this->dmtxImage, 1);
	if(this->dmtxDecode == NULL){
		dmtxImageDestroy(&(this->dmtxImage));
		asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error unable to allocate decoder\n", pluginName, functionName);
		return asynError;
	}
	return asynSuccess;
}


asynStatus NDPluginDmtx::decode_dmtx_image(){
	const char* functionName = "decode_dmtx_image";
	this->dmtxRegion = dmtxRegionFindNext(this->dmtxDecode, NULL);
	if(this->dmtxRegion != NULL){
		this->message = dmtxDecodeMatrixRegion(this->dmtxDecode, this->dmtxRegion, DmtxUndefined);
		if(this->message != NULL){
			int number_codes;
			char decoded_message[256];
			//fputs("output: \"", stdout);
            //fwrite(this->message->output, sizeof(unsigned  char),  this->message->outputIdx, stdout);
            //fputs("\"\n", stdout);
			epicsSnprintf(decoded_message, sizeof(decoded_message), "%s", this->message->output);
			setStringParam(NDPluginDmtxCodeMessage, decoded_message);
			getIntegerParam(NDPluginDmtxNumberCodes, &number_codes);
			setIntegerParam(NDPluginDmtxNumberCodes, number_codes + 1);
			setIntegerParam(NDPluginDmtxCodeFound, 1);
		}
		else{
			dmtxRegionDestroy(&(this->dmtxRegion));
			setIntegerParam(NDPluginDmtxCodeFound, 0);
			return asynError;
		}
	}
	else{
		setIntegerParam(NDPluginDmtxCodeFound, 0);
		return asynDisabled;
	}
	return asynSuccess;
}

void static process_frame_wrapper(void* obj_instance, NDArray* pArray){
	NDPluginDmtx* pPlugin = (NDPluginDmtx*) obj_instance;
	pPlugin->process_incoming_frame(pArray);
}


void NDPluginDmtx::process_incoming_frame(NDArray* pArray){
	const char* functionName = "process_incoming_frame";
	//printf("started processing thread\n");
	NDArrayInfo arrayInfo;
	pArray->getInfo(&arrayInfo);
	asynStatus status = init_dmtx_structs(pArray, arrayInfo.xSize, arrayInfo.ySize);
	if(status == asynError) asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error, unable to initialize decoder\n", pluginName, functionName);
	else{
		status = decode_dmtx_image();
		if(status == asynDisabled) asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s No code found in image\n", pluginName, functionName);
		else if(status == asynSuccess){
			dmtxMessageDestroy(&(this->message));
		}
	}
	this->processing = false;
	//printf("finished processing thread\n");
}


/* Process callbacks function inherited from NDPluginDriver.
 * You must implement this function for your plugin to accept NDArrays
 *
 * @params[in]: pArray -> NDArray recieved by the plugin from the camera
 * @return: void
*/
void NDPluginDmtx::processCallbacks(NDArray *pArray){
	static const char* functionName = "processCallbacks";
	asynStatus status = asynSuccess;
	int code_found;

	//call base class and get information about frame
	NDPluginDriver::beginProcessCallbacks(pArray);

	//unlock the mutex for the processing portion
	this->unlock();

	if(!this->processing){

		this->processing = true;
		thread processing_thread(process_frame_wrapper, this, pArray);
		processing_thread.detach();

	}

	this->lock();

	if(status == asynError){
		asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Error, image not processed correctly\n", pluginName, functionName);
		return;
	}

	callParamCallbacks();
}



//constructror from base class
NDPluginDmtx::NDPluginDmtx(const char *portName, int queueSize, int blockingCallbacks,
		const char *NDArrayPort, int NDArrayAddr,
		int maxBuffers, size_t maxMemory,
		int priority, int stackSize)
		/* Invoke the base class constructor */
		: NDPluginDriver(portName, queueSize, blockingCallbacks,
		NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,
		asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
		asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
		ASYN_MULTIDEVICE, 1, priority, stackSize, 1)
{

	createParam(NDPluginDmtxCodeFoundString, asynParamInt32, &NDPluginDmtxCodeFound);
	createParam(NDPluginDmtxCodeMessageString, asynParamOctet, &NDPluginDmtxCodeMessage);
	createParam(NDPluginDmtxNumberCodesString, asynParamInt32, &NDPluginDmtxNumberCodes);

	char versionString[25];



	setStringParam(NDPluginDriverPluginType, "NDPluginDmtx");
	epicsSnprintf(versionString, sizeof(versionString), "%d.%d.%d", DMTX_VERSION, DMTX_REVISION, DMTX_MODIFICATION);
	setStringParam(NDDriverVersion, versionString);
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
		int priority, int stackSize){

	NDPluginDmtx *pPlugin = new NDPluginDmtx(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
		maxBuffers, maxMemory, priority, stackSize);
	return pPlugin->start();
}


/* IOC shell arguments passed to the plugin configure function */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
					&initArg1,
					&initArg2,
					&initArg3,
					&initArg4,
					&initArg5,
					&initArg6,
					&initArg7,
					&initArg8};


// Define the path to your plugin's extern configure function above
static const iocshFuncDef initFuncDef = {"NDDmtxConfigure",9,initArgs};


/* link the configure function with the passed args, and call it from the IOC shell */
static void initCallFunc(const iocshArgBuf *args){
	NDDmtxConfigure(args[0].sval, args[1].ival, args[2].ival,
			args[3].sval, args[4].ival, args[5].ival,
			args[6].ival, args[7].ival, args[8].ival);
}


/* function to register the configure function in the IOC shell */
extern "C" void NDDmtxRegister(void){
	iocshRegister(&initFuncDef,initCallFunc);
}


/* Exports plugin registration */
extern "C" {
	epicsExportRegistrar(NDDmtxRegister);
}
