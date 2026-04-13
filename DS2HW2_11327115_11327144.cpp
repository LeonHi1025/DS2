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
                auto cleanNum = [](string s) {
                    s.erase(remove(s.begin(), s.end(), '\"'), s.end());
                    s.erase(remove(s.begin(), s.end(), ','), s.end());
                    return s;
                };
                newRecord.day_night = (tokens.size() > 4) ? tokens[4] : ""; // 索引 4: 日間/進修別
                newRecord.level = (tokens.size() > 5) ? tokens[5] : "";     // 索引 5: 等級別
                newRecord.students = (tokens.size() > 6) ? cleanNum(tokens[6]) : "";  // 索引 6: 學生數
                newRecord.graduates = (tokens.size() > 8) ? cleanNum(tokens[8]) : ""; // 索引 8: 畢業生數
                
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

// --- 2-3 Tree 實作部分 ---

struct TwoThreeNode {
    vector<int> keys;             // 鍵值：上學年度畢業生數
    vector<vector<int>> id_lists; // 儲存對應每個鍵值的序號清單
    vector<TwoThreeNode*> children;
    TwoThreeNode* parent;

    TwoThreeNode(TwoThreeNode* p = nullptr) : parent(p) {}
    bool isLeaf() const { return children.empty(); }
};

class TwoThreeTree {
private:
    TwoThreeNode* root;
    int node_count;

    // 負責處理 2-3 Tree 節點容量溢出 (超過 2 個鍵值) 的分裂機制
    void Split(TwoThreeNode* node) {
        // 如果節點鍵值尚未達到 3 個，不需要分裂
        if (node->keys.size() < 3) return;

        // 取出中間的鍵值與其對應的紀錄 ID 清單，準備提拔(promote)到父節點
        int midKey = node->keys[1];
        vector<int> midIds = node->id_lists[1];

        // 1. 建立左方新節點，存放原本節點中最左邊 (最小) 的鍵值與資料
        TwoThreeNode* leftNode = new TwoThreeNode();
        leftNode->keys.push_back(node->keys[0]);
        leftNode->id_lists.push_back(node->id_lists[0]);

        // 2. 建立右方新節點，存放原本節點中最右邊 (最大) 的鍵值與資料
        TwoThreeNode* rightNode = new TwoThreeNode();
        rightNode->keys.push_back(node->keys[2]);
        rightNode->id_lists.push_back(node->id_lists[2]);
        
        // 分裂後原本 1 個節點變成 2 個，總節點數淨增加 1
        node_count++; 

        // 如果原本的節點不是葉節點，需要一併將原有的子樹分配給左右新節點
        if (!node->isLeaf()) {
            // 左半部的子樹歸左節點
            leftNode->children = {node->children[0], node->children[1]};
            // 右半部的子樹歸右節點
            rightNode->children = {node->children[2], node->children[3]};
            // 重新將子節點的父指標指向它們的新父節點
            for (auto c : leftNode->children) c->parent = leftNode;
            for (auto c : rightNode->children) c->parent = rightNode;
        }

        // 3. 處理中間鍵值的提拔 (Promote)
        if (!node->parent) {
            // 情況 A: 如果當前分裂的是根節點 (root)，代表樹必須長高
            // 建立一個全新的根節點來容納被提拔的中間鍵值
            root = new TwoThreeNode();
            root->keys.push_back(midKey);
            root->id_lists.push_back(midIds);
            root->children = {leftNode, rightNode}; // 連接新建立的左右節點
            leftNode->parent = rightNode->parent = root;
            
            // 全新的根產生，總節點數額外再加 1
            node_count++;
        } else {
            // 情況 B: 上方還有父節點，將中間值推入父節點
            TwoThreeNode* p = node->parent;
            
            // 找出中間鍵值在父節點中的適當插入位置，以維持鍵值的遞增排序
            auto it = lower_bound(p->keys.begin(), p->keys.end(), midKey);
            int pos = distance(p->keys.begin(), it);
            
            // 將中間鍵值與對應 ID 插入到父節點陣列中
            p->keys.insert(it, midKey);
            p->id_lists.insert(p->id_lists.begin() + pos, midIds);
            
            // 更新父節點的子節點指標：移除原本指向舊溢出節點的指標
            p->children.erase(p->children.begin() + pos);
            // 改為插入新的右節點與左節點 (先插右再插左，順序才會正)
            p->children.insert(p->children.begin() + pos, rightNode);
            p->children.insert(p->children.begin() + pos, leftNode);
            // 更新父母參照
            leftNode->parent = rightNode->parent = p;
            
            // 考量提拔上去的鍵值可能使得父節點也爆滿 (剛好有 3 個鍵)，需遞迴分裂
            Split(p);
        }

        // 釋放舊的已經分裂完畢的節點記憶體
        delete node;
    }

    void ClearTree(TwoThreeNode* node) {
        if (!node) return;
        for (auto child : node->children) ClearTree(child);
        delete node;
    }

    int GetHeight(TwoThreeNode* node) {
        if (!node) return 0;
        return 1 + (node->isLeaf() ? 0 : GetHeight(node->children[0]));
    }

    // 以「反向中序追蹤 (Reverse In-Order Traversal)」走訪 2-3 Tree
    // 順序：最右子樹 -> 最大鍵值 -> 中間子樹 -> 中間鍵值 -> 左子樹
    // 目的：能由大到小將畢業生數量最高的紀錄優先輸出 (Top-K 邏輯)
    void ReverseInOrder(TwoThreeNode* node, const vector<Record>& records, int& k, int& serial) {
        if (!node || k <= 0) return; // 如果走到空節點，或是已滿足 K 個額度，提前返回
        
        if (node->isLeaf()) {
            // 葉節點：因為沒有子樹，只需要從陣列尾端往回印出鍵值即可
            for (int i = (int)node->keys.size() - 1; i >= 0 && k > 0; i--) {
                vector<int> ids = node->id_lists[i];
                // 如果有多筆紀錄具備相同數值的畢業生數，需依據原始 ID 排序確保「同值時依序號遞增」
                sort(ids.begin(), ids.end());
                for (int rid : ids) {
                    const Record& r = records[rid - 1];
                    cout << serial++ << ": [" << r.id << "] " << r.school_name << ", " 
                         << r.dept_name << ", " << r.day_night << ", " << r.level << ", " 
                         << r.students << ", " << r.graduates << endl;
                }
                // 一次印出所有同「畢業生數」的紀錄後才扣減 k 額度，包含所有平手同值的人
                k -= ids.size();
            }
        } else {
            // 內部節點：交替拜訪「子樹」與「鍵值」(由右至左)
            for (int i = (int)node->children.size() - 1; i >= 0 && k > 0; i--) {
                // 1. 先進入相對最右邊的子樹
                ReverseInOrder(node->children[i], records, k, serial);
                
                // 2. 子樹結束後，印出夾在該子樹左側的鍵值資料 (如果有)
                if (i - 1 >= 0 && k > 0) {
                    vector<int> ids = node->id_lists[i - 1];
                    sort(ids.begin(), ids.end());
                    for (int rid : ids) {
                        const Record& r = records[rid - 1];
                        cout << serial++ << ": [" << r.id << "] " << r.school_name << ", " 
                             << r.dept_name << ", " << r.day_night << ", " << r.level << ", " 
                             << r.students << ", " << r.graduates << endl;
                    }
                    k -= ids.size();
                }
            }
        }
    }

public:
    TwoThreeTree() : root(nullptr), node_count(0) {}
    ~TwoThreeTree() { Clear(); }

    void Clear() {
        ClearTree(root);
        root = nullptr;
        node_count = 0;
    }

    // 對 2-3 Tree 插入新的鍵值 (畢業生數量 grads) 與原始序號 (id)
    void Insert(int grads, int id) {
        // 第一種情況：如果樹完全是空的，直接把資料放進根節點
        if (!root) {
            root = new TwoThreeNode();
            root->keys.push_back(grads);
            root->id_lists.push_back({id});
            node_count = 1;
            return;
        }

        TwoThreeNode* curr = root;
        // 不斷往下尋找適當的葉節點來插入新資料
        while (!curr->isLeaf()) {
            bool found = false;
            // 如果在內部節點中提早找到了一模一樣的畢業生數量 (鍵值相同)
            // 由於 2-3 Tree 不允許重複鍵值建立新節點，只要把序號追加到陣列中即可
            for (size_t i = 0; i < curr->keys.size(); i++) {
                if (grads == curr->keys[i]) {
                    curr->id_lists[i].push_back(id);
                    return; // 插入陣列後直接結束
                }
            }
            
            // 如果不是相同鍵值，根據大小關係決定要走哪個分支往下降
            // 1. 比最小的鍵值還小 -> 走最左邊分支
            if (grads < curr->keys[0]) curr = curr->children[0];
            // 2. 只有一個鍵值且比較大，或是夾在兩個鍵值之間 -> 走中間分支
            else if (curr->keys.size() == 1 || grads < curr->keys[1]) curr = curr->children[1];
            // 3. 比兩個鍵值都還要大 -> 走最右邊分支
            else curr = curr->children[2];
        }

        // 抵達葉節點後，再次檢查有沒有剛好相等的鍵值可以合併
        for (size_t i = 0; i < curr->keys.size(); i++) {
            if (grads == curr->keys[i]) {
                curr->id_lists[i].push_back(id);
                return;
            }
        }

        // 如果真的沒有相同的鍵值，把新的畢業生數量與序號依循大小順序插入目前的葉節點中
        auto it = lower_bound(curr->keys.begin(), curr->keys.end(), grads);
        int pos = distance(curr->keys.begin(), it);
        curr->keys.insert(it, grads);
        curr->id_lists.insert(curr->id_lists.begin() + pos, {id});

        // 插入過後，葉節點的容量可能超過 2 個鍵值 (變成 3 個)
        // 此時呼叫 Split 機制來向上分裂並重新平衡樹狀結構
        if (curr->keys.size() > 2) Split(curr);
    }

    void PrintResult(const vector<Record>& records) {
        cout << "Tree height = " << GetHeight(root) << endl;
        cout << "Number of nodes = " << node_count << endl;
        if (!root) return;

        int serial = 1;
        for (size_t i = 0; i < root->keys.size(); i++) {
            vector<int> ids = root->id_lists[i];
            sort(ids.begin(), ids.end());
            for (int rid : ids) {
                const Record& r = records[rid - 1];
                cout << serial++ << ": [" << r.id << "] " << r.school_name << ", " 
                     << r.dept_name << ", " << r.day_night << ", " << r.level << ", " 
                     << r.students << ", " << r.graduates << endl;
            }
        }
        cout << endl;
    }

    bool IsEmpty() const { return root == nullptr; }

    void PrintTopK(const vector<Record>& records, int K) {
        int serial = 1;
        int remaining_K = K;
        ReverseInOrder(root, records, remaining_K, serial);
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

    // 執行插入資料的遞迴函數，包含資料插入以及背後的自動平衡機制
    // node: 當前遞迴走訪要處理的樹節點 (一開始傳入的是 root)
    // name: 準備要插入的學校名稱 (AVL Tree 的比較基準 Key)
    // id: 當前處理資料列對應的序號 id
    AVLNode* InsertNode(AVLNode* node, string name, int id) {
        // 第一步驟：執行標準的二元搜尋樹 (BST) 插入
        // 情況 1: 若往下走到空指標，代表找到了最終正確的位置，建立並返回新節點
        if (!node) { 
            node_count++; // 代表成功新增了一個不重複的節點，總數加一
            return new AVLNode(name, id);
        }

        // 以學校名稱 (字串) 為依據，判斷要往左走還是往右走
        if (name.compare(node->school_name) < 0) {
            // 字串較小，朝左方子樹繼續遞迴，並更新左子樹指標
            node->left = InsertNode(node->left, name, id);
        } else if (name.compare(node->school_name) > 0) {
            // 字串較大，朝右方子樹繼續遞迴，並更新右子樹指標
            node->right = InsertNode(node->right, name, id);
        } else {
            // 情況 2: 當兩者字串完全相等 (即遇到了相同的學校名稱)
            // 由於整棵樹限定每個學校只能有一個節點，因此將新資料的序號(id)
            // 直接推入該學校節點專屬的紀錄陣列 (record_ids) 中即可
            node->record_ids.push_back(id);
            // 由於沒有長出新節點，樹的結構並未改變，故無需後續的高度更新與旋轉平衡
            return node; 
        }

        // 第二步驟：更新當前節點的高度 (由底向上在遞迴回溯時更新)
        UpdateHeight(node);

        // 第三步驟：計算當前節點的平衡因子(Balance Factor)，用以偵測是否發生了失衡
        int balance = GetBalance(node);

        // 第四步驟：若發現失衡情況 (平衡因子超過範圍 [-1, 1])，
        // 即依照發生的 4 種失衡模式(LL, LR, RR, RL) 來進行對應的旋轉搶救

        if (balance > 1) { // 如果 BF > 1 代表「左方子樹太重」
            // 檢查左子節點的 BF：若為正數或零，代表連左小孩也是左邊重，這屬於「LL 失衡」
            // 解決方式：對當前節點 (Node) 直接進行一次向右旋轉 (Right Rotation)
            if (GetBalance(node->left) >= 0) {
                return LL_Rotate(node);
            } 
            // 檢查左子節點的 BF：若為負數，代表左小孩反而是右邊重，這屬於「LR 失衡」
            // 解決方式：先對 左子節點 進行向左旋轉，強制化為 LL 失衡，再對 自己 進行向右旋轉
            else {
                node->left = RR_Rotate(node->left);
                return LL_Rotate(node);
            }
        } 
        else if (balance < -1) { // 如果 BF < -1 代表「右方子樹太重」
            // 檢查右子節點的 BF：若為負數或零，代表連右小孩也是右邊重，這屬於「RR 失衡」
            // 解決方式：對當前節點 (Node) 直接進行一次向左旋轉 (Left Rotation)
            if (GetBalance(node->right) <= 0) {
                return RR_Rotate(node);
            } 
            // 檢查右子節點的 BF：若為正數，代表右小孩反而是左邊重，這屬於「RL 失衡」
            // 解決方式：先對 右子節點 進行向右旋轉，強制化為 RR 失衡，再對 自己 進行向左旋轉
            else {
                node->right = LL_Rotate(node->right);
                return RR_Rotate(node);
            }
        }

        // 如果節點原本就是平衡的，不需任何旋轉，直接回傳更新後的自己即可
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
    vector<int> SearchSchool(string name) {
        AVLNode* curr = root;
        while (curr) {
            int cmp = name.compare(curr->school_name);
            if (cmp == 0) {
                return curr->record_ids;
            } else if (cmp < 0) {
                curr = curr->left;
            } else {
                curr = curr->right;
            }
        }
        return vector<int>();
    }

    void PrintTopKForSchool(const vector<Record>& records, string target_school, int K) {
        vector<int> ids = SearchSchool(target_school);
        if (ids.empty()) return;
        
        sort(ids.begin(), ids.end(), [&records](int idA, int idB) {
            const Record& rA = records[idA - 1];
            const Record& rB = records[idB - 1];
            int gradA = rA.graduates.empty() ? 0 : stoi(rA.graduates);
            int gradB = rB.graduates.empty() ? 0 : stoi(rB.graduates);
            if (gradA != gradB) return gradA > gradB; // 遞減排序
            return rA.id < rB.id; // 同值時依序號遞增排序
        });

        int rank = 1;
        int last_graduates = -1;
        
        for (size_t i = 0; i < ids.size(); i++) {
            const Record& r = records[ids[i] - 1];
            int current_grad = r.graduates.empty() ? 0 : stoi(r.graduates);
            
            if (i < (size_t)K) {
                last_graduates = current_grad;
                cout << rank++ << ": [" << r.id << "] " << r.school_name << ", " 
                     << r.dept_name << ", " << r.day_night << ", " << r.level << ", " 
                     << r.students << ", " << r.graduates << endl;
            } else if (current_grad == last_graduates) {
                cout << rank++ << ": [" << r.id << "] " << r.school_name << ", " 
                     << r.dept_name << ", " << r.day_night << ", " << r.level << ", " 
                     << r.students << ", " << r.graduates << endl;
            } else {
                break;
            }
        }
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

        // 依序由小到大顯示樹根內儲存的每筆資料
        for (size_t index = 0; index < root->record_ids.size(); index++) {
            int recordSequenceId = root->record_ids[index];
            const Record& matchedRecord = records[recordSequenceId - 1]; 
            
            cout << index + 1 << ": [" << matchedRecord.id << "] " 
                 << matchedRecord.school_name << ", " 
                 << matchedRecord.dept_name << ", "
                 << matchedRecord.day_night << ", "
                 << matchedRecord.level << ", "
                 << matchedRecord.students << ", "
                 << matchedRecord.graduates << endl;
        }
        cout << endl;
    }
};

// 進入主迴圈與給使用者的 UI 的介面提示字元函式
void ReadCommand(int &commandChoice) {
    commandChoice = -1; 
    string inputStr;
    while (commandChoice < 0 || commandChoice > 4) { 
        cout << "* Data Structures and Algorithms *" << endl;
        cout << "****** Balanced Search Tree ******" << endl;
        cout << "* 0. QUIT                        *" << endl;
        cout << "* 1. Build 23 tree               *" << endl;
        cout << "* 2. Build AVL tree              *" << endl;
        cout << "* 3. Top-K max search on 23 tree *" << endl;
        cout << "* 4. Exact search on AVL tree    *" << endl;
        cout << "**********************************" << endl;
        cout << "Input a choice(0, 1, 2, 3, 4): ";
        
        cin >> inputStr;

        if (inputStr == "0") {
            commandChoice = 0;
        } else if (inputStr == "1") {
            commandChoice = 1;
        } else if (inputStr == "2") {
            commandChoice = 2;
        } else if (inputStr == "3") {
            commandChoice = 3;
        } else if (inputStr == "4") {
            commandChoice = 4;
        } else {
            cout << "\nCommand does not exist!\n" << endl;
            commandChoice = -1; 
        }
    }
}

// 主程式起點
int main() {
    DataStore dataStore;
    TwoThreeTree ttTree;
    AVLTree avlTree;
    int commandChoice = 0;
    bool isAVLBuilt = false;
    
    // 初始化獲取第一次的命令
    ReadCommand(commandChoice);

    // 輸入 0 的時候就會停止程式
    while (commandChoice != 0) {
        if (commandChoice == 1) {
            string filename;
            bool loadSuccess = false;

            while (!loadSuccess) {
                cout << "\nInput a file number ([0] Quit): ";
                cin >> filename;

                if (filename == "0") {
                    dataStore.Clear(); // 關鍵：退出時清空 DataStore，這樣 command 2 就會顯示 Choose 1 first
                    break; 
                }
                // 這裡只呼叫一次 LoadFromFile
                if (dataStore.LoadFromFile(filename)) {
                    loadSuccess = true;
                    ttTree.Clear();
                    avlTree.Clear();
                    isAVLBuilt = false;
            
                    vector<Record>& records = dataStore.GetRecords();
                    for (const auto& r : records) {
                        if (!r.graduates.empty()) {
                            ttTree.Insert(stoi(r.graduates), r.id);
                        }
                    }
                    ttTree.PrintResult(records);
                }
                // 如果失敗，LoadFromFile 內部會印出錯誤訊息，然後因為 !loadSuccess 繼續迴圈
            }
        } else if (commandChoice == 2) {
            vector<Record>& records = dataStore.GetRecords();
            if (records.empty()) {
                cout << "### Choose 1 first. ###" << endl;
            } else if (isAVLBuilt) {
                cout << "### AVL tree has been built. ###" << endl;
                avlTree.PrintResult(records);
            } else {
                avlTree.BuildFromRecords(records);
                avlTree.PrintResult(records);
                isAVLBuilt = true; 
            }
        } else if (commandChoice == 3) {
            vector<Record>& records = dataStore.GetRecords();
            if (records.empty() || ttTree.IsEmpty()) {
                cout << "### Choose 1 first. ###" << endl;
            } else {
                string kStr;
                cout << "\nEnter K in [1," << records.size() << "]: ";
                cin >> kStr;
                try {
                    int K = stoi(kStr);
                    if (K > 0 && K <= records.size()) {
                        ttTree.PrintTopK(records, K);
                        cout << endl;
                    }
                } catch (...) {
                    // Ignore invalid input securely
                }
            }
        } else if (commandChoice == 4) {
            if (ttTree.IsEmpty()) {
                cout << "### Choose 1 first. ###" << endl;
            } else if (!isAVLBuilt) {
                cout << "### Choose 2 first. ###" << endl;
            } else {
                string schoolName;
                cout << "Enter a college name to search: ";
                cin >> schoolName;
                    
                vector<int> ids = avlTree.SearchSchool(schoolName);
                if (ids.empty()) {
                    // School not found, no output or specific error usually.
                    cout << "\n" << schoolName << " is not found!" << endl;
                } else {
                    string kStr;
                    cout << "\nEnter K in [1," << ids.size() << "]: ";
                    cin >> kStr;
                    try {
                        int K = stoi(kStr);
                        if (K > 0 && K <= ids.size()) {
                            avlTree.PrintTopKForSchool(dataStore.GetRecords(), schoolName, K);
                            cout << endl;
                        }
                    } catch (...) {                            // Ignore invalid input securely
                    }
                }
            }
        }
        cout << endl;
        ReadCommand(commandChoice);
    }

    return 0; 
}
