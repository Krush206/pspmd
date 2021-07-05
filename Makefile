TARGET := pspmd
INSTALL_PATH := /usr/src/busybox/_install/usr/bin

OBJS = pspmdmain.o pspmd.o pspmdstates.o

CC := mipsel-linux-gcc
CXX := mipsel-linux-g++
CFLAGS = -fno-jump-tables
CXXFLAGS = -fno-jump-tables
MAPFLAGS = -Wl,-Map -Wl,$(TARGET).map
LDFLAGS = -static -elf2flt

.PHONY: all
all: $(TARGET)
	@echo "*** Done ***"

.PHONY: install
install: $(TARGET)
	cp $(TARGET) $(INSTALL_PATH)/$(TARGET)
	@echo "*** Done ***"

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


# Dependencies
pspmd.o: pspmd.cpp pspmd.h pspmdstates.h
pspmdmain.o: pspmdmain.cpp pspmd.h pspmdstates.h
pspmdstates.o: pspmdstates.cpp pspmd.h pspmdstates.h


.PHONY: clean
clean:
	rm -f $(TARGET) $(TARGET).map *.o *.gdb
