#ifndef XDR_STATIONLIST_H_
#define XDR_STATIONLIST_H_

#define STATIONLIST_BUFF 1024
#define STATIONLIST_DEBUG 1

void stationlist_init();
gpointer stationlist_server(gpointer);
void stationlist_parse(char*);
void stationlist_cmd(gchar*, gchar*);
void stationlist_freq(gint);
void stationlist_stop();

#endif


