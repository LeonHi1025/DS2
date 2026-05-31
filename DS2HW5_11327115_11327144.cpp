// 11327115 郭琮禮 & 11327144 莊有隆
// 環境資訊：請依據實際編譯環境填寫 (例如：g++ (GCC) 11.4.0)

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
    
    const int BUF_SIZE = 100; // 三個 buffer 各 100，總和剛好符合 300 的規定
    int sizeA = fread(bufferA.data(), sizeof(Record), BUF_SIZE, inA);
    int sizeB = fread(bufferB.data(), sizeof(Record), BUF_SIZE, inB);
    int sizeOut = 0;
    
    int idxA = 0, idxB = 0;
    bool eofA = (sizeA == 0), eofB = (sizeB == 0);
    
    while ((idxA < sizeA || !eofA) && (idxB < sizeB || !eofB)) {
        if (idxA >= sizeA && !eofA) {
            sizeA = fread(bufferA.data(), sizeof(Record), BUF_SIZE, inA);
            idxA = 0;
            if (sizeA == 0) eofA = true;
        }
        if (idxB >= sizeB && !eofB) {
            sizeB = fread(bufferB.data(), sizeof(Record), BUF_SIZE, inB);
            idxB = 0;
            if (sizeB == 0) eofB = true;
        }
        
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
        
        if (sizeOut == BUF_SIZE) {
            fwrite(bufferOut.data(), sizeof(Record), sizeOut, out);
            sizeOut = 0;
        }
    }
    
    while (idxA < sizeA || !eofA) {
        if (idxA >= sizeA && !eofA) {
            sizeA = fread(bufferA.data(), sizeof(Record), BUF_SIZE, inA);
            idxA = 0;
            if (sizeA == 0) { eofA = true; break; }
        }
        bufferOut[sizeOut++] = bufferA[idxA++];
        if (sizeOut == BUF_SIZE) {
            fwrite(bufferOut.data(), sizeof(Record), sizeOut, out);
            sizeOut = 0;
        }
    }
    
    while (idxB < sizeB || !eofB) {
        if (idxB >= sizeB && !eofB) {
            sizeB = fread(bufferB.data(), sizeof(Record), BUF_SIZE, inB);
            idxB = 0;
            if (sizeB == 0) { eofB = true; break; }
        }
        bufferOut[sizeOut++] = bufferB[idxB++];
        if (sizeOut == BUF_SIZE) {
            fwrite(bufferOut.data(), sizeof(Record), sizeOut, out);
            sizeOut = 0;
        }
    }
    
    if (sizeOut > 0) {
        fwrite(bufferOut.data(), sizeof(Record), sizeOut, out);
    }
    
    fclose(inA);
    fclose(inB);
    fclose(out);
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
    
    auto start_internal = chrono::high_resolution_clock::now();
    
    FILE* inFile = fopen(filename.c_str(), "rb");
    int numRuns = 0;
    const int CHUNK_SIZE = 300; 
    vector<Record> chunkBuffer(CHUNK_SIZE);
    
    while (true) {
        size_t recordsRead = fread(chunkBuffer.data(), sizeof(Record), CHUNK_SIZE, inFile);
        if (recordsRead == 0) break;
        
        stable_sort(chunkBuffer.begin(), chunkBuffer.begin() + recordsRead, compareRecords);
        
        string runFileName = "temp_0_" + to_string(numRuns) + ".bin";
        FILE* runFile = fopen(runFileName.c_str(), "wb");
        fwrite(chunkBuffer.data(), sizeof(Record), recordsRead, runFile);
        fclose(runFile);
        
        numRuns++;
    }
    fclose(inFile);
    
    auto end_internal = chrono::high_resolution_clock::now();
    double time_internal = chrono::duration<double, milli>(end_internal - start_internal).count();
    
    cout << "\nThe internal sort is completed. Check the initial sorted runs! \n";
    cout << "\nNow there are " << numRuns << " runs.\n";
    
    auto start_external = chrono::high_resolution_clock::now();
    
    int pass = 0;
    int currentNumRuns = numRuns;
    
    // 預先宣告合併用的緩衝區，避免在迴圈內反覆配置記憶體
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
    
    string finalTempName = "temp_" + to_string(pass) + "_0.bin";
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

int main() {
    string fileNum;
    string command;

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
        
        cout << "\nInput the file name: [0]Quit\n";
        if (!(cin >> fileNum)) {
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }
        
        if (fileNum == "0") {
            break;
        }
        
        bool success = runMission1(fileNum);
        
        if (success) {
            cout << "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
            cout << "Mission 2: Build the primary index \n";
            cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
            
            runMission2(fileNum);
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
