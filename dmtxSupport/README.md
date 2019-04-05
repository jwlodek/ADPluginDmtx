# ADPluginDmtx test programs + examples

ADPluginDmtx will use libdmtx for decodeing data matrix barcodes. This directory contains programs that are used to test 
your libdmtx intallation.

To install libdmtx on a debian based system with apt:
```
sudo apt-get install libdmtx-dev dmtx-utils
```
To run the example program compile with:
```
make
```
and run with
```
./test_decode
```
The output should read: `output: "XF28ID-DMTXTest"`