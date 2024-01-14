ifeq ($(HOSTTYPE),)
	HOSTTYPE = $(shell uname -m)_$(shell uname -s)
endif

NAME = libft_malloc_$(HOSTTYPE).so
LINK = libft_malloc.so

CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -fPIC -g

LN = ln -sf
RM = rm -f

INCDIR = include

SRCDIR = src
OBJDIR = obj
CFILES = memory.c
HFILES = memory.h
SRC = $(addprefix $(SRCDIR)/, $(CFILES))
INC = $(addprefix $(INCDIR)/, $(HFILES))
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

fmt:
	@clang-format -i $(SRC) $(INC)

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME) $(LINK)

re: fclean all

.PHONY: all clean fclean re
