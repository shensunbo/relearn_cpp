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
    LOG("oldStr "<<oldStr.size());
    LOG("newStr "<<newStr.size());
    // oldStr = std::string("new");
    oldStr = "new";
    LOG("oldStr "<<oldStr.size());
    LOG("=======================");

    //2
    std::shared_ptr<std::string> oldPtr = std::make_shared<std::string>(std::string("blahblah"));
    LOG("null? " << (nullptr == oldPtr));
    // std::shared_ptr newPtr(std::move(oldPtr));
    std::shared_ptr newPtr = (std::move(oldPtr));

    LOG("null? " << (nullptr == oldPtr));
    LOG(*newPtr);
    LOG("=======================");

    //3
    std::vector oldVec{1,2,3,4,5,6,7,8};
    std::vector newVec = std::move(oldVec);
    LOG("oldVec "<<oldVec.size());
    LOG("newVec "<<newVec.size());
    oldVec = std::move(newVec);
    LOG("oldVec "<<oldVec.size());
    LOG("newVec "<<newVec.size());

}