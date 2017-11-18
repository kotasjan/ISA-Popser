CFLAGS=-Wall -Wextra -pedantic -std=c++17
LDFLAGS=-Wall -Wextra -pedantic -g -lpthread

COMPILE=g++ $(CFLAGS) -c
LINK=g++ $(LDFLAGS) -o

OBJ=popser.o thread.o server.o client.o email.o md5.o
BINDIR=./bin
NAME=popser


all: $(NAME) 

$(NAME): $(OBJ)
	$(LINK) $(NAME) $(OBJ)

%.o:	$(SRCDIR)/%.cpp
	$(COMPILE) $< -o $@

clean:
	rm -f $(OBJ) $(NAME)