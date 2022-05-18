third_party/c-chess-cli/c-chess-cli -each tc=1+0 option.Hash=8 option.Threads=1 \
    -engine cmd=out/zgkm.exe \
    -engine cmd=out/zgkm.exe \
    -games 2 -concurrency 2 \
    -resign count=3 score=700 -draw number=40 count=8 score=10 -pgn out.pgn 2 \
    -log
