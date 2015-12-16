#ifndef XDR_SIGNAL_H_
#define XDR_SIGNAL_H_

typedef struct s_data
{
    gfloat value;
    gboolean rds;
    gboolean stereo;
} s_data_t;

typedef struct s
{
    s_data_t *data;
    gint len;
    gint pos;
    gfloat max;
    gboolean stereo;
    gboolean rds;
    gint64 rds_reset_timer;
    gdouble sum;
    guint samples;
} s_t;

s_t s;

void signal_push(gfloat level, gboolean stereo, gboolean rds);
void signal_separator();
void signal_clear();
s_data_t* signal_get_prev_i(gint);
s_data_t* signal_get_prev();
s_data_t* signal_get();
s_data_t* signal_get_i(gint);
s_data_t* signal_get_next_i(gint);
gfloat signal_level(gfloat value);
const gchar* signal_level_unit();

#endif
