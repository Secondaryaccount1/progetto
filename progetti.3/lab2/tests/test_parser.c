
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "parse_rescuers.h"
#include "parse_emergency_types.h"

int main(void) {
    // Crea due piccoli file di test
    FILE *f;
    f = fopen("tests/_resc.txt","w");
    fprintf(f,"Amb 5 2 1 1\nVig 3 1 2 2\n");
    fclose(f);

    rescuer_type_t *rl = NULL;
    int nrl = 0;
    assert(parse_rescuers_file("tests/_resc.txt", &rl, &nrl) == 0);
    assert(nrl == 2);
    assert(strcmp(rl[0].name,"Amb")==0 && rl[0].speed==5 && rl[0].number==2);
    free(rl);

    f = fopen("tests/_et.txt","w");
    fprintf(f,"Fire 2 15 1 0\nMala 1 10 2 1 0\n");
    fclose(f);

    emergency_type_t *el = NULL;
    int nel = 0;
    assert(parse_emergency_types_file("tests/_et.txt", &el, &nel) == 0);
    assert(nel == 2);
    assert(strcmp(el[0].name,"Fire")==0 && el[0].priority==2 && el[0].n_required==1);
    assert(strcmp(el[1].name,"Mala")==0 && el[1].n_required==2);
    free(el);

    printf("[test_parsers] all tests passed\n");
    return 0;
}
