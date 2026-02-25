pushd lib/lin
clang++ -c ../../inc/imgui/*.cpp
ar rcs libimgui.a *.o
popd
