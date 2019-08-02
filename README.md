# ADPluginDmtx

Author: Jakub Wlodek  
Corresponding author: Kazimierz Gofron  
Copyright (c): Brookhaven Science Associates 2019

An EPICS areaDetector Plugin for reading data matrix barcodes.

**NOTE: This plugin was built with the help of [ADPluginTemplate](https://github.com/epicsNSLS2-areaDetector/ADPluginTemplate)**

### Installation

To install ADPluginDmtx, start by installing its dependencies: `libdmtx`, `ADSupport`, and `ADCore`. Libdmtx can be installed on a debian-based system through apt:
```
sudo apt install libdmtx-dev
```
Alternatively, build it from [source](https://github.com/dmtx/libdmtx).

Next, enter `areaDetector/configure`, and edit the following file:

In `RELEASE_PRODS.local` add:
```
ADPLUGINDMTX=$(AREA_DETECTOR)/ADPluginDmtx
```

Next, open `areaDetector/ADCore/iocBoot`, and edit `commonPlugins.cmd`:
```
NDDmtxConfigure("DMTX1", $(QSIZE), 0, "$(PORT)", 0, 0, 0, 0, 0, $(MAX_THREADS=5))
dbLoadRecords("$(ADPLUGINDMTX)/db/NDPluginDmtx.template", "P=$(PREFIX), R=Dmtx1:, PORT=DMTX1, ADDR=0, TIMEOUT=1, NDARRAY_PORT=$(PORT), NAME=DMTX1, NCHANS=$(XSIZE)")
set_requestfile_path("$(ADPLUGINDMTX)/adcvApp/Db")
```

Finally, in `areaDetector/ADCore/ADApp`, edit `commonDriverMakefile`:
```
ifdef ADPLUGINDMTX
  $(DBD_NAME)_DBD  += NDPluginDmtx.dbd
  PROD_LIBS	+= NDPluginDmtx
  ifdef DMTX_LIB
    dmtx_DIR += $(DMTX_LIB)
    PROD_LIBS       += dmtx
  else
    PROD_SYS_LIBS   += dmtx
  endif
endif
```

You may now build `ADCore`, `ADPluginDmtx`, and any driver in that order. Remember to add ADPluginDmtx to the `envPaths` before running the driver IOC.

### Usage

ADPluginDmtx currently only supports 8 bit RGB images. This limitation can be circumvented by usng [ADCompVision](https://github.com/areaDetector/ADCompVision), and its Convert Format function. To use ADPluginDmtx, simply run the driver IOC, pass the array port to ADPluginDmtx, and enable the plugin. If a code is detected, it will be decoded, the counter will be incremented, and the code detected flag will be set to `YES`. From here, you may grab the code messgae from the appropriate PV.
