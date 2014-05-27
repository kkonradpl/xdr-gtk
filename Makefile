CC = gcc
CFLAGS = -Wall -pedantic -std=c99 -c `pkg-config --cflags gtk+-2.0` -D_GNU_SOURCE -O2
LIBS = `pkg-config --libs gtk+-2.0` -lgthread-2.0 -lm

xdr-gtk:			alignment.o clipboard.o connection.o connection_read.o graph.o gui.o gui_net.o keyboard.o main.o menu.o pattern.o rdsspy.o scan.o settings.o sha1.o stationlist.o
					$(CC) -o xdr-gtk alignment.o clipboard.o connection.o connection_read.o graph.o gui.o gui_net.o keyboard.o main.o menu.o pattern.o rdsspy.o scan.o settings.o sha1.o stationlist.o $(LIBS)

alignment.o:		alignment.c menu.h gui.h connection.h alignment.h graph.h
					$(CC) $(CFLAGS) alignment.c

clipboard.o:		clipboard.c gui.h settings.h clipboard.h connection.h
					$(CC) $(CFLAGS) clipboard.c

connection.o:		connection.c gui.h connection.h graph.h settings.h scan.h menu.h pattern.h sha1.h
					$(CC) $(CFLAGS) connection.c

connection_read.o:	connection_read.c gui.h connection.h connection_read.h graph.h settings.h scan.h pattern.h rdsspy.h
					$(CC) $(CFLAGS) connection_read.c
        
graph.o:			graph.c settings.h connection_read.h gui.h graph.h
					$(CC) $(CFLAGS) graph.c

gui.o:				gui.c gui.h connection.h graph.h menu.h settings.h clipboard.h keyboard.h stationlist.h
					$(CC) $(CFLAGS) gui.c

gui_net.o:			gui_net.c gui.h connection.h
					$(CC) $(CFLAGS) gui_net.c

keyboard.o:			keyboard.c gui.h connection.h settings.h
					$(CC) $(CFLAGS) keyboard.c

main.o:				main.c gui.h settings.h graph.h rdsspy.h stationlist.h
					$(CC) $(CFLAGS) main.c

menu.o:				menu.c gui.h menu.h scan.h settings.h connection.h graph.h alignment.h pattern.h rdsspy.h
					$(CC) $(CFLAGS) menu.c

pattern.o:			pattern.c gui.h settings.h pattern.h connection.h menu.h
					$(CC) $(CFLAGS) pattern.c

rdsspy.o:			rdsspy.c rdsspy.h gui.h menu.h settings.h pattern.h connection.h graph.h
					$(CC) $(CFLAGS) rdsspy.c

scan.o:				scan.c gui.h settings.h scan.h connection.h menu.h
					$(CC) $(CFLAGS) scan.c

settings.o:			settings.c gui.h settings.h graph.h menu.h rdsspy.h stationlist.h
					$(CC) $(CFLAGS) settings.c

sha1.o:				sha1.c sha1.h
					$(CC) $(CFLAGS) sha1.c

stationlist.o:		stationlist.c stationlist.h settings.h gui.h connection.h strsep.h
					$(CC) $(CFLAGS) stationlist.c

clean:
					rm -f *.o
