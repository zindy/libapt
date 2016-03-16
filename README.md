# libapt
An Open Source rewrite in C of the Thorlabs APT.dll based on D2XX

##What?
The idea is to re-implement all the functions defined in APTAPI.h using the FTDI D2XX library. APTAPI.h is copyright (c) Thorlabs.

This was my starting point:
http://stackoverflow.com/questions/20218961/getting-started-with-thorlabs-apt

##Why?
In micromanager, my [ThorlabsAPTStage adapter](https://micro-manager.org/wiki/ThorlabsAPTStage) relies on the APT.dll DLL provided by Thorlabs. The DLL is **32 bit only**. There is **no 64 bit APT.dll** that I know of (but if you don't believe me, do ask Thorlabs about it).

Which means:

* **The ThorlabsAPTStage device adapter will only work with a 32 bit Windows OS.**
* **The ThorlabsAPTStage device adapter will not work in 64 bit Windows.**
* **The ThorlabsAPTStage device adapter will not work in Linux (32 or 64 bit).**
* **The ThorlabsAPTStage device adapter will not work on Mac OSX (32 or 64 bit, either Intel, PowerPC or 68000 CPU).**

##So where are we at?
I got a few of the commands done, but if I remember, my code fell apart at MOT_MoveHome().

I was compiling the code with GCC / MinGW, not Visual Studio. If someone wants to write a Makefile / project file, send me a pull request.
Eventually, using the [libFTDI](http://www.intra2net.com/en/developer/libftdi/index.php) would be nice.

Unfortunately, due to other projects **I haven't had time to look into this for years.**

##Contrib files

* Download [from Thorlabs](https://www.thorlabs.de/software_pages/ViewSoftwarePage.cfm?Code=apt). In particular, the [protocol manual](https://www.thorlabs.de/software/apt/APT_Communications_Protocol_Rev_15.pdf) (v15).
* Download the D2XX drivers [from FTDI](http://www.ftdichip.com/Drivers/D2XX.htm).

**TL;DR** Do you want this fixed and finished? Please contribute. I will gladly accept any pull requests.

Cheers!
