// 11327144 莊有隆
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>

using namespace std;

// Node 結構體，代表資料的基本單位
// 規定：節點只存放一筆資料對應的序號 (即檔案順序) 及上學年度畢業生數
struct Node {
    int id;         // 存入原始檔內的檔案出現順序 (例如第一筆有效的學校紀錄 id 就是 1)
    int graduates;  // 該校該系的上學年度畢業生數，這是建立Heap排序的主要依據
    string school_name;
    string dept_name;
    string day_night;
    string level;
};

// DataStore 類別宣告
// 專門負責讀取檔案、解析資料，並將有效紀錄儲存起來供後續各項任務重複使用
class DataStore {
 private:
    // 儲存所有成功解析的有效資料
    vector<Node> records;

 public:
    // 讀取檔案並且儲存所有節點資料
    bool LoadFromFile(string filename);

    // 提供外部取得解析後資料的方法
    vector<Node>& GetRecords();
};

// DataStore 類別實作
bool DataStore::LoadFromFile(string filename) {
    filename = "input" + filename + ".txt";
    // 先清空上次的資料 (如果有)
    records.clear();

    // 開啟輸入檔案
    ifstream file(filename);
    // 若無法開啟則回傳失敗
    if (!file.is_open()) {
        cout << "\n### " << filename << " does not exist! ###" << endl;
        return false;
    }

    string line;
    // 記錄原始檔內的檔案有效順序，從 1 開始編號
    int file_order = 1;

    // 逐行讀取檔案內容
    while (getline(file, line)) {
        // 跳過空行或只是回車(CR)的行，避開檔頭或檔尾無意義的列
        if (line.empty() || line == "\r") continue;

        // 檢查是否為資料行 (第一欄是序號或學校代碼，開頭必定為數字)，過濾掉表頭
        if (!isdigit(line[0])) continue;

        stringstream ss(line);
        string token;
        vector<string> tokens;
        
        // 將整行資料透過 tab ('\t') 字元進行切割，切分成很多個欄位
        while (getline(ss, token, '\t')) {
            tokens.push_back(token);
        }

        // 依據檔案格式，上學年度畢業生數位於索引 8 的位置，因此至少需要 9 個欄位
        if (tokens.size() >= 9) {
            Node node;
            // 將當前檔案的順序賦值給 id，同時讓 order +1 準備給下一筆用
            node.id = file_order++;

            // 取出索引 8 作為畢業生數字串
            string gradStr = tokens[8];

            try {
                // Remove commas from gradStr if any exists
                string numStr = "";
                for (char c : gradStr) {
                    if (c >= '0' && c <= '9') {
                        numStr += c;
                    }
                }
                node.graduates = stoi(numStr);
                node.school_name = tokens[1];
                node.dept_name = tokens[3];
                // Handle possible missing tokens smoothly
                node.day_night = (tokens.size() > 4) ? tokens[4] : "";
                node.level = (tokens.size() > 5) ? tokens[5] : "";
                // 只要轉換成功，就把該節點放進緩存陣列中保存
                records.push_back(node); 
            } catch (...) {
                // 萬一字串轉換失敗 (例如裡面不是純數字)，這筆不算成功，因此要把順序編號退回
                file_order--; 
            }
        }
    }
    // 關閉檔案串流
    // 成功讀取並解析完畢
    file.close(); 
    return true;  
}

// 用於取得存取資料
vector<Node>& DataStore::GetRecords() {
    return records;
}

// MaxHeap 類別宣告
// 專門處理 Heap 資料結構相關操作，從已儲存的資料中依序建立
class MaxHeap {
 private:
    // 內部使用 vector 來模擬儲存完全二元樹
    vector<Node> heap;

 public:
    // 清空並重新從給定的資料列表建立 Max Heap
    void BuildFromRecords(vector<Node>& records);

    // 將單一筆 Node 資料插入至最大堆積的最末端，然後進行上浮調整
    void Insert(Node newNode);

    // 刪除最大節點
    void DeleteMax();

    // 輸出檢查結果
    void PrintResult();
};

// MaxHeap 類別實作
void MaxHeap::BuildFromRecords(vector<Node>& records) {
    heap.clear();
    for (int i = 0; i < records.size(); i++) {
        Insert(records[i]);
    }
}

void MaxHeap::Insert(Node newNode) {
    // 先將新節點放在陣列的最後面 (完全二元樹的最後一個位置)
    heap.push_back(newNode);
     // 目前節點的索引位置
    int curr = heap.size() - 1;
    
    // 開始進行上浮調整，直到到達根節點為止
    while (curr > 0) {
        int parent = (curr - 1) / 2; // 計算其父節點的索引位置 (公式: (i-1)/2)
        
        // 依照上學年度畢業生數來建立最大堆積
        // 如果當前節點的畢業生數「大於」父節點，代表違反了 Max Heap 性質 (父 >= 子)
        if (heap[parent].graduates < heap[curr].graduates) {
            // 將父節點與當前節點進行交換
            swap(heap[parent], heap[curr]);
            // 將目前的索引位置移至父節點的位置，繼續往上檢查
            curr = parent;
        } else {
            // 若已經符合 Max Heap 性質，代表插入已經調整好，即可提早結束迴圈
            break;
        }
    }
}

void MaxHeap::PrintResult() {
    // 如果無任何資料，顯示提示訊息
    if (heap.empty()) {
        return;
    }

    // 標題
    cout << "<max heap>" << endl;

    // <樹根> : 就是陣列的第一個元素 (索引 0)
    int n = heap.size();
    cout << "root: [" << heap[0].id << "] " << heap[0].graduates << endl;
    
    // <底部節點> : 完全二元樹中的最後一個節點，等同於陣列的最後一個元素 (索引 n-1)
    cout << "bottom: [" << heap[n - 1].id << "] " << heap[n - 1].graduates << endl;

    // <最左下角的節點> : 位於最後一層的最左邊。
    // 其索引位置為不大於總節點數的最大 2 的次方的一半再減 1。
    int power = 1;
    while (power <= n) power *= 2; 
    int bottom_left_idx = (power / 2) - 1; // 這便是最後一層最左邊的元素的索引值
    
    // 確保算出來的索引沒有超出陣列範圍才印出
    if (bottom_left_idx < n) {
        cout << "leftmost bottom: [" << heap[bottom_left_idx].id << "] " << heap[bottom_left_idx].graduates << endl;
    }
}

// Deap 類別宣告
// 提供雙端優先權佇列 (Double Ended Priority Queue) 功能
class Deap {
private:
    // 內部使用 vector 來模擬 Deap (根節點索引 0 放空，實際資料從 1 開始)
    vector<Node> heap;

    // 判斷該位置是否在 Min Heap 側 (左子樹)
    bool InMinHeap(int idx);

    // 獲取對應 Max Heap 側的節點partner (若不存在，則回傳其父節點)
    int MaxPartner(int idx);

    // 獲取對應 Min Heap 側的節點partner
    int MinPartner(int idx);

    // 向上調整 Min Heap
    void InsertMin(int idx);

    // 向上調整 Max Heap
    void InsertMax(int idx);

public:
    Deap();

    // 清空並重新從給定的資料列表建立 Deap
    void BuildFromRecords(vector<Node>& records);

    // 插入一筆資料到 Deap
    void Insert(Node newNode);

    // 輸出結果
    void PrintResult();
};

// Deap 類別實作

// 判斷該位置是否在 Min Heap 側 (左子樹)，不是的話就是在 Max Heap 側(右子數)
bool Deap::InMinHeap(int idx) {
    // 不斷往上找父節點，直到來到相當於第二層的地方 (index 1 或是 index 2)
    // index 1 為 Min Heap 的頭，index 2 為 Max Heap 的頭
    while (idx > 2) {
        idx = (idx - 1) / 2;
    }
    // 如果是 1 代表在 min-heap
    return idx == 1;
}

// 找看看是在底幾層，算出對應的2的冪次方，即可取得partner
int Deap::MaxPartner(int idx) {
    int log_val = 0;
    int temp = idx;
    while (temp > 1) {
        temp = (temp - 1) / 2;
        log_val++;
    }
    // 1 << log_val，即為1去行左移(2的n(log_val)次方)
    int partner = idx + (1 << log_val);

    // 不存在對應點，那就回傳父母
    if (partner >= this->heap.size()) {
        return (partner - 1) / 2;
    }
    return partner;
}

// 同上
int Deap::MinPartner(int idx) {
    int log_val = 0;
    int temp = idx;
    while (temp > 2) {
        temp = (temp - 1) / 2;
        log_val++;
    }
    int partner = idx - (1 << (log_val));
    return partner;
}

void Deap::InsertMin(int idx) {
    int parent = (idx - 1) / 2;
    // 與父節點比較並上浮，Dummy node 的 index 為 0，因此 parent > 0 才需比較
    while (parent > 0) {
        if (this->heap[idx].graduates < this->heap[parent].graduates) {
            swap(this->heap[idx], this->heap[parent]);
            idx = parent;
            parent = (idx - 1) / 2;
        } else {
            break;
        }
    }
}

void Deap::InsertMax(int idx) {
    int parent = (idx - 1) / 2;
    // 與父節點比較並上浮，Dummy node 的 index 為 0，因此 parent > 0 才需比較
    while (parent > 0) {
        if (this->heap[idx].graduates > this->heap[parent].graduates) {
            swap(this->heap[idx], this->heap[parent]);
            idx = parent;
            parent = (idx - 1) / 2;
        } else {
            break;
        }
    }
}

Deap::Deap() {
    // 需要放入一個假節點 (索引 0 佔位)
    Node dummy = {-1, -1};
    this->heap.push_back(dummy); // 佔位 index 0
}

void Deap::BuildFromRecords(vector<Node>& records) {
    this->heap.clear();
    Node dummy = {-1, -1};
    this->heap.push_back(dummy); // 佔位 index 0

    for (int i = 0; i < records.size(); i++) {
        this->Insert(records[i]);
    }
}

void Deap::Insert(Node newNode) {
    this->heap.push_back(newNode);
    int curr = this->heap.size() - 1;

    // 第 1 個位置是整個 Deap 的 Min Root
    if (curr == 1) {
        return;
    }

    // 第 2 個位置是整個 Deap 的 Max Root，必須比 Min Root 大
    if (curr == 2) {
        if (this->heap[2].graduates < this->heap[1].graduates) {
            swap(this->heap[2], this->heap[1]);
        }
        return;
    }

    if (InMinHeap(curr)) {
        int partner = MaxPartner(curr);
        if (this->heap[curr].graduates > this->heap[partner].graduates) {
            swap(this->heap[curr], this->heap[partner]);
            InsertMax(partner);
        } else {
            InsertMin(curr);
        }
    } else {
        int partner = MinPartner(curr);
        if (this->heap[curr].graduates < this->heap[partner].graduates) {
            swap(this->heap[curr], this->heap[partner]);
            InsertMin(partner);
        } else {
            InsertMax(curr);
        }
    }
}

void Deap::PrintResult() {
    if (this->heap.size() <= 1) {
        return;
    }

    // 成功建立後，印出節點的總數量 (不包含 dummy)
    int size = this->heap.size() - 1;
    cout << "<DEAP>" << endl;

    // <底部節點> : 陣列的最後一個元素
    cout << "bottom: [" << this->heap[size].id << "] " << this->heap[size].graduates << endl;

    // <最左下角的節點> : 位於最後一層的最左邊。
    int power = 1; 
    while (power * 2 <= size) power *= 2; 

    // 左下角節點的 0-based index 就是 power - 1
    int bottom_left_idx = power - 1;
    
    // 確保他合法
    if (bottom_left_idx <= size && bottom_left_idx < this->heap.size()) {
        cout << "leftmost bottom: [" << this->heap[bottom_left_idx].id << "] " << this->heap[bottom_left_idx].graduates << endl;
    }
}

// MinMaxHeap 類別宣告
// 提供 Min-Max Heap 功能，依據上學年度畢業生數排列
class MinMaxHeap {
private:
    vector<Node> heap;

    // 判斷當前層數是否為 Min 層 (回傳 true) 或 Max 層 (回傳 false)
    bool IsMinLevel(int idx);

    // 向上調整
    void BubbleUp(int idx);
    void BubbleUpMin(int idx);
    void BubbleUpMax(int idx);

    // 向下調整
    void TrickleDown(int idx);
    void TrickleDownMin(int idx);
    void TrickleDownMax(int idx);

    void findBestGrandchildMin(int idx);
    void findBestGrandchildMax(int idx);
public:
    MinMaxHeap();

    bool IsEmpty() { return heap.empty(); }

    // 從資料列表建立 Min-Max Heap
    void BuildFromRecords(vector<Node>& records);

    // 插入一筆新資料
    void Insert(Node newNode);

    Node DeleteMax();

    // 輸出特定格式化的資訊
    void PrintResult();

    // 取出並輸出前 K 筆紀錄
    void PrintTopK();
};

// MinMaxHeap 類別實作
MinMaxHeap::MinMaxHeap() {}

bool MinMaxHeap::IsMinLevel(int idx) {
    int level = 0;
    while (idx > 0) {
        idx = (idx - 1) / 2;
        level++;
    }
    // 根節點 (level 0) 是 Min 層，偶數層(0, 2, 4...) 是 Min 層，奇數層是 Max 層
    return (level % 2 == 0);
}

void MinMaxHeap::BubbleUp(int idx) {
    int parent = (idx - 1) / 2;
    if (idx == 0) return; // 已經到了根節點

    if (IsMinLevel(idx)) {
        if (heap[idx].graduates > heap[parent].graduates) {
            swap(heap[idx], heap[parent]);
            BubbleUpMax(parent);
        } else {
            BubbleUpMin(idx);
        }
    } else {
        if (heap[idx].graduates < heap[parent].graduates) {
            swap(heap[idx], heap[parent]);
            BubbleUpMin(parent);
        } else {
            BubbleUpMax(idx);
        }
    }
}

void MinMaxHeap::BubbleUpMin(int idx) {
    int parent = (idx - 1) / 2;
    int grandparent = (parent - 1) / 2;
    while (idx > 2) {
        if (heap[idx].graduates < heap[grandparent].graduates) {
            swap(heap[idx], heap[grandparent]);
            idx = grandparent;
            parent = (idx - 1) / 2;
            grandparent = (parent - 1) / 2;
        } else {
            break;
        }
    }
}

void MinMaxHeap::BubbleUpMax(int idx) {
    int parent = (idx - 1) / 2;
    int grandparent = (parent - 1) / 2;
    while (idx > 2) {
        if (heap[idx].graduates > heap[grandparent].graduates) {
            swap(heap[idx], heap[grandparent]);
            idx = grandparent;
            parent = (idx - 1) / 2;
            grandparent = (parent - 1) / 2;
        } else {
            break;
        }
    }
}

void MinMaxHeap::TrickleDown(int idx) {
    if (IsMinLevel(idx)) {
        TrickleDownMin(idx);
    } else {
        TrickleDownMax(idx);
    }
}

void MinMaxHeap::TrickleDownMax(int idx) {
    int n = heap.size();
    if (n == 0) return;

    while (true) {
        // Step 1: 檢查孩子
        int leftChild = 2 * idx + 1;
        int rightChild = 2 * idx + 2;
        int largestChild = -1;

        if (leftChild < n) largestChild = leftChild;
        if (rightChild < n && heap[rightChild].graduates > heap[largestChild].graduates) {
            largestChild = rightChild;
        }

        if (largestChild != -1 && heap[largestChild].graduates > heap[idx].graduates) {
            swap(heap[largestChild], heap[idx]);
        }

        // Step 2: 孩子沒交換 → 檢查孫子
        int bestGrandchild = -1;
        int max_val = heap[idx].graduates;

        for (int c = 2*idx+1; c <= 2*idx+2; c++) {
            if (c < n) {
                for (int gc = 2*c+1; gc <= 2*c+2; gc++) {
                    if (gc < n && heap[gc].graduates > max_val) {
                        bestGrandchild = gc;
                        max_val = heap[gc].graduates;
                    }
                }
            }
        }

        if (bestGrandchild == -1) break; // 沒有孫子了

        if (heap[bestGrandchild].graduates > heap[idx].graduates) {
            swap(heap[bestGrandchild], heap[idx]);
            idx = bestGrandchild; // 更新 idx，繼續往下檢查
        } else {
            break; // 規則已滿足，結束
        }
    }
}


void MinMaxHeap::TrickleDownMin(int idx) {
    int n = heap.size();
    if (n == 0) return;

    while (true) {
        // Step 1: 檢查孩子
        int leftChild = 2 * idx + 1;
        int rightChild = 2 * idx + 2;
        int smallestChild = -1;

        if (leftChild < n) smallestChild = leftChild;
        if (rightChild < n && heap[rightChild].graduates < heap[smallestChild].graduates) {
            smallestChild = rightChild;
        }

        if (smallestChild != -1 && heap[smallestChild].graduates < heap[idx].graduates) {
            swap(heap[smallestChild], heap[idx]);
        }

        // Step 2: 檢查孫子
        int bestGrandchild = -1;
        int min_val = heap[idx].graduates;

        for (int c = 2*idx+1; c <= 2*idx+2; c++) {
            if (c < n) {
                for (int gc = 2*c+1; gc <= 2*c+2; gc++) {
                    if (gc < n && heap[gc].graduates < min_val) {
                        bestGrandchild = gc;
                        min_val = heap[gc].graduates;
                    }
                }
            }
        }

        if (bestGrandchild == -1) break; // 沒有孫子了

        if (heap[bestGrandchild].graduates < heap[idx].graduates) {
            swap(heap[bestGrandchild], heap[idx]);
            idx = bestGrandchild; // 更新 idx，繼續往下檢查
        } else {
            break; // 規則已滿足，結束
        }
    }
}


Node MinMaxHeap::DeleteMax() {
    if (heap.size() == 1) {
        Node maxNode = heap[0];
        heap.pop_back();
        return maxNode;
    }
    if (heap.size() == 2) {
        Node maxNode = heap[1];
        heap.pop_back();
        return maxNode;
    }
    
    // Find max among heap[1] and heap[2]
    int maxIdx = 1;
    if (heap[2].graduates > heap[1].graduates) {
        maxIdx = 2; // > to strictly prefer left one on tie
    }

    Node maxNode = heap[maxIdx];
    Node lastNode = heap.back();
    heap.pop_back();

    if (maxIdx < heap.size()) {
        heap[maxIdx] = lastNode;
        TrickleDown(maxIdx);
    }

    return maxNode;
}

void MinMaxHeap::BuildFromRecords(vector<Node>& records) {
    this->heap.clear();

    for (int i = 0; i < records.size(); i++) {
        this->Insert(records[i]);
    }
}

void MinMaxHeap::Insert(Node newNode) {
    this->heap.push_back(newNode);
    int curr = this->heap.size() - 1;
    // index 0 不用調整
    if (curr > 0) {
        BubbleUp(curr);
    }
}

void MinMaxHeap::PrintResult() {
    if (this->heap.empty()) {
        return;
    }

    int n = this->heap.size();

    cout << "<min-max heap>" << endl;

    // <樹根>
    cout << "root: [" << this->heap[0].id << "] " << this->heap[0].graduates << endl;

    // <底部節點>
    cout << "bottom: [" << this->heap[n - 1].id << "] " << this->heap[n - 1].graduates << endl;

    // <最左下角的節點>
    int power = 1; 
    while (power <= n) power *= 2; 
    int bottom_left_idx = (power / 2) - 1;
    
    // 確保算出來的索引沒有超出陣列範圍才印出
    if (bottom_left_idx < n) {
        cout << "leftmost bottom: [" << this->heap[bottom_left_idx].id << "] " << this->heap[bottom_left_idx].graduates << endl;
    }
}

void MinMaxHeap::PrintTopK() {
    if (IsEmpty()) {
        cout << "\n### Execute command 3 first! ###" << endl;
        return;
    }

    // for (int i = 0; i < heap.size(); i++) {
    //     cout << i << ":" << heap[i].dept_name << endl;
    // }
    
    int k;
    cout << "\nEnter the value of K in [1," << heap.size() << "]: ";
    cin >> k;

    if (k > heap.size() || k <= 0) {
        cout << "\n### The value of K is out of range! ###" << endl;
        return;
    }

    for (int i = 0; i < k; i++) {
        if (IsEmpty()) {
            cout << "\n### Execute command 3 first! ###" << endl;
        break;
    }
        Node node = DeleteMax();
        if (i + 1 < 1000) {
            cout << "Top" << setw(4) << (i + 1) << ": " 
         << "[" << node.id << "] " ;
        }

        else {
            cout << "Top" << setw(5) << (i + 1) << ": " 
         << "[" << node.id << "] " ;
        }
         cout << node.school_name
         << node.dept_name << ", "
         << node.day_night << ", "
         << node.level << ", "
         << node.graduates << endl;
    }
}



// 主程式 Main
void ReadCommand(int &cmd) {
    cmd = -1; 
    while (cmd < 0 || cmd > 4) {
        std::cout << R"(* Data Structures and Algorithms *
*** Heap Construction and Use ****
* 0. QUIT                        *
* 1. Build a max heap            *
* 2. Build a DEAP                *
* 3. Build a min-max heap        *
* 4: Top-K max from min-max heap *
**********************************
Input a choice(0, 1, 2, 3, 4): )";
        cin >> cmd;
        // 指令不存在，請持續輸入
        if (cmd < 0 || cmd > 4) {
            std::cout << std::endl;
            std::cout << "Command does not exist!" << std::endl;
            std::cout << std::endl;
        }
    }
}


int main() {
    string filename;
    DataStore dataStore;
    DataStore dataStore_command_3;
    MaxHeap maxHeap;
    Deap deap;
    MinMaxHeap minMaxHeap;
    int cmd = 0;
    ReadCommand(cmd);

    while (cmd != 0) {
        if (cmd != 4) {
            // 提示使用者輸入欲讀取的檔案名稱
            cout << "\nInput a file number ([0] Quit): ";
            cin >> filename;
            if (filename == "0") {
                cout << endl;
                ReadCommand(cmd);
                continue;
            }

            while (!dataStore.LoadFromFile(filename)) {
                cout << "\nInput a file number ([0] Quit): ";
                cin >> filename;
                if (filename == "0") {
                    break;
                }
            }

            if (filename == "0") {
                cout << endl;
                ReadCommand(cmd);
                continue;
            }
        }
        
        vector<Node>& records = dataStore.GetRecords();

        if (cmd == 1) {
            maxHeap.BuildFromRecords(records);
            maxHeap.PrintResult();
        }

        else if (cmd == 2) {
            deap.BuildFromRecords(records);
            deap.PrintResult();
        }

        else if (cmd == 3) {
            minMaxHeap.BuildFromRecords(records);
            minMaxHeap.PrintResult();
        }

        else if (cmd == 4) {
            minMaxHeap.PrintTopK();
        }
        cout << endl;
        ReadCommand(cmd);
    }

    return 0;
}
