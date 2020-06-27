#include <cassert>
#include <iostream>

#include "bitboard.h"
#include "movegen.h"
#include "notation.h"
#include "uci.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    bboard::initialize();

    UCI::initialize(argc, argv);

    UCI::loop();

    return 0;
}
