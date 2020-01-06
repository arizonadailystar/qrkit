QR Kit
======

Generate QR codes with a variety of features using an input configuration JSON file.

Features
--------

* Control the pattern of the dots and style of the alignment corners.
* Control the quality, padding, and scale.
* Customize various colors.
* Embed an icon into the generated QR code.
* Command-line application.
* Ultra-fast QR code generation at any quality level.

Build Instructions
------------------

In the `src` directory, run:

```
make
```

Upon successful compile, run in the parent directory:

```
./qrkit --usage
```

To get a list of options.

JSON Options
------------

* minECL - A string containing one of "L", "M", "Q", or "H".  The minimum Error Correction Level.  Default is "L".
* padding - An integer containing the number of pixels of padding around the QR code.  Default is 10.
* scale - An integer containing the scale of each dot.  Default is 10.
* background - A string containing a HTML hex color for the background.  Default is "#FFFFFF".
* pattern - A string containing a HTML hex color for the pattern.  Default is "#000000".
* align - A string containing a HTML hex color for the alignment.  Default is "#000000".
* code - A string containing a HTML hex color for the code.  Default is "#000000".
* icon - A string containing a HTML hex color for the center icon, if used.  Default is "#000000".
* style - A string containing one of "none", "dots", "hdots", "vdots", or "hvdots".  Default is "none".
* patstyle - A string containing one of "none", "rounded", "circle".  Default is "none".
* corners - An array of strings containing one or more of "tl", "tr", "bl", "br".  Only used when patstyle is "rounded".  Default is none.
