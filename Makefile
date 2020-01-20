# Terribly handwritten makefile, use cmake
CC=clang++
CFLAGS=-I. -std=c++11
LDFLAGS=-framework Metal -framework CoreGraphics -framework Foundation
BUILDDIR=build
OBJDIR=$(BUILDDIR)
OBJ=$(OBJDIR)/main.o


all: $(OBJ)
	$(CC) -o app $(OBJ) $(LDFLAGS)


$(OBJDIR)/%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(BUILDDIR)/* app