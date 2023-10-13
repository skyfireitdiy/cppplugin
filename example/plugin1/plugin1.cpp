#include <stdio.h>

extern "C" {
int plugin1_func(const char* param);
}

int plugin1_func(const char* param)
{
    printf("plugin1_func: %s\n", param);
    return 50;
}
