LIBS=-lpthread -lX11

all: release

release:
	gcc main.c -Os -o dwm-su $(LIBS)

debug:
	gcc main.c -o dwm-su $(LIBS) -g -DDEBUG
