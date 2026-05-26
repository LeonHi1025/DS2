// 11327115 郭琮禮 & 11327144 莊有隆

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <queue>
#include <chrono>

using namespace std;

// 資料結構定義區
// 依據規定，讀取二進位檔案的結構
// 使用 #pragma pack(push, 1) 取消編譯器的記憶體對齊 (Memory Alignment) 最佳化
// 確保這個 struct 佔用的空間精確為 12 + 12 + 4 = 28 bytes，避免讀取二進位檔時發生錯位。
#pragma pack(push, 1)
struct Record {
    char putID[12]; // 發訊者學號 (12 bytes)
    char getID[12]; // 收訊者學號 (12 bytes)
    float weight;   // 權重 (4 bytes)
};
#pragma pack(pop)

// 輔助函式：安全提取 12 bytes 字元陣列中的學號並轉成 C++ string
// 因為讀取進來的字元陣列可能會有空白、換行，或者沒有結尾字元 '\0'
string trimID(const char* rawID) {
    string s = "";
    for(int i = 0; i < 12; ++i) {
        char c = rawID[i];
        
        // 遇到正常的 C-style 字串結束符號立刻中斷
        if (c == '\0') break; 
        
        // 遇到空白或特殊換行符號時的處理
        if (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
            if (!s.empty()) break; // 如果已經讀到部分學號，遇到空白代表學號結束
            continue; // 如果還沒讀到學號，遇到空白則跳過（忽略前導空白）
        }
        s += c; // 將有效字元加入字串
    }
    return s;
}

// 相鄰串列上的「邊」節點 (包含收訊者學號與量化權重)
struct Node {
    string getID;
    float weight;
    
    // 多載小於運算子 (<)，讓 sort() 函式可以依照「收訊者學號字串」由小到大排序
    bool operator<(const Node& other) const {
        return getID < other.getID;
    }
};

// 主陣列的「頂點」元素 (包含發訊者學號，以及其對應的所有邊)
struct Vertex {
    string id;
    vector<Node> edges; // 儲存該發訊者連出去的所有邊 (即相鄰串列)
    
    // 多載小於運算子 (<)，讓 sort() 函式可以依照「發訊者學號字串」由小到大排序
    bool operator<(const Vertex& other) const {
        return id < other.id;
    }
};

// 核心圖形類別 (Adjacency List)
class AdjacencyList {
private:
    vector<Vertex> adjList; // 主陣列，存放所有獨立的發訊者

    // 輔助函式：利用「二元搜尋法 (Binary Search)」快速找出學號在主陣列中的索引位置
    // 時間複雜度：O(log N)，比線性搜尋快很多
    int findVertexIndex(const string& target) {
        int left = 0, right = adjList.size() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (adjList[mid].id == target) return mid;         // 找到了
            if (adjList[mid].id < target) left = mid + 1;      // 目標在右半邊
            else right = mid - 1;                              // 目標在左半邊
        }
        return -1; // 找不到回傳 -1
    }

public:
    // 清空圖形資料
    void clearGraph() {
        adjList.clear();
    }

    // 檢查圖形是否為空 (用於任務二、三、四的防呆)
    bool isEmpty() const {
        return adjList.empty();
    }

    // 任務一：建立相鄰串列 (Build adjacency lists)
    void buildList(const string& fileNum) {
        string filename = "pairs" + fileNum + ".bin";
        ifstream inFile(filename, ios::binary);
        if (!inFile) {
            cout << "\n### " << filename << " does not exist! ###\n\n";
            return;
        }

        vector<Record> rawRecords;
        vector<string> uniqueIDs;
        Record tempRec;

        // 1. 讀取所有二進位紀錄到記憶體中
        while (inFile.read(reinterpret_cast<char*>(&tempRec), sizeof(Record))) {
            rawRecords.push_back(tempRec);
            
            // 清理字串並存入，用來統計有哪些不重複的學號
            string putIDStr = trimID(tempRec.putID);
            string getIDStr = trimID(tempRec.getID);
            
            uniqueIDs.push_back(putIDStr);
            uniqueIDs.push_back(getIDStr);
        }
        inFile.close();

        // 2. 將所有的學號排序並去重，建立主陣列的基礎 (發訊者清單)
        sort(uniqueIDs.begin(), uniqueIDs.end());
        uniqueIDs.erase(unique(uniqueIDs.begin(), uniqueIDs.end()), uniqueIDs.end());

        // 初始化主陣列
        adjList.clear();
        for (const string& id : uniqueIDs) {
            Vertex v;
            v.id = id;
            adjList.push_back(v);
        }

        // 3. 再次掃描所有紀錄，將邊 (Edge) 建立在對應的相鄰串列上
        int totalEdges = 0;
        for (const Record& rec : rawRecords) {
            string putIDStr = trimID(rec.putID);
            string getIDStr = trimID(rec.getID);
            
            int vIndex = findVertexIndex(putIDStr); // 找到發訊者的索引
            if (vIndex != -1) {
                Node newNode;
                newNode.getID = getIDStr;
                newNode.weight = rec.weight;
                adjList[vIndex].edges.push_back(newNode); // 將收訊者加入串列
                totalEdges++;
            }
        }

        // 4. 依照要求，將每個串列上的節點依照【收訊者學號】由小到大排序
        for (Vertex& v : adjList) {
            sort(v.edges.begin(), v.edges.end());
        }

        // 5. 輸出終端機統計資訊
        cout << "\n<<< There are " << adjList.size() << " IDs in total. >>>\n";
        cout << "\n<<< There are " << totalEdges << " nodes in total. >>>\n\n";

        // 6. 將相鄰串列結果寫入 .adj 文字檔
        string outFilename = "pairs" + fileNum + ".adj";
        ofstream outFile(outFilename);
        
        outFile << "<<< There are " << adjList.size() << " IDs in total. >>>\n";
        
        for (size_t i = 0; i < adjList.size(); ++i) {
            outFile << "[" << setw(3) << i + 1 << "] " << adjList[i].id << ": \n";
            
            if (!adjList[i].edges.empty()) {
                for (size_t j = 0; j < adjList[i].edges.size(); ++j) {
                    outFile << "\t(" << setw(2) << j + 1 << ") " 
                            << adjList[i].edges[j].getID << "," 
                            << setw(7) << defaultfloat << adjList[i].edges[j].weight;
                    
                    // 每輸出 12 筆紀錄就換行 (測資格式要求)
                    if ((j + 1) % 12 == 0) outFile << "\n";
                }
                outFile << "\n";
            }
        }
        
        outFile << "<<< There are " << totalEdges << " nodes in total. >>>\n";
        outFile.close();
    }
    
    // 任務二：計算連通數 (Compute connection counts)
    void computeCounts(const string& fileNum) {
        if (adjList.empty()) {
            cout << "### There is no graph and choose 1 first. ###\n\n";
            return;
        }

        // 宣告一個內部結構來儲存計算結果，方便後續排序
        struct ConnectionResult {
            string putID;               
            int count;                  
            vector<string> reachedIDs;  
            
            // 排序規則：連通數由大到小排序；若相同則按學號由小到大
            bool operator<(const ConnectionResult& other) const {
                if (count != other.count) return count > other.count; 
                return putID < other.putID; 
            }
        };

        vector<ConnectionResult> results;
        
        // 對每一位發訊者執行廣度優先搜尋 (BFS)
        for (size_t i = 0; i < adjList.size(); ++i) {
            vector<bool> visited(adjList.size(), false); // 紀錄是否走訪過，避免無窮迴圈
            queue<int> q; // BFS 使用的佇列
            vector<string> reached; // 記錄所有成功抵達的收訊者學號
            
            visited[i] = true;
            q.push(i);
            
            while (!q.empty()) {
                int u = q.front();
                q.pop();
                
                // 走訪該節點連出去的所有邊
                for (const Node& edge : adjList[u].edges) {
                    int v = findVertexIndex(edge.getID);
                    
                    if (v != -1 && !visited[v]) {
                        visited[v] = true;             
                        reached.push_back(edge.getID); 
                        q.push(v);                     
                    }
                }
            }
            
            // 將找出的收訊者排序
            sort(reached.begin(), reached.end());
            
            ConnectionResult res;
            res.putID = adjList[i].id;
            res.count = reached.size();    
            res.reachedIDs = reached;      
            results.push_back(res);
        }
        
        // 依照規定排序發訊者的結果
        sort(results.begin(), results.end());

        cout << "\n<<< There are " << results.size() << " IDs in total. >>>\n\n";

        // 寫入 pairsXXX.cnt 檔案
        string outFilename = "pairs" + fileNum + ".cnt";
        ofstream outFile(outFilename);
        
        outFile << "<<< There are " << results.size() << " IDs in total. >>>\n";
        
        for (size_t i = 0; i < results.size(); ++i) {
            outFile << "[" << setw(3) << i + 1 << "] " << results[i].putID << "(" << results[i].count << "): \n";
            
            if (!results[i].reachedIDs.empty()) {
                for (size_t j = 0; j < results[i].reachedIDs.size(); ++j) {
                    outFile << "\t(" << setw(2) << j + 1 << ") " << results[i].reachedIDs[j];
                    if ((j + 1) % 12 == 0) outFile << "\n";
                }
                outFile << "\n";
            }
        }
        
        outFile.close();
    }

    // 任務三：計算影響力 (Estimate influence values)
    void estimateInfluence(const string& fileNum) {
        if (adjList.empty()) {
            cout << "### There is no graph and choose 1 first. ###\n\n";
            return;
        }

        float threshold = 0.0f;
        // 防呆機制：持續要求輸入直到給出合法範圍 [0.66, 1.0] 內的數字
        while (true) {
            cout << "\nInput a real number in [0.66,1.0]: ";
            
            string inputStr;
            // 如果 cin 失敗 (例如遇到 EOF)，清空錯誤旗標並重試
            if (!(cin >> inputStr)) {
                cin.clear();
                cin.ignore(10000, '\n');
                continue;
            }

            // 計算輸入字串中小數點 '.' 的數量
            int dotCount = count(inputStr.begin(), inputStr.end(), '.');
            
            // 如果小數點超過或等於 2 個 (例如輸入 0.6.6)，視為無效，直接印出錯誤訊息
            if (dotCount >= 2) {
                cout << "\n### It is NOT in [0.66,1.0] ###\n";
                continue;
            }

            try {
                // 嘗試將字串轉為 float 型態
                threshold = stof(inputStr);
                
                // 檢查是否在合法門檻範圍內
                if (threshold >= 0.66f && threshold <= 1.0f) {
                    break; // 合法輸入，跳出迴圈
                } else if (threshold >= 0) {
                    // 如果大於等於 0 且超出範圍，印出錯誤訊息
                    // (若是小於 0 的負數，則按照原本的設計，默默忽略不印訊息並重來)
                    cout << "\n### It is NOT in [0.66,1.0] ###\n";
                }
            } catch (...) {
                // stof 拋出例外，代表字串含有非數字 (例如 "abc")，默默忽略重新要求輸入
                continue;
            }
        }

        struct ConnectionResult {
            string putID;
            int count;
            vector<string> reachedIDs;
            
            // 排序規則：影響力由大到小排序，若相同則學號由小到大
            bool operator<(const ConnectionResult& other) const {
                if (count != other.count) return count > other.count;
                return putID < other.putID;
            }
        };

        vector<ConnectionResult> results;

        // 對每一位發訊者執行深度優先搜尋 (DFS)
        for (size_t i = 0; i < adjList.size(); ++i) {
            vector<bool> visited(adjList.size(), false);
            vector<string> reached;

            visited[i] = true; // 將起點本身設為已造訪，避免將自己算入收訊者中

            // 使用 C++14 支援的 Lambda 遞迴進行 DFS 走訪 (self 寫法)
            auto dfs = [&](auto& self, int currentIdx) -> void {
                for (const Node& edge : adjList[currentIdx].edges) {
                    // 只有當權重 >= 使用者輸入的 threshold 門檻，才算是「有效邊」繼續往下走
                    if (edge.weight >= threshold) {
                        int v = findVertexIndex(edge.getID);
                        if (v != -1 && !visited[v]) {
                            visited[v] = true;             // 標記為已造訪
                            reached.push_back(edge.getID); // 紀錄這個收訊者
                            self(self, v);                 // 遞迴繼續探索
                        }
                    }
                }
            };

            // 從節點 i 開始 DFS 走訪
            dfs(dfs, i);

            // 依規定，只保留影響力大於 0 (正值) 的發訊者
            if (!reached.empty()) {
                sort(reached.begin(), reached.end());
                ConnectionResult res;
                res.putID = adjList[i].id;
                res.count = reached.size();
                res.reachedIDs = reached;
                results.push_back(res);
            }
        }

        // 針對發訊者的影響力結果進行整體排序
        sort(results.begin(), results.end());

        cout << "\n<<< There are " << results.size() << " IDs in total. >>>\n\n";

        // 寫入 pairsXXX.inf 檔案
        string outFilename = "pairs" + fileNum + ".inf";
        ofstream outFile(outFilename);
        
        outFile << "<<< There are " << results.size() << " IDs in total. >>>\n";
        
        for (size_t i = 0; i < results.size(); ++i) {
            outFile << "[" << setw(3) << i + 1 << "] " << results[i].putID << "(" << results[i].count << "): \n";
            
            for (size_t j = 0; j < results[i].reachedIDs.size(); ++j) {
                outFile << "\t(" << setw(2) << j + 1 << ") " << results[i].reachedIDs[j];
                
                // 測資格式要求：印滿 12 個學號立刻換行
                if ((j + 1) % 12 == 0) outFile << "\n";
            }
            outFile << "\n";
        }
        outFile.close();
    }
    
    // 任務四：找出前 K 名估計影響力 (Find top-k influence values)
    void findTopKInfluence(const string& fileNum) {
        // 任務防呆：若圖尚未建立 (主陣列為空)，輸出錯誤訊息並返回
        if (adjList.empty()) {
            cout << "### There is no graph and choose 1 first. ###\n\n";
            return;
        }

        struct InfluenceResult {
            string putID;
            int influence;

            // 排序規則：影響力由大到小 (Descending)，若同分則學號由小到大 (Ascending)
            bool operator<(const InfluenceResult& other) const {
                if (influence != other.influence) {
                    return influence > other.influence; 
                }
                return putID < other.putID; 
            }
        };

        vector<InfluenceResult> results;
        results.reserve(adjList.size()); // 預先配置記憶體，加速 push_back

        // 開始測量執行時間
        auto start = chrono::high_resolution_clock::now();

        // 走訪主陣列，計算每人的「估計影響力」，門檻固定為 0.66 (這裡使用 BFS)
        for (size_t i = 0; i < adjList.size(); ++i) {
            vector<bool> visited(adjList.size(), false);
            queue<int> q;
            int count = 0;

            visited[i] = true;
            q.push(i);

            while (!q.empty()) {
                int u = q.front();
                q.pop();

                for (const Node& edge : adjList[u].edges) {
                    // 任務四固定門檻 0.66
                    if (edge.weight >= 0.66) {
                        int v = findVertexIndex(edge.getID);
                        if (v != -1 && !visited[v]) {
                            visited[v] = true;
                            count++; // 計算影響人數 (收訊者總數)
                            q.push(v);
                        }
                    }
                }
            }

            InfluenceResult res;
            res.putID = adjList[i].id;
            res.influence = count;
            results.push_back(res);
        }

        // 結束測量時間並計算總耗時 (毫秒)
        auto end = chrono::high_resolution_clock::now();
        auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        // 排序所有計算結果
        sort(results.begin(), results.end());

        // 依照規定，只保留估計影響力為正值者 (>0)
        vector<InfluenceResult> positiveResults;
        for (const auto& res : results) {
            if (res.influence > 0) {
                positiveResults.push_back(res);
            }
        }

        int M = positiveResults.size(); // M 代表具有正影響力的人數上限

        // 輸出執行時間 (格式化要求)
        cout << "\n[Elapsed time] " << elapsed_ms << " ms\n\n";

        // 防呆機制：處理使用者輸入的前 K 名
        int K = 0;
        string inputStr;
        while (true) {
            cout << "Input an integer to show top-K in [1," << M << "]: ";
            
            if (!(cin >> inputStr)) {
                cin.clear();
                cin.ignore(10000, '\n');
                continue;
            }

            // 如果字串中包含小數點 '.'，視為非整數輸入，默默換行重試
            if (inputStr.find('.') != string::npos) {
                cout << endl;
                continue;
            }

            try {
                size_t pos;
                K = stoi(inputStr, &pos); // 字串轉為整數
                
                // pos 會回傳成功轉換的字元數量，如果跟字串長度不符，代表有混雜英文字母 (例如 "12abc")
                if (pos != inputStr.length()) {
                    continue; 
                }

                // 檢查是否在合法範圍 [1, M]
                if (K >= 1 && K <= M) {
                    break; // 合法輸入，跳出迴圈
                } else if (K < 0) {
                    cout << endl;
                    continue; // 負數默默換行重試，不印錯誤訊息
                } else {
                    // K == 0 或是 K > M 時，印出指定錯誤訊息
                    cout << "\n### " << K << " is NOT in [1," << M << "] ###\n\n";
                }
            } catch (...) {
                cout << endl;
                // stof 拋出例外 (純英文字母等)，默默換行重試
                continue;
            }
        }

        // 計算要輸出的實際筆數 (因為如果第 K 名有同分者，同分的必須全部印出)
        int outputCount = 0;
        if (!positiveResults.empty()) {
            if (K >= M) {
                outputCount = M; // 要求的數量已經超過或等於全部人數
            } else {
                int thresholdValue = positiveResults[K - 1].influence; // 找出第 K 名的影響力分數
                outputCount = K;
                // 繼續往後找，若分數跟第 K 名一樣，就增加輸出數量
                while (outputCount < M && positiveResults[outputCount].influence == thresholdValue) {
                    outputCount++;
                }
            }
        }

        cout << endl;

        // 逐筆輸出前 K 名至終端機，格式：<rank> ID: influence
        for (int i = 0; i < outputCount; ++i) {
            cout << "<" << i + 1 << "> " << positiveResults[i].putID << ": " << positiveResults[i].influence << endl;
        }
        cout << endl;
    }
};

void printMenu() {
    cout << "* Data Structures and Algorithms *" << endl;
    cout << "**** Graph data manipulation *****" << endl;
    cout << "* 0. QUIT                        *" << endl;
    cout << "* 1. Build adjacency lists       *" << endl;
    cout << "* 2. Compute connection counts   *" << endl;
    cout << "* 3. Estimate influence values   *" << endl;
    cout << "* 4. Find top-k influence values *" << endl;
    cout << "**********************************" << endl;
    cout << "Input a choice(0, 1, 2, 3, 4): ";
}

int main() {
    string choice;
    string fileNum;
    AdjacencyList graph;

    // 無窮迴圈，直到使用者輸入 0 才會 break 結束程式
    while (true) {
        printMenu();
        
        // 讀取選單指令的防呆
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "\nCommand does not exist!\n\n";
            continue;
        }

        if (choice == "0") {
            break; // 0: 退出程式
        } else if (choice == "1") {
            cout << "\nInput a file number ([0] Quit): ";
            cin >> fileNum;
            
            if (fileNum == "0") {
                graph.clearGraph(); // 依照規格，若在選項1輸入0，需清空原本建立的圖
                cout << endl;
                continue;
            }
            graph.buildList(fileNum);
        } else if (choice == "2") {
            graph.computeCounts(fileNum);
        } else if (choice == "3") {
            graph.estimateInfluence(fileNum);
        } else if (choice == "4") {
            // 任務四：在呼叫前再做一次圖形是否為空的防呆檢查
            if (graph.isEmpty()) {
                cout << "### There is no graph and choose 1 first. ###\n\n";
            } else {
                graph.findTopKInfluence(fileNum);
            }
        } else {
            // 如果輸入的不是 0~4 之間的字串，印出錯誤訊息
            cout << "\nCommand does not exist!\n\n";
        }
    }
    
    return 0;
}
