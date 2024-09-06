# **************************************************************************** #
#                                                                              #
#                                                         ::::::::             #
#    Makefile                                           :+:    :+:             #
#                                                      +:+                     #
#    By: kaltevog <kaltevog@student.codam.nl>         +#+                      #
#                                                    +#+                       #
#    Created: 2024/08/08 23:18:33 by kaltevog      #+#    #+#                  #
#    Updated: 2024/08/08 23:18:33 by kaltevog      ########   odam.nl          #
#                                                                              #
# **************************************************************************** #

C++ = c++
C++_FLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic
INFILES = 	main.cpp\
			ClapTrap.cpp\

OBJFILES = $(INFILES:.cpp=.o)

NAME = claptrap

all: $(NAME)

$(NAME):$(OBJFILES)
	$(C++) $(C++_FLAGS) $(OBJFILES) -o $(NAME) 

%.o: %.cpp
	$(C++) $(C++_FLAGS) -c $< -o $@

clean:
	rm -f $(OBJFILES)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re