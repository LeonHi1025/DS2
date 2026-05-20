// 11327115 郭琮禮 & 11327144 莊有隆

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <queue>

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
    
    // 任務二：計算連通數
    // 定義：學號 X 的連通數 = 從學號 X 開始傳遞訊息(有向路徑)走訪過的相異學號總數。
    // 實作方式：對每一位「發訊者」執行廣度優先搜尋 (BFS)，並計算可抵達的「相異收訊者」數量。
    void computeCounts(const string& fileNum) {
        // 若圖尚未建立 (主陣列為空)，依規定輸出錯誤訊息並返回
        if (adjList.empty()) {
            cout << "### There is no graph and choose 1 first. ###\n\n";
            return;
        }

        // 內部結構：用來儲存單一發訊者的連通數計算結果
        struct ConnectionResult {
            string putID;               // 發訊者學號
            int count;                  // 連通數 (可達的相異收訊者數量)
            vector<string> reachedIDs;  // 走訪過程收集到的相異收訊者學號名單
            
            // 自訂排序規則：供 sort() 使用，以滿足輸出格式要求
            bool operator<(const ConnectionResult& other) const {
                if (count != other.count) {
                    // 1. 主要排序：依照連通數由大到小排序 (Descending)
                    return count > other.count; 
                }
                // 2. 次要排序 (Tie-breaker)：若連通數相同，依照發訊者學號字串由小到大排序 (Ascending)
                return putID < other.putID; 
            }
        };

        // 儲存全班每一位發訊者的最終連通結果
        vector<ConnectionResult> results;
        
        // 走訪主陣列上的每一位發訊者，逐一進行 BFS
        for (size_t i = 0; i < adjList.size(); ++i) {
            // visited 用於記錄各節點是否已造訪過，避免無限迴圈。大小等同於圖的節點總數
            vector<bool> visited(adjList.size(), false);
            // BFS 使用的佇列 (Queue)，儲存節點在主陣列中的索引位置
            queue<int> q;
            // 記錄本次 BFS 找到的相異收訊者學號
            vector<string> reached;
            
            // 將起點 (發訊者) 標記為已造訪並推入佇列
            visited[i] = true;
            q.push(i);
            
            // 開始 BFS 走訪
            while (!q.empty()) {
                // 取出佇列最前方的節點 (當前的發訊者)
                int u = q.front();
                q.pop();
                
                // 遍歷當前節點 u 所有相鄰的邊 (即所有發送出的訊息目標)
                for (const Node& edge : adjList[u].edges) {
                    // 利用二元搜尋快速找到目標收訊者的索引
                    int v = findVertexIndex(edge.getID);
                    
                    // 若目標節點有效，且在本次 BFS 尚未被造訪過
                    if (v != -1 && !visited[v]) {
                        visited[v] = true;             // 標記為已造訪，防止重複處理
                        reached.push_back(edge.getID); // 加入相異收訊者清單
                        q.push(v);                     // 推入佇列以進行下一層搜尋
                    }
                }
            }
            
            // 將走訪蒐集到的所有收訊者學號，依字串由小到大排序 (輸出格式規定)
            sort(reached.begin(), reached.end());
            
            // 將本發訊者的計算結果封裝並推入 results 陣列
            ConnectionResult res;
            res.putID = adjList[i].id;
            res.count = reached.size();    // 總連通數
            res.reachedIDs = reached;      // 已排序的收訊者清單
            results.push_back(res);
        }
        
        // 針對所有的發訊者結果進行整體排序 (依據先前的 operator< 規則)
        sort(results.begin(), results.end());

        // 終端機輸出：顯示資料集內的發訊者總數
        cout << "\n<<< There are " << results.size() << " IDs in total. >>>\n\n";

        // --- 將連通數結果格式化並寫入同檔名的 .cnt 延伸文字檔 ---
        string outFilename = "pairs" + fileNum + ".cnt";
        ofstream outFile(outFilename);
        
        // 檔案開頭統計資訊
        outFile << "<<< There are " << results.size() << " IDs in total. >>>\n";
        
        // 遍歷所有排序好的結果並輸出
        for (size_t i = 0; i < results.size(); ++i) {
            // 輸出發訊者的名次、學號及連通數
            // 格式例如：[  1] 10227116(21): 
            outFile << "[" << setw(3) << i + 1 << "] " << results[i].putID << "(" << results[i].count << "): \n";
            
            // 若該發訊者有連通到任何收訊者，則將名單印出
            if (!results[i].reachedIDs.empty()) {
                for (size_t j = 0; j < results[i].reachedIDs.size(); ++j) {
                    // 輸出單一收訊者學號
                    // 格式例如：	( 1) 10127135
                    outFile << "\t(" << setw(2) << j + 1 << ") " << results[i].reachedIDs[j];
                    
                    // 格式排版規定：每印滿 12 個學號立刻換行
                    if ((j + 1) % 12 == 0) outFile << "\n";
                }
                
                // 每個發訊者的名單印完後，無條件換行。
                // 若恰好為 12 的倍數，此換行會創造一個空白行，確保與範例排版的一致性。
                outFile << "\n";
            }
        }
        
        // 關閉檔案，確保資料寫入磁碟
        outFile.close();
    }
};

// 顯示主選單的函式
void printMenu() {
    cout << "* Data Structures and Algorithms *" << endl;
    cout << "**** Graph data manipulation *****" << endl;
    cout << "* 0. QUIT                        *" << endl;
    cout << "* 1. Build adjacency lists       *" << endl;
    cout << "* 2. Compute connection counts   *" << endl;
    cout << "**********************************" << endl;
    cout << "Input a choice(0, 1, 2): ";
}

// 程式主進入點
int main() {
    int choice;
    string fileNum;
    AdjacencyList graph;

    // 無窮迴圈，直到使用者選擇離開 (QUIT)
    while (true) {
        printMenu();
        
        // 防呆機制：若輸入的不是整數 (例如英文字母)
        if (!(cin >> choice)) {
            cin.clear();              // 清除錯誤旗標
            cin.ignore(10000, '\n');  // 捨棄輸入緩衝區內的所有錯誤字元
            cout << "\nInvalid input! Please try again.\n\n";
            continue;
        }

        // 依據使用者的選擇執行對應功能
        if (choice == 0) {
            // 離開程式
            break;
        } else if (choice == 1) {
            // 任務一：輸入檔案編號，讀取資料並建立圖的相鄰串列
            cout << "\nInput a file number ([0] Quit): ";
            cin >> fileNum;
            
            // 若輸入 0，則取消載入並回到主選單
            if (fileNum == "0") {
                cout << endl;
                continue;
            }
            graph.buildList(fileNum);
        } else if (choice == 2) {
            // 任務二：計算已建立圖的連通數，並使用相同的檔名輸出
            graph.computeCounts(fileNum);
        } else {
            // 若輸入 0~2 以外的數字
            cout << "\nCommand does not exist!\n\n";
        }
    }
    
    return 0;
}
