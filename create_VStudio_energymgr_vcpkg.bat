@ECHO OFF

CALL "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64

SET PATH=C:\Tools\cmake-3.31.8-windows-x86_64\bin;%PATH%;C:\Tools\Ninja

set VCPKG_ROOT=C:\Projects\Haus\energymgr\vcpkg
REM set PATH=%VCPKG_ROOT%;%PATH%

SET VCPKG_PATH=%VCPKG_ROOT%
SET VCPKG_INSTALL_PATH=%VCPKG_PATH%\installed\x64-windows

mkdir build_2022
cd build_2022

cmake -Wno-dev -G "Visual Studio 17 2022" -A x64 -DCMAKE_CONFIGURATION_TYPES=Release;RelWithDebInfo;Debug -DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%\scripts\buildsystems\vcpkg.cmake "-DCMAKE_VS_GLOBALS=UseMultiToolTask=true;EnforceProcessCountAcrossBuilds=true" ../src

cd ..
pause
