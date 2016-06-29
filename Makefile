CC = cc
SRC = src/main.c

NAME = vk_grabber

CFLAGS = -g -O2 -Wall
LDFLAGS = -ljansson -lcurl

vk_grabber:
	${CC} ${SRC} ${CFLAGS} ${LDFLAGS} -o ${NAME}

clean:
	rm -f ${NAME}
