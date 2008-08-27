OBJECTS=xerxes.cxx socket.cxx epoll.cxx

BIN=xerxes
RUNNER=xerxes_runner.rb
CONFFILE=xerxes.conf

BINDIR=/usr/bin/
CONFDIR=/etc/

$(BIN): $(OBJECTS)
	g++ -o $(BIN) $(OBJECTS) -Wall -pedantic -ggdb -lboost_regex -lboost_program_options

all: $(BIN)

test: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN) *.o *.cxx~ *.hxx~ Makefile~

install:
	install -m755 $(BIN) $(DESTDIR)$(BINDIR)
	install -m755 $(RUNNER) $(DESTDIR)$(BINDIR)
	install -m644 $(CONFFILE) $(DESTDIR)$(CONFDIR)

.PHONY: all
