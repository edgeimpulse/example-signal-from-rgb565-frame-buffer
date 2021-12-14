NAME = edge-impulse-standalone

CC = gcc
CXX = g++
CFLAGS ?= -Wall

MACROS += -DTF_LITE_DISABLE_X86_NEON
CXXFLAGS += -std=c++11
CFLAGS += -I.
CFLAGS += -Isource
CFLAGS += -Iedge-impulse-sdk
CFLAGS += -Imodel-parameters
CFLAGS += -Itflite-model
CFLAGS += -lstdc++
CFLAGS += -g

all: build

.PHONY: build clean

--private-build:
	mkdir -p build
	rm -rf *.gcda
	rm -rf *.gcno
	$(CXX) $(MACROS) $(CXXFLAGS) $(CFLAGS) $(LFLAGS) $(MAIN) edge-impulse-sdk/dsp/image/processing.cpp -o build/$(NAME)
	rm -rf *.gcda
	rm -rf *.gcno

--cutout:
	$(eval MAIN = source/main_using_cutout.cpp)

--squash:
	$(eval MAIN = source/main_using_squash.cpp)

build: --cutout --private-build
cutout: build
squash: --squash --private-build

clean:
	rm $(NAME)
