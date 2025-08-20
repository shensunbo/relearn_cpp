#include <iostream>
#include <cstdio>
// before main test demo
// for GCC

static int i = 0;

__attribute__((constructor)) void beforeMain() {
    i++;
    printf("Before main() - __attribute__((constructor)), i %d\n", i);
    return; 
}

__attribute__((destructor)) void afterMain() {
    printf("After main() - __attribute__((destructor))\n");
    return; 
}

int main(int argc, char *argv[])
{
    std::cout << "Hello, World! i: " << i << std::endl;
    return 0;
}

