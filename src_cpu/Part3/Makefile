GEM5_DIR := /root/gem5
GEM5 := $(GEM5_DIR)/build/RISCV/gem5.opt
CONFIG := $(GEM5_DIR)/configs/deprecated/example/se.py

CC := riscv64-linux-gnu-g++
CFLAGS := -static -O2 -march=rv64gcv

BIN_SCALAR := main_scalar
BIN_RVV := main_rvv
BIN_UNROLL := main_unroll

all: $(BIN_SCALAR) $(BIN_RVV) $(BIN_UNROLL)

$(BIN_SCALAR): main.cpp
	$(CC) $(CFLAGS) $< -o $@

$(BIN_RVV): main_rvv.cpp
	$(CC) $(CFLAGS) $< -o $@

$(BIN_UNROLL): main_unroll.cpp
	$(CC) $(CFLAGS) $< -o $@

run_scalar: $(BIN_SCALAR)
	$(GEM5) $(CONFIG) -c $(CURDIR)/$(BIN_SCALAR) --cpu-type=TimingSimpleCPU --mem-size=256MB --caches --l1i_size=32kB --l1i_assoc=8 --l1d_size=32kB --l1d_assoc=8 --cacheline=32
	grep -E "simSeconds|simTicks|numCycles|simInsts|simOps|hostSeconds|overallMisses|overallMissRate" m5out/stats.txt

run_rvv: $(BIN_RVV)
	$(GEM5) $(CONFIG) -c $(CURDIR)/$(BIN_RVV) --cpu-type=TimingSimpleCPU --mem-size=256MB --caches --l1i_size=32kB --l1i_assoc=8 --l1d_size=32kB --l1d_assoc=8 --cacheline=32
	grep -E "simSeconds|simTicks|numCycles|simInsts|simOps|hostSeconds|overallMisses|overallMissRate" m5out/stats.txt

run_unroll: $(BIN_UNROLL)
	$(GEM5) $(CONFIG) -c $(CURDIR)/$(BIN_UNROLL) --cpu-type=TimingSimpleCPU --mem-size=256MB --caches --l1i_size=32kB --l1i_assoc=8 --l1d_size=32kB --l1d_assoc=8 --cacheline=32
	grep -E "simSeconds|simTicks|numCycles|simInsts|simOps|hostSeconds|overallMisses|overallMissRate" m5out/stats.txt

clean:
	rm -f $(BIN_SCALAR) $(BIN_RVV) $(BIN_UNROLL)
	rm -rf m5out

.PHONY: all run_scalar run_rvv run_unroll clean