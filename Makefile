CC=g++

#CPLUS_INCLUDE_PATH=/new/include/dir
#export CPLUS_INCLUDE_PAATH

COPTS=-O3 -Wall -pthread

INCLUDE=-I/user/include -I./utransfer -I./common -I./etransfer

LIB=-L/lib/x86_64-linux-gnu/ -L./utransfer -L./common -L./etransfer

libs = -pthread

all: main epbscli epbssrv

epbssrv.o:
	g++ -c epbssrv.c ${COPTS} ${INCLUDE} ${LIB} -lcommon -letransfer
libmsg.o:
	g++ -c libmsg.c ${COPTS} ${INCLUDE} ${LIB} -lcommon -letransfer
libhost.o:
	g++ -c libhost.c ${COPTS} ${INCLUDE} ${LIB} -lcommon -letransfer
libjobq.o:
	g++ -c libjobq.c ${COPTS} ${INCLUDE} ${LIB} -lcommon -letransfer
libuser.o:
	g++ -c libuser.c ${COPTS} ${INCLUDE} ${LIB} -lcommon -letransfer
host_monitor.o:
	g++ -c host_monitor.c ${COPTS} ${INCLUDE} ${LIB} -lcommon -letransfer
eTransfer.o:
	g++ -c eTransfer.cpp ${INCLUDE} ${LIB} -lcommon -letransfer
utcp_socket.o:
	g++ -c utcp_socket.cpp ${INCLUDE} ${LIB} -lcommon -letransfer
NetAux.o:
	g++ -c NetAux.cpp ${INCLUDE} ${LIB} -lcommon -letransfer
SockTcp.o:
	g++ -c SockTcp.cpp ${INCLUDE} ${LIB} -lcommon -letransfer

epbslib: libmsg.o libhost.o epbssrv.o libjobq.o libuser.o host_monitor.o
	ar cru epbslib.a libhost.o epbssrv.o libjobq.o libuser.o libmsg.o host_monitor.o
	
epbssrv: libhost.o libjobq.o libuser.o libmsg.o host_monitor.o eTransfer.o utcp_socket.o NetAux.o SockTcp.o SockBase.o
#	g++ -o epbssrv ${COPTS} ${INCLUDE} ${LIB} ./epbslib.a -lcommon -letransfer
	g++ -o epbssrv epbssrv.c ${INCLUDE} ${LIB} $(libs) libhost.o libjobq.o libuser.o libmsg.o host_monitor.o eTransfer.o utcp_socket.o NetAux.o SockTcp.o SockBase.o -letransfer -lcommon
epbscli: libhost.o libuser.o eTransfer.o utcp_socket.o NetAux.o SockTcp.o SockBase.o
#	g++ -o epbscli epbscli.c ${COPTS} ${INCLUDE} ${LIB} libmsg.o libhost.o libjobq.o libuser.o -lutransfer -lcommon
	g++ -o epbscli epbscli.c ${INCLUDE} ${LIB} $(libs) libhost.o libuser.o eTransfer.o utcp_socket.o NetAux.o SockTcp.o SockBase.o -lcommon -letransfer
#include test/Makefile

main:
	g++ -o main ./etransfer_server.cpp ${INCLUDE} ${LIB} $(libs) -letransfer -lcommon

clean:
	rm -f *.o epbssrv epbscli main
