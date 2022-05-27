#define CATCH_CONFIG_MAIN
#include "catch2.hpp"

#include "position.h"
#include "notation.h"
#include "search.h"

class testRunListener : public Catch::TestEventListenerBase {
public:
    using Catch::TestEventListenerBase::TestEventListenerBase;

    void testRunStarting(Catch::TestRunInfo const&) override {
		zobrist::initialize();
		bboard::initialize();
		init_eval_tables();
    }
};

CATCH_REGISTER_LISTENER(testRunListener)

#define REQUIRE_M(cond, msg) do { INFO(msg); REQUIRE(cond); } while((void)0, 0)

Position position_from_file(std::string fname) {
    std::ifstream infile;
    infile.open("fixtures/" + fname);
	assert(infile.good());
	return Position(infile);
}

Position position_from_fen(std::string fen) {
	std::istringstream iss(fen);
	return Position(iss);
}

TEST_CASE( "sees one move ahead and considers material", "[search-eval]" ) {

	SECTION( "white sees mate-in-one" ) {
		Position p = position_from_fen("rnbqkbnr/pppppppp/8/7B/8/5R2/PPPPPPPP/RNB1K1NQ w Qkq - 0 1");

		REQUIRE( depth_search(p, 1).best_move == create_normal_move(SQ_H5, SQ_F7) );
		REQUIRE( depth_search(p, 2).best_move == create_normal_move(SQ_H5, SQ_F7) );
	}

	SECTION( "black sees mate-in-one" ) {
		Position p = position_from_fen("rnb1k2r/ppppppbp/8/5n1K/5q2/8/PPPPPPPP/RNBQ1BNR b kq - 0 1");
		
		Move move = depth_search(p, 1).best_move;
		REQUIRE_M(move == create_normal_move(SQ_F4, SQ_H4), "actual move: " + notation::dump_uci_move(move) );
		move = depth_search(p, 2).best_move;
		REQUIRE_M(move == create_normal_move(SQ_F4, SQ_H4), "actual move: " + notation::dump_uci_move(move) );
	}

	SECTION( "white captures queen" ) {
		Position p = position_from_fen("rnb1kbnr/pppppppp/8/3q4/8/4N3/PPPPPPPP/RNBQKB1R w KQkq - 0 1");
		REQUIRE( depth_search(p, 1).best_move == create_normal_move(SQ_E3, SQ_D5) );
		REQUIRE( depth_search(p, 2).best_move == create_normal_move(SQ_E3, SQ_D5) );
		REQUIRE( depth_search(p, 3).best_move == create_normal_move(SQ_E3, SQ_D5) );
	}

	SECTION( "black exchanges knight for rook" ) {
		Position p = position_from_fen("rnbqkb1r/pppppppp/8/6n1/8/5R2/PPPPPPPP/RNBQKBN1 b Qkq - 0 1");
		Move move = depth_search(p, 4).best_move;
		REQUIRE_M( move == create_normal_move(SQ_G5, SQ_F3), "actual move: " + notation::dump_uci_move(move) );
	}
}

TEST_CASE("sees mate in N", "mate") {
	SECTION( "Mate in 3: Roberto Grau vs. Edgar Colle" ) {
		Position p = position_from_fen("1k5r/pP3ppp/3p2b1/1BN1n3/1Q2P3/P1B5/KP3P1P/7q w - - 1 1");
		SearchResult result = depth_search(p, 6);
		REQUIRE( result.eval == SCORE_POS_INFTY );
		REQUIRE_M( result.best_move == create_normal_move(SQ_C5, SQ_A6),
			"actual move: " + notation::dump_uci_move(result.best_move) );
	}
}

