cd ..
make clean
make sim-cache
./sim-cache -config cache-config/cache-lru-nextline.cfg /cad2/ece552f/benchmarks/compress.eio
./sim-cache -redir:sim 1_nl.txt -config cache-config/cache-lru-nextline.cfg /cad2/ece552f/benchmarks/compress.eio
mv ./1_nl.txt ./files
./sim-cache -redir:sim 2_nl.txt -config cache-config/cache-lru-nextline.cfg /cad2/ece552f/benchmarks/gcc.eio
mv ./2_nl.txt ./files
./sim-cache -redir:sim 3_nl.txt -config cache-config/cache-lru-nextline.cfg /cad2/ece552f/benchmarks/go.eio
mv ./3_nl.txt ./files
cd files
