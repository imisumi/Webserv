NAME = Webserv

# CFLAGS = -Wall -Wextra -Werror
CFLAGS += --std=c++20 -Ofast

INCLUDE = -I./src

MFLAGS = -MMD

DIR_OBJ = obj
DIR_SRC = src
DIR_CONFIG = Config
DIR_CGI = Cgi
DIR_CORE = Core
DIR_SERVER = Server
DIR_API = Api

SRC = main.cpp
SRC_CONFIG = ConfigParser.cpp Config.cpp \
	ServerSettings.cpp ConfigInputValidators.cpp \
	ConfigInputConverters.cpp ConfigDirectiveHandlers.cpp \
	ConfigTokenizer.cpp
SRC_CGI = Cgi.cpp
SRC_CORE = 
SRC_SERVER = ConnectionManager.cpp HttpRequestParser.cpp \
	RequestHandler.cpp ResponseGenerator.cpp \
	ResponseSender.cpp Server.cpp \
	NewHttpParser.cpp Client.cpp
SRC_API = Api.cpp

SRC_CONFIG := ${addprefix ${DIR_CONFIG}/, ${SRC_CONFIG}}
SRC_CGI := ${addprefix ${DIR_CGI}/, ${SRC_CGI}}
SRC_CORE := ${addprefix ${DIR_CORE}/, ${SRC_CORE}}
SRC_SERVER := ${addprefix ${DIR_SERVER}/, ${SRC_SERVER}}
SRC_API := ${addprefix ${DIR_API}/, ${SRC_API}}

SRC := ${addprefix ${DIR_SRC}/, ${SRC} ${SRC_CONFIG} ${SRC_CGI} ${SRC_CORE} ${SRC_SERVER} ${SRC_API}}

OBJ = ${subst ${DIR_SRC}/, ${DIR_OBJ}/, ${SRC:.cpp=.o}}

DEP = ${OBJ:.o=.d}

multi:
	@${MAKE} -j 8 all

${NAME}: ${OBJ}
	c++ ${CFLAGS} ${MFLAGS} ${INCLUDE} $^ -o $@ 

all: ${NAME}

${OBJ}: ${DIR_OBJ}/%.o: ${DIR_SRC}/%.cpp
	@mkdir -p ${@D}
	c++ ${CFLAGS} ${MFLAGS} ${INCLUDE} -c $< -o $@

-include ${DEP}

debug: CFLAGS += -gdwarf-4
debug: re


clean:
	rm -fr ${DIR_OBJ}

fclean: clean
	rm -f ${NAME}

re: fclean multi

.PHONY: all multi clean fclean re debug