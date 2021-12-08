#include <cstdio>
#include <cstdlib>
#include <errno.h>

const int MAX_CHILDREN_QUANTITY = 10000;

long int get_num(int argc, char* argv[]) {
    long int num = 0;
    char* end;

    if(argc != 2) {
        fprintf(stderr, "Incorrect number of arguments!!!\n");

        return -1;
    }

    num = strtol(argv[1], &end, 10);

    if(num <= 0 || num > MAX_CHILDREN_QUANTITY) {
        fprintf(stderr, "Write correct number instead of: %s!!!\n", argv[1]);

        return -1;
    }
    else if (errno != 0) {
        fprintf (stderr, "Overflow!\n");

        return -1;
    }
    else {
        return num;
    }
}

int main (int argc, char* argv[]) {
    int children_quantity = get_num (argc, argv);
    if (children_quantity <= 0) {
        exit (EXIT_FAILURE);
    }

    for (int i = 1; i <= children_quantity; i++) {
        fprintf (stdout, "%d\n", i);
    }

    return 0;
}