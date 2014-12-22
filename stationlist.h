#ifndef XDR_STATIONLIST_H_
#define XDR_STATIONLIST_H_

#define STATIONLIST_BUFF        1024
#define STATIONLIST_DEBUG       0
#define STATIONLIST_AF_BUFF_LEN 25

typedef struct
{
    gchar* param;
    gchar* value;
} sl_data_t;

gboolean stationlist_is_up();
void stationlist_init();
gpointer stationlist_server(gpointer);
void stationlist_parse(char*);
void stationlist_cmd(gchar*, gchar*);
gboolean stationlist_bw_main(gpointer);

void stationlist_freq(gint);
void stationlist_rcvlevel(gint);
void stationlist_pi(gint);
void stationlist_pty(gint);
void stationlist_ecc(guchar);
void stationlist_ps(gchar*);
void stationlist_rt(gint, gchar*);
void stationlist_bw(gint filter);
void stationlist_af(gint af);
void stationlist_af_clear();

void stationlist_add(gchar*, gchar*);
void stationlist_clear_rds();
gboolean stationlist_send(gchar*);
void stationlist_free(gpointer);
void stationlist_stop();

#endif


