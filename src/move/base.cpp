#include "base.h"
#include "log.h"
#include <string>
#include <iostream>
#include <utility>
#include <memory>
#include <vector>

void move_basic_concept(){
    //1
    std::string oldStr = "hello world";
    LOG("oldStr "<<oldStr.size());

    std::string newStr(std::move(oldStr));
    LOG("oldStr "<<oldStr.size());//0
    LOG("newStr "<<newStr.size());
    // oldStr = std::string("new");
    oldStr = "new";
    LOG("oldStr "<<oldStr.size());
    LOG("=======================");

    //2
    std::shared_ptr<std::string> oldPtr = std::make_shared<std::string>(std::string("blahblah"));
    LOG("null? " << (nullptr == oldPtr));//0
    // std::shared_ptr newPtr(std::move(oldPtr));
    std::shared_ptr newPtr = (std::move(oldPtr));

    LOG("null? " << (nullptr == oldPtr));//1
    LOG(*newPtr);
    LOG("=======================");

    //3
    std::vector oldVec{1,2,3,4,5,6,7,8};
    std::vector newVec = std::move(oldVec);
    LOG("oldVec "<<oldVec.size());//0
    LOG("newVec "<<newVec.size());
    oldVec = std::move(newVec);
    LOG("oldVec "<<oldVec.size());
    LOG("newVec "<<newVec.size());//0

}


class MoveTest{
public:
    MoveTest(std::string s, std::vector<int> v):str(s), vec(v) { }
    MoveTest() = delete;
public:
    std::string str;
    std::vector<int> vec;
};

void move_class(){
    LOG("MoveTest: " << sizeof(MoveTest));//56
    std::vector<int> vec;
    for(auto i = 0; i < 1000; i++){
        vec.push_back(i);
    }

    MoveTest oldClass("abcdefghijk", vec);
    LOG("oldClass: " << sizeof(oldClass));//56

    MoveTest newClass(std::move(oldClass));
    LOG("oldClass: " << sizeof(oldClass));//56
    LOG("newClass: " << sizeof(newClass));//56
    LOG("oldClass str " << oldClass.str.size());//0
    LOG("oldClass str " << oldClass.vec.size());//0
}
