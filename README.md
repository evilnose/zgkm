# ZGKM Chess Engine

## To build
gcc with C++17. Run `make` or `make DEBUG=1` or `make DEBUG=0`. `out/zgkm.exe` is the resulting
(sort of) UCI-compliant engine.

## Maintenance goals
* delete `search.cpp` and `search.h`
* resolve all the TODOs

## Planned goals and features
* parallel search
* PV search
* null window search
* qsearch pruning
* set hash table size (TT)
* Statistically rigorous measure of playing strength
* Testing on Longer time controls
* Opening book and endgame tablebases
* Pondering
* (possibly, maybe) my own hand-tuned eval

## gurrent features
* basically working chess engine that plays maybe around 1800 on Lichess
* bitboard & magic bitboard move generation
* basic qsearch with no pruning
* fixed-size TT
* basic move ordering
* PeSTO
* basic time management


## Tricky positions
`position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1`

`go depth 6`

### qsearch
`position fen r1b1k2r/pppp1ppp/1qn5/2b5/3n4/1P3N2/PBP1PPPP/RN1QKB1R w KQkq - 0 1`

`go depth 6`

`position startpos moves b1c3 b8c6 e2e3 e7e6 g1f3 g8f6 f1d3 f8d6 c3e4 c6b4 e1e2 f6e4 d3e4 d8f6 g2g4 h7h6 d1f1 e8e7 f1h3 h8e8 h1e1 b4a6 d2d4 c7c6 c1d2 a6c7 d2c3 c7b5 c3a5 b7b6 a5d2 b5c7 e4d3 c8b7 e3e4 c6c5 d4c5 d6c5 d2c3 f6g6 h3h4 f7f6 e4e5 g6f7 e5f6 g7f6 f3e5 f7g8 e5g6 e7d8 h4f6 d8c8 f6g7 c7d5 c3e5 d5b4 g7g8 e8g8 g6f4 g8g4 e2f1 g4h4 f4g6 h4h3 e5g3 b4d3 c2d3 c5b4 e1c1 b7c6 g6f4 h3g3 h2g3 c8b7 f1e2 a8e8 c1c4 b4d6 f4g6 e8g8 g6h4 c6d5 c4c1 d6g3 f2g3 g8g3 e2f2 g3g4 c1h1 d5h1 a1h1 g4b4 b2b3 b4b5 h4f3 b5a5 h1a1 b7c6 f2g3 c6d6 f3d2 d6d5 d2c4 a5a6 g3f4 h6h5 f4g5 b6b5 c4d2 b5b4 d2f3 a6a5 g5h5 d5d6 h5g4 d6e7 g4f4 e7f6`

`go wtime 33218 btime 32514 winc 0 binc 0`

### weird
`position startpos moves b2b3 c7c5 g1f3 b8c6 e2e3 e7e6 e1e2 c5c4 b3c4 e6e5 b1c3 g8f6 d1e1 e5e4`

`go wtime 138252 btime 136284 winc 0 binc 0`

### weird2 (!!QSEARCH EXAMPLE)`
`position startpos moves e2e3 b8c6 g1f3`

`go wtime 165010 btime 172208 winc 0 binc 0`
