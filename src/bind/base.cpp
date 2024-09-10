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
    std::function<int(int)> add = std::bind(std::plus<int>(), std::placeholders::_1, 2);
    std::function<int(int)> custom_add = [add](int x) { return add(x) * 20; };
    int ret = custom_add(1);
    std::cout<<"bind_lambda_test1 "<< ret<<std::endl; //ret = 60

    //when the api need a 1 params function as input param, you can use bind to change a 2 params function to 1 param
    auto fAdd2 = std::bind(std::plus<float>(), _1, 2.0f);
    auto lambda_test2 = [](std::function<float(float)> add2)->float {
        float ret = 1000.0f + add2(0.05f);
        return ret;
    };
    float fRet = lambda_test2(fAdd2);
    std::cout<<"lambda_test2 "<<fRet<<std::endl; //1002.05f
}

void bind_algo_adapt_test(){
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    // transform(_InputIterator __first, _InputIterator __last, _OutputIterator __result, _UnaryOperation __unary_op)
    std::transform(numbers.begin(), numbers.end(), numbers.begin(),
                   std::bind(std::multiplies<int>(), std::placeholders::_1, 2));
    for(auto i:numbers){
        std::cout<<i<<" "; //2 4 6 8 10 
    }
    std::cout<<std::endl;
}

template <typename T, typename F>
void foreach(std::vector<T>& vec, F operate) {
    for (T& elem : vec) {
        elem = operate(elem);
    }
}

void bind_high_order_func(){
    {
        std::vector<int> numbers = {1, 2, 3};
        auto trible = std::bind(std::multiplies<int>(), std::placeholders::_1, 3);
        //the compiler can infer the type, maybe try to explore when learn template
        foreach(numbers, trible);
        std::cout<<"trible "<<numbers.at(0)<<" "<<numbers.at(1)<<" "<<numbers.at(2)<<std::endl;
    }

    {
        std::vector<int> numbers = {1, 2, 3};
        auto plus10 = std::bind(std::plus<int>(), std::placeholders::_1, 10);
        foreach(numbers, plus10);
        std::cout<<"plus10 "<<numbers.at(0)<<" "<<numbers.at(1)<<" "<<numbers.at(2)<<std::endl;
    }

    {
        std::vector<int> numbers = {1, 2, 3};
        auto minus5 = std::bind(std::minus<int>(), std::placeholders::_1, 5);
        //assign a specific template type
        foreach<int, std::function<int(int)>>(numbers, minus5);
        std::cout<<"minus5 "<<numbers.at(0)<<" "<<numbers.at(1)<<" "<<numbers.at(2)<<std::endl;
    }

    {
        std::vector<float> numbers = {1.0f, 2.0f, 3.0f};
        auto minus10 = std::bind(std::minus<float>(), std::placeholders::_1, 10.05f);
        //assign a specific template type
        foreach(numbers, minus10);//<float, std::function<float(float)>>
        std::cout<<"minus10 float "<<numbers.at(0)<<" "<<numbers.at(1)<<" "<<numbers.at(2)<<std::endl;
    }
}