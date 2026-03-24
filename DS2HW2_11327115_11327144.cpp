// 11327115 郭琮禮 & 11327144 莊有隆
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>

using namespace std;

// Record 結構體，代表每筆資料的基本單位 (包含所需要輸出的 7 個欄位)
struct Record {
    int id;             // 存入原始檔內的資料出現順序 (即「序號」)
    string school_name; // 學校名稱 (AVL Tree 的鍵值 Key)
    string dept_name;   // 科系名稱
    string day_night;   // 日間/進修別
    string level;       // 等級別
    string students;    // 學生數
    string graduates;   // 上學年度畢業生數
};

// DataStore 類別專門負責處理檔案的讀取與解析工作
class DataStore {
private:
    vector<Record> records; // 儲存所有成功解析的有效資料

public:
    // 清空現有資料
    void Clear() {
        records.clear();
    }

    // 負責讀取指定編號的輸入檔案
    bool LoadFromFile(string filename) {
        // 組合出完整檔名，例如 "input201.txt"
        filename = "input" + filename + ".txt";
        records.clear();
        ifstream file(filename);

        // 如果無法開啟檔案，輸出錯誤訊息並回傳 false
        if (!file.is_open()) {
            cout << "\n### " << filename << " does not exist! ###" << endl;
            return false;
        }

        string line;
        int file_order = 1; // 用來作為序號的計數器

        // 逐行讀取整個檔案
        while (getline(file, line)) {
            // 跳過空行或僅有換行特殊字元的列
            if (line.empty() || line == "\r") continue;

            // 只有開頭是數字的行才算是資料行 (過濾掉表頭)
            if (!isdigit(static_cast<unsigned char>(line[0]))) continue;

            stringstream ss(line);
            string token;
            vector<string> tokens;
            
            // 將整行依據 tab ('\t') 進行分割
            while (getline(ss, token, '\t')) {
                // 移除 token 尾端可能殘留的 '\r' (尤其是 Windows 換行符號)
                if (!token.empty() && token.back() == '\r') {
                    token.pop_back();
                }
                tokens.push_back(token);
            }

            // 確保有足夠的欄位。上學年度畢業生數位於索引 8，所以切分後長度至少需 >= 9
            if (tokens.size() >= 9) {
                // 如果讀取到的學校名稱為空，代表這可能是格式錯誤的表頭，跳過它
                if (tokens[1].empty()) continue; 
                
                Record newRecord;
                newRecord.id = file_order++;        // 分配序號後遞增給下一筆資料
                newRecord.school_name = tokens[1];  // 索引 1: 學校名稱
                newRecord.dept_name = tokens[3];    // 索引 3: 科系名稱
                // 以下欄位需要再進一步防呆檢查存取範圍
                newRecord.day_night = (tokens.size() > 4) ? tokens[4] : ""; // 索引 4: 日間/進修別
                newRecord.level = (tokens.size() > 5) ? tokens[5] : "";     // 索引 5: 等級別
                newRecord.students = (tokens.size() > 6) ? tokens[6] : "";  // 索引 6: 學生數
                newRecord.graduates = (tokens.size() > 8) ? tokens[8] : ""; // 索引 8: 畢業生數
                
                // 將成功解析的這筆紀錄存進 vector 裡
                records.push_back(newRecord);
            }
        }
        file.close(); // 讀取完畢後關閉檔案
        return true;
    }

    // 提供外部取得已經解析好存放資料的 Reference
    vector<Record>& GetRecords() {
        return records;
    }
};

// AVLNode 結構體，代表 AVL Tree 的每個節點
struct AVLNode {
    string school_name;     // 節點中的鍵值 (學校名稱)
    vector<int> record_ids; // 因為相同學校名稱可能有多筆資料，以陣列儲存這些紀錄的序號(id)
    int height;             // 樹的高度，用做計算左右平衡因子
    AVLNode* left;          // 左子樹指標
    AVLNode* right;         // 右子樹指標

    // 節點初始化建構子
    AVLNode(string name, int id) {
        school_name = name;
        record_ids.push_back(id);
        height = 1;         // 新節點的初始高度均設為 1
        left = nullptr;
        right = nullptr;
    }
};

// AVL Tree 的主要實作類別處理旋轉與平衡
class AVLTree {
private:
    AVLNode* root;      // 指向樹根的指標
    int node_count;     // 整棵 AVL Tree 總共的實際節點數目

    // 取得給定節點的高度 (若為 nullptr 則回傳 0)
    int GetHeight(AVLNode* node) {
        if (!node) return 0;
        return node->height;
    }

    // 計算特定節點的平衡因子 (Balance Factor)
    // 等於左子樹高度減去右子樹高度
    int GetBalance(AVLNode* node) {
        if (!node) return 0;
        return GetHeight(node->left) - GetHeight(node->right);
    }

    // 根據其左右子樹的高度，更新給定節點的高度資訊
    void UpdateHeight(AVLNode* node) {
        if (node) {
            node->height = 1 + max(GetHeight(node->left), GetHeight(node->right));
        }
    }

    // 實行 Right Rotation (LL 失衡使用的右旋操作)
    AVLNode* LL_Rotate(AVLNode* unbalancedNode) {
        AVLNode* pivotNode = unbalancedNode->left;
        AVLNode* rightSubtreeOfPivot = pivotNode->right;

        // 進行旋轉：將樞紐節點 (pivotNode) 提拔，並把原樞紐節點的右子樹轉歸失衡節點 (unbalancedNode) 的左子樹
        pivotNode->right = unbalancedNode;
        unbalancedNode->left = rightSubtreeOfPivot;

        // 更新高度 (先更新子節點 unbalancedNode，再更新新的父節點 pivotNode)
        UpdateHeight(unbalancedNode);
        UpdateHeight(pivotNode);

        // 回傳旋轉後的新局部樹根
        return pivotNode;
    }

    // 實行 Left Rotation (RR 失衡使用的左旋操作)
    AVLNode* RR_Rotate(AVLNode* unbalancedNode) {
        AVLNode* pivotNode = unbalancedNode->right;
        AVLNode* leftSubtreeOfPivot = pivotNode->left;

        // 進行旋轉：將樞紐節點提拔，並把原樞紐節點的左子樹轉歸失衡節點的右子樹
        pivotNode->left = unbalancedNode;
        unbalancedNode->right = leftSubtreeOfPivot;

        // 更新高度 (同樣先下而上更新)
        UpdateHeight(unbalancedNode);
        UpdateHeight(pivotNode);

        return pivotNode;
    }

    // 執行插入資料的遞迴函數，包含資料插入以及平衡機制
    AVLNode* InsertNode(AVLNode* node, string name, int id) {
        // 第一步驟：正常的二元搜尋樹 (BST) 插入
        if (!node) {
            // 如果跑到葉子之外，即建立新節點，增加節點總數
            node_count++;
            return new AVLNode(name, id);
        }

        // 以學校名稱的字串先後順序決定去左邊還去右邊
        if (name < node->school_name) {
            node->left = InsertNode(node->left, name, id);
        } else if (name > node->school_name) {
            node->right = InsertNode(node->right, name, id);
        } else {
            // 當兩者字串相等 (即遇到相同學校名稱時):
            // 由於整棵樹只有唯一的此校名稱節點，將該筆資料的 id 存入這個陣列即可
            node->record_ids.push_back(id);
            return node; // 相等時無需更新高度與旋轉，因為並沒有增加新節點
        }

        // 第二步驟：更新當前節點的高度
        UpdateHeight(node);

        // 第三步驟：獲取當前節點的平衡因子，用來檢查這顆節點是否失衡
        int balance = GetBalance(node);

        // 第四步驟：若發現失衡，依據四種情況 (LL, RR, LR, RL) 進行相對應調整

        // LL (Left Left) 情境：新增節點被插入在失衡點左子樹的左側
        if (balance > 1 && name < node->left->school_name) {
            return LL_Rotate(node); 
        }
        // RR (Right Right) 情境：新增節點被插入在失衡點右子樹的右側
        if (balance < -1 && name > node->right->school_name) {
            return RR_Rotate(node);
        }
        // LR (Left Right) 情境：新增節點被插入在失衡點左子樹的右側
        if (balance > 1 && name > node->left->school_name) {
            // 先針對左節點進行一次左旋轉 (轉成 LL)
            node->left = RR_Rotate(node->left);
            // 再對整個失衡節點進行一次右旋轉
            return LL_Rotate(node);
        }
        // RL (Right Left) 情境：新增節點被插入在失衡點右子樹的左側
        if (balance < -1 && name < node->right->school_name) {
            // 先針對右節點進行一次右旋轉 (轉成 RR)
            node->right = LL_Rotate(node->right);
            // 再對整個失衡節點進行一次左旋轉
            return RR_Rotate(node);
        }

        // 當不用旋轉時，直接回傳原節點指標
        return node;
    }

    // 遞迴釋放樹中所有節點的記憶體
    void ClearTree(AVLNode* node) {
        if (!node) return;
        ClearTree(node->left);
        ClearTree(node->right);
        delete node; // 刪除記憶體
    }

public:
    // 建構子，初始化根節點與節點數目
    AVLTree() : root(nullptr), node_count(0) {}
    
    // 解構子，負責自動清理釋放動態宣告的資源
    ~AVLTree() { ClearTree(root); }

    // 主動清理現存的 AVL Tree
    void Clear() {
        ClearTree(root);
        root = nullptr;
        node_count = 0;
    }

    // 供外部呼叫的 Insert 功能入口
    void Insert(string name, int id) {
        // 更新遞迴過後的嶄新樹根
        root = InsertNode(root, name, id);
    }

    // 透過整個 Records 陣列一筆一筆建立成 AVLTree
    void BuildFromRecords(const vector<Record>& records) {
        Clear(); // 建立之前確保過去舊資料先被清乾淨
        for (const auto& recordItem : records) {
            Insert(recordItem.school_name, recordItem.id);
        }
    }

    // 印出樹的結果：包括高度、總節點數、以及樹根所存有的資料細節
    void PrintResult(const vector<Record>& records) {
        if (!root) {
            // 若樹是空的
            cout << "Tree height = 0" << endl;
            cout << "Number of nodes = 0" << endl;
            return;
        }

        // 以 root 的節點記錄來視為樹高度
        cout << "Tree height = " << GetHeight(root) << endl;
        // 樹內所有總節點數 (學校數量)
        cout << "Number of nodes = " << node_count << endl;

        // 依序由小到大顯示樹根內儲存的每筆資料，
        // 因為當初是一筆一筆依照順序推入到 record_ids 因此自然已經以 id 由小到大排序好。
        for (size_t index = 0; index < root->record_ids.size(); index++) {
            int recordSequenceId = root->record_ids[index];
            
            // recordSequenceId 是「序號」，為 1-based (從1開始) 的資料
            // 查詢對應的記錄時，需回到 records 從 0 開始的陣列裡面搜尋所以減一
            const Record& matchedRecord = records[recordSequenceId - 1]; 
            
            // "index+1" 為題目要求的流水號
            cout << index + 1 << ": [" << matchedRecord.id << "] " 
                 << matchedRecord.school_name << ", " 
                 << matchedRecord.dept_name << ", "
                 << matchedRecord.day_night << ", "
                 << matchedRecord.level << ", "
                 << matchedRecord.students << ", "
                 << matchedRecord.graduates << endl;
        }
    }
};

// 進入主迴圈與給使用者的 UI 的介面提示字元函式
void ReadCommand(int &commandChoice) {
    commandChoice = -1; 
    string inputStr;
    while (commandChoice < 0 || commandChoice > 2) {
        cout << R"(* Data Structures and Algorithms *
****** Balanced Search Tree ******
* 0. QUIT                        *
* 1. Build 23 tree               *
* 2. Build AVL tree              *
**********************************
Input a choice(0, 1, 2): )";
        cin >> inputStr;

        // 防呆機制：將使用者輸入當作字串讀取，手動轉為指令號碼
        // 這樣可以避免輸入 'a', '!' 或其他特殊字元導致 cin 崩潰陷入無窮迴圈
        if (inputStr == "0") {
            commandChoice = 0;
        } else if (inputStr == "1") {
            commandChoice = 1;
        } else if (inputStr == "2") {
            commandChoice = 2;
        } else {
            cout << "\nCommand does not exist!\n" << endl;
            commandChoice = -1; // 繼續留在迴圈內
        }
    }
}

// 主程式起點
int main() {
    DataStore dataStore;
    AVLTree avlTree;
    int commandChoice = 0;
    
    // 初始化獲取第一次的命令
    ReadCommand(commandChoice);

    // 輸入 0 的時候就會停止程式
    while (commandChoice != 0) {
        if (commandChoice == 1) {
            string filename;
            cout << "\nInput a file number ([0] Quit): ";
            cin >> filename;
            
            // 若輸入檔案為 "0"，跳出此次詢問，重新顯示選單
            if (filename == "0") {
                cout << endl;
                ReadCommand(commandChoice);
                continue;
            }

            // 若 LoadFromFile 失敗會回傳 false，需要在此利用 while 一直嘗試請他重新輸入
            while (!dataStore.LoadFromFile(filename)) {
                cout << "\nInput a file number ([0] Quit): ";
                cin >> filename;
                if (filename == "0") {
                    break;
                }
            }

            // 若成功讀取，可以提示一下，或直接回主畫面
        } else if (commandChoice == 2) {
            // 當檔案成功讀取完畢後，透過選項2獲取所有紀錄並構建至 AVL Tree 當中
            vector<Record>& records = dataStore.GetRecords();
            if (records.empty()) {
                cout << "### Choose 1 first. ###" << endl;
            } else {
                avlTree.BuildFromRecords(records);
                avlTree.PrintResult(records);
            }
        }

        // 操作結束，空出一行，等待下一個主要命令
        cout << endl;
        ReadCommand(commandChoice);
    }

    return 0; // 順利退出
}
