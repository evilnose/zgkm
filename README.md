# ZGKM Chess Engine

## Features Planned for the CSE 573 Final Project
* Good evaluation function (TODO)
* More metrics recorded: PV, nodes, nodes rate, etc and send to UCI
* Time management: start off with a basic one from Cray Blitz: https://www.chessprogramming.org/Time_Management#Extra_Time.
* Simple tournament tool for measuring extremely rough estimates of my engine's playing strength
* Simple tool that has engine search on test positions and benchmark them (speed, nodes searched, correctness)
* Better move ordering
* Transposition tables
* Quiet search
* Parallel search

## Possible Features
* Better early termination? e.g. check every 1024 nodes searched: https://chess.stackexchange.com/questions/17510/how-do-chess-engines-manage-time-when-they-play-real-games


## Non-goals
* Pondering
* Statistically rigorous measure of playing strength
* Longer time controls
* Opening book and endgame tablebases

## Goals Week of 5/22/2022
* Implement better time management (DONE)
* Slightly better eval (DONE)
* Load test positions (SKIPPED)
* Better eval? (WIP)
* refactor hashtable to be a class rather than a file, since we might need more than one
* Position hashing & transposition table (DONE)
* extract PV (DONE)
* load test positions (SKIPPED)
* how do I evict from the table? (DONE)
* verify that zobrist seed is good (WIP)
* Add metrics (WIP)
* PeSTO's evaluation function (DONE): https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function
* Get table working (TODO)
* Implement PV search (TODO)
* Implement depth + always replace scheme as comparison (MAYBE)
* Run tournaments; get some metrics (TODO)
* Done.

## Goals Week of 5/29/2022
* if we're doing well in terms of chess engine performance, then either parallel search or aspiration window
* otherwise, fix shit.

For some reason, instead of making the obvious pawn capture-promotion, some other weird move is played,
by both evaluation functions. (Also you can just view the PGNs)
