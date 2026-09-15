update_block:
  gr_modtool bind ieee802_15_4_variant_decoder

build:
  #!/usr/bin/env bash
  cd build
  cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=RELEASE
  make -j $(nproc)

install:
  #!/usr/bin/env bash
  cd build
  sudo make install

plz:
  just build
  just install

clean-build:
  rm -rf ./build
  mkdir build
  just build

test:
  python ./test.py
