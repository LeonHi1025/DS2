// 11327115 郭琮禮 & 11327144 莊有隆

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cstdio>
#include <map>

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

// 輔助函式：將 char array 轉換為 string 並去除多餘字元
string toString(const char* arr, int maxLen) {
    string s;
    for (int i = 0; i < maxLen && arr[i] != '\0' && arr[i] != ' '; ++i) {
        s += arr[i];
    }
    return s;
}

class ExternalSort {
private:
    int bufferSize = 300;
    int mergeChunkSize = 100;

    vector<IndexRecord> primaryIndex;
    map<string, vector<long long>> secondaryIndex;

    void refill(FILE* file, vector<Record>& buffer, int& size, int& idx, bool& eof) {
        if (idx >= size && !eof) {
            size = fread(buffer.data(), sizeof(Record), mergeChunkSize, file);
            idx = 0;
            if (size == 0) eof = true;
        }
    }

    void flush(FILE* file, vector<Record>& buffer, int& size) {
        if (size > 0) {
            fwrite(buffer.data(), sizeof(Record), size, file);
            size = 0;
        }
    }

    void copyRemaining(FILE* inFile, FILE* outFile, vector<Record>& bufferIn, int& sizeIn, int& idxIn, bool& eofIn,
                       vector<Record>& bufferOut, int& sizeOut) {
        while (idxIn < sizeIn || !eofIn) {
            if (idxIn >= sizeIn && !eofIn) {
                sizeIn = fread(bufferIn.data(), sizeof(Record), mergeChunkSize, inFile);
                idxIn = 0;
                if (sizeIn == 0) { eofIn = true; break; }
            }
            bufferOut[sizeOut++] = bufferIn[idxIn++];
            if (sizeOut == mergeChunkSize) {
                flush(outFile, bufferOut, sizeOut);
            }
        }
    }

public:
    void setBufferSize(int size) {
        bufferSize = size;
        mergeChunkSize = size / 3;
        if (mergeChunkSize < 1) mergeChunkSize = 1;
    }

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
        
        int sizeA = fread(bufferA.data(), sizeof(Record), mergeChunkSize, inA);
        int sizeB = fread(bufferB.data(), sizeof(Record), mergeChunkSize, inB);
        int sizeOut = 0;
        
        int idxA = 0, idxB = 0;
        bool eofA = (sizeA == 0), eofB = (sizeB == 0);
        
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
            
            if (sizeOut == mergeChunkSize) {
                flush(out, bufferOut, sizeOut);
            }
        }
        
        copyRemaining(inA, out, bufferA, sizeA, idxA, eofA, bufferOut, sizeOut);
        copyRemaining(inB, out, bufferB, sizeB, idxB, eofB, bufferOut, sizeOut);
        
        flush(out, bufferOut, sizeOut);
        
        fclose(inA);
        fclose(inB);
        fclose(out);
    }

    int generateInitialRuns(const string& filename) {
        FILE* inFile = fopen(filename.c_str(), "rb");
        if (!inFile) return 0;
        
        int numRuns = 0;
        vector<Record> chunkBuffer(bufferSize);
        
        while (true) {
            size_t recordsRead = fread(chunkBuffer.data(), sizeof(Record), bufferSize, inFile);
            if (recordsRead == 0) break;
            
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

    void mergeAllRuns(int numRuns, int& finalPass) {
        int pass = 0;
        int currentNumRuns = numRuns;
        vector<Record> bufferA(mergeChunkSize), bufferB(mergeChunkSize), bufferOut(mergeChunkSize);
        
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
        
        FILE* testFile = fopen(filename.c_str(), "rb");
        if (!testFile) {
            cout << "\n" << filename << " does not exist!!!\n";
            return false;
        }
        fclose(testFile);
        
        auto start_internal = chrono::high_resolution_clock::now();
        int numRuns = generateInitialRuns(filename);
        auto end_internal = chrono::high_resolution_clock::now();
        double time_internal = chrono::duration<double, milli>(end_internal - start_internal).count();
        
        cout << "\nThe internal sort is completed. Check the initial sorted runs!\n";
        cout << "\nNow there are " << numRuns << " runs.\n";
        
        auto start_external = chrono::high_resolution_clock::now();
        int finalPass = 0;
        mergeAllRuns(numRuns, finalPass);
        
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
        primaryIndex.clear();
        string filename = "order" + fileNum + ".bin";
        FILE* inFile = fopen(filename.c_str(), "rb");
        if (!inFile) {
            cout << "\n" << filename << " does not exist!!!\n";
            return;
        }
        
        vector<Record> chunkBuffer(bufferSize);
        long long currentOffset = 0; 
        float lastWeight = -1.0f;
        bool isFirst = true;
        
        while (true) {
            size_t recordsRead = fread(chunkBuffer.data(), sizeof(Record), bufferSize, inFile);
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

    void runMission3and4(const string& fileNum) {
        if (primaryIndex.empty()) return;

        string filename = "order" + fileNum + ".bin";
        FILE* inFile = fopen(filename.c_str(), "rb");
        if (!inFile) return;

        while (true) {
            cout << "\n##################################\n";
            cout << "* 3: Range search to build index *\n";
            cout << "##################################\n";
            
            cout << "\nInput two values in (0,1] for range search.\n";
            float val1, val2;
            cout << "\nInput a floating number in [0.01, 1]: ";
            cin >> val1;
            cout << "\nInput a floating number in [0.01, 1]: ";
            cin >> val2;

            float high = max(val1, val2);
            float low = min(val1, val2);

            long long startOffset = -1;
            long long endOffset = -1;
            
            fseek(inFile, 0, SEEK_END);
            long long fileTotalRecords = ftell(inFile) / sizeof(Record);

            // 在 Primary Index 尋找對應權重範圍的起點與終點位移量
            for (size_t i = 0; i < primaryIndex.size(); ++i) {
                if (primaryIndex[i].weight <= high && startOffset == -1) {
                    startOffset = primaryIndex[i].offset;
                }
                if (primaryIndex[i].weight < low) {
                    endOffset = primaryIndex[i].offset - 1;
                    break;
                }
            }
            
            // 如果只有找到起點但沒有觸發小於下限的條件，代表範圍直到檔尾
            if (startOffset != -1 && endOffset == -1) {
                endOffset = fileTotalRecords - 1;
            }

            if (startOffset == -1 || startOffset > endOffset) {
                cout << "\nThere are 0 records in total.\n";
                cout << "There are 0 senders in total.\n";
            } else {
                secondaryIndex.clear();
                long long rangeTotal = endOffset - startOffset + 1;
                
                fseek(inFile, startOffset * sizeof(Record), SEEK_SET);
                long long recordsLeft = rangeTotal;
                vector<Record> chunkBuffer(bufferSize);
                long long currentOffset = startOffset;
                
                // 批次讀取範圍內的資料建立 Secondary Index
                while (recordsLeft > 0) {
                    size_t toRead = min((long long)bufferSize, recordsLeft);
                    size_t recordsRead = fread(chunkBuffer.data(), sizeof(Record), toRead, inFile);
                    if (recordsRead == 0) break;
                    
                    for (size_t i = 0; i < recordsRead; ++i) {
                        string pstr = toString(chunkBuffer[i].putID, 10);
                        secondaryIndex[pstr].push_back(currentOffset);
                        currentOffset++;
                    }
                    recordsLeft -= recordsRead;
                }
                
                cout << "\nThere are " << rangeTotal << " records in total.\n";
                cout << "There are " << secondaryIndex.size() << " senders in total.\n";
                
                int count = 1;
                for (auto const& pair : secondaryIndex) {
                    cout << "[" << setw(4) << right << count++ << "]   " 
                         << setw(8) << right << pair.first 
                         << setw(12) << right << pair.second.size() << "\n";
                }

                // --- 任務四: 利用輔助索引檢索資料 ---
                while (true) {
                    cout << "\nInput a student ID ([4] Quit): ";
                    string queryID;
                    cin >> queryID;
                    
                    if (queryID == "4") {
                        break;
                    }
                    
                    auto it = secondaryIndex.find(queryID);
                    if (it == secondaryIndex.end()) {
                        cout << "Sender " << queryID << " does not exist.\n";
                    } else {
                        cout << "Sender " << queryID << " has " << it->second.size() << " records.\n";
                        int rcount = 1;
                        for (long long offset : it->second) {
                            Record rec;
                            fseek(inFile, offset * sizeof(Record), SEEK_SET);
                            fread(&rec, sizeof(Record), 1, inFile);
                            
                            string gidStr = toString(rec.getID, 10);
                            // 在輸出前先設定好浮點數格式與欄位寬度
                            cout << "[" << setw(3) << right << rcount++ << "]   " 
                                 << setw(8) << right << gidStr << "        " 
                                 << fixed << setprecision(2) << rec.weight << "\n";
                        }
                    }
                }
            }

            cout << "\n[3]Quit or [Any other key]continue?\n";
            string cmd;
            cin >> cmd;
            if (cmd == "3") {
                break;
            }
        }
        fclose(inFile);
    }
};

int main() {
    string fileNum;
    string command;
    ExternalSort exSort;

    cout << "* Data Structures and Algorithms *\n";
    cout << "**********************************\n";
    cout << "* 1. External merge sort on file *\n";
    cout << "* 2: Construct the primary index *\n";
    cout << "* 3: Range search to build index *\n";
    cout << "* 4: Retrieve records from index *\n";
    cout << "**********************************\n";
    cout << "*** The buffer size is 300\n";
    cout << "Input a new buffer size in [300, 60000]: ";
    
    int bufSize;
    if (!(cin >> bufSize)) {
        bufSize = 300;
    }
    exSort.setBufferSize(bufSize);

    while (true) {
        cout << "\n##################################\n";
        cout << "* 1. External merge sort on file *\n";
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
            if (exSort.runMission1(fileNum)) {
                cout << "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
                cout << "* 2: Construct the primary index *\n";
                cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
                exSort.runMission2(fileNum);
                
                exSort.runMission3and4(fileNum);
            }
        }
        
        cout << "\n[0]Quit or [Any other key]continue?\n";
        cin >> command;
        if (command == "0") {
            break;
        }
    }
    return 0;
}
