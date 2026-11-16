#include <memory>
#include <cstdio>
#include <string>
using namespace std;
int main() {
    std::shared_ptr<string> r = std::make_shared<string>("abcdefg");
    printf("%s", *r);
    return 0;
}