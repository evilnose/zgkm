#include "uci.h"
#include "movegen.h"
#include "utils.h"

#include <iostream>
#include <sstream>
#include <string>

using std::cout;
using std::cin;
using std::endl;
using std::istringstream;
using std::string;

static Position position;

void run_position(istringstream& iss) {
    string fen;
    iss >> fen;
    if (fen == "startpos") {
        fen = STARTING_FEN;
    } else {
        assert(fen == "fen");
        std::getline(iss, fen);
        utils::trim(fen);
    }
    istringstream fen_ss(fen);
    position.load_fen(fen_ss);
    // TODO load moves
}

void run_go_perft(int depth) {
    int result = perft(position, depth);
    cout << result << endl;
}

namespace {
void delegate_command(const string& command, istringstream& liness) {
    if (command == "uci") {
        cout << "id name ZGKM 0.0\n";
        cout << "id author Gary Geng" << endl;
    } else if (command == "debug") {
        cout << "not implemented" << endl;
    } else if (command == "isready") {
        cout << "not implemented" << endl;
    } else if (command == "setoption") {
        cout << "not implemented" << endl;
    } else if (command == "ucinewgame") {
        cout << "not implemented" << endl;
    } else if (command == "position") {
        run_position(liness);
    } else if (command == "go") {
        string subcommand;
        if (liness >> subcommand) {
            if (subcommand == "searchmoves") {
                cout << "not implemented" << endl;
            } else if (subcommand == "ponder") {
                cout << "not implemented" << endl;
            } else if (subcommand == "wtime") {
                cout << "not implemented" << endl;
            } else if (subcommand == "btime") {
                cout << "not implemented" << endl;
            } else if (subcommand == "winc") {
                cout << "not implemented" << endl;
            } else if (subcommand == "bwinc") {
                cout << "not implemented" << endl;
            } else if (subcommand == "movestogo") {
                cout << "not implemented" << endl;
            } else if (subcommand == "depth") {
                cout << "not implemented" << endl;
            } else if (subcommand == "nodes") {
                cout << "not implemented" << endl;
            } else if (subcommand == "mate") {
                cout << "not implemented" << endl;
            } else if (subcommand == "movetime") {
                cout << "not implemented" << endl;
            } else if (subcommand == "infinite") {
                cout << "not implemented" << endl;
            } else if (subcommand == "perft") {
                int depth = 5;
                liness >> depth;
                assert(depth > 0);
                run_go_perft(depth);
            }
        }
    } else if (command == "stop") {
        cout << "not implemented" << endl;
    } else if (command == "ponderhit") {
        cout << "not implemented" << endl;
    } else if (command == "quit") {
        exit(0);
    } else {
        cout << "Unknown command: " << command << endl;
    }
}
}  // namespace

void UCI::initialize(int argc, char* argv[]) {}

void UCI::loop() {
    string line;
    istringstream liness;
    string command;
    while (cin) {
        getline(cin, line);
        liness = istringstream{line};
        if (liness >> command) {
            delegate_command(command, liness);
        }
    }
}
