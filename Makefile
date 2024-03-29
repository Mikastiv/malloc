ifeq ($(HOSTTYPE),)
	HOSTTYPE = $(shell uname -m)_$(shell uname -s)
endif

NAME = libft_malloc_$(HOSTTYPE).so
LINK = libft_malloc.so

CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wpedantic -fPIC -fno-strict-aliasing

LN = ln -sf
RM = rm -f

INCDIR = include

SRCDIR = src
OBJDIR = obj
CFILES = memory.c utils.c chunk.c heap.c arena.c freelist.c debug.c
HFILES = types.h utils.h arena.h chunk.h heap.h freelist.h debug.h
SRC = $(addprefix $(SRCDIR)/, $(CFILES))
INC = $(addprefix $(SRCDIR)/, $(HFILES))
OBJ = $(addprefix $(OBJDIR)/, $(CFILES:.c=.o))

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

all: $(NAME) $(LINK)

$(NAME): $(OBJDIR) $(OBJ)
	$(CC) -shared $(OBJ) -o $(NAME)
	$(LN) $(NAME) $(LINK)

$(LINK):
	$(LN) $(NAME) $(LINK)

$(OBJDIR):
	mkdir -p $(OBJDIR)

debug: CFLAGS += -g
debug: all

release: CFLAGS += -flto -O3
release: all

test: debug
	$(CC) -g main.c -L. -lft_malloc -Iinclude -Wl,-rpath,.

fmt:
	@clang-format -i $(SRC) $(INC) $(INCDIR)/memory.h

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME) $(LINK)

re: fclean all

.PHONY: all clean fclean re release debug test
