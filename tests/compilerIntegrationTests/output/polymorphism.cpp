#include <memory>
#include <cstdio>
#include <string>
using namespace std;
class Base {
    public: 
        virtual void run() = 0;
        virtual void reset() {
        }
};
class Derived :  public Base {
    public: 
        void run() {
        }
};
int main() {
    std::shared_ptr<Base> animal = std::make_shared<Derived>();
    animal->run();
    return 0;
}