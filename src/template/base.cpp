#include "base.h"
#include <iostream>
#include <string>
#include <set>

template <typename T>
T max(T a, T b) {
    return (a > b) ? a : b;
}
void template_func_param1(){
    //C++ 17
    std::cout<<" int "<<max(1, 10)<<std::endl;
    std::cout<<" float "<<max(10.05f, 5.666f)<<std::endl;

    //C++ 14
    std::cout<<" int "<<max<>(1, 10)<<std::endl;

    //C++ 11
    std::cout<<" int "<<max<int>(1, 10)<<std::endl;
}

template <typename T>
class Container {
private:
    T value;
public:
    Container(T val) : value(val) {}
    T getValue() { return value; }
};

void template_class(){
    std::string str = "hello world";
    Container ctn1(str);
    std::cout<<ctn1.getValue()<<std::endl;

    Container ctn2(2);
    std::cout<<ctn2.getValue()<<std::endl;

    std::set iset = {3, 5, 1, 0, 9};
    Container ctn3(iset);
    for (const int& element : ctn3.getValue()) {
        std::cout << element << " ";
    }
    std::cout << std::endl;

    //C++ 11
    Container<float> ctn4(3.1415926f);
    std::cout<<ctn4.getValue()<<std::endl;
}


