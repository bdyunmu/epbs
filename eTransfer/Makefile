TOPDIR=/home/p2plive/epbs/epbs

LIBDIR := -L$(TOPDIR)/util -L$(TOPDIR)/utransfer -L$(TOPDIR)/common

libs := -lutransfer -lutil -lpthread -lcommon

TOPDIR = $(shell cd /home/p2plive/epbs/epbs/test; pwd)

TARGET=libetransfer.a

INCLUDE=-I/home/p2plive/epbs/epbs/utransfer/ -I/home/p2plive/epbs/epbs/common 

CXX=g++

all: $(TARGET)

eTransfer.o:
	g++ -g /home/p2plive/epbs/epbs/eTransfer/eTransfer.cpp ${LIBDIR} ${INCLUDE} $(libs)

libetransfer.a: eTransfer.o
	ar cru eTransfer.o
clean:
	rm -f $(TARGET) $(OBJS) $(TOPDIR)/*.o
