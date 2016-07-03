CC = cc
SRC = src/main.c

NAME = vkgrab
PREFIX = /usr/local

CFLAGS = -O2 -Wall --std=c99 -s
# LDFLAGS = -ljansson -lcurl
LDFLAGS := $(shell pkg-config --libs jansson libcurl)

${NAME}:
	${CC} ${SRC} ${CFLAGS} ${LDFLAGS} -o ${NAME}

clean:
	rm -f ${NAME}

install:
	cp -i ${NAME} ${PREFIX}/bin

uninstall:
	rm -i ${PREFIX}/bin/${NAME}
