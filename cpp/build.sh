rm -rf build && mkdir build && cd build
# cmake .. -DICU_ROOT="$(brew --prefix icu4c)" -DCMAKE_BUILD_TYPE=RelWithDebInfo
# cmake .. -DICU_ROOT="$(brew --prefix icu4c)" -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make
