mkdir -p _build
cd _build
../configure --host=m68k-amigaos --build=i686-pc-linux-gnu --with-cpu=68060
make bindist -j12
