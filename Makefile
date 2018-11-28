CC = cc

SRC = src/main.c src/methods.c src/curl_req.c src/utils.c
SRC_SND = dirty_tools/${NAME_SND}/main.c dirty_tools/${NAME_SND}/functions.c src/methods.c src/curl_req.c src/utils.c
OBJS = $(SRC:.c=.o)

NAME = vkgrab
NAME_SND = blacklist_finder
PREFIX = /usr/local

CFLAGS = -O2 -Wall -Wextra -Wpedantic --std=c99 -D_DEFAULT_SOURCE
LDFLAGS := $(shell pkg-config --libs jansson libcurl)

all: clean options ${NAME}

options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

${NAME}:
	${CC} ${SRC} ${CFLAGS} ${LDFLAGS} -o ${NAME}

${NAME_SND}: options clean
	${CC} ${SRC_SND} ${CFLAGS} ${LDFLAGS} -o ${NAME_SND}
	mv ${NAME_SND} dirty_tools/${NAME_SND}/

clean:
	rm -f ${NAME}
	rm -f ${NAME_SND}
	rm -f ./*.o

install:
	cp -i ${NAME} ${PREFIX}/bin

uninstall:
	rm -i ${PREFIX}/bin/${NAME}

$(OBJ_DIR):
	mkdir -p $@

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

.PHONY: all install uninstall clean
