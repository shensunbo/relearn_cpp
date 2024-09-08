#include "base.h"
#include "string"
#include "iostream"
#include "functional"

using namespace std::placeholders;

static int old_function(int i, bool b, std::string str){
    std::cout<<"i = "<<i<<std::endl;
    std::cout<<"b = "<<b<<std::endl;
    std::cout<<"str = "<<str<<std::endl;
    return 0;
}

class FuncTest{
public:
    FuncTest(std::string str):s_(str) {}
    FuncTest() = delete;
    void foo(int i, std::string str){
        std::cout<<"i = "<<i<<std::endl;
        std::cout<<"str = "<<str<<std::endl;

    }
private:
    std::string s_;
};

void bind_base_test(){

    auto new_func1 = std::bind(old_function, 1, true, "new_func1");
    new_func1();

    std::function<int()> new_func2 = std::bind(old_function, 2, false, "new_func2");
    new_func2();

    auto new_func3 = std::bind(old_function, _1, true, _2);
    new_func3(3, "new_func3 ---");

    std::function<int(int, std::string)> new_func4 = std::bind(old_function, _1, true, _2);
    new_func3(4, "new_func4 +++");


    std::function<int(std::string, int)> new_func5 = std::bind(old_function, _2, false, _1);
    new_func5("new_func5", 5);
}

void bind_class_test(){
    FuncTest ft("FuncTest :->");

    std::function<void(int)> class_test1 = std::bind(&FuncTest::foo, &ft, _1, "class_test1");
    class_test1(1);
}

void bind_lambda_test(){
    //TODO: 
}