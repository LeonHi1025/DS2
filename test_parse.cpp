#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
using namespace std;
int main() {
    ifstream file("input201.txt");
    string line;
    int order = 1;
    while(getline(file, line)) {
        if (line.empty() || line == "\r") continue;
        if (!isdigit(static_cast<unsigned char>(line[0]))) continue;
        stringstream ss(line);
        string token;
        vector<string> tokens;
        while(getline(ss, token, '\t')) tokens.push_back(token);
        if (tokens.size() >= 9) {
            cout << "Order: " << order++ << " Token1: " << tokens[1] << endl;
        }
    }
}
