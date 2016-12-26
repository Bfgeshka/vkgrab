CC = cc

SRC = src/main.c src/methods.c src/curl_req.c
OBJS = $(SRC:.c=.o)

NAME = vkgrab
PREFIX = /usr/local

CFLAGS = -Os -Wall -Wextra -Wpedantic --std=c99 -D_DEFAULT_SOURCE
LDFLAGS := $(shell pkg-config --libs jansson libcurl)

all: clean options ${NAME}

options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

${NAME}:
	${CC} ${SRC} ${CFLAGS} ${LDFLAGS} -o ${NAME}

clean:
	rm -f ${NAME}
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
