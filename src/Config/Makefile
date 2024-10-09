NAME = Parser

# CFLAGS = -Wall -Wextra -Werror
INCLUDE = -I../
CFLAGS += --std=c++20

MFLAGS = -MMD

DIR_DEP = dep
DIR_OBJ = obj
DIR_SRC = .

SRC = ConfigParser.cpp Config.cpp \
	ServerSettings.cpp ConfigInputValidators.cpp \
	ConfigInputConverters.cpp ConfigDirectiveHandlers.cpp \
	ConfigTokenizer.cpp test.cpp 

SRC := ${addprefix ${DIR_SRC}/, ${SRC}}

OBJ = ${subst ${DIR_SRC}/, ${DIR_OBJ}/, ${SRC:.cpp=.o}}

DEP = ${subst ${DIR_OBJ}/, ${DIR_DEP}/, ${OBJ:.o=.d}}

${NAME}: ${OBJ}
	c++ ${CFLAGS} ${MFLAGS} ${INCLUDE} $^ -o $@ 

all: ${NAME}

${OBJ}: ${DIR_OBJ}/%.o: ${DIR_SRC}/%.cpp
	@mkdir -p ${@D}
	@mkdir -p ${DIR_DEP}
	c++ ${CFLAGS} ${MFLAGS} ${INCLUDE} -c $< -o $@ -MF ${DIR_DEP}/$*.d

-include ${DEP}

debug: CFLAGS += -gdwarf-4
debug: re

clean:
	rm -f ${OBJ} ${DEP}

fclean: clean
	rm -f ${NAME}

re: fclean all

.PHONY: all clean fclean re debug