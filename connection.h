#ifndef XDR_CONNECTION_H
#define XDR_CONNECTION_H
#ifdef G_OS_WIN32
#include <windows.h>
#endif

#define DEBUG_READ 0
#define DEBUG_WRITE 0
#define SERIAL_BUFFER 10000

#define MODE_FM 0
#define MODE_AM 1

typedef struct
{
#ifdef G_OS_WIN32
    HANDLE serial;
    SOCKET socket;
#else
    gint serial;
#endif

    gint mode;
    gint freq;
    gint prevfreq;
    guchar daa;

    gint pi;
    gint prevpi;
    gint prevpty;
    gint prevtp;
    gint prevta;
    gint prevms;

    gchar ps[9];
    guchar ps_err[8];
    gboolean ps_avail;
    gchar rt[2][65];

    gint rds_timer;
    gint64 rds_reset_timer;
    gfloat max_signal;
    gint online;
    gboolean stereo;
    gboolean rds;
}  tuner_struct;

tuner_struct tuner;

extern volatile gboolean thread;
extern volatile gboolean ready;
extern short filters[];
extern short filters_n;

void connection_dialog();
gboolean connection();
void connection_cancel(GtkWidget*, gpointer);
gpointer read_thread(gpointer);
void read_parse(gchar, gchar[]);
int open_serial(const gchar*);
int open_socket(const gchar*, guint, const gchar*);
void xdr_write(gchar*);
void tune(gint);
#define tune_r(f) tune(((f)/100)*100 + (((f)%100<50)?0:100))

#endif
