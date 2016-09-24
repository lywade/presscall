CXXFLAGS= -g -Wall
CFLAGS= -g -Wall
INC+= -Iinclude/
LIB+= -lpthread

OBJ= \
	presscall.o \
	TSocket.o \
	cJSON.o \
	errmsg.o \
	mytime.o \
	http.o \
	redis_proto_parser.o \
	hash.o \
	user_func.o \
	buffer.o \
    loadgrid.o \

TARGET=presscall
#############################################################
$(TARGET):$(OBJ)
	g++ $(CXXFLAGS) -o $@ $^ $(LIB) 

%.o: %.cpp
	g++ $(CXXFLAGS) $(INC) -c -o $@ $< 	 

clean:
	rm -f *.o
	rm -f $(TARGET) 
