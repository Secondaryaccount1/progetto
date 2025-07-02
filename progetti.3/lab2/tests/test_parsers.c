#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "parse_rescuers.h"
#include "parse_emergency_types.h"

int main(void) {
    // Crea due piccoli file di test
    FILE *f;
    f = fopen("tests/_resc.txt","w");
    fprintf(f,"[Amb][2][5][1;1]\n[Vig][1][3][2;2]\n");
    fclose(f);

    rescuer_type_t *rl = NULL;
    int nrl = 0;
    assert(parse_rescuers_file("tests/_resc.txt", &rl, &nrl) == 0);
    assert(nrl == 2);
    assert(strcmp(rl[0].name,"Amb")==0 && rl[0].speed==5 && rl[0].number==2);

    f = fopen("tests/_et.txt","w");
    fprintf(f,"[Fire] [2] Amb:1,5;\n");
    fclose(f);

    emergency_type_t *el = NULL;
    int nel = 0;
    assert(parse_emergency_types_file("tests/_et.txt", rl, nrl, &el, &nel) == 0);
    assert(nel == 1);
    assert(strcmp(el[0].name,"Fire")==0 && el[0].priority==2 && el[0].n_required==1);
    free(el);

    // Unknown rescuer type should be logged and the line skipped
    f = fopen("tests/_et.txt","w");
    fprintf(f,"[Crash] [1] Foo:1,3;\n");
    fclose(f);

    el = NULL;
    nel = 0;
    assert(parse_emergency_types_file("tests/_et.txt", rl, nrl, &el, &nel) == 0);
    assert(nel == 0);
    free(el);

    // Mixed valid and unknown rescuer names in the same line
    f = fopen("tests/_et.txt","w");
    fprintf(f,"[Mixed] [1] Amb:1,5; Foo:2,3;\n");
    fclose(f);

    el = NULL;
    nel = 0;
    assert(parse_emergency_types_file("tests/_et.txt", rl, nrl, &el, &nel) == 0);
    assert(nel == 0);
    free(el);

    free(rl);

    printf("[test_parsers] all tests passed\n");
    return 0;
}
