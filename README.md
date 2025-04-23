# Music Catalog

* catalog metadata from audio files
* rename files based on structured metadata
* generate `.m3u8` playlists using query patterns

useful for organizing music libraries on playback devices or applications without feature-rich metadata querying, such as portable players or basic media apps (unlike foobar2000 desktop)

## Python version installation guide

### pyicu

```sh
export PATH="/opt/homebrew/Cellar/icu4c@77/77.1/bin:/opt/homebrew/Cellar/icu4c@77/77.1/sbin:$PATH"
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/opt/homebrew/Cellar/icu4c@77/77.1/lib/pkgconfig"
export CC="$(which gcc)" CXX="$(which g++)"
pip install --no-binary=:pyicu: pyicu
```

### pytaglib

```sh
# in pytaglib repo, or any other place
python build_native_taglib.py
# in your project
export TAGLIB_HOME="/path/to/taglib-cpp"
pip install --no-binary=pytaglib --no-cache-dir pytaglib
rm -r lib # can safely delete taglib built by build_native_taglib.py
```

## C++ version

about 10x faster
