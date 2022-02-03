#include "buildinfo/version.h"
#include <stdio.h>

#ifndef str
#define str(x)         _stringify(x)
#define _stringify(x)  #x
#endif

int main(void) {
    printf("%s\n", MINT_VERS_PATH_STRING);
    return 0;
}
