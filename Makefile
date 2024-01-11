ifeq ($(HOSTTYPE),)
	HOSTTYPE = $(shell uname -m)_$(shell uname -s)
endif

CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic

RM = rm -f

NAME = libft_malloc_$(HOSTTYPE).so

INCDIR = include

SRCDIR = src
OBJDIR = obj
CFILES = memory.c
HFILES = memory.h
SRC = $(addprefix $(SRCDIR)/, $(CFILES))
INC = $(addprefix $(INCDIR)/, $(HFILES))
OBJ = $(addprefix $(OBJDIR)/, $(CFILES:.c=.o))

$(OBJDIR)/%.o: $(SRCDIR)/%.c
		$(CC) $(CFLAGS) -c -I$(INCDIR) $< -o $@
all: $(NAME)

$(NAME): $(OBJDIR) $(OBJ)
	$(CC) -shared $(OBJ) -o $(NAME)

$(OBJDIR):
	mkdir -p $(OBJDIR)

fmt:
	@clang-format -i $(SRC) $(INC)

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
