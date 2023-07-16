#include "logic.h"
#include "hal.h"


const char * on_cmd(const char *msg) {
    print(msg);
    print("\r\n$");
}

