#include <memory>
#include <cstdio>
#include <string>
using namespace std;

string toString(int i) {
    if (i == 0) {
        return "0";
    }
    string res = "";
    while (i > 0) {
        int dig = i % 10;
        char c = '0' + dig;
        string digitStr = "";
        digitStr += c;
        res = digitStr + res;
        i /= 10;
    }
    return res;
}
string calcScore(string s) {
    int score = 0;
    for (int i = 0; i < s.length(); i += 1) {
        score += s.at(i) - '0';
    }
    return toString(score);
}
string digitSum(string s, int k) {
    while (s.length() > k) {
        string round = "";
        string curr = "";
        for (int i = 0; i < s.length(); i += 1) {
            curr += s.at(i);
            if ((i + 1) % k == 0) {
                string total = calcScore(curr);
                curr = "";
                round = round + total;
            }
        }
        if (!curr.empty()) {
            string total = calcScore(curr);
            round = round + total;
        }
        s = round;
    }
    return s;
}
int main() {
    string ans = digitSum("11111222223", 3);
    printf("%s", ans);
    return 0;
}