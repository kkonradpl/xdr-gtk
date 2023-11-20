#ifndef XDR_STATIONLIST_H_
#define XDR_STATIONLIST_H_

typedef struct
{
    gchar* param;
    gchar* value;
} sl_data_t;

void stationlist_init();
gboolean stationlist_is_up();
void stationlist_stop();

void stationlist_freq(gint);
void stationlist_rcvlevel(gint);
void stationlist_pi(gint);
void stationlist_pty(gint);
void stationlist_ecc(guchar);
void stationlist_ps(const gchar*);
void stationlist_rt(gint, const gchar*);
void stationlist_bw(gint);
void stationlist_af(gint);
void stationlist_af_clear();

#endif


