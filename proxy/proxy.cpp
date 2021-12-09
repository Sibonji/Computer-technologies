const int arg_quantity = 3;

#include "MyLib.hpp"

int main (int argc, char* argv[]) {
    int children_quantity = get_num (argc, argv, arg_quantity);
    if (children_quantity < 0) {
        exit (EXIT_FAILURE);
    }

    return 0;
}