pushd build/lin
clang -g -o sdfs ../../src/main.c -I../../inc -lSDL3
popd
