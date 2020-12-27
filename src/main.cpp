#include <cassert>
#include <iostream>

#include "bitboard.h"
#include "movegen.h"
#include "notation.h"
#include "uci.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    uci::initialize(argc, argv);

    uci::loop();

    return 0;
}
