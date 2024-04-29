#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    //setup_default_uart();
    while(true){
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
