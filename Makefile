OBJECTS=xerxes.cxx socket.cxx epoll.cxx
BIN=xerxes

$(BIN): $(OBJECTS)
	g++ -o $(BIN) $(OBJECTS) -Wall -pedantic -ggdb -lboost_regex -lboost_program_options

all: $(BIN)

test: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN) *.o *.cxx~ *.hxx~ Makefile~

.PHONY: all
