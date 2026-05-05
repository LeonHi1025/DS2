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
    if (n <= 1) return false;                   // 1 或以下非質數
    if (n == 2 || n == 3) return true;          // 2 與 3 皆為質數
    if (n % 2 == 0 || n % 3 == 0) return false; // 排除 2 與 3 的倍數
    
    // 只檢查 6k - 1 與 6k + 1 的因數，大幅加快運算速度
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }
    return true;
}

// 取得大於等於給定數值 n 的最小質數，主要用於決定雜湊表的大小
int getPrime(int n) {
    int candidate = n + 1; // 從 n+1 開始尋找下一個質數
    while (!isPrime(candidate)) {
        candidate++;
    }
    return candidate;
}

// 雜湊表中的每個插槽結構體，負責記錄該位置是否已有資料及儲存的內容
struct HashSlotX {
    unsigned long long hvalue; // 雜湊鍵值 (Hash Value)
    string sid;                // 學號字串
    string sname;              // 姓名字串
    float mean;                // 平均分數
    bool isEmpty;              // 標記此插槽是否為空
    
    // 建構子：初始化時將插槽預設為空，並給予初始值
    HashSlotX() : hvalue(0), mean(0.0f), isEmpty(true) {}
};

// DataManager 類別：負責處理資料的讀取、寫入與管理
class DataManager {
private:
    vector<StudentRecord> records; // 動態陣列，用以儲存在記憶體中的所有學生紀錄

public:
    // 清空目前載入的所有學生紀錄資料
    void Clear() {
        records.clear();
    }

    // 任務零：讀取文字檔並將其轉換為二進位檔
    bool ExecuteTask0(string fileNumber) {
        string txtFilename = "input" + fileNumber + ".txt";
        string binFilename = "input" + fileNumber + ".bin";

        Clear(); // 先清空既有資料，避免重複載入

        // 嘗試開啟對應的輸入文字檔
        ifstream txtFile(txtFilename);
        if (!txtFile.is_open()) {
            return false;
        }

        string line;
        // 逐行讀取文字檔內容
        while (getline(txtFile, line)) {
            // 略過空白行或僅包含換行符號的行
            if (line.empty() || line == "\r") continue;

            stringstream ss(line);
            string token;
            vector<string> tokens;

            // 以 Tab (\t) 作為分隔符號，切割每一行的資料
            while (getline(ss, token, '\t')) {
                // 若字串尾端夾帶 \r 則將其移除，確保字串純淨
                if (!token.empty() && token.back() == '\r') {
                    token.pop_back();
                }
                tokens.push_back(token);
            }

            // 確保該行至少具有 9 個欄位 (學號, 姓名, 6個成績, 1個平均)
            if (tokens.size() >= 9) {
                StudentRecord record;
                memset(&record, 0, sizeof(StudentRecord)); // 將結構體記憶體歸零初始化

                // 使用 strncpy 複製字串並限制長度以防溢位
                strncpy(record.sid, tokens[0].c_str(), 10);
                strncpy(record.sname, tokens[1].c_str(), 10);

                // 將 6 科成績由字串轉換為整數再轉成 unsigned char
                for (int i = 0; i < 6; ++i) {
                    record.score[i] = static_cast<unsigned char>(stoi(tokens[2 + i]));
                }
                // 將平均成績字串轉為浮點數
                record.average = stof(tokens[8]);

                records.push_back(record);
            }
        }
        txtFile.close();

        // 建立並寫入二進位檔 (以二進位模式開啟)
        ofstream binFile(binFilename, ios::binary);
        if (!binFile.is_open()) {
            return false;
        }

        // 將記憶體中的結構體資料直接逐筆寫入二進位檔案
        for (const auto& record : records) {
            binFile.write(reinterpret_cast<const char*>(&record), sizeof(StudentRecord));
        }
        binFile.close();

        return true;
    }

    // 讀取指定編號的二進位檔案，若不存在則呼叫 ExecuteTask0 嘗試從 txt 轉檔
    bool LoadData(string fileNumber) {
        string binFilename = "input" + fileNumber + ".bin";
        ifstream binFile(binFilename, ios::binary);

        if (binFile.is_open()) {
            Clear();
            StudentRecord record;
            // 迴圈讀取二進位檔內容，每次讀取一個 StudentRecord 的大小
            while (binFile.read(reinterpret_cast<char*>(&record), sizeof(StudentRecord))) {
                records.push_back(record);
            }
            binFile.close();
            return true;
        } else {
            // 若 .bin 不存在，則自動嘗試讀取 .txt 並建立 .bin
            if (ExecuteTask0(fileNumber)) {
                cout << "\n### " << "input" + fileNumber + ".bin does not exist! ###" << endl;
                return true; 
            }

            // 若兩種檔案都無法順利讀取，輸出錯誤訊息
            cout << "\n### " << "input" + fileNumber + ".bin does not exist! ###" << endl;
            cout << "\n### " << "input" + fileNumber + ".txt does not exist! ###" << endl;
            return false;
        }
    }

    // 提供外部唯讀存取資料紀錄
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

        // 計算雜湊表大小：必須是大於資料筆數 1.15 倍的最小質數
        int tableSize = getPrime(static_cast<int>(records.size() * 1.15));
        vector<HashSlotX> hashTable(tableSize); // 建立對應大小的雜湊表

        int successfulInsertions = 0; // 記錄成功插入的資料筆數
        unsigned long long totalSuccessfulProbes = 0; // 記錄插入所有資料時總共探測的次數

        // 遍歷所有學生紀錄並將其插入雜湊表
        for (const auto& rec : records) {
            // 取得實際長度的學號字串，避免尾端含有無效字元
            string sidStr = string(rec.sid, strnlen(rec.sid, 10));
            
            unsigned long long hashValue = 1;
            // 計算學號的初始雜湊值：將每個字元的 ASCII 數值依序相乘再取餘數
            for (char c : sidStr) {
                hashValue = (hashValue * static_cast<unsigned char>(c)) % tableSize;
            }

            int step = 0; // 探測的位移步數
            
            // 執行平方探測，step < tableSize 作為防止無窮迴圈的保護機制
            while (step < tableSize) {
                // 平方探測公式：(初始雜湊值 + 步數的平方) 取餘數
                int probeIndex = (hashValue + step * step) % tableSize;
                
                // 找到空插槽，進行資料寫入
                if (hashTable[probeIndex].isEmpty) {
                    hashTable[probeIndex].hvalue = hashValue;
                    hashTable[probeIndex].sid = sidStr;
                    hashTable[probeIndex].sname = string(rec.sname, strnlen(rec.sname, 10));
                    hashTable[probeIndex].mean = rec.average;
                    hashTable[probeIndex].isEmpty = false;
                    
                    successfulInsertions++;
                    totalSuccessfulProbes += (step + 1); // 本次插入探測了 step + 1 次
                    break;
                }
                step++; // 若發生碰撞，增加步數尋找下一個位址
            }
        }

        // 模擬 Unsuccessful search 的探測次數：從每個可能位址起頭，到遇見空槽為止的碰撞次數總和
        unsigned long long totalUnsuccessfulProbes = 0;
        for (int i = 0; i < tableSize; ++i) {
            int step = 0;
            while (step < tableSize) {
                int probeIndex = (i + step * step) % tableSize;
                // 遇到空插槽即代表搜尋宣告失敗，終止探測
                if (hashTable[probeIndex].isEmpty) {
                    totalUnsuccessfulProbes += step; // 僅加總發生碰撞的次數
                    break;
                }
                step++;
            }
        }

        // 計算 unsuccessful search 與 successful search 的平均次數
        double avgUnsuccessful = static_cast<double>(totalUnsuccessfulProbes) / tableSize;
        double avgSuccessful = 0.0;
        if (successfulInsertions > 0) {
            avgSuccessful = static_cast<double>(totalSuccessfulProbes) / successfulInsertions;
        }

        cout << "\nHash table has been successfully created by Quadratic probing" << endl;
        cout << fixed << setprecision(4);
        cout << "unsuccessful search: " << avgUnsuccessful << " comparisons on average" << endl;
        cout << "successful search: " << avgSuccessful << " comparisons on average" << endl;

        // 準備輸出至對應編號的文字檔
        string outFilename = "quadratic" + fileNumber + ".txt";
        ofstream outFile(outFilename);
        if (!outFile.is_open()) {
            cout << "無法建立輸出檔案 " << outFilename << endl;
            return false;
        }

        outFile << " --- Hash table created by Quadratic probing ---" << endl;

        // 格式化輸出整個雜湊表內容
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

        // 雜湊表大小：必須是大於資料筆數 1.15 倍的最小質數
        int tableSize = getPrime(static_cast<int>(records.size() * 1.15));
        // 最高步階常數：必須是大於 (資料筆數 / 5) 的最小質數
        int maxStepPrime = getPrime(static_cast<int>(records.size() / 5));

        vector<HashSlotX> hashTable(tableSize);

        int successfulInsertions = 0;
        unsigned long long totalSuccessfulProbes = 0;

        for (const auto& rec : records) {
            string sidStr = string(rec.sid, strnlen(rec.sid, 10));
            
            unsigned long long hashValue = 1;
            unsigned long long stepValueProduct = 1;
            
            // 依照作業規範，分別計算雜湊值與步階函式的初始乘積
            for (char c : sidStr) {
                hashValue = (hashValue * static_cast<unsigned char>(c)) % tableSize;
                stepValueProduct = (stepValueProduct * static_cast<unsigned char>(c)) % maxStepPrime;
            }

            // 雙重雜湊專用的步階計算公式：最高步階 - (乘積除以最高步階的餘數)
            int stepKey = maxStepPrime - stepValueProduct;

            int i = 0;
            // 利用雙重雜湊尋找空插槽，i 代表探測的步數倍率
            while (i < tableSize) {
                int probeIndex = (hashValue + i * stepKey) % tableSize;
                
                // 找到空插槽，進行資料寫入
                if (hashTable[probeIndex].isEmpty) {
                    hashTable[probeIndex].hvalue = hashValue;
                    hashTable[probeIndex].sid = sidStr;
                    hashTable[probeIndex].sname = string(rec.sname, strnlen(rec.sname, 10));
                    hashTable[probeIndex].mean = rec.average;
                    hashTable[probeIndex].isEmpty = false;
                    
                    successfulInsertions++;
                    totalSuccessfulProbes += (i + 1); // 總探測次數為目前迴圈次數 + 1
                    break;
                }
                i++; // 若碰撞則擴大步數倍率繼續探測
            }
        }

        // 計算 successful search 的平均次數
        double avgSuccessful = 0.0;
        if (successfulInsertions > 0) {
            avgSuccessful = static_cast<double>(totalSuccessfulProbes) / successfulInsertions;
        }

        cout << "\nHash table has been successfully created by Double hashing   " << endl;
        cout << fixed << setprecision(4);
        cout << "successful search: " << avgSuccessful << " comparisons on average" << endl;

        // 準備輸出至對應編號的 double 格式文字檔
        string outFilename = "double" + fileNumber + ".txt";
        ofstream outFile(outFilename);
        if (!outFile.is_open()) {
            return false;
        }

        outFile << " --- Hash table created by Double hashing    ---" << endl;

        // 格式化輸出整個雜湊表內容
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

        return true;
    }
};

// 負責顯示選單與讀取使用者指令，提供防呆機制
void ReadCommand(int &commandChoice) {
    commandChoice = -1; 
    string inputStr;
    // 若輸入不合法則不斷重新提示
    while (commandChoice < 0 || commandChoice > 2) { 
        cout << "\n* Data Structures and Algorithms *" << endl;
        cout << "************ Hash Table **********" << endl;
        cout << "* 0. QUIT                        *" << endl;
        cout << "* 1. Quadratic probing           *" << endl;
        cout << "* 2. Double hashing              *" << endl;
        cout << "**********************************" << endl;
        cout << "Input a choice(0, 1, 2): ";
        
        cin >> inputStr;

        // 判斷使用者選擇的模式
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

// 主程式進入點
int main() {
    int commandChoice = 0;
    DataManager dataManager;
    QuadraticProbing qp;
    DoubleHashing dh;
    string currentFileNum = ""; // 用於記憶任務一載入的檔案編號，供任務二接續使用

    ReadCommand(commandChoice);

    // 當使用者未選擇離開 (0) 時，持續執行主迴圈
    while (commandChoice != 0) {
        if (commandChoice == 1) {
            string fileNum;
            bool loadSuccess = false;

            // 迴圈直到檔案成功載入或使用者選擇退出
            while (!loadSuccess) {
                cout << "\nInput a file number ([0] Quit): ";
                cin >> fileNum;

                if (fileNum == "0") {
                    dataManager.Clear();
                    currentFileNum = ""; // 清除暫存的檔案編號
                    cout << endl;
                    break;
                }
                
                // 嘗試載入資料，成功後進行平方探測
                if (dataManager.LoadData(fileNum)) {
                    loadSuccess = true;
                    currentFileNum = fileNum; // 記錄目前檔案編號
                    qp.Execute(dataManager, fileNum);
                }
                else {
                    cout << endl;
                    break;
                }
            }
        } else if (commandChoice == 2) {
            // 防呆：確認是否已經載入資料且檔案編號存在，避免任務二讀不到東西
            if (dataManager.GetRecords().empty() || currentFileNum.empty()) {
                cout << "### Command 1 first. ###\n" << endl;
            } else {
                dh.Execute(dataManager, currentFileNum);
            }
        }
        
        // 單次任務執行完畢後，再次提示輸入新指令
        ReadCommand(commandChoice);
    }

    return 0;
}
