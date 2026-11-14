int main() {
    std::shared_ptr<string> r = std::make_shared<string>("abcdefg");
    print("%s", *r);
    return 0;
}