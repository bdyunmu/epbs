CC=g++

#CPLUS_INCLUDE_PATH=/new/include/dir
#export CPLUS_INCLUDE_PATH

COPTS=-O3 -Wall -pthread

INCLUDE=-I/user/include -I./utransfer -I./common -I./etransfer

LIB=-L/lib/x86_64-linux-gnu/ -L./utransfer -L./common -L./etransfer -L./

libs = -pthread

all: epbscli 

epbssrv.o:
	g++ -c epbssrv.c ${COPTS} ${INCLUDE} ${LIB} -lcommon
libmsg.o:
	g++ -c libmsg.c ${COPTS} ${INCLUDE} ${LIB} -lcommon
libhost.o:
	g++ -c libhost.c ${COPTS} ${INCLUDE} ${LIB} -lcommon
libjobq.o:
	g++ -c libjobq.c ${COPTS} ${INCLUDE} ${LIB} -lcommon
libuser.o:
	g++ -c libuser.c ${COPTS} ${INCLUDE} ${LIB} -lcommon
host_monitor.o:
	g++ -c host_monitor.c ${COPTS} ${INCLUDE} ${LIB} -lcommon
eTransfer.o: NetAux.o SockTcp.o utcp_socket.o
	g++ -c eTransfer.cpp ${INCLUDE} ${LIB} NetAux.o SockTcp.o utcp_socket.o -lcommon
utcp_socket.o:
	g++ -c utcp_socket.cpp ${INCLUDE} ${LIB} -lcommon
NetAux.o:
	g++ -c NetAux.cpp ${INCLUDE} ${LIB} -lcommon
SockTcp.o:
	g++ -c SockTcp.cpp ${INCLUDE} ${LIB} -lcommon

libepbs: utcp_socket.o NetAux.o SockTcp.o eTransfer.o SockBase.o
	ar rs libepbs.a utcp_socket.o NetAux.o SockTcp.o eTransfer.o SockBase.o
	
epbssrv: libhost.o libjobq.o libuser.o libmsg.o host_monitor.o eTransfer.o utcp_socket.o NetAux.o SockTcp.o SockBase.o
#	g++ -o epbssrv ${COPTS} ${INCLUDE} ${LIB} ./epbslib.a -lcommon -letransfer
	g++ -o epbssrv epbssrv.c ${INCLUDE} ${LIB} $(libs) libhost.o libjobq.o libuser.o libmsg.o host_monitor.o eTransfer.o utcp_socket.o NetAux.o SockTcp.o SockBase.o -letransfer -lcommon
epbscli: libhost.o libuser.o libepbs
#	g++ -o epbscli epbscli.c ${COPTS} ${INCLUDE} ${LIB} libmsg.o libhost.o libjobq.o libuser.o -lutransfer -lcommon
	g++ -o epbscli epbscli.c ${INCLUDE} ${LIB} $(libs) libhost.o libuser.o -L./ ./libepbs.a -lcommon
#include test/Makefile

main:
	g++ -o main ./etransfer_server.cpp ${INCLUDE} ${LIB} $(libs) -letransfer -lcommon

clean:
	rm -f *.o epbssrv epbscli main
