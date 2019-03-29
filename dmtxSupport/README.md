# ADPluginDmtx test programs + examples

ADPluginDmtx will use libdmtx for decodeing data matrix barcodes. This directory contains programs that are used to test 
your libdmtx intallation.

To install libdmtx on a debain based system with apt:
```
sudo apt-get install libdmtx-dev dmtx-utils
```

To generate an example image with a data matrix barcode:
```
echo -n Message | dmtxwrite > example.png
```