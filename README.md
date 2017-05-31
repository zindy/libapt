# libapt
An Open Source rewrite in C of the Thorlabs APT.dll based on the [APT communication protocol](https://www.thorlabs.com/Software/Motion%20Control/APT_Communications_Protocol_Rev_19.pdf) and the [libftdi1](https://www.intra2net.com/en/developer/libftdi/) library for low-level communication.

## What?
The idea is to re-implement all the functions defined in the APTAPI.h protocol file, or at least the ones we need to make the micro-manager [ThorlabsAPTStage](https://micro-manager.org/wiki/ThorlabsAPTStage) device adapter work. By using [libftdi1](https://www.intra2net.com/en/developer/libftdi/) and by re-using the APTAPI.h protocol file, replacing the APT.dll file provided by Thorlabs with the one provided here should be *relatively* straightforward. APTAPI.h is copyright (c) Thorlabs.

* [This was my starting point](http://stackoverflow.com/questions/20218961/getting-started-with-thorlabs-apt) when I first explored the possibility of rewriting the library (using D2XX).
* Tim Rae independently reimplemented the APT communication protocol in his [Python library](https://github.com/timrae/drivepy/tree/master/thorlabs).
* Tobias Gehring wrote some ctypes [python bindings](https://github.com/qpit/thorlabs_apt) for APT.dll.

## Why?
In micromanager, my [ThorlabsAPTStage adapter](https://micro-manager.org/wiki/ThorlabsAPTStage) relies on the APT.dll DLL provided by Thorlabs. The DLL although now available both for [32 bit Windows](https://micro-manager.org/wiki/File:APT.zip) as well as [64 bit Windows](https://micro-manager.org/wiki/File:APT_x64.zip) still ties the APT device adapter exclusively to the Windows platform.

Which means:

* **The ThorlabsAPTStage device adapter will only work with a Windows OS.**
* **The ThorlabsAPTStage device adapter only works with the TDC001 controller which is now obsolete.**
* **The ThorlabsAPTStage device adapter will not work in Linux (32 or 64 bit).**
* **The ThorlabsAPTStage device adapter will not work on Mac OSX (32 or 64 bit, either Intel, PowerPC or 68000 CPU).**

## So where are we at with the opensource adapter?
I got a few of the commands done, but there are still many I'm stuck on. On the TDC001 controller, MGMSG_MOT_REQ_PMDSTAGEAXISPARAMS does not return any information at all, for example. Time to look at some USB traffic with Wireshark methinks...

Still, you should be able to initialize the stage, home it, set positions, etc. The library, although far from being complete already shows some promise.

Other things I still need to look at: I have a very limited understanding of Autoconf. If someone wants to help with that, please send me a pull request. I think I wrote enough code to do a autoreconf -fvi;./configure;make;make install but there are no usable Python SWIG bindings, and I haven't worked-out yet how to reintegrate the main() part of my test_main.c with the libapt library.

As I said before, I'll spare whatever time I can on this project but don't expect too much. For now if you want to test this, you can type (to make libapt.so and install it):

```
autoreconf -fvi
./configure
make
sudo make install
```

Then if you want to run the sample application, type:
```
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"
gcc -o test_main src/test_main.c -lapt
./test_main
```

We're getting close everyone! :-)

## Contrib files
* Download [from Thorlabs](https://www.thorlabs.de/software_pages/ViewSoftwarePage.cfm?Code=apt). In particular, the [protocol manual](https://www.thorlabs.de/software/apt/APT_Communications_Protocol_Rev_19.pdf) (v19).
* Install libftdi1 on your platform.

**TL;DR** Do you want this fixed and finished? Please contribute. I will gladly accept any pull requests.

Cheers!
