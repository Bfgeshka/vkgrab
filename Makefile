CC = cc
SRC = src/main.c

NAME = vkgrab

CFLAGS = -g -O2 -Wall
LDFLAGS = -ljansson -lcurl

${NAME}:
	${CC} ${SRC} ${CFLAGS} ${LDFLAGS} -o ${NAME}

clean:
	rm -f ${NAME}
