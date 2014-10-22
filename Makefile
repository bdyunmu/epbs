
CC=gcc

COPTS=-O3 -Wall -pthread 

INCLUDE=-I/user/include -I/home/ut/hadoopwork/dowork

LIB=-L/lib/x86_64-linux-gnu/ 

all: epbslib epbssrv epbscli

epbssrv.o:
	${CC} -c epbssrv.c ${COPTS} ${INCLUDE} ${LIB}
libmsg.o:
	${CC} -c libmsg.c ${COPTS} ${INCLUDE} ${LIB}
libhost.o:
	${CC} -c libhost.c ${COPTS} ${INCLUDE} ${LIB}
libjobq.o:
	${CC} -c libjobq.c ${COPTS} ${INCLUDE} ${LIB} 
libuser.o:
	${CC} -c libuser.c ${COPTS} ${INCLUDE} ${LIB}

epbslib: libmsg.o libhost.o epbssrv.o libjobq.o libuser.o
	ar cru epbslib.a libmsg.o libhost.o epbssrv.o libjobq.o libuser.o

epbssrv: 
	${CC} -o epbssrv ${COPTS} ${INCLUDE} ${LIB} ./epbslib.a

epbscli: 
	${CC} -o epbscli epbscli.c ${COPTS} ${INCLUDE} ${LIB} libmsg.o libhost.o libjobq.o libuser.o

clean:
	rm -f *.o *.a epbssrv epbscli
