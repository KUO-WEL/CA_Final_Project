#include <iostream>
#include <chrono>

// 透過相對路徑引入生成的測資標頭檔
#include "../include/dataset.h"

int main() {
    std::cout << "=== Part 3: RVV Loop Unrolling (Factor = 2) ===" << std::endl;
    
    // 總共需要計算的次數為: 接收訊號長度 - 前導碼長度 + 1
    int num_windows = RX_LEN - PREAMBLE_LEN + 1;
    float* correlation_result = new float[num_windows];

    // 取得第 0 組接收訊號的起始指標
    const float* rx_pattern_0 = &RX_SIGNALS[0 * RX_LEN];
    
    // 用來記錄最大相關性與其對應的索引 (Peak Detection)
    float max_corr = -1e9; 
    int detected_idx = -1;

    std::cout << "\n開始執行 CPU RVV (迴圈展開) 向量運算..." << std::endl;

    // [加入 chrono 計時器] 
    auto st = std::chrono::high_resolution_clock::now();

    // ---------------------------------------------------------
    // RVV 雙層迴圈 (Loop Unrolling 實作)
    // ---------------------------------------------------------
    
    // 外層迴圈 i：代表滑動視窗在 Rx 訊號上的起始位置
    for (int i = 0; i < num_windows; ++i) {
        
        int avl = PREAMBLE_LEN;                 // 剩下需要處理的元素數量
        const float* ptr_p = PREAMBLE;          // Preamble 讀取指標
        const float* ptr_rx = &rx_pattern_0[i]; // Rx 讀取指標
        
        float final_sum = 0.0f;                 // 用來接最後結果的 C++ 變數
        float initial_sum = 0.0f;               // 歸約初始值 0.0f

        // 宣告一個長度為 32 的陣列，防止 vse32.v 越界寫入 (Stack Smashing)
        float reduction_result[32] = {0.0f};

        // 🚀 迴圈展開 (Unroll Factor = 2)
        while (avl > 0) {
            size_t vl; // 儲存硬體回傳的有效向量長度
            
            // ==========================================
            // 第一個運算區塊 (Unroll Block 1)
            // ==========================================
            asm volatile(
                "vsetvli %[vl], %[avl], e32, m1, ta, ma \n\t"
                "vle32.v v8, (%[ptr_p]) \n\t"
                "vle32.v v16, (%[ptr_rx]) \n\t"
                "vfmul.vv v24, v8, v16 \n\t"
                "flw f0, (%[init_sum_ptr]) \n\t"
                "vfmv.s.f v0, f0 \n\t"
                "vfredusum.vs v0, v24, v0 \n\t"
                "vse32.v v0, (%[res_ptr]) \n\t"
                : [vl] "=r" (vl)
                : [avl] "r" (avl), 
                  [ptr_p] "r" (ptr_p), 
                  [ptr_rx] "r" (ptr_rx),
                  [init_sum_ptr] "r" (&initial_sum), 
                  [res_ptr] "r" (reduction_result)
                : "f0", "memory" 
            );

            // 累加 Block 1 的結果並更新指標
            final_sum += reduction_result[0];
            ptr_p  += vl;
            ptr_rx += vl;
            avl    -= vl;

            // ==========================================
            // 第二個運算區塊 (Unroll Block 2)
            // ==========================================
            if (avl > 0) {
                asm volatile(
                    "vsetvli %[vl], %[avl], e32, m1, ta, ma \n\t"
                    "vle32.v v8, (%[ptr_p]) \n\t"
                    "vle32.v v16, (%[ptr_rx]) \n\t"
                    "vfmul.vv v24, v8, v16 \n\t"
                    "flw f0, (%[init_sum_ptr]) \n\t"
                    "vfmv.s.f v0, f0 \n\t"
                    "vfredusum.vs v0, v24, v0 \n\t"
                    "vse32.v v0, (%[res_ptr]) \n\t"
                    : [vl] "=r" (vl)
                    : [avl] "r" (avl), 
                      [ptr_p] "r" (ptr_p), 
                      [ptr_rx] "r" (ptr_rx),
                      [init_sum_ptr] "r" (&initial_sum), 
                      [res_ptr] "r" (reduction_result)
                    : "f0", "memory" 
                );
                
                // 累加 Block 2 的結果並更新指標
                final_sum += reduction_result[0];
                ptr_p  += vl;
                ptr_rx += vl;
                avl    -= vl;
            }
        }

        // 將該視窗的總和存入陣列
        correlation_result[i] = final_sum;

        // 同時尋找最大值 (找 Peak)
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