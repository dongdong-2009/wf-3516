include ../Makefile.param
LIBDIR= ./lib

CALL_LIBS=-L$(LIBDIR) -lpkdetectcv -lopencv_core -lopencv_highgui -lopencv_imgproc -lzlg7290 -lht1382

SRC  := $(wildcard *.c)
OBJ  := $(SRC:%.c=%.o)
ISP_PATH := $(SDK_PATH)/mpp/component/isp

3A_INC  := $(ISP_PATH)/3a/include
VREG_INC := $(ISP_PATH)/firmware/vreg

#CFLAGS += -D__LINUX__ -Wall  -I../include  -DIV_DEBUG
CFLAGS +=  -I../include
LDFLAGS += -L$(LIBDIR) 

ifndef RELEASE
	CFLAGS += -D_DEBUG -g
else
	CFLAGS += -O2
endif

MPI_LIBS := $(REL_LIB)/libmpi.a
IVE_LIBS := $(REL_LIB)/libive.a

TARGET = ipc_venc

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	
.PHONY: clean

$(TARGET): %: $(OBJ) $(COMM_OBJ)

	$(C++) $(CFLAGS) -o $@ $^ $(MPI_LIBS) $(IVE_LIBS) $(AUDIO_LIBA) -I$(3A_INC) -I$(VREG_INC) -I$(VREG_INC)/arch/$(HIARCH) $(SENSOR_LIBS) -lpthread -lm -lrt $(CALL_LIBS) 

	
#$(STRIP) $(TARGET)
	rm -f ~/hi/Hi3516A_SDK_V1.0.6.0/mpp/test/app/$(TARGET)
	rm -f /home/lcp/code/fileSystem/nfsroot/app/$(TARGET)
	cp $(TARGET) ~/hi/Hi3516A_SDK_V1.0.6.0/mpp/test/app
	cp $(TARGET) /home/lcp/code/fileSystem/nfsroot/app
clean:
	rm -f *.o $(TARGET)
	@rm -f $(COMM_OBJ)

