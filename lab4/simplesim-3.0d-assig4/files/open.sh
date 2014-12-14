cd ..
make clean
make sim-cache
./sim-cache -redir:sim 1.txt -config cache-config/cache-lru-open.cfg /cad2/ece552f/benchmarks/compress.eio
mv ./1.txt ./files
./sim-cache -redir:sim 2.txt -config cache-config/cache-lru-open.cfg /cad2/ece552f/benchmarks/gcc.eio
mv ./2.txt ./files
./sim-cache -redir:sim 3.txt -config cache-config/cache-lru-open.cfg /cad2/ece552f/benchmarks/go.eio
mv ./3.txt ./files
cd files
