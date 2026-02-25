pushd build\win
clang -g -o sdfs.exe ../../src/main.c -I../../inc -L../../lib/win -lSDL3.lib -limgui.lib
popd
