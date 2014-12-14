cd ..
make clean
make  sim-safe
gdb sim-safe
run -max:inst 1000000 /cad2/ece552f/benchmarks/gcc.eio
