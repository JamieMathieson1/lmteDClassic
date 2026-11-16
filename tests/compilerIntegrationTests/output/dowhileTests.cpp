#include <memory>
#include <cstdio>
#include <string>
using namespace std;
int main() {
    int count = 0;
    int total = 0;
    while (count < 3) {
        total += count;
        count += 1;
    }
    do {
        total += count;
        count -= 1;
    }
    while (count > 0);
    return total;
}