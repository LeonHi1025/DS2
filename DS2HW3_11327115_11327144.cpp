// 11327115 郭琮禮 & 11327144 莊有隆 
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <cmath>
#include <algorithm>

using namespace std;

// 學生紀錄結構體：用於儲存從文字檔解析出的單筆學生資料
struct StudentRecord {
    char sid[10];            // 儲存學生學號 (最多 10 個字元)
    char sname[10];          // 儲存學生姓名 (最多 10 個字元)
    unsigned char score[6];  // 儲存 6 個科目的分數 (0~255)
    float average;           // 儲存平均分數
};

// 判斷給定整數是否為質數的輔助函式，使用 6k ± 1 的演算法進行最佳化
bool isPrime(int n) {
    if (n <= 1) return false;                   
    if (n == 2 || n == 3) return true;          
    if (n % 2 == 0 || n % 3 == 0) return false; 
    
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }
    return true;
}

// 取得大於等於給定數值 n 的最小質數
int getPrime(int n) {
    int candidate = n + 1; 
    while (!isPrime(candidate)) {
        candidate++;
    }
    return candidate;
}

// 雜湊表中的每個插槽結構體
struct HashSlotX {
    unsigned long long hvalue; // 雜湊鍵值
    string sid;                // 學號字串
    string sname;              // 姓名字串
    float mean;                // 平均分數
    bool isEmpty;              // 標記此插槽是否為空
    
    HashSlotX() : hvalue(0), mean(0.0f), isEmpty(true) {}
};

// DataManager 類別：負責處理資料的讀取、寫入與管理
class DataManager {
private:
    vector<StudentRecord> records; 

public:
    void Clear() {
        records.clear();
    }

    bool ExecuteTask0(string fileNumber) {
        string txtFilename = "input" + fileNumber + ".txt";
        string binFilename = "input" + fileNumber + ".bin";

        Clear(); 

        ifstream txtFile(txtFilename);
        if (!txtFile.is_open()) {
            return false;
        }

        string line;
        while (getline(txtFile, line)) {
            if (line.empty() || line == "\r") continue;

            stringstream ss(line);
            string token;
            vector<string> tokens;

            while (getline(ss, token, '\t')) {
                if (!token.empty() && token.back() == '\r') {
                    token.pop_back();
                }
                tokens.push_back(token);
            }

            if (tokens.size() >= 9) {
                StudentRecord record;
                memset(&record, 0, sizeof(StudentRecord)); 

                strncpy(record.sid, tokens[0].c_str(), 10);
                strncpy(record.sname, tokens[1].c_str(), 10);

                for (int i = 0; i < 6; ++i) {
                    record.score[i] = static_cast<unsigned char>(stoi(tokens[2 + i]));
                }
                record.average = stof(tokens[8]);

                records.push_back(record);
            }
        }
        txtFile.close();

        ofstream binFile(binFilename, ios::binary);
        if (!binFile.is_open()) {
            return false;
        }

        for (const auto& record : records) {
            binFile.write(reinterpret_cast<const char*>(&record), sizeof(StudentRecord));
        }
        binFile.close();

        return true;
    }

    bool LoadData(string fileNumber) {
        string binFilename = "input" + fileNumber + ".bin";
        ifstream binFile(binFilename, ios::binary);

        if (binFile.is_open()) {
            Clear();
            StudentRecord record;
            while (binFile.read(reinterpret_cast<char*>(&record), sizeof(StudentRecord))) {
                records.push_back(record);
            }
            binFile.close();
            return true;
        } else {
            if (ExecuteTask0(fileNumber)) {
                cout << "\n### " << "input" + fileNumber + ".bin does not exist! ###" << endl;
                return true; 
            }

            cout << "\n### " << "input" + fileNumber + ".bin does not exist! ###" << endl;
            cout << "\n### " << "input" + fileNumber + ".txt does not exist! ###" << endl;
            return false;
        }
    }

    const vector<StudentRecord>& GetRecords() const {
        return records;
    }
};

// 任務一：以平方探測法 (Quadratic Probing) 建立雜湊表
class QuadraticProbing {
public:
    bool Execute(DataManager& dataManager, string fileNumber) {
        const vector<StudentRecord>& records = dataManager.GetRecords();
        if (records.empty()) {
            cout << "沒有資料可供建立雜湊表！" << endl;
            return false;
        }

        int tableSize = getPrime(static_cast<int>(records.size() * 1.15));
        vector<HashSlotX> hashTable(tableSize); 

        int successfulInsertions = 0; 
        unsigned long long totalSuccessfulProbes = 0; 
        int c = 0;

        // 建立雜湊表
        for (const auto& rec : records) {
            
            string sidStr = string(rec.sid, strnlen(rec.sid, 10));
            
            unsigned long long hashValue = 1;
            for (char c : sidStr) {
                hashValue = (hashValue * static_cast<unsigned char>(c)) % tableSize;
            }

            int step = 0; 
            bool inserted = false;
            
            while (step < tableSize) {
                int probeIndex = (hashValue + step * step) % tableSize;
                
                if (hashTable[probeIndex].isEmpty) {
                    hashTable[probeIndex].hvalue = hashValue;
                    hashTable[probeIndex].sid = sidStr;
                    hashTable[probeIndex].sname = string(rec.sname, strnlen(rec.sname, 10));
                    hashTable[probeIndex].mean = rec.average;
                    hashTable[probeIndex].isEmpty = false;
                    
                    successfulInsertions++;
                    totalSuccessfulProbes += (step + 1); 
                    inserted = true;
                    break;
                }
                step++; 
            }
            // 只有在完整探測後依然找不到空位時，才算真正的「碰撞失敗」
            if (!inserted) {
                cout << "### Failed at [" << c << "]. ###" << endl;
            }
            c++;
        }

        // 模擬 Unsuccessful search 的探測次數：從每一個槽開始尋訪到空位為止
        unsigned long long totalUnsuccessfulProbes = 0;
        for (int i = 0; i < tableSize; ++i) {
            int step = 0;
            while (step < tableSize) {
                int probeIndex = (i + step * step) % tableSize;
                totalUnsuccessfulProbes++; // 每檢查一個位置就是一次比較 (包含空的)
                if (hashTable[probeIndex].isEmpty) {
                    break;
                }
                step++;
            }
        }

        double avgUnsuccessful = static_cast<double>(totalUnsuccessfulProbes) / tableSize;
        double avgSuccessful = 0.0;
        if (successfulInsertions > 0) {
            avgSuccessful = static_cast<double>(totalSuccessfulProbes) / successfulInsertions;
        }

        cout << "\nHash table has been successfully created by Quadratic probing" << endl;
        cout << fixed << setprecision(4);
        cout << "unsuccessful search: " << avgUnsuccessful << " comparisons on average" << endl;
        cout << "successful search: " << avgSuccessful << " comparisons on average" << endl;

        string outFilename = "quadratic" + fileNumber + ".txt";
        ofstream outFile(outFilename);
        if (!outFile.is_open()) {
            cout << "無法建立輸出檔案 " << outFilename << endl;
            return false;
        }

        outFile << " --- Hash table created by Quadratic probing ---" << endl;

        for (int i = 0; i < tableSize; ++i) {
            if (hashTable[i].isEmpty) {
                outFile << "[" << setw(3) << i << "] " << endl;
            } else {
                outFile << right << "[" << setw(3) << i << "]" 
                        << setw(11) << hashTable[i].hvalue << ","
                        << setw(11) << hashTable[i].sid << ","
                        << setw(11) << hashTable[i].sname << ","
                        << setw(11) << hashTable[i].mean << endl;
            }
        }

        outFile << " ----------------------------------------------------- " << endl;
        outFile.close();

        // ==========================================
        // 任務三：以雜湊表搜尋 (Quadratic Probing)
        // ==========================================
        string targetId;
        // 提供防呆機制，持續要求使用者輸入直到輸入 0 退出為止
        while (true) {
            cout << "Input a student ID to search ([0] Quit): ";
            cin >> targetId;
            if (targetId == "0") {
                cout << endl;
                break; // 輸入 0 則結束搜尋
            }

            // 計算搜尋目標學號的初始雜湊值 (與任務一建表時的演算法一致)
            unsigned long long hashValue = 1;
            for (char c : targetId) {
                hashValue = (hashValue * static_cast<unsigned char>(c)) % tableSize;
            }

            int step = 0;
            bool found = false;
            
            // 執行平方探測來尋找目標學號，限制最多探測 tableSize 次以避免進入無窮迴圈
            while (step < tableSize) { 
                // 平方探測公式：(初始雜湊值 + 步數的平方) 取餘數
                int probeIndex = (hashValue + step * step) % tableSize;
                
                // 若遇到空插槽，代表該探測序列已斷，學號必定不存在於表中
                if (hashTable[probeIndex].isEmpty) {
                    break;
                }
                
                // 若該插槽的學號與我們尋找的目標相符，即宣告搜尋成功
                if (hashTable[probeIndex].sid == targetId) {
                    found = true;
                    cout.unsetf(ios::fixed); // 取消小數點強制補零，以符合範例格式要求 (例如 74.17 而非 74.1700)
                    cout << setprecision(6);
                    cout << "\n{ " << targetId << ", " << hashTable[probeIndex].sname << ", " 
                         << hashTable[probeIndex].mean << " } is found after " << (step + 1) << " probes.\n\n";
                    break;
                }
                
                // 若發生碰撞(有資料但學號不符)，則增加步數繼續探測
                step++; 
            }
            
            // 若完整走訪探測序列或遇到空槽仍未找到，輸出失敗提示及總探測次數
            if (!found) {
                int probes = (step < tableSize) ? step + 1 : tableSize;
                cout << "\n" << targetId << " is not found after " << probes << " probes.\n\n";
            }
        }

        return true;
    }
};

// 任務二：以雙重雜湊法 (Double Hashing) 建立雜湊表 Y
class DoubleHashing {
public:
    bool Execute(DataManager& dataManager, string fileNumber) {
        const vector<StudentRecord>& records = dataManager.GetRecords();
        if (records.empty()) {
            return false;
        }

        int tableSize = getPrime(static_cast<int>(records.size() * 1.15));
        int maxStepPrime = getPrime(static_cast<int>(records.size() / 5));

        vector<HashSlotX> hashTable(tableSize);

        int successfulInsertions = 0;
        unsigned long long totalSuccessfulProbes = 0;
        int c = 0;

        for (const auto& rec : records) {
            string sidStr = string(rec.sid, strnlen(rec.sid, 10));
            
            unsigned long long hashValue = 1;
            unsigned long long stepValueProduct = 1;
            
            for (char c : sidStr) {
                hashValue = (hashValue * static_cast<unsigned char>(c)) % tableSize;
                stepValueProduct = (stepValueProduct * static_cast<unsigned char>(c)) % maxStepPrime;
            }

            int stepKey = maxStepPrime - stepValueProduct;

            int i = 0;
            bool inserted = false;
            while (i < tableSize) {
                int probeIndex = (hashValue + i * stepKey) % tableSize;
                
                if (hashTable[probeIndex].isEmpty) {
                    hashTable[probeIndex].hvalue = hashValue;
                    hashTable[probeIndex].sid = sidStr;
                    hashTable[probeIndex].sname = string(rec.sname, strnlen(rec.sname, 10));
                    hashTable[probeIndex].mean = rec.average;
                    hashTable[probeIndex].isEmpty = false;
                    
                    successfulInsertions++;
                    totalSuccessfulProbes += (i + 1); 
                    inserted = true;
                    break;
                }
                i++; 
            }
            // 只有在完整探測後依然找不到空位時，才算真正的「碰撞失敗」
            if (!inserted) {
                cout << "### Failed at [" << c << "]. ###" << endl;
            }
            c++;
        }

        double avgSuccessful = 0.0;
        if (successfulInsertions > 0) {
            avgSuccessful = static_cast<double>(totalSuccessfulProbes) / successfulInsertions;
        }

        cout << "\nHash table has been successfully created by Double hashing   " << endl;
        cout << fixed << setprecision(4);
        cout << "successful search: " << avgSuccessful << " comparisons on average" << endl;

        string outFilename = "double" + fileNumber + ".txt";
        ofstream outFile(outFilename);
        if (!outFile.is_open()) {
            return false;
        }

        outFile << " --- Hash table created by Double hashing    ---" << endl;

        for (int i = 0; i < tableSize; ++i) {
            if (hashTable[i].isEmpty) {
                outFile << "[" << setw(3) << i << "] " << endl;
            } else {
                outFile << right << "[" << setw(3) << i << "]" 
                        << setw(11) << hashTable[i].hvalue << ","
                        << setw(11) << hashTable[i].sid << ","
                        << setw(11) << hashTable[i].sname << ","
                        << setw(11) << hashTable[i].mean << endl;
            }
        }

        outFile << " ----------------------------------------------------- " << endl;
        outFile.close();

        // ==========================================
        // 任務四：以雜湊表搜尋 (Double Hashing)
        // ==========================================
        string targetId;
        // 提供防呆機制，持續要求使用者輸入直到輸入 0 退出為止
        while (true) {
            cout << "Input a student ID to search ([0] Quit): ";
            cin >> targetId;
            if (targetId == "0") {
                cout << endl;
                break; // 輸入 0 則結束搜尋
            }

            // 計算搜尋目標學號的初始雜湊值與步階常數 (與任務二建表時的演算法一致)
            unsigned long long hashValue = 1;
            unsigned long long stepValueProduct = 1;
            
            // 依照作業規範，分別計算雜湊值與步階函式的初始乘積
            for (char c : targetId) {
                hashValue = (hashValue * static_cast<unsigned char>(c)) % tableSize;
                stepValueProduct = (stepValueProduct * static_cast<unsigned char>(c)) % maxStepPrime;
            }
            // 雙重雜湊專用的步階計算公式：最高步階 - (乘積除以最高步階的餘數)
            int stepKey = maxStepPrime - stepValueProduct;

            int i = 0;
            bool found = false;
            
            // 執行雙重雜湊探測來尋找目標學號，限制最多探測 tableSize 次以避免進入無窮迴圈
            while (i < tableSize) { 
                // 雙重雜湊探測公式：(初始雜湊值 + 探測次數 * 專用步階) 取餘數
                int probeIndex = (hashValue + i * stepKey) % tableSize;
                
                // 若遇到空插槽，代表該探測序列已斷，學號必定不存在於表中
                if (hashTable[probeIndex].isEmpty) {
                    break;
                }
                
                // 若該插槽的學號與我們尋找的目標相符，即宣告搜尋成功
                if (hashTable[probeIndex].sid == targetId) {
                    found = true;
                    cout.unsetf(ios::fixed); // 取消小數點強制補零，以符合範例格式要求
                    cout << setprecision(6);
                    cout << "\n{ " << targetId << ", " << hashTable[probeIndex].sname << ", " 
                         << hashTable[probeIndex].mean << " } is found after " << (i + 1) << " probes.\n\n";
                    break;
                }
                
                // 若發生碰撞，則增加步數倍率 (i) 繼續向後探測
                i++; 
            }
            
            // 若完整走訪探測序列或提早遇到空槽仍未找到，輸出失敗提示及總探測次數
            if (!found) {
                int probes = (i < tableSize) ? i + 1 : tableSize;
                cout << "\n" << targetId << " is not found after " << probes << " probes.\n\n";
            }
        }

        return true;
    }
};

void ReadCommand(int &commandChoice) {
    commandChoice = -1; 
    string inputStr;
    while (commandChoice < 0 || commandChoice > 2) { 
        cout << "\n* Data Structures and Algorithms *" << endl;
        cout << "************ Hash Table **********" << endl;
        cout << "* 0. QUIT                        *" << endl;
        cout << "* 1. Quadratic probing           *" << endl;
        cout << "* 2. Double hashing              *" << endl;
        cout << "**********************************" << endl;
        cout << "Input a choice(0, 1, 2): ";
        
        cin >> inputStr;

        if (inputStr == "0") {
            commandChoice = 0;
        } else if (inputStr == "1") {
            commandChoice = 1;
        } else if (inputStr == "2") {
            commandChoice = 2;
        } else {
            cout << "\nCommand does not exist!\n" << endl;
            commandChoice = -1; 
        }
    }
}

int main() {
    int commandChoice = 0;
    DataManager dataManager;
    QuadraticProbing qp;
    DoubleHashing dh;
    string currentFileNum = ""; 

    ReadCommand(commandChoice);

    while (commandChoice != 0) {
        if (commandChoice == 1) {
            string fileNum;
            bool loadSuccess = false;

            while (!loadSuccess) {
                cout << "\nInput a file number ([0] Quit): ";
                cin >> fileNum;

                if (fileNum == "0") {
                    dataManager.Clear();
                    currentFileNum = ""; 
                    cout << endl;
                    break;
                }
                
                if (dataManager.LoadData(fileNum)) {
                    loadSuccess = true;
                    currentFileNum = fileNum; 
                    qp.Execute(dataManager, fileNum);
                }
                else {
                    cout << endl;
                    break;
                }
            }
        } else if (commandChoice == 2) {
            if (dataManager.GetRecords().empty() || currentFileNum.empty()) {
                cout << "### Command 1 first. ###\n" << endl;
            } else {
                dh.Execute(dataManager, currentFileNum);
            }
        }
        
        ReadCommand(commandChoice);
    }

    return 0;
}
