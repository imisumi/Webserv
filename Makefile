NAME = Webserv

# CFLAGS = -Wall -Wextra -Werror
CFLAGS += --std=c++17

LIB = -Lbin/Linux -lspdlog
INCLUDE = -I./src -I./dep/spdlog/include

MFLAGS = -MMD

DIR_OBJ = obj
DIR_SRC = src
DIR_CONFIG = Config
DIR_CGI = Cgi
DIR_CORE = Core
DIR_SERVER = Server

SRC = main.cpp
SRC_CONFIG = ConfigParser.cpp Config.cpp \
	ServerSettings.cpp ConfigInputValidators.cpp \
	ConfigInputConverters.cpp ConfigDirectiveHandlers.cpp \
	ConfigTokenizer.cpp
SRC_CGI = Cgi.cpp
SRC_CORE = Log.cpp
SRC_SERVER = ConnectionManager.cpp HttpRequestParser.cpp \
	RequestHandler.cpp ResponseGenerator.cpp \
	ResponseSender.cpp Server.cpp

SRC_CONFIG := ${addprefix ${DIR_CONFIG}/, ${SRC_CONFIG}}
SRC_CGI := ${addprefix ${DIR_CGI}/, ${SRC_CGI}}
SRC_CORE := ${addprefix ${DIR_CORE}/, ${SRC_CORE}}
SRC_SERVER := ${addprefix ${DIR_SERVER}/, ${SRC_SERVER}}

SRC := ${addprefix ${DIR_SRC}/, ${SRC} ${SRC_CONFIG} ${SRC_CGI} ${SRC_CORE} ${SRC_SERVER}}

OBJ = ${subst ${DIR_SRC}/, ${DIR_OBJ}/, ${SRC:.cpp=.o}}

DEP = ${OBJ:.o=.d}}

multi:
	@${MAKE} -j 8 all

${NAME}: ${OBJ}
	c++ ${CFLAGS} ${MFLAGS} ${INCLUDE} $^ ${LIB} -o $@ 

all: ${NAME}

${OBJ}: ${DIR_OBJ}/%.o: ${DIR_SRC}/%.cpp
	@mkdir -p ${@D}
	c++ ${CFLAGS} ${MFLAGS} ${INCLUDE} -c $< -o $@

-include ${DEP}

debug: CFLAGS += -gdwarf-4
debug: LIB := -Lbin/Linux -lspdlogd
debug: re


clean:
	rm -f ${OBJ} ${DEP}

fclean: clean
	rm -f ${NAME}

re: fclean all

.PHONY: all clean fclean re debug