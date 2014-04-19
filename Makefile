CC = g++

UGOC_ROOT=./
CFLAGS = -Wall -Werror -ansi -std=c++0x\
	 -I include \
	 -I libdtw/include/ \
	 -I libutility/include \
	 -I libsegtree/include \
	 -I libfeature/include

MACHINE = $(shell uname -m)
LDFLAGS = -ldtw \
	  -lfeature \
	  -lsegtree \
	  -lutility

LDPATH = -L libdtw/lib/$(MACHINE) \
	 -L libutility/lib/$(MACHINE) \
	 -L libsegtree/lib/$(MACHINE) \
	 -L libfeature/lib/$(MACHINE)

.PHONY: all clean

EXECUTABLES=dtw
all: CFLAGS +=  -O2
debug: CFLAGS += -g

all: $(EXECUTABLES)
debug: $(EXECUTABLES)

$(EXECUTABLES): % : %.cpp $(OBJ)
	${CC} ${CFLAGS} $(LDPATH) $< ${LDFLAGS} -o $@

clean:
	rm -f $(EXECUTABLES)
