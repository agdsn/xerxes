OBJECTS=xerxes.cxx
BIN=xerxes

$(BIN): $(OBJECTS)
	g++ -o $(BIN) $(OBJECTS) -Wall -pedantic -ggdb