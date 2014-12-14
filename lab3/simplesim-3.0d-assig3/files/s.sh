cd ..
make clean
make  sim-safe
sim-safe -max:inst 1000000  /cad2/ece552f/benchmarks/gcc.eio
sim-safe -max:inst 1000000  /cad2/ece552f/benchmarks/go.eio
sim-safe -max:inst 1000000  /cad2/ece552f/benchmarks/compress.eio
#sim-safe -redir:sim ./files/1.txt -max:inst 1000000  /cad2/ece552f/benchmarks/gcc.eio
#sim-safe -redir:sim ./files/2.txt -max:inst 1000000  /cad2/ece552f/benchmarks/go.eio
#sim-safe -redir:sim ./files/3.txt -max:inst 1000000  /cad2/ece552f/benchmarks/compress.eio
cd files
