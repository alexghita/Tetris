LIBDIRS= -L/usr/X11R6/lib
LDLIBS = -lglut -lGL -lGLU -lX11 -lm 

CPPFLAGS= -O3 
LDFLAGS= $(CPPFLAGS) $(LIBDIRS)

TARGETS = coursework

SRCS = coursework.cpp

OBJS =  $(SRCS:.cpp=.o)

CXX = g++

default: $(TARGETS)
