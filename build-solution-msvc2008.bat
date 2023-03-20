@echo OFF

call "%PROGRAMFILES%\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"

IF "%BOOST_ROOT%"=="" set BOOST_ROOT=%SOCLE_EXTERN_ROOT%\win32-msvc2008sp1\boost_1_53_0

SET MY_DIR=%CD%
::SET LibEPG_ROOT=../libEPG

rmdir /Q /S solution-msvc2008 
mkdir solution-msvc2008 

cd solution-msvc2008  

cmake -G"Visual Studio 9 2008" -DCMAKE_INSTALL_PREFIX="%MY_DIR%" ..\

cd ..

PAUSE