class Dog {
    public: 
        int age;
        int weight;
};
int main() {
    std::shared_ptr<Dog> r = std::make_shared<Dog>();
    return 0;
}