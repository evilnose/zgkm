#include "uci.h"
#include "movegen.h"
#include "utils.h"
#include "search.h"
#include "threading.h"
#include "logger.h"
#include "notation.h"
#include "hash.h"
#include "evaluate.h"

#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>
#include <atomic>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::istringstream;
using std::string;

void run_position(istringstream &iss)
{
    string fen;
    iss >> fen;
    Position pos;
    if (fen == "startpos")
    {
        // already default FEN
    }
    else
    {
        if (fen != "fen") {
            cerr << "malformed command" << endl;;
            return;
        }
        pos.load_fen(iss);
    }

    string placeholder;
    if ((iss >> placeholder) && placeholder != "moves") {
        cerr << "Unknown token '" << placeholder << "'. Should be 'moves' instead." << endl;
        return;
    }

    // parse and make moves
    string move_str;
    while (iss >> move_str) {
        Move move = notation::parse_uci_move(pos, move_str);
        pos.make_move(move);
    }

    thread::set_position(pos);
}

void run_go_perft(int depth)
{
    int result = perft(thread::get_position(), depth);
    cout << result << endl;
}

std::unordered_map<string, string> parse_keyvalue(istringstream &iss)
{
    std::unordered_map<string, string> ret;
    auto it = std::istream_iterator<string>(iss);

    while (it != std::istream_iterator<string>()) {
        auto key = *it;
        ret[key] = *(++it);
        it++;
    }

    return ret;
}

void delegate_command(const string &command, istringstream &liness)
{
}

void uci::initialize(int argc, char *argv[])
{
    bboard::initialize();
    zobrist::initialize();
    init_eval_tables();
    thread::set_num_threads(1);
}

void uci::loop()
{
    string line;
    istringstream liness;
    string command;
    #if USE_PESTO
    string ENGINE_NAME = "ZGKM-PESTO-TABLE";
    #else
    string ENGINE_NAME = "ZGKM-NO-TABLE";
    #endif
    #ifdef MAT_ONLY
        ENGINE_NAME += "-MAT-ONLY";
    #endif
    std::cerr << ENGINE_NAME << " (alpha) by Gary Geng" << std::endl;
    Position pos;
    istringstream temp(STARTING_FEN);
    while (cin)
    {
        getline(cin, line);
        liness = istringstream{line};
        if (liness >> command)
        {
            if (command == "uci")
            {
                cout << "id name " << ENGINE_NAME << endl;
                cout << "id author Gary Geng" << endl;
                cout << "uciok" << endl;
            }
            else if (command == "debug")
            {
                cerr << "debug not implemented" << endl;
            }
            else if (command == "isready")
            {
                cerr << "readyok" << endl;
            }
            else if (command == "setoption")
            {
                cerr << "setoption not implemented" << endl;
            }
            else if (command == "ucinewgame")
            {
                cerr << "ucinewgame not implemented" << endl;
            }
            else if (command == "position")
            {
                run_position(liness);
            }
            else if (command == "go")
            {
                std::unordered_map<string, string> args = parse_keyvalue(liness);

                std::vector<string> constraints{"wtime", "depth", "nodes", "mate", "movetime", "infinite", "perft"};
                bool constraint_found = false;
                std::string constraint;
                std::string value;
                for (const string& cons : constraints) {
                    auto it = args.find(cons);
                    if (it != args.end()) {
                        // assert that the number of constraints is exactly one
                        constraint_found = true;
                        constraint = it->first;
                        value = it->second;
                        break;
                    }
                }
                if (!constraint_found) {
                    std::cerr << "search constraint not found!";
                    continue;
                }

                SearchLimit slimit = {};
                if (constraint == "wtime") {
                    int wtime = std::stoi(value);
                    int btime = std::stoi(args["btime"]);
                    int winc = std::stoi(utils::get_or_default(args, string("winc"), string("0")));
                    int binc = std::stoi(utils::get_or_default(args, string("binc"), string("0")));
                    slimit.tc = TimeControlParams{wtime, btime, winc, binc};
                    // TODO search in a new thread
                } else if (constraint == "depth") {
                    slimit.depth = std::stoi(value);
                    assert(slimit.depth > 0);
                } else if (constraint == "movetime") {
                    slimit.fixed_time = std::stoi(value);
                    assert(slimit.fixed_time > 0);
                } else if (constraint == "infinite") {
                    // Don't set any constraints
                    // LOG(logWARNING) << "infinite constraint not actually implemented.";
                } else if (constraint == "perft") {
                    run_go_perft(std::stoi(value));
                    continue;
                } else {
                    LOG(logERROR) << "Constraint not implemented: " << constraint;
                    return;
                }

                thread::start_search(slimit);

            } else if (command == "stop") {
                thread::stop_search();
            } else {
                LOG(logERROR) << "Unknown command: '" << command << "'";
            }
        }
    }
}

void uci::cleanup() {
    thread::cleanup();
}
