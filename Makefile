CC = cc
SRC = src/main.c

NAME = vkgrab
PREFIX = /usr/local

CFLAGS = -O2 -Wall --std=c99
LDFLAGS = -ljansson -lcurl

${NAME}:
	${CC} ${SRC} ${CFLAGS} ${LDFLAGS} -o ${NAME}

clean:
	rm -f ${NAME}

install:
	sudo cp -i ${NAME} ${PREFIX}/bin

uninstall:
	sudo rm -i ${PREFIX}/bin/${NAME}
