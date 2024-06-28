# Requirements
- cmake^3.25
- OpenCV:
```sh
sudo apt-get install -y libopencv-dev
```
o
```sh
sudo pacman -S opencv glew hdf5 vtk
```

- Gdal:
```sh
sudo apt-get install -y libgdal-dev
```
o
```sh
sudo pacman -S gdal
```

# Build
```sh
mkdir build && cd build
cmake -GNinja -DRUN_TESTS=1 ..
ninja
```

# Run Tests
```sh 
ctest --test-dir tests -VV
```
