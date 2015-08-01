![t6_logo](http://i.imgur.com/xHh9MAY.png)
-------

Torque 6 Editor is an MIT licensed editing suite for the [Torque 6](https://github.com/andr3wmac/Torque6/) engine.

Links
--------

 - [Torque 6](https://github.com/andr3wmac/Torque6/)
 - [Twitter](https://twitter.com/torque6engine)
 - [YouTube](https://www.youtube.com/channel/UCD--TmjTZU9FstD5yg4yKDg)
 - [Forums](http://forums.torque3d.org/viewforum.php?f=32)

Building
--------

Torque 6 Editor uses [GENie](https://github.com/bkaradzic/genie) to generate projects to build the editor and the engine.  The binaries and scripts are found in the build directory. Windows users can use build/GENERATE_VS**.BAT to generate Visual Studio projects. Torque 6 Editor has a dependency on wxWidgets. See "wxWidgets Setup" below for more information.

The engine folder is a git submodule of the main engine repo. You don't have to clone the engine, it will automatically be cloned and updated as a git submodule.

wxWidgets Setup (Windows)
--------

 - Downloaded and install [wxWidgets 3.0.2](http://sourceforge.net/projects/wxwindows/files/3.0.2/wxMSW-3.0.2-Setup.exe/download).
 - Set windows environment variable WXWIN to path of wxWidgets folder.
 - Find your matching Visual Studio version project in the wxWidgets folder under build/msw (Example: wx_vc12.sln)
 - Build both Debug and Release for Win32 and x64.
