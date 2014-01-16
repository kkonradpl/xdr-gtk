#ifndef XDR_CONNECTION_H
#define XDR_CONNECTION_H

#ifdef G_OS_WIN32
#include <windows.h>
HANDLE serial;
extern SOCKET serial_socket;
#else
gint serial;
#endif

#define DEBUG_READ 0
#define DEBUG_WRITE 0
#define SERIAL_BUFFER 10000

extern gint freq, prevfreq;
extern gchar daa;
extern gint prevpi, pi;
extern gchar ps_data[9], rt_data[2][65];
extern gboolean ps_available;
extern short prevpty, prevtp, prevta, prevms;
extern gint rds_timer;
extern gfloat max_signal;
extern gint online;
gboolean stereo, rds;

extern volatile gboolean thread;
extern volatile gboolean ready;

extern short filters[];
extern short filters_n;

void connection_dialog();
gboolean connection();
void connection_cancel(GtkWidget*, gpointer);
gpointer read_thread(gpointer);
void read_parse(gchar, gchar[]);
int open_serial(gchar*);
int open_socket(const gchar*, guint);
void xdr_write(gchar*);
void tune(gint);
#define tune_r(f) tune(((f)/100)*100 + (((f)%100<50)?0:100))

#endif
