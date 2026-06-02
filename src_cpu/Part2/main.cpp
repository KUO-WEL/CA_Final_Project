#include <iostream>
#include <chrono>

// 透過相對路徑引入生成的測資標頭檔
#include "../include/dataset.h"

int main() {
    std::cout << "=== Part 2: RVV Inline Assembly ===" << std::endl;
    
    int num_windows = RX_LEN - PREAMBLE_LEN + 1;
    float* correlation_result = new float[num_windows];

    const float* rx_pattern_0 = &RX_SIGNALS[0 * RX_LEN];
    
    float max_corr = -1e9; 
    int detected_idx = -1;

    std::cout << "\n開始執行 CPU RVV (Inline Assembly) 向量運算..." << std::endl;

    auto st = std::chrono::high_resolution_clock::now();

    // ---------------------------------------------------------
    // RVV 雙層迴圈 (Inline Assembly 實作)
    // ---------------------------------------------------------
    
    for (int i = 0; i < num_windows; ++i) {
        
        int avl = PREAMBLE_LEN;                 // 剩下需要處理的元素數量
        const float* ptr_p = PREAMBLE;          // Preamble 讀取指標
        const float* ptr_rx = &rx_pattern_0[i]; // Rx 讀取指標
        
        float final_sum = 0.0f;                 // 用來接最後結果的 C++ 變數
        float initial_sum = 0.0f;               // 歸約初始值 0.0f

        // 宣告一個長度為 1 的陣列，用來把結果從純量暫存器搬到記憶體，再交給 C++
        float reduction_result[32] = {0.0f}; // 開大一點，防止 vse32.v 越界寫入

        while (avl > 0) {
            size_t vl; // 儲存硬體回傳的有效向量長度
            
            // --- 核心 Inline Assembly 區塊 ---
            asm volatile(
                // 1. 設定向量長度與型態 (e32 代表 32-bit float, m1 代表使用 1 個暫存器群組)
                "vsetvli %[vl], %[avl], e32, m1, ta, ma \n\t"
                
                // 2. 向量載入 (Load)
                "vle32.v v8, (%[ptr_p]) \n\t"    // 從 ptr_p 載入 Preamble 到 v8
                "vle32.v v16, (%[ptr_rx]) \n\t"  // 從 ptr_rx 載入 Rx 到 v16
                
                // 3. 向量乘法 (Multiply)
                "vfmul.vv v24, v8, v16 \n\t"     // v24 = v8 * v16
                
                // 4. 準備歸約初始值 (將 initial_sum 放進純量浮點暫存器 f0，再搬到 v0 的第 0 個位置)
                "flw f0, (%[init_sum_ptr]) \n\t" 
                "vfmv.s.f v0, f0 \n\t"
                
                // 5. 向量歸約 (Reduction Sum)
                // 將 v24 的所有元素加總，再加上 v0[0]，結果存回 v0[0]
                "vfredusum.vs v0, v24, v0 \n\t"  
                
                // 6. 將結果搬回記憶體 (儲存 v0 的第一個元素到 reduction_result)
                "vse32.v v0, (%[res_ptr]) \n\t"

                // 輸出/輸入對應綁定
                : [vl] "=r" (vl)                                // 輸出 vl 到暫存器
                : [avl] "r" (avl),                              // 輸入 avl
                  [ptr_p] "r" (ptr_p),                          // 輸入 Preamble 指標
                  [ptr_rx] "r" (ptr_rx),                        // 輸入 Rx 指標
                  [init_sum_ptr] "r" (&initial_sum),            // 輸入初始值的記憶體位址
                  [res_ptr] "r" (reduction_result)              // 輸入存放結果的記憶體位址
                // 告訴編譯器我們弄髒了哪些暫存器，叫它不要亂用
                : "f0", "memory" 
            );

            // 把剛剛那輪的歸約結果累加到 final_sum 中
            final_sum += reduction_result[0];

            // 更新指標與剩下的元素數量
            ptr_p  += vl;
            ptr_rx += vl;
            avl    -= vl;
        }

        correlation_result[i] = final_sum;

        if (final_sum > max_corr) {
            max_corr = final_sum;
            detected_idx = i;
        }
    }

    auto ed = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = ed - st;

    // ---------------------------------------------------------
    // 驗證與結果輸出
    // ---------------------------------------------------------
    std::cout << "\n[運算結果]" << std::endl;
    std::cout << "偵測到的 Preamble 起始位置: " << detected_idx << std::endl;
    std::cout << "最大相關性數值 (Peak Value): " << max_corr << std::endl;
    std::cout << "CPU 執行時間: " << elapsed.count() * 1000 << " ms\n" << std::endl;

    if (detected_idx == GROUND_TRUTH[0]) {
        std::cout << "=> 驗證成功！演算法完美找到前導碼位置。" << std::endl;
    } else {
        std::cout << "=> 驗證失敗！找出的位置與正確解答不符。" << std::endl;
    }

    delete[] correlation_result;

    return 0;
}