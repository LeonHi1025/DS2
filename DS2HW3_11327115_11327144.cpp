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
struct StudentRecord {
    char sid[10];            
    char sname[10];          
    unsigned char score[6];  
    float average;           
};

// 判斷給定整數是否為質數的輔助函式
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

// 取得大於指定數值 n 的最小質數
int getPrime(int n) {
    int candidate = n + 1;
    while (!isPrime(candidate)) {
        candidate++;
    }
    return candidate;
}

// 雜湊表位址結構體 (HashSlotX)
struct HashSlotX {
    unsigned long long hvalue; 
    string sid;                
    string sname;              
    float mean;                
    bool isEmpty;              
    
    HashSlotX() : hvalue(0), mean(0.0f), isEmpty(true) {}
};

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

        for (const auto& rec : records) {
            string sidStr = string(rec.sid, strnlen(rec.sid, 10));
            
            unsigned long long hashValue = 1;
            for (char c : sidStr) {
                hashValue = (hashValue * static_cast<unsigned char>(c)) % tableSize;
            }

            int step = 0;
            
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
                    break;
                }
                step++;
            }
        }

        unsigned long long totalUnsuccessfulProbes = 0;
        for (int i = 0; i < tableSize; ++i) {
            int step = 0;
            while (step < tableSize) {
                int probeIndex = (i + step * step) % tableSize;
                if (hashTable[probeIndex].isEmpty) {
                    totalUnsuccessfulProbes += step; 
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

        return true;
    }
};

// 任務二：以雙重雜湊 (Double Hashing) 建立雜湊表 Y
class DoubleHashing {
public:
    bool Execute(DataManager& dataManager, string fileNumber) {
        const vector<StudentRecord>& records = dataManager.GetRecords();
        if (records.empty()) {
            return false;
        }

        // 雜湊表大小 = 大於 1.15 倍資料總筆數的最小質數
        int tableSize = getPrime(static_cast<int>(records.size() * 1.15));
        // 最高步階 = 大於(資料總筆數 / 5)的最小質數
        int maxStepPrime = getPrime(static_cast<int>(records.size() / 5));

        vector<HashSlotX> hashTable(tableSize);

        int successfulInsertions = 0;
        unsigned long long totalSuccessfulProbes = 0;

        for (const auto& rec : records) {
            string sidStr = string(rec.sid, strnlen(rec.sid, 10));
            
            unsigned long long hashValue = 1;
            unsigned long long stepValueProduct = 1;
            
            // 依照規範計算雜湊值與步階函式的乘積部分
            for (char c : sidStr) {
                hashValue = (hashValue * static_cast<unsigned char>(c)) % tableSize;
                stepValueProduct = (stepValueProduct * static_cast<unsigned char>(c)) % maxStepPrime;
            }

            // 步階函數：最高步階 - (乘積除以最高步階取餘數)
            int stepKey = maxStepPrime - stepValueProduct;

            int i = 0;
            // 使用雙重雜湊處理碰撞
            while (i < tableSize) {
                int probeIndex = (hashValue + i * stepKey) % tableSize;
                
                if (hashTable[probeIndex].isEmpty) {
                    hashTable[probeIndex].hvalue = hashValue;
                    hashTable[probeIndex].sid = sidStr;
                    hashTable[probeIndex].sname = string(rec.sname, strnlen(rec.sname, 10));
                    hashTable[probeIndex].mean = rec.average;
                    hashTable[probeIndex].isEmpty = false;
                    
                    successfulInsertions++;
                    totalSuccessfulProbes += (i + 1); // 探測次數為 i + 1 
                    break;
                }
                i++;
            }
        }

        double avgSuccessful = 0.0;
        if (successfulInsertions > 0) {
            avgSuccessful = static_cast<double>(totalSuccessfulProbes) / successfulInsertions;
        }

        cout << "\nHash table has been successfully created by Double hashing   " << endl;
        cout << fixed << setprecision(4);
        cout << "successful search: " << avgSuccessful << " comparisons on average" << endl;

        // 輸出至 double[編號].txt 文字檔
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
    string currentFileNum = ""; // 用於記憶任務一載入的檔案編號

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
                    currentFileNum = fileNum; // 成功讀取後記錄編號供任務二使用
                    qp.Execute(dataManager, fileNum);
                }
                else {
                    cout << endl;
                    break;
                }
            }
        } else if (commandChoice == 2) {
            // 檢查是否已存在資料與檔案編號
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
