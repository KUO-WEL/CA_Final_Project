#include <iostream>
#include <chrono>

// 透過相對路徑引入剛剛生成的測資標頭檔
// 因為 main.cpp 在 src_cpu/ 內，所以要用 ../ 回到上一層找 include/
#include "../include/dataset.h"

int main() {
    std::cout << "=== Part 1: Scalar Baseline ===" << std::endl;
    
    // 驗證巨集與陣列是否正確讀取
    std::cout << "前導碼長度 (PREAMBLE_LEN): " << PREAMBLE_LEN << std::endl;
    std::cout << "接收訊號長度 (RX_LEN): " << RX_LEN << std::endl;
    std::cout << "總測試 Pattern 數量: " << NUM_PATTERNS << std::endl;

    std::cout << "\n[驗證資料]" << std::endl;
    std::cout << "Preamble[0] = " << PREAMBLE[0] << std::endl;
    
    // 取得第 0 組接收訊號的起始指標
    const float* rx_pattern_0 = &RX_SIGNALS[0 * RX_LEN];
    std::cout << "Rx_Signal_0[0] = " << rx_pattern_0[0] << std::endl;
    
    // 對答案用的正確插入位置
    std::cout << "Pattern 0 的正確前導碼起始位置: " << GROUND_TRUTH[0] << std::endl;

    // ---------------------------------------------------------
    // Part 1 的雙層迴圈 (Cross-Correlation)
    // ---------------------------------------------------------
    
    // 準備一個陣列來儲存每一個滑動視窗位置的運算結果
    // 總共需要計算的次數為: 接收訊號長度 - 前導碼長度 + 1
    int num_windows = RX_LEN - PREAMBLE_LEN + 1;
    float* correlation_result = new float[num_windows];

    // 用來記錄最大相關性與其對應的索引 (Peak Detection)
    float max_corr = -1e9; // 初始設為極小值
    int detected_idx = -1;

    std::cout << "\n開始執行 CPU Scalar Baseline 運算..." << std::endl;

    // [加入專題要求使用的 chrono 計時器] 
    auto st = std::chrono::high_resolution_clock::now();

    // 外層迴圈 i：代表滑動視窗在 Rx 訊號上的起始位置
    for (int i = 0; i < num_windows; ++i) {
        
        float sum = 0.0f; // 歸約 (Reduction) 用的累加器
        
        // 內層迴圈 j：計算 Preamble 與當前視窗內 Rx 訊號的內積 (MAC)
        for (int j = 0; j < PREAMBLE_LEN; ++j) {
            sum += PREAMBLE[j] * rx_pattern_0[i + j];
        }
        
        // 將該視窗的計算結果存入陣列
        correlation_result[i] = sum;

        // 同時尋找最大值 (找 Peak)
        if (sum > max_corr) {
            max_corr = sum;
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

    // 與我們在 dataset.h 裡面存好的正確答案比對
    if (detected_idx == GROUND_TRUTH[0]) {
        std::cout << "=> 驗證成功！演算法完美找到前導碼位置。" << std::endl;
    } else {
        std::cout << "=> 驗證失敗！找出的位置與正確解答不符。" << std::endl;
    }

    // 釋放動態配置的記憶體
    delete[] correlation_result;

    return 0;
}