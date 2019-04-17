# ADPluginDmtx Releases

Author: Jakub Wlodek
Copyright (c): Brookhaven Science associates 2019

<!--RELEASE START-->

## R0-1 (17-April-2019)

* Initial release of ADPluginDmtx
* Features:
    * Decoding Data Matrix Barcodes
    * Barcode detected flag
    * Number Barcodes detected counter
    * Custom CSS screen
* Limitations
    * Currently only supports 8 bit RGB images. This can be circumvented by using ADCompVision and Convert Format.
    * Detection Uses Threads to allow the plugin to decode codes without blocking. However, this means there can be a slight delay to responsiveness of the plugin to the image displayed on the screen
