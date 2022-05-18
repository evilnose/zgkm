CC = g++
SDIR = src
ODIR = obj
TDIR = test
SOURCES = $(wildcard $(SDIR)/*.cpp)
OBJECTS = $(patsubst $(SDIR)/%.cpp,$(ODIR)/%.o,$(SOURCES))
TST_SOURCES = $(wildcard $(TDIR)/*.cpp)
OBJECTS_WITHOUT_MAIN = $(filter-out $(ODIR)/main.o,$(OBJECTS))
TST_OBJECTS = $(patsubst $(TDIR)/%.cpp,$(ODIR)/%.o,$(TST_SOURCES)) 

CFLAGS := -std=c++17 -Wall -pthread
CFLAGS_DEBUG := -g -O0
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

CFLAGS_TEST := $(CFLAGS) -Ithird_party/inc/ -Isrc/

OUT = out/zgkm.exe

.PHONY: clean

all : $(OUT)

$(ODIR)/%.o: $(SDIR)/%.cpp $(SDIR)/%.h .buildmode
	$(CC) -c $(INC) -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(TDIR)/%.cpp
	$(CC) -c $(INC) -o $@ $< $(CFLAGS_TEST)

# main does not have a .h file
$(ODIR)/main.o: $(SDIR)/main.cpp .buildmode
	$(CC) -c $(INC) -o $@ $< $(CFLAGS)

$(OUT): $(OBJECTS) .buildmode
	$(CC) $(CFLAGS) $(OBJECTS) $(LINK) -o $(OUT)

test: $(TST_OBJECTS) $(OBJECTS_WITHOUT_MAIN)
	$(CC) $(CFLAGS_TEST) $(TST_OBJECTS) $(OBJECTS_WITHOUT_MAIN) -o out/test.exe
	out/test.exe

clean:
	rm -f $(OUT)
	rm -f $(ODIR)/*
	rm -f *.exp
