#include <bits/stdc++.h> 
int main() {
    std::shared_ptr<int> x = std::make_shared<int>(10);
    std::shared_ptr<int> y = std::make_shared<int>(20);
    int sum = *x + *y;
    if (sum > 25) {
        x.reset();
    }
    else {
        y.reset();
    }
    printf("sum: %d", sum);
    return 0;
}