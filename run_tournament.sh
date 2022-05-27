third_party/c-chess-cli/c-chess-cli -each tc=60+0 timeout=20 option.Hash=8 option.Threads=1 \
    -engine cmd=out/zgkm.exe \
    -engine cmd=out/pesto.exe \
    -games 2 -concurrency 1 \
    -resign count=3 score=700 -draw number=60 count=8 score=10 -pgn out.pgn 2 \
    -log
