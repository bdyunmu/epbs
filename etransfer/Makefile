TOPDIR=/home/p2plive/epbs/epbs

LIBDIR := -L$(TOPDIR)/util -L$(TOPDIR)/utransfer -L$(TOPDIR)/common -L$(TOPDIR)/etransfer

libs := -lutransfer -lutil -lpthread -lcommon -letransfer

WORKDIR = $(shell cd /home/p2plive/epbs/epbs/test; pwd)

TARGET=libetransfer.a server

INCLUDE=-I/home/p2plive/epbs/epbs/utransfer/ -I/home/p2plive/epbs/epbs/common 

CXX=g++

all: $(TARGET)

eTransfer.o:
	g++ -c ./eTransfer.cpp ${LIBDIR} ${INCLUDE} $(libs)

NetAux.o:
	g++ -c ./NetAux.cpp ${LIBDIR} ${INCLUDE} ${libs}

SockBase.o:
	g++ -c ./SockBase.cpp ${LIBDIR} ${INCLUDE} ${libs}

SockTcp.o:
	g++ -c ./SockTcp.cpp ${LIBDIR} ${INCLUDE} ${libs}

utcp_socket.o:
	g++ -c ./utcp_socket.cpp ${LIBDIR} ${INCLUDE} ${libs}

server:
	g++ -o server ./etransfer_server ${LIBDIR} ${INCLUDE} ${libs} -letransfer

libetransfer.a: eTransfer.o NetAux.o SockBase.o SockTcp.o utcp_socket.o
	ar cru libetransfer.a eTransfer.o NetAux.o SockBase.o SockTcp.o utcp_socket.o

clean:
	rm -f $(TARGET) $(OBJS) ./*.o
