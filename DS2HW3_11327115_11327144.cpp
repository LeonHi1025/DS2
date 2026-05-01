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

// 定義任務零要求的資料結構體
// 此結構體用於暫存每筆從 txt 檔案解析出來的學生紀錄
struct StudentRecord {
    char sid[10];            // 學號：固定大小 10 個字元的字元陣列
    char sname[10];          // 姓名：固定大小 10 個字元的字元陣列
    unsigned char score[6];  // 分數：6 個科目，各自以 unsigned char (整數 0~255) 儲存
    float average;           // 平均分數：以 float 浮點數型態儲存
};

// 判斷給定整數是否為質數的輔助函式
bool isPrime(int n) {
    if (n <= 1) return false;                   // 1 或以下不是質數
    if (n == 2 || n == 3) return true;          // 2, 3 是最基本的質數
    if (n % 2 == 0 || n % 3 == 0) return false; // 排除 2 與 3 的倍數，加速後續運算
    
    // 利用 6k ± 1 的特性進行快速質數驗證
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }
    return true;
}

// 取得大於指定數值 n 的最小質數
// 用於計算「大於 1.15 倍資料總筆數的最小質數」來作為雜湊表大小
int getPrime(int n) {
    int candidate = n + 1;
    while (!isPrime(candidate)) {
        candidate++;
    }
    return candidate;
}

// 任務一的雜湊表位址結構體 (HashSlotX)
// 負責存放雜湊表中每個位置的實際資料與狀態
struct HashSlotX {
    unsigned long long hvalue; // 雜湊值 (初始的 hash key 值)
    string sid;                // 學號
    string sname;              // 姓名
    float mean;                // 平均分數
    bool isEmpty;              // 標記此位址目前是否為空，用來判斷是否可以插入資料
    
    // 建構子：預設初始化時，將位址標示為空 (isEmpty = true)
    HashSlotX() : hvalue(0), mean(0.0f), isEmpty(true) {}
};

class DataManager {
private:
    vector<StudentRecord> records; // 使用動態陣列 (vector) 儲存解析後的二進位檔案資料，滿足不固定筆數之規定

public:
    // 清空現有記憶體中的資料
    void Clear() {
        records.clear();
    }

    // 任務零：讀取 txt 文字檔，將各個欄位轉型後寫入 .bin 二進位檔案
    bool ExecuteTask0(string fileNumber) {
        string txtFilename = "input" + fileNumber + ".txt";
        string binFilename = "input" + fileNumber + ".bin";

        // 清空之前的資料，避免疊加
        Clear();

        ifstream txtFile(txtFilename);
        if (!txtFile.is_open()) {
            cout << "### " << txtFilename << " does not exist! ###\n" << endl;
            return false;
        }

        string line;
        // 逐行讀取 txt 的學生紀錄 (沒有標題列)
        while (getline(txtFile, line)) {
            // 跳過空行或是純換行符號 (\r)
            if (line.empty() || line == "\r") continue;

            stringstream ss(line);
            string token;
            vector<string> tokens;

            // 依據 tab ('\t') 進行字串切割，提取所有欄位
            while (getline(ss, token, '\t')) {
                // 處理 Windows 結尾可能殘留的 \r 符號
                if (!token.empty() && token.back() == '\r') {
                    token.pop_back();
                }
                tokens.push_back(token);
            }

            // 確保一行至少包含 9 個欄位 (sid, sname, 6 個 score, average)
            if (tokens.size() >= 9) {
                StudentRecord record;
                memset(&record, 0, sizeof(StudentRecord)); // 初始化結構體記憶體為 0

                // 複製學號 (最多 10 字元)，超出的部分會被自動截斷
                strncpy(record.sid, tokens[0].c_str(), 10);
                
                // 複製姓名 (最多 10 字元)
                strncpy(record.sname, tokens[1].c_str(), 10);

                // 將 6 個分數轉為整數 (int) 後，再強制轉型為 unsigned char
                for (int i = 0; i < 6; ++i) {
                    record.score[i] = static_cast<unsigned char>(stoi(tokens[2 + i]));
                }

                // 處理平均分數，字串轉為 float
                record.average = stof(tokens[8]);

                // 存入動態陣列 vector
                records.push_back(record);
            }
        }
        txtFile.close();

        // 以二進位模式開啟並寫入資料至 .bin
        ofstream binFile(binFilename, ios::binary);
        if (!binFile.is_open()) {
            return false;
        }

        // 使用 reinterpret_cast 將結構體記憶體完整寫入檔案
        for (const auto& record : records) {
            binFile.write(reinterpret_cast<const char*>(&record), sizeof(StudentRecord));
        }
        binFile.close();

        return true;
    }

    // 檢查並讀取資料 (未來其他任務使用)
    bool LoadData(string fileNumber) {
        string binFilename = "input" + fileNumber + ".bin";
        ifstream binFile(binFilename, ios::binary);

        // 如果二進位檔案存在，優先讀取
        if (binFile.is_open()) {
            Clear();
            StudentRecord record;
            while (binFile.read(reinterpret_cast<char*>(&record), sizeof(StudentRecord))) {
                records.push_back(record);
            }
            binFile.close();
            return true;
        } else {
            // 若只有 txt，則先執行任務零轉檔
            cout << "\n### " << binFilename << " does not exist! ###" << endl;
            if (ExecuteTask0(fileNumber)) {
                return true; // 已經轉檔且載入至記憶體
            }
            return false;
        }
    }
    
    // 取得資料
    const vector<StudentRecord>& GetRecords() const {
        return records;
    }

    // 任務一：以平方探測 (Quadratic Probing) 建立雜湊表 X
    // 處理發生碰撞的情況，並計算搜尋的平均比較次數
    bool ExecuteTask1(string fileNumber) {
        // 先嘗試載入二進位檔案資料
        if (!LoadData(fileNumber)) {
            return false;
        }

        if (records.empty()) {
            cout << "沒有資料可供建立雜湊表！" << endl;
            return false;
        }

        // (1) 計算雜湊表大小 = 大於 1.15 倍資料總筆數的最小質數
        int tableSize = getPrime(static_cast<int>(records.size() * 1.15));
        
        // 建立一個指定大小的 Hash 表格，預設全部都是 empty 狀態
        vector<HashSlotX> hashTable(tableSize);

        // 用來追蹤「搜尋現存值」的統計變數
        int successfulInsertions = 0;
        unsigned long long totalSuccessfulProbes = 0;

        // 依序讀取每筆 record 並插入雜湊表
        for (const auto& rec : records) {
            // 從 char array 中取出實際字串，只取到第一個 null char，以防原本資料尾端有補零
            string sidStr = string(rec.sid, strnlen(rec.sid, 10));
            
            // 計算初始雜湊值: 
            // 根據規定「學號每個字元對應的 ASCII 編碼相乘」，為了防止 32-bit int 溢位，採用 unsigned long long。
            // 每次相乘後即刻取餘數，保持數值在合理範圍內。
            unsigned long long hashValue = 1;
            for (char c : sidStr) {
                hashValue = (hashValue * static_cast<unsigned char>(c)) % tableSize;
            }

            int step = 0;
            bool inserted = false;
            
            // 採用平方探測 (Quadratic Probing) 解決碰撞: H_i = (H(key) + step^2) % tableSize
            // 條件設定 step < tableSize 作為防呆機制，避免無空位時產生無限迴圈
            while (step < tableSize) {
                int probeIndex = (hashValue + step * step) % tableSize;
                
                // 如果探測的位址為空，則成功插入資料
                if (hashTable[probeIndex].isEmpty) {
                    hashTable[probeIndex].hvalue = hashValue;
                    hashTable[probeIndex].sid = sidStr;
                    
                    string snameStr = string(rec.sname, strnlen(rec.sname, 10));
                    hashTable[probeIndex].sname = snameStr;
                    
                    hashTable[probeIndex].mean = rec.average;
                    hashTable[probeIndex].isEmpty = false;
                    inserted = true;
                    
                    // 紀錄成功加入的數量與消耗的探測次數
                    successfulInsertions++;
                    totalSuccessfulProbes += (step + 1); // 探測次數為 step + 1 (因為 step 從 0 開始)
                    break;
                }
                // 若位址被佔用，則增加 step 準備下一次平方探測
                step++;
            }

            // 若 step 超過 tableSize 仍未找到空位，則放棄該筆資料
            if (!inserted) {
                cout << "無法加入資料: 學號 " << sidStr << endl;
            }
        }

        // 搜尋不存在值的平均比較次數 (Unsuccessful Search)
        // 模擬從每一個可能發生的 Hash Index (0 ~ tableSize-1) 出發，
        // 需要經過幾次「被佔用」的位址碰撞，才會遇到第一個「空位址」宣告搜尋失敗。
        unsigned long long totalUnsuccessfulProbes = 0;
        for (int i = 0; i < tableSize; ++i) {
            int step = 0;
            while (step < tableSize) {
                int probeIndex = (i + step * step) % tableSize;
                // 遇到空位即停止探測
                if (hashTable[probeIndex].isEmpty) {
                    // 依據範例輸出，比較空位不計入 collision 次數，因此僅加上 step
                    totalUnsuccessfulProbes += step; 
                    break;
                }
                step++;
            }
        }

        // 計算精準的平均值
        double avgUnsuccessful = static_cast<double>(totalUnsuccessfulProbes) / tableSize;
        double avgSuccessful = 0.0;
        if (successfulInsertions > 0) {
            avgSuccessful = static_cast<double>(totalSuccessfulProbes) / successfulInsertions;
        }

        // 輸出統計結果至螢幕 (要求輸出至小數點後四位)
        cout << "\nHash table has been successfully created by Quadratic probing" << endl;
        cout << fixed << setprecision(4);
        cout << "unsuccessful search: " << avgUnsuccessful << " comparisons on average" << endl;
        cout << "successful search: " << avgSuccessful << " comparisons on average" << endl;

        // 輸出雜湊表 X 的內容至 quadratic[編號].txt 文字檔
        string outFilename = "quadratic" + fileNumber + ".txt";
        ofstream outFile(outFilename);
        if (!outFile.is_open()) {
            cout << "無法建立輸出檔案 " << outFilename << endl;
            return false;
        }

        // 依序從位址 [0] 開始逐筆輸出
        for (int i = 0; i < tableSize; ++i) {
            if (hashTable[i].isEmpty) {
                // 無資料則在位址後留空白
                outFile << "[" << i << "]\t" << endl;
            } else {
                // 有資料則印出規定欄位
                outFile << "[" << i << "]\t" 
                        << hashTable[i].hvalue << "\t"
                        << hashTable[i].sid << "\t"
                        << hashTable[i].sname << "\t"
                        << hashTable[i].mean << endl;
            }
        }
        outFile.close();

        return true;
    }
};

int main() {
    int command;
    DataManager dataManager;

    do {
        cout << "\n* Data Structures and Algorithms *" << endl;
        cout << "************ Hash Table **********" << endl;
        cout << "* 0. QUIT                        *" << endl;
        cout << "* 1. Quadratic probing           *" << endl;
        cout << "**********************************" << endl;
        cout << "Input a choice(0, 1, 2): ";
        cin >> command;

        if (command == 0) {
            break;
        } else if (command == 1) {
            string fileNum;
            cout << "\nInput a file number ([0] Quit): ";
            cin >> fileNum;
            if (fileNum == "0") continue;
            dataManager.ExecuteTask1(fileNum);
        } else {
            cout << "\nCommand not exist!" << endl;
        }

    } while (true);

    return 0;
}

