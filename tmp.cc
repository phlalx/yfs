#include <iostream>

class A {
  int x;
  int y;
public:
  A() {
    std::cout << "A constructor called" << std::endl;
  }

 A(int x, int y) : x(x), y(y) {
    std::cout << "A constructor 2 called" << std::endl;
  }

  ~A() {
    std::cout << "A destructor called" << std::endl;
  }

  virtual char f() { return 'a'; }
};

class B : public A {
  A a1, a2;
public:
   B() : A(1,1), a1(1,1), a2(0,1) {
    std::cout << "B constructor called" << std::endl;
  }
  
   ~B() {
    std::cout << "B destructor called" << std::endl;
  }

  virtual char f() { return 'b'; }
};

class C : B {
  std::string toto;
public:
   C() {
    std::cout << "C constructor called" << std::endl;
  }
   ~C() {
    std::cout << "C destructor called" << std::endl;
  }
  char f() { return 'c'; }
};

int main(int argc, char *argv[]) {

  // std::cout << "sizeof(A)" << " =  " << sizeof(A) <<  std::endl;
  // std::cout << "sizeof(B)" << " = " << sizeof(B) <<  std::endl;
  // std::cout << "sizeof(C)" << " = " << sizeof(C) <<  std::endl;
  A *a;
  B *b = new B();
//  C c;
  a = b;
  std::cout << "a.f()" << " = " << a->f() << std::endl;
  std::cout << "b.f()" << " = " << b->f() << std::endl;
//  std::cout << "c.f()" << " = " << c.f() << std::endl;
}
