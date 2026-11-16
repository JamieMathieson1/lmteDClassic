#include <memory>
#include <cstdio>
#include <string>
using namespace std;
int main() {
    string a = "6";
    string b = a;
    string c = b;
    printf("%s", c);
    return 0;
}