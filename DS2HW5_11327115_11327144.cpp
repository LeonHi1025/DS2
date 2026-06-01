// 11327115 郭琮禮 & 11327144 莊有隆

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cstdio> // 引入 C-style I/O 以大幅提升讀寫效能

using namespace std;

#pragma pack(push, 1)
struct Record {
    char putID[10];
    char getID[10];
    float weight;
};
#pragma pack(pop)

struct IndexRecord {
    float weight;
    long long offset; 
};

bool compareRecords(const Record& a, const Record& b) {
    return a.weight > b.weight;
}

class ExternalSort {
private:
    // 輔助函式：當輸入緩衝區用盡且檔案未結束時，從檔案重新讀入資料
    void refill(FILE* file, vector<Record>& buffer, int& size, int& idx, bool& eof) {
        if (idx >= size && !eof) {
            size = fread(buffer.data(), sizeof(Record), 100, file);
            idx = 0;
            if (size == 0) eof = true;
        }
    }

    // 輔助函式：將輸出緩衝區中的資料寫入硬碟並清空緩衝區
    void flush(FILE* file, vector<Record>& buffer, int& size) {
        if (size > 0) {
            fwrite(buffer.data(), sizeof(Record), size, file);
            size = 0;
        }
    }

    // 輔助函式：當其中一個檔案處理完畢後，將另一個檔案剩餘的所有資料複製到輸出檔
    void copyRemaining(FILE* inFile, FILE* outFile, vector<Record>& bufferIn, int& sizeIn, int& idxIn, bool& eofIn,
                       vector<Record>& bufferOut, int& sizeOut) {
        while (idxIn < sizeIn || !eofIn) {
            if (idxIn >= sizeIn && !eofIn) {
                sizeIn = fread(bufferIn.data(), sizeof(Record), 100, inFile);
                idxIn = 0;
                if (sizeIn == 0) { eofIn = true; break; }
            }
            bufferOut[sizeOut++] = bufferIn[idxIn++];
            if (sizeOut == 100) {
                flush(outFile, bufferOut, sizeOut);
            }
        }
    }

public:
    // 透過傳參考共用 buffer，避免迴圈中重複配置記憶體
    void mergeRuns(const string& fileA, const string& fileB, const string& fileOut, 
                   vector<Record>& bufferA, vector<Record>& bufferB, vector<Record>& bufferOut) {
        
        FILE* inA = fopen(fileA.c_str(), "rb");
        FILE* inB = fopen(fileB.c_str(), "rb");
        FILE* out = fopen(fileOut.c_str(), "wb");
        
        if (!inA || !inB || !out) {
            if (inA) fclose(inA);
            if (inB) fclose(inB);
            if (out) fclose(out);
            return;
        }
        
        int sizeA = fread(bufferA.data(), sizeof(Record), 100, inA);
        int sizeB = fread(bufferB.data(), sizeof(Record), 100, inB);
        int sizeOut = 0;
        
        int idxA = 0, idxB = 0;
        bool eofA = (sizeA == 0), eofB = (sizeB == 0);
        
        // 雙指針合併兩個已排序的 runs
        while ((idxA < sizeA || !eofA) && (idxB < sizeB || !eofB)) {
            refill(inA, bufferA, sizeA, idxA, eofA);
            refill(inB, bufferB, sizeB, idxB, eofB);
            
            if (idxA < sizeA && idxB < sizeB) {
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
            
            if (sizeOut == 100) {
                flush(out, bufferOut, sizeOut);
            }
        }
        
        // 複製剩餘的紀錄
        copyRemaining(inA, out, bufferA, sizeA, idxA, eofA, bufferOut, sizeOut);
        copyRemaining(inB, out, bufferB, sizeB, idxB, eofB, bufferOut, sizeOut);
        
        flush(out, bufferOut, sizeOut);
        
        fclose(inA);
        fclose(inB);
        fclose(out);
    }

    // 階段一：讀取並排序初始區塊 (Internal Sort)，生成初始 runs 並回傳總數量
    int generateInitialRuns(const string& filename) {
        FILE* inFile = fopen(filename.c_str(), "rb");
        if (!inFile) return 0;
        
        int numRuns = 0;
        const int CHUNK_SIZE = 300; 
        vector<Record> chunkBuffer(CHUNK_SIZE);
        
        while (true) {
            size_t recordsRead = fread(chunkBuffer.data(), sizeof(Record), CHUNK_SIZE, inFile);
            if (recordsRead == 0) break;
            
            // 穩定排序以保留同權重紀錄的原始順序
            stable_sort(chunkBuffer.begin(), chunkBuffer.begin() + recordsRead, compareRecords);
            
            string runFileName = "temp_0_" + to_string(numRuns) + ".bin";
            FILE* runFile = fopen(runFileName.c_str(), "wb");
            if (runFile) {
                fwrite(chunkBuffer.data(), sizeof(Record), recordsRead, runFile);
                fclose(runFile);
            }
            numRuns++;
        }
        fclose(inFile);
        return numRuns;
    }

    // 階段二：對所有 runs 進行兩兩合併 (External Sort)，並由 reference 帶回最終 pass 數
    void mergeAllRuns(int numRuns, int& finalPass) {
        int pass = 0;
        int currentNumRuns = numRuns;
        vector<Record> bufferA(100), bufferB(100), bufferOut(100);
        
        while (currentNumRuns > 1) {
            int nextNumRuns = 0;
            for (int i = 0; i < currentNumRuns; i += 2) {
                string outName = "temp_" + to_string(pass + 1) + "_" + to_string(nextNumRuns) + ".bin";
                if (i + 1 < currentNumRuns) {
                    string inNameA = "temp_" + to_string(pass) + "_" + to_string(i) + ".bin";
                    string inNameB = "temp_" + to_string(pass) + "_" + to_string(i + 1) + ".bin";
                    
                    mergeRuns(inNameA, inNameB, outName, bufferA, bufferB, bufferOut);
                    
                    remove(inNameA.c_str());
                    remove(inNameB.c_str());
                } else {
                    string inName = "temp_" + to_string(pass) + "_" + to_string(i) + ".bin";
                    rename(inName.c_str(), outName.c_str());
                }
                nextNumRuns++;
            }
            currentNumRuns = nextNumRuns;
            pass++;
            cout << "\nNow there are " << currentNumRuns << " runs.\n";
        }
        finalPass = pass;
    }

    bool runMission1(const string& fileNum) {
        string filename = "pairs" + fileNum + ".bin";
        
        // 檢查檔案是否存在
        FILE* testFile = fopen(filename.c_str(), "rb");
        if (!testFile) {
            cout << "\n" << filename << " does not exist!!!\n";
            return false;
        }
        fclose(testFile);
        
        // --- 1. 內部排序與初始 Runs 生成 ---
        auto start_internal = chrono::high_resolution_clock::now();
        int numRuns = generateInitialRuns(filename);
        auto end_internal = chrono::high_resolution_clock::now();
        double time_internal = chrono::duration<double, milli>(end_internal - start_internal).count();
        
        cout << "\nThe internal sort is completed. Check the initial sorted runs! \n";
        cout << "\nNow there are " << numRuns << " runs.\n";
        
        // --- 2. 外部合併階段 ---
        auto start_external = chrono::high_resolution_clock::now();
        int finalPass = 0;
        mergeAllRuns(numRuns, finalPass);
        
        // 重新命名最後合併完成的檔案為 order<fileNum>.bin
        string finalTempName = "temp_" + to_string(finalPass) + "_0.bin";
        string finalName = "order" + fileNum + ".bin";
        
        if (numRuns == 1) {
            finalTempName = "temp_0_0.bin";
        }
        
        if (numRuns > 0) {
            remove(finalName.c_str());
            rename(finalTempName.c_str(), finalName.c_str());
        } else {
            FILE* emptyOut = fopen(finalName.c_str(), "wb");
            if (emptyOut) fclose(emptyOut);
        }
        
        auto end_external = chrono::high_resolution_clock::now();
        double time_external = chrono::duration<double, milli>(end_external - start_external).count();
        
        double time_total = time_internal + time_external;
        
        cout << "\nThe execution time ...\n";
        cout << fixed << setprecision(3);
        cout << "Internal Sort = " << time_internal << " ms\n";
        cout << "External Sort = " << time_external << " ms\n";
        cout << "Total Execution Time = " << time_total << " ms\n";
        
        cout.unsetf(ios::fixed);
        cout.precision(6);
        
        return true;
    }

    void runMission2(const string& fileNum) {
        string filename = "order" + fileNum + ".bin";
        FILE* inFile = fopen(filename.c_str(), "rb");
        if (!inFile) {
            cout << "\n" << filename << " does not exist!!!\n";
            return;
        }
        
        vector<IndexRecord> primaryIndex;
        const int CHUNK_SIZE = 300;
        vector<Record> chunkBuffer(CHUNK_SIZE);
        
        long long currentOffset = 0; 
        float lastWeight = -1.0f;
        bool isFirst = true;
        
        while (true) {
            size_t recordsRead = fread(chunkBuffer.data(), sizeof(Record), CHUNK_SIZE, inFile);
            if (recordsRead == 0) break;
            
            for (size_t i = 0; i < recordsRead; ++i) {
                if (isFirst || chunkBuffer[i].weight != lastWeight) {
                    primaryIndex.push_back({chunkBuffer[i].weight, currentOffset});
                    lastWeight = chunkBuffer[i].weight;
                    isFirst = false;
                }
                currentOffset++; 
            }
        }
        fclose(inFile);
        
        cout << "\n<Primary index>: (key, offset)\n";
        for (size_t i = 0; i < primaryIndex.size(); ++i) {
            cout << "[" << i + 1 << "] (" << primaryIndex[i].weight << ", " << primaryIndex[i].offset << ")\n";
        }
    }
};

int main() {
    string fileNum;
    string command;
    ExternalSort exSort;

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
        
        while (true) {
            cout << "\nInput the file name: [0]Quit\n";
            if (!(cin >> fileNum)) {
                cin.clear();
                cin.ignore(10000, '\n');
                continue;
            }
            
            if (fileNum == "0") {
                break;
            }
            
            string filename = "pairs" + fileNum + ".bin";
            FILE* testFile = fopen(filename.c_str(), "rb");
            if (!testFile) {
                cout << "\n" << filename << " does not exist!!!\n";
                continue;
            }
            fclose(testFile);
            break;
        }
        
        if (fileNum != "0") {
            exSort.runMission1(fileNum);
            
            cout << "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
            cout << "Mission 2: Build the primary index \n";
            cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
            
            exSort.runMission2(fileNum);
        }
        
        cout << "\n[0]Quit or [Any other key]continue?\n";
        cin >> command;
        if (command == "0") {
            break;
        }
        cout << "\n";
    }
    return 0;
}
