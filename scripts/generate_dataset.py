import numpy as np
import os

# --- 參數設定 ---
PREAMBLE_LEN = 256      # 前導碼長度 (256 是一個適合放進 GPU Shared Memory 的大小)
RX_LEN = 4096           # 單一接收訊號的長度 (適中大小，避免 gem5 模擬跑太久)
NUM_PATTERNS = 8        # 接收訊號的數量 (Part 5 使用)
OUTPUT_FILE = "include/dataset.h" # 輸出 C 標頭檔的路徑

def generate_bpsk(length):
    """生成 BPSK 訊號 (+1.0 或 -1.0)"""
    return np.random.choice([-1.0, 1.0], size=length)

def main():
    print("開始生成測資...")
    
    # 1. 生成前導碼 (Preamble)
    preamble = generate_bpsk(PREAMBLE_LEN)
    
    # 2. 生成多組接收訊號 (Rx Signals)
    rx_signals = []
    ground_truths = []
    
    for i in range(NUM_PATTERNS):
        # 產生背景白雜訊(長度為 4096、平均值為 0、標準差為 0.5 的雜訊底噪)
        noise = np.random.normal(0, 0.5, RX_LEN)
        
        # 隨機決定前導碼要插入的位置 (確保有足夠空間塞入完整前導碼)
        insert_idx = np.random.randint(0, RX_LEN - PREAMBLE_LEN)
        ground_truths.append(insert_idx)
        
        # 疊加前導碼到雜訊中
        rx = noise.copy()
        rx[insert_idx:insert_idx + PREAMBLE_LEN] += preamble
        rx_signals.append(rx)
        print(f"Pattern {i} - Preamble inserted at index: {insert_idx}")

    # 3. 輸出為 C-style 標頭檔 (dataset.h)
    os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)
    with open(OUTPUT_FILE, "w") as f:
        f.write("#ifndef DATASET_H\n")
        f.write("#define DATASET_H\n\n")
        
        # 寫入巨集常數
        f.write(f"#define PREAMBLE_LEN {PREAMBLE_LEN}\n")
        f.write(f"#define RX_LEN {RX_LEN}\n")
        f.write(f"#define NUM_PATTERNS {NUM_PATTERNS}\n\n")
        
        # 寫入 Preamble 陣列 (宣告為 const 確保編譯器將其放入唯讀記憶體區段)
        f.write("const float PREAMBLE[PREAMBLE_LEN] = {\n    ")
        f.write(", ".join([f"{val:.1f}f" for val in preamble]))
        f.write("\n};\n\n")
        
        # 寫入多維 Rx 陣列 (攤平為 1D 陣列方便 C/CUDA 進行指標操作)
        f.write("const float RX_SIGNALS[NUM_PATTERNS * RX_LEN] = {\n")
        for i, rx in enumerate(rx_signals):
            f.write(f"    // Pattern {i}\n    ")
            # 每 16 個數字換一行，增加可讀性
            for j in range(0, RX_LEN, 16):
                chunk = rx[j:j+16]
                f.write(", ".join([f"{val:.4f}f" for val in chunk]))
                if j + 16 < RX_LEN:
                    f.write(",\n    ")
            if i < NUM_PATTERNS - 1:
                f.write(",\n")
            else:
                f.write("\n")
        f.write("};\n\n")
        
        # 寫入解答，方便你們在 C++ 中驗證演算法是否算對
        f.write("const int GROUND_TRUTH[NUM_PATTERNS] = {\n    ")
        f.write(", ".join([str(val) for val in ground_truths]))
        f.write("\n};\n\n")
        
        f.write("#endif // DATASET_H\n")
        
    print(f"測資生成完畢，已儲存至 {OUTPUT_FILE}")

if __name__ == "__main__":
    main()