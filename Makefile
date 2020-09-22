
CC = gcc
CPPFLAGS = -g -O2 -I/usr/local/include/ncnn -I/usr/local/include/opencv4
CPP = gcc -E
LINK = $(CC)
LIBS = -L$(LIB)
LIB = /usr/local/lib/ncnn

OBJS_YOLOV4 = yolov4.o
OBJS_VIDEO = video.o MJPEGWriter.o

PROGS = yolov4 video

yolov4: ${OBJS_YOLOV4}
	$(LINK) -o yolov4 ${OBJS_YOLOV4} ${LIBS} -lpthread -lm -lncnn -lstdc++ -fopenmp -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs

video: ${OBJS_VIDEO}
	$(LINK) -o video ${OBJS_VIDEO} ${LIBS} -lpthread -lm -lncnn -lstdc++ -fopenmp -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio

all: ${PROGS}

clean:
	rm -f *.o ${PROGS}

