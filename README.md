# Building

In order to build gr-uniclogs you need:

* make
* CMake
* gr_modtool, used to update python bindings
* just (optional), used with the justfile as a command runner

```
mkdir build && cd build
cmake ..
make
```
To then install gr-uniclogs run:

```
sudo make install
```

The CMake will use the default installation prefix.
If you're using Linux, depending on your distrubution your installation path may vary.
Use the `CMAKE_INSTALL_PREFIX` flag to set your path.

```
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
```

## Modifying Code Then Building

If you have modified the function definitions of a block or class, you must update the python bindings.
Use `gr_modtool` to update the bindings.

For example updating the `frame_decoder` block with `gr_modtool`

```
gr_modtool bind frame_decoder
```

