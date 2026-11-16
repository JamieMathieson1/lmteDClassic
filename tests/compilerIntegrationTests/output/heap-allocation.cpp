#include <memory>
#include <cstdio>
#include <string>
using namespace std;
int main() {
    std::shared_ptr<string> r = std::make_shared<string>("1220");
    printf("%d", *r);
    return 0;
}