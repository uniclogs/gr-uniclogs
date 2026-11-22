build_flags := "-DCMAKE_INSTALL_PREFIX=/usr / -DCMAKE_BUILD_TYPE=Debug -DCMAKE_LINKER_TYPE=MOLD"
cmake_cmd := if os_family() == "windows" { "cmake .." } else { f"cmake ..{{build_flags}} " }
make_cmd := if os_family() == "windows" { "make" } else { "make -j $(nproc)" }

update_block mod:
  gr_modtool bind {{mod}}

build:
  #!/usr/bin/env bash
  mkdir build -p
  cd build
  {{ cmake_cmd }}
  {{ make_cmd }}

instl-build: build
	#!/usr/bin/env bash
	cd build
	sudo make install

install:
  #!/usr/bin/env bash
  cd build
  sudo make install

clean-build:
  rm -rf ./build
  mkdir build -p
  just build

test:
	#!/usr/bin/env bash
	cd ./test
	grcc FEC_encode_decode.grc
	python FEC_encode_decode.py
