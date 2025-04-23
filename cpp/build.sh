rm -rf build && mkdir build && cd build
cmake .. -DICU_ROOT="$(brew --prefix icu4c)"
# cmake .. -DICU_ROOT="$(brew --prefix icu4c)" -DCMAKE_PREFIX_PATH="$(brew --prefix)"
make
