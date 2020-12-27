CC = g++
#CC = clang-8
SDIR = src
ODIR = obj
TDIR = test
SOURCES = bitboard.cpp main.cpp movegen.cpp position.cpp types.cpp utils.cpp \
		notation.cpp uci.cpp logger.cpp parallel.cpp evaluate.cpp search.cpp threading.cpp
_TESTS = test_bitboard.cpp
TESTS = $(patsubst %,$(_TESTS)/%,$(TDIR))
_OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS = $(patsubst %,$(ODIR)/%,$(_OBJECTS))
INC = -Iinclude/
LINK = 

CFLAGS := -std=c++17 -Wall
CFLAGS_DEBUG := -Wall -g -O0
CFLAGS_RELEASE := -DNDEBUG -O3

OLDMODE = $(shell cat .buildmode)
ifeq ($(DEBUG), 1)
CFLAGS := $(CFLAGS_DEBUG) $(CFLAGS)
ifneq ($(OLDMODE),debug)
$(shell echo debug > .buildmode)
endif
else
CFLAGS := $(CFLAGS_RELEASE) $(CFLAGS)
ifneq ($(OLDMODE),nodebug)
$(shell echo nodebug > .buildmode)
endif
endif

OUT = out/zgkm

.PHONY: clean

all : $(OUT)

$(ODIR)/%.o: $(SDIR)/%.cpp $(SDIR)/%.h .buildmode
	$(CC) -c $(INC) -o $@ $< $(CFLAGS)

# main does not have a .h file
$(ODIR)/main.o: $(SDIR)/main.cpp .buildmode
	$(CC) -c $(INC) -o $@ $< $(CFLAGS)

$(OUT): $(OBJECTS) .buildmode
	$(CC) $(CFLAGS) $(OBJECTS) $(LINK) -o $(OUT)

# test: $(OBJECTS)
# 	$(CC) $(CFLAGS) $(INCLUDE) -o test.exe $(OBJ) $(TESTS)

clean:
	rm -f $(OUT)
	rm -f $(ODIR)/*
	rm -f *.exp
