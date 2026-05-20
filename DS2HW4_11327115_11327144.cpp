// 11327115 郭琮禮 & 11327144 莊有隆

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <iomanip>

using namespace std;

// 依據規定，讀取二進位檔案的結構
// 發訊者(12 bytes) + 收訊者(12 bytes) + 權重(4 bytes)
#pragma pack(push, 1)
struct Record {
    char putID[12];
    char getID[12];
    float weight;
};
#pragma pack(pop)

// 輔助函式：安全提取 12 bytes 陣列中的學號
// 遇到第一個空白或 \0 即中斷，防止讀入二進位檔末端的亂碼
string trimID(const char* rawID) {
    string s = "";
    for(int i = 0; i < 12; ++i) {
        char c = rawID[i];
        
        // 遇到字串結束符號立刻中斷
        if (c == '\0') break; 
        
        // 遇到空白或換行符號
        if (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
            if (!s.empty()) break; // 若已開始記錄學號，遇到空白表示學號已結束，忽略後續所有字元
            continue; // 跳過前導的空白字元
        }
        s += c;
    }
    return s;
}

// 相鄰串列上的節點 (包含收訊者學號與量化權重)
struct Node {
    string getID;
    float weight;
    
    // 依照收訊者學號由小到大排序
    bool operator<(const Node& other) const {
        return getID < other.getID;
    }
};

// 主陣列的元素 (包含學號以及其對應的串列)
struct Vertex {
    string id;
    vector<Node> edges;
    
    // 依照學號字串由小到大排序
    bool operator<(const Vertex& other) const {
        return id < other.id;
    }
};

// 將相鄰串列定義為專屬的 C++ 類別
class AdjacencyList {
private:
    vector<Vertex> adjList;

    // 輔助函式：利用二元搜尋快速找出學號在主陣列中的索引
    int findVertexIndex(const string& target) {
        int left = 0, right = adjList.size() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (adjList[mid].id == target) return mid;
            if (adjList[mid].id < target) left = mid + 1;
            else right = mid - 1;
        }
        return -1;
    }

public:
    // 任務一：建立相鄰串列
    void buildList(const string& fileNum) {
        string filename = "pairs" + fileNum + ".bin";
        ifstream inFile(filename, ios::binary);
        if (!inFile) {
            cout << "\nError: Cannot open file " << filename << "\n\n";
            return;
        }

        vector<Record> rawRecords;
        vector<string> uniqueIDs;
        Record tempRec;

        // 讀取所有二進位紀錄
        while (inFile.read(reinterpret_cast<char*>(&tempRec), sizeof(Record))) {
            rawRecords.push_back(tempRec);
            
            string putIDStr = trimID(tempRec.putID);
            string getIDStr = trimID(tempRec.getID);
            
            uniqueIDs.push_back(putIDStr);
            uniqueIDs.push_back(getIDStr);
        }
        inFile.close();

        // 將所有的學號排序並去重，做為主陣列的基礎
        sort(uniqueIDs.begin(), uniqueIDs.end());
        uniqueIDs.erase(unique(uniqueIDs.begin(), uniqueIDs.end()), uniqueIDs.end());

        // 初始化主陣列
        adjList.clear();
        for (const string& id : uniqueIDs) {
            Vertex v;
            v.id = id;
            adjList.push_back(v);
        }

        // 將學生互動關係存入對應的串列中
        int totalEdges = 0;
        for (const Record& rec : rawRecords) {
            string putIDStr = trimID(rec.putID);
            string getIDStr = trimID(rec.getID);
            
            int vIndex = findVertexIndex(putIDStr);
            if (vIndex != -1) {
                Node newNode;
                newNode.getID = getIDStr;
                newNode.weight = rec.weight;
                adjList[vIndex].edges.push_back(newNode);
                totalEdges++;
            }
        }

        // 將每個串列上的節點依照【收訊者學號】由小到大排序
        for (Vertex& v : adjList) {
            sort(v.edges.begin(), v.edges.end());
        }

        // 輸出終端機統計資訊
        cout << "\n<<< There are " << adjList.size() << " IDs in total. >>>\n";
        cout << "\n<<< There are " << totalEdges << " nodes in total. >>>\n\n";

        // --- 將相鄰串列依序寫入 .adj 文字檔 ---
        string outFilename = "pairs" + fileNum + ".adj";
        ofstream outFile(outFilename);
        
        // 檔案開頭統計資訊
        outFile << "<<< There are " << adjList.size() << " IDs in total. >>>\n";
        
        // 輸出格式化相鄰串列
        for (size_t i = 0; i < adjList.size(); ++i) {
            outFile << "[" << setw(3) << i + 1 << "] " << adjList[i].id << ": \n";
            
            if (!adjList[i].edges.empty()) {
                for (size_t j = 0; j < adjList[i].edges.size(); ++j) {
                    outFile << "\t(" << setw(2) << j + 1 << ") " 
                            << adjList[i].edges[j].getID << "," 
                            << setw(7) << defaultfloat << adjList[i].edges[j].weight;
                    
                    // 測資格式關鍵 1：每印滿 12 個節點立刻印出換行
                    if ((j + 1) % 12 == 0) {
                        outFile << "\n";
                    }
                }
                // 測資格式關鍵 2：每個學號的所有連線印完後，無條件換行
                // (如果數量剛好是 12 的倍數，這行就會產生測資裡的那個「空行」，完美吻合排版)
                outFile << "\n";
            }
        }
        
        // 檔案結尾統計資訊
        outFile << "<<< There are " << totalEdges << " nodes in total. >>>\n";
        
        outFile.close();
    }
    
    // 預留任務二的方法成員介面
    void computeCounts() {
        // TODO: 實作連通數計算
    }
};

void printMenu() {
    cout << "* Data Structures and Algorithms *" << endl;
    cout << "**** Graph data manipulation *****" << endl;
    cout << "* 0. QUIT                        *" << endl;
    cout << "* 1. Build adjacency lists       *" << endl;
    cout << "* 2. Compute connection counts   *" << endl;
    cout << "**********************************" << endl;
    cout << "Input a choice(0, 1, 2): ";
}

int main() {
    int choice;
    string fileNum;
    AdjacencyList graph;

    while (true) {
        printMenu();
        
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "\nInvalid input! Please try again.\n\n";
            continue;
        }

        if (choice == 0) {
            break;
        } else if (choice == 1) {
            cout << "\nInput a file number ([0] Quit): ";
            cin >> fileNum;
            if (fileNum == "0") continue;
            graph.buildList(fileNum);
        } else if (choice == 2) {
            cout << "\nTask 2 (Compute connection counts) is not fully implemented yet.\n\n";
        } else {
            cout << "\nInvalid choice, please try again.\n\n";
        }
    }
    
    return 0;
}
