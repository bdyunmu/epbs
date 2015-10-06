TOPDIR=/home/p2plive/epbs/epbs

LIBDIR := -L$(TOPDIR)/util -L$(TOPDIR)/etransfer -L$(TOPDIR)/common

libs :=

WORKDIR = $(shell cd /home/p2plive/epbs/epbs/test; pwd)

TARGET=libetransfer.a server client

INCLUDE= -I/home/p2plive/epbs/epbs/etransfer/ -I/home/p2plive/epbs/epbs/common 

CXX=g++

all: $(TARGET)

eTransfer.o:
	g++ -c ./eTransfer.cpp -lrt -Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed ${INCLUDE} 

utcp_socket.o:
	g++ -c ./utcp_socket.cpp -lrt -Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed ${INCLUDE}

NetAux.o:
	g++ -c ./NetAux.cpp -lrt -Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed ${INCLUDE}

SockTcp.o:
	g++ -c ./SockTcp.cpp -lrt -Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed ${INCLUDE}

SockBase.o:
	g++ -c ./SockBase.cpp -lrt -Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed ${INCLUDE}

etransfer_server.o:
	g++ -c ./etransfer_server.cpp ${INCLUDE}

libetransfer.a: eTransfer.o utcp_socket.o NetAux.o SockTcp.o SockBase.o
	ar rs libetransfer.a eTransfer.o utcp_socket.o NetAux.o SockTcp.o SockBase.o
#	ar cru libetransfer.a eTransfer.o utcp_socket.o 
server: 
	g++ -o server ./etransfer_server.cpp ${LIBDIR} ${INCLUDE} $(libs) -letransfer -lcommon -lpthread

client:
	g++ -o client ./etransfer_client.cpp ${LIBDIR} ${INCLUDE} $(libs) -letransfer -lcommon	

clean:
	rm -f $(TARGET) $(OBJS) ./*.o
