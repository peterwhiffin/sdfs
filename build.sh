pushd build/lin
clang -g -o sdfs ../../src/main.c -I../../inc -L../../lib/lin -limgui -lstdc++ -lSDL3 -lm
popd
