#include <bits/stdc++.h>
class Dog {
  int age;
  int weight;
  void setage(int newage) { age = newage; }
  int getage() { return age; }
  void setweight(int newweight) { weight = newweight; }
  int getweight() { return weight; }
};
int main() {
  Dog rex;
  Dog fido;
  rex.setage(3);
  rex.setweight(20);
  fido.setage(9);
  fido.setweight(35);
  printf("rex:  age %d weight %d\n", rex.getage(), rex.getweight());
  printf("fido: age %d weight %d\n", fido.getage(), fido.getweight());
  rex.setage(4);
  printf("rex:  age %d\n", rex.getage());
  printf("fido: age %d\n", fido.getage());
  return 0;
}