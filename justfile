cmake_cmd := if os_family() == "windows" { "cmake .." } else { "cmake .. -DCMAKE_INSTALL_PREFIX=/usr" }
make_cmd := if os_family() == "windows" { "make" } else { "make -j $(nproc)" }

update_block mod:
  gr_modtool bind {{mod}}

build:
  #!/usr/bin/env bash
  mkdir build -p
  cd build
  {{ cmake_cmd }}
  {{ make_cmd }}

install: build
  #!/usr/bin/env bash
  cd build
  sudo make install

clean-build:
  rm -rf ./build
  mkdir build -p
  just build
