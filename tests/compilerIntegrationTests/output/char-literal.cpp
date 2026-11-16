#include <memory>
#include <cstdio>
#include <string>
using namespace std;
int main() {
    char a = 'a';
    int comb = 'a' + 'b';
    string text = "a";
    text += a;
    char b = a + 3;
    printf("%d%s", comb, b);
    return 0;
}