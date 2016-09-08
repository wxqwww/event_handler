vpath %.h ./include
vpath %.c ./src
vpath %.cpp ./src

CPPFLAGS=-std=c++0x -I ../include

test1: ../event_handler.o test1.o
	g++ -o $@ $^ $(CPPFLAGS)
%.o : %.c
	g++ $@ $< $(CPPFLAGS)

.PHONY: claan
clean:
	rm test1.o test1
