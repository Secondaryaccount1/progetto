#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "parse_env.h"

int main(void) {
    FILE *f = fopen("tests/_env.conf", "w");
    fprintf(f, "\nqueue=/tmpq\nwidth=5\nheight=7\n");
    fclose(f);

    env_config_t cfg;
    assert(parse_env_file("tests/_env.conf", &cfg) == 0);
    assert(strcmp(cfg.queue_name, "/tmpq") == 0);
    assert(cfg.width == 5);
    assert(cfg.height == 7);

    printf("[test_parse_env] all tests passed\n");
    return 0;
}
