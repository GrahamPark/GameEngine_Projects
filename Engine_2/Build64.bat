call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" x64
cls

mkdir projects\helloworld\Build\x64
pushd projects\helloworld\Build\x64
cl -FC -Zi ..\..\helloworld.cpp User32.lib Gdi32.lib
popd