// 11327115 郭琮禮 & 11327144 莊有隆

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cstdio>

using namespace std;

// 設定結構體對齊為 1 位元組，確保 Record 結構體的大小剛好為 24 位元組 (10 + 10 + 4)
#pragma pack(push, 1)
struct Record {
    char putID[10];   // 發訊者學號 (10 bytes)
    char getID[10];   // 收訊者學號 (10 bytes)
    float weight;     // 互動關係的量化權重 (4 bytes)
};
#pragma pack(pop)

// 比較函式：依權重由大到小排序 (用於 std::stable_sort)
bool compareRecords(const Record& a, const Record& b) {
    return a.weight > b.weight;
}

// 合併兩個 run 檔案的函式，嚴格限制記憶體中的緩衝區總量不超過 300 筆紀錄
void mergeRuns(const string& fileA, const string& fileB, const string& fileOut) {
    ifstream inA(fileA, ios::binary);
    ifstream inB(fileB, ios::binary);
    ofstream out(fileOut, ios::binary);
    
    if (!inA || !inB || !out) {
        // cerr << "合併時開啟檔案失敗: " << fileA << ", " << fileB << " -> " << fileOut << endl;
        return;
    }
    
    // 將 300 筆緩衝區上限分配為：輸入 A 緩衝區 100 筆、輸入 B 緩衝區 100 筆、輸出緩衝區 100 筆
    const int BUF_SIZE = 100;
    vector<Record> bufferA(BUF_SIZE);
    vector<Record> bufferB(BUF_SIZE);
    vector<Record> bufferOut(BUF_SIZE);
    
    int sizeA = 0, sizeB = 0, sizeOut = 0;
    int idxA = 0, idxB = 0;
    bool eofA = false, eofB = false;
    
    // 輔助 Lambda：當輸入 A 緩衝區用盡且檔案未結束時，重新從 A 檔案讀入資料
    auto refillA = [&]() {
        if (idxA >= sizeA && !eofA) {
            inA.read(reinterpret_cast<char*>(bufferA.data()), BUF_SIZE * sizeof(Record));
            sizeA = inA.gcount() / sizeof(Record);
            idxA = 0;
            if (sizeA == 0) eofA = true;
        }
    };
    
    // 輔助 Lambda：當輸入 B 緩衝區用盡且檔案未結束時，重新從 B 檔案讀入資料
    auto refillB = [&]() {
        if (idxB >= sizeB && !eofB) {
            inB.read(reinterpret_cast<char*>(bufferB.data()), BUF_SIZE * sizeof(Record));
            sizeB = inB.gcount() / sizeof(Record);
            idxB = 0;
            if (sizeB == 0) eofB = true;
        }
    };
    
    // 輔助 Lambda：將輸出緩衝區中的資料寫入硬碟並清空緩衝區
    auto flushOut = [&]() {
        if (sizeOut > 0) {
            out.write(reinterpret_cast<const char*>(bufferOut.data()), sizeOut * sizeof(Record));
            sizeOut = 0;
        }
    };
    
    // 初始化讀取
    refillA();
    refillB();
    
    // 雙指針合併兩個已排序的 runs
    while ((idxA < sizeA || !eofA) && (idxB < sizeB || !eofB)) {
        refillA();
        refillB();
        
        if (idxA < sizeA && idxB < sizeB) {
            // 依權重由大到小排序。
            // 當權重相等時，優先選擇來自 A 的紀錄以保持穩定排序 (因 A 對應於檔案中較早出現的 run)
            if (bufferA[idxA].weight >= bufferB[idxB].weight) {
                bufferOut[sizeOut++] = bufferA[idxA++];
            } else {
                bufferOut[sizeOut++] = bufferB[idxB++];
            }
        } else if (idxA < sizeA) {
            bufferOut[sizeOut++] = bufferA[idxA++];
        } else if (idxB < sizeB) {
            bufferOut[sizeOut++] = bufferB[idxB++];
        }
        
        // 輸出緩衝區滿了則寫入硬碟
        if (sizeOut == BUF_SIZE) {
            flushOut();
        }
    }
    
    // 將 A 剩餘的所有資料寫入輸出緩衝區
    while (idxA < sizeA || !eofA) {
        refillA();
        if (idxA < sizeA) {
            bufferOut[sizeOut++] = bufferA[idxA++];
            if (sizeOut == BUF_SIZE) {
                flushOut();
            }
        }
    }
    
    // 將 B 剩餘的所有資料寫入輸出緩衝區
    while (idxB < sizeB || !eofB) {
        refillB();
        if (idxB < sizeB) {
            bufferOut[sizeOut++] = bufferB[idxB++];
            if (sizeOut == BUF_SIZE) {
                flushOut();
            }
        }
    }
    
    // 寫入最後殘留的資料
    flushOut();
}

void runMission1() {
    string fileNum;
    string filename;
    
    // 輸入檔名循環，防呆並驗證檔案是否存在
    while (true) {
        cout << "\nInput the file name: [0]Quit\n";
        if (!(cin >> fileNum)) {
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }
        if (fileNum == "0") {
            return;
        }
        
        filename = "pairs" + fileNum + ".bin";
        ifstream testFile(filename, ios::binary);
        if (!testFile) {
            cout << "\n" << filename << " does not exist!!!\n";
            continue;
        }
        testFile.close();
        break;
    }
    
    // 步驟 1：內部排序與初始 Runs 生成 (Internal Sort Phase)
    auto start_internal = chrono::high_resolution_clock::now();
    
    ifstream inFile(filename, ios::binary);
    int numRuns = 0;
    const int CHUNK_SIZE = 300; // 記憶體緩衝區上限為 300 筆資料
    vector<Record> chunkBuffer(CHUNK_SIZE);
    
    while (inFile) {
        // 從二進位檔讀入最多 300 筆紀錄
        inFile.read(reinterpret_cast<char*>(chunkBuffer.data()), CHUNK_SIZE * sizeof(Record));
        int recordsRead = inFile.gcount() / sizeof(Record);
        if (recordsRead == 0) break;
        
        // 記憶體內穩定排序 (當權重相同時，保持原檔案中的次序)
        stable_sort(chunkBuffer.begin(), chunkBuffer.begin() + recordsRead, compareRecords);
        
        // 寫入此 run 的臨時二進位檔案
        string runFileName = "temp_0_" + to_string(numRuns) + ".bin";
        ofstream runFile(runFileName, ios::binary);
        runFile.write(reinterpret_cast<const char*>(chunkBuffer.data()), recordsRead * sizeof(Record));
        runFile.close();
        
        numRuns++;
    }
    inFile.close();
    
    auto end_internal = chrono::high_resolution_clock::now();
    double time_internal = chrono::duration<double, milli>(end_internal - start_internal).count();
    
    cout << "\nThe internal sort is completed. Check the initial sorted runs! \n";
    cout << "\nNow there are " << numRuns << " runs.\n";
    
    // 步驟 2：外部兩兩合併階段 (External Merge Phase)
    auto start_external = chrono::high_resolution_clock::now();
    
    int pass = 0;
    int currentNumRuns = numRuns;
    
    // 當剩餘 runs 大於 1 時持續兩兩合併
    while (currentNumRuns > 1) {
        int nextNumRuns = 0;
        for (int i = 0; i < currentNumRuns; i += 2) {
            string outName = "temp_" + to_string(pass + 1) + "_" + to_string(nextNumRuns) + ".bin";
            if (i + 1 < currentNumRuns) {
                // 有成對的 runs 可以進行合併
                string inNameA = "temp_" + to_string(pass) + "_" + to_string(i) + ".bin";
                string inNameB = "temp_" + to_string(pass) + "_" + to_string(i + 1) + ".bin";
                
                mergeRuns(inNameA, inNameB, outName);
                
                // 合併完後立刻刪除舊的暫存檔案以節省空間
                remove(inNameA.c_str());
                remove(inNameB.c_str());
            } else {
                // 若為奇數 run 無法成對，直接將其重新命名並傳遞至下一輪
                string inName = "temp_" + to_string(pass) + "_" + to_string(i) + ".bin";
                rename(inName.c_str(), outName.c_str());
            }
            nextNumRuns++;
        }
        currentNumRuns = nextNumRuns;
        pass++;
        cout << "\nNow there are " << currentNumRuns << " runs.\n";
    }
    
    // 確定最後產生的排序檔案名稱
    string finalTempName = "temp_" + to_string(pass) + "_0.bin";
    string finalName = "order" + fileNum + ".bin";
    
    // 若原檔案較小且只產生了 1 個 initial run，此時未進入合併迴圈
    if (numRuns == 1) {
        finalTempName = "temp_0_0.bin";
    }
    
    if (numRuns > 0) {
        // 先移除可能已存在的舊檔案，再將最終的 run 重新命名為 order<fileNum>.bin
        remove(finalName.c_str());
        rename(finalTempName.c_str(), finalName.c_str());
    } else {
        // 若輸入為空檔，則建立空的 order 檔案
        ofstream emptyOut(finalName, ios::binary);
        emptyOut.close();
    }
    
    auto end_external = chrono::high_resolution_clock::now();
    double time_external = chrono::duration<double, milli>(end_external - start_external).count();
    
    double time_total = time_internal + time_external;
    
    // 輸出耗費時間資訊
    cout << "\nThe execution time ...\n";
    cout << fixed << setprecision(3);
    cout << "Internal Sort = " << time_internal << " ms\n";
    cout << "External Sort = " << time_external << " ms\n";
    cout << "Total Execution Time = " << time_total << " ms\n";
}

int main() {
    // 程式一開始即直接印出選單資訊，並直接進入任務一 (等待輸入檔名)
    while (true) {
        cout << "* Data Structures and Algorithms *\n";
        cout << "**********************************\n";
        cout << "* 1. External merge sort on file *\n";
        cout << "* 2: Construct the primary index *\n";
        cout << "**********************************\n";
        cout << "*** The buffer size is 300\n";
        cout << "##################################\n";
        cout << "Mission 1: External merge sort \n";
        cout << "##################################\n";
        
        runMission1();
        
        // 執行完後詢問使用者是否繼續或結束
        cout << "\n[0]Quit or [Any other key]continue? ";
        string cont;
        if (!(cin >> cont)) {
            break;
        }
        if (cont == "0") {
            break;
        }
        cout << endl; // 印出換行以保持選單外觀一致
    }
    return 0;
}
