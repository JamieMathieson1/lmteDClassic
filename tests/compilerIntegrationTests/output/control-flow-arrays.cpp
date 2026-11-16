#include <memory>
#include <cstdio>
#include <string>
using namespace std;
class Dog {
    public: 
        int age;
        int weight;
};
int add(int a, int b) {
    return a + b;
}
int main() {
    int total = 0;
    int nums[3] = {1, 2, 3};
    nums[0] = 5;
 {
        int i = 0;
        total += nums[i];
        i += 1;
        total += nums[i];
        i += 1;
        total += nums[i];
        i += 1;
    }
    if (total > 5) {
        int x = 1;
        total += x;
    }
    if (total > 100) {
        int x = 2;
        total += x;
    }
    std::shared_ptr<Dog> r = std::make_shared<Dog>();
    r->age = add(total, 2);
    r->weight = r->age / 2;
    r.reset();
    return 0;
}