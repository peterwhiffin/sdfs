pushd lib\win
cl /c /I"X:/Repos/SDL/include" ..\..\inc\imgui\*.cpp
lib /OUT:imgui.lib *.obj
popd
