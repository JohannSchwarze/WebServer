CC=g++


CFLAGS=-I.

libs=-lpthread

outname=httpServer

objfiles=main.o Server.o Logger.o DbgServer.o HttpMsgModel.o HttpServer.o

httpServer: $(objfiles) 
	$(CC) $(CFLAGS) $(objfiles) $(libs) -o $(outname)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< $(libs)




testHttpMsgModel: testHttpMsgModel.cpp HttpMsgModel.o
	$(CC) $(CFLAGS) testHttpMsgModel.cpp HttpMsgModel.o $(libs) -o testHttpMsgModel







all: httpServer testHttpMsgModel

clean:
	rm -rf *.o $(outname) testHttpMsgModel
