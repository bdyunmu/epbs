CC=g++

COPTS=-O3 -Wall -pthread 

INCLUDE=-I/user/include -I./utransfer -I./common

LIB=-L/lib/x86_64-linux-gnu/  -L./utransfer -L./common

libs = -lutransfer -lepbslib -lcommon

all: epbslib epbssrv epbscli

epbssrv.o:
	g++ -c epbssrv.c ${COPTS} ${INCLUDE} ${LIB}
libmsg.o:
	g++ -c libmsg.c ${COPTS} ${INCLUDE} ${LIB} -lutransfer
libhost.o:
	g++ -c libhost.c ${COPTS} ${INCLUDE} ${LIB}
libjobq.o:
	g++ -c libjobq.c ${COPTS} ${INCLUDE} ${LIB} 
libuser.o:
	g++ -c libuser.c ${COPTS} ${INCLUDE} ${LIB}

epbslib: libmsg.o libhost.o epbssrv.o libjobq.o libuser.o
	ar cru epbslib.a libhost.o epbssrv.o libjobq.o libuser.o libmsg.o

epbssrv: 
	g++ -o epbssrv ${COPTS} ${INCLUDE} ${LIB} ./epbslib.a -lutransfer -lcommon

epbscli: 
	g++ -o epbscli epbscli.c ${COPTS} ${INCLUDE} ${LIB} libmsg.o libhost.o libjobq.o libuser.o -lutransfer -lcommon

#include test/Makefile

clean:
	rm -f *.o epbslib.a epbssrv epbscli
