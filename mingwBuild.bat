:: Run this script if on windows and not using visual studio.

if not exist build mkdir build
cd build
cmake -S ../ -B . -G "MinGW Makefiles"
mingw32-make.exe && mingw32-make.exe Shaders
cd ../