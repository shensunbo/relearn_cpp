#include "specialization.h"
#include <string>
#include <iostream>
#include <vector>
#include <set>

// specialization in class
template <typename T>
class Foo {
public:
    Foo(T v): value(v) {}
    void print() {
        std::cout << "Default implementation: " << std::endl;
    }

    T value;
};

// Specialization for int type
template <>
class Foo<int> {
public:
    Foo(int v): value(v) {}
    void print() {
        std::cout << "Specialized implementation for int: " << std::endl;
        std::cout << value << std::endl;
    }

    int value;
};

// Specialization for std::string type
template <>
class Foo<std::string> {
public:
    Foo(std::string v): value(v) {}
    void print() {
        std::cout << "Specialized implementation for std::string: " << std::endl;
        std::cout << value << std::endl;
    }
    
    std::string value;
};

void template_specialization_class(){
    Foo<float> foof(3.1415f);
    foof.print();
    std::cout<<"foof "<<foof.value<<std::endl;

    Foo<int> fooi(3);
    fooi.print();

    std::string str = "i am a string";
    Foo foos(str);
    foos.print();

    Foo<std::vector<int>> foov(std::vector<int>{1, 2, 3, 4, 5});
    foov.print();
    std::cout<<foov.value[0]<<foov.value[1]<<foov.value[2]<<foov.value[3]<<foov.value[4]<<std::endl;
}

//template_specialization_function
template<typename T>
void myprint(T value){
    std::cout<<"default -> "<<value<<std::endl;
}

template<>
void myprint<std::vector<int>>(std::vector<int> value){
    for (auto& element : value) {
        std::cout << element << " ";
    }
    std::cout << std::endl;
}

template<>
void myprint<std::set<int>>(std::set<int> value){
    for (auto& element : value) {
        std::cout << element << " ";
    }
    std::cout << std::endl;
}

void template_specialization_func(){
    myprint<int>(10);
    myprint<std::vector<int>>(std::vector<int>{111,222,333,444,999});
    myprint<std::set<int>>(std::set<int>{1,-1,-8,5,-9,30});
}

