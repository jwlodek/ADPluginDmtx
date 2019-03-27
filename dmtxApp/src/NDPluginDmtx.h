/**
 * Template header file fo NDPlugins
 * 
 * 
 * Author: 
 * Created on: 
 * 
 */

#ifndef NDPluginDmtxH
#define NDPluginDmtxH

//Define necessary includes here

using namespace std;

//include base plugin driver
#include "NDPluginDriver.h"

//version numbers
#define DMTX_VERSION      	0
#define DMTX_REVISION     	0
#define DMTX_MODIFICATION 	0



// Define the PVStrings for all of your PV values here in the following format
//#define NDPluginDmtxPVNameString 	"PV_STRING" 		//asynOctet


// define all necessary structs and enums here


/* Plugin class, extends plugin driver */
class NDPluginDmtx : public NDPluginDriver {
	public:
		NDPluginDmtx(const char *portName, int queueSize, int blockingCallbacks,
			const char* NDArrayPort, int NDArrayAddr, int maxBuffers,
			size_t maxMemory, int priority, int stackSize);

		//~NDPlugin___();

		void processCallbacks(NDArray *pArray);

		virtual asynStatus writeInt32(asynUser* pasynUser, epicsInt32 value);

	protected:

		//in this section i define the coords of database vals

		//Place PV indexes here, define first and last as appropriate, replace PLUGINNAME with name, 
		#define ND_DMTX_FIRST_PARAM FIRSTPVINDEX
		#define ND_DMTX_LAST_PARAM LASTPVINDEX

	private:

        // init all global variables here

        // init all plugin additional functions here

};

// Replace PLUGINNAME with plugin name ex. BAR
#define NUM_DMTX_PARAMS ((int)(&ND_DMTX_LAST_PARAM - &ND_DMTX_FIRST_PARAM + 1))

#endif