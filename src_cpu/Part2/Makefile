GEM5_DIR := /root/gem5
GEM5 := $(GEM5_DIR)/build/RISCV/gem5.opt
CONFIG := $(GEM5_DIR)/configs/deprecated/example/se.py

CC := riscv64-linux-gnu-g++
# 加上 -march=rv64gcv 來開啟 RVV 向量指令集的支援
CFLAGS := -static -O2 -march=rv64gcv

# 定義兩支目標程式
BIN_SCALAR := main_scalar
BIN_RVV := main_rvv

all: $(BIN_SCALAR) $(BIN_RVV)

# 編譯 Scalar 版本
$(BIN_SCALAR): main.cpp
	$(CC) $(CFLAGS) $< -o $@

# 編譯 RVV 版本
$(BIN_RVV): main_rvv.cpp
	$(CC) $(CFLAGS) $< -o $@

# 執行 Scalar 版本並印出報表
run_scalar: $(BIN_SCALAR)
	$(GEM5) $(CONFIG) -c $(CURDIR)/$(BIN_SCALAR) --cpu-type=TimingSimpleCPU --mem-size=256MB --caches --l1i_size=32kB --l1i_assoc=8 --l1d_size=32kB --l1d_assoc=8 --cacheline=32
	grep -E "simSeconds|simTicks|numCycles|simInsts|simOps|hostSeconds|overallMisses|overallMissRate" m5out/stats.txt

# 執行 RVV 版本並印出報表
run_rvv: $(BIN_RVV)
	$(GEM5) $(CONFIG) -c $(CURDIR)/$(BIN_RVV) --cpu-type=TimingSimpleCPU --mem-size=256MB --caches --l1i_size=32kB --l1i_assoc=8 --l1d_size=32kB --l1d_assoc=8 --cacheline=32
	grep -E "simSeconds|simTicks|numCycles|simInsts|simOps|hostSeconds|overallMisses|overallMissRate" m5out/stats.txt

clean:
	rm -f $(BIN_SCALAR) $(BIN_RVV)
	rm -rf m5out

.PHONY: all run_scalar run_rvv clean