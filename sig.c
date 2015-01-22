#include <gtk/gtk.h>
#include "sig.h"
#include "tuner.h"
#include "settings.h"

void signal_push(gfloat level, gboolean stereo, gboolean rds)
{
    s.pos = (s.pos + 1)%s.len;
    signal_get()->value = level;
    signal_get()->stereo = stereo;
    signal_get()->rds = rds;

    if(level > s.max)
    {
        s.max = level;
    }
}

void signal_clear()
{
    gint i;
    for(i=0; i<s.len; i++)
    {
        s.data[i].value = -1;
        s.data[i].rds = FALSE;
        s.data[i].stereo = FALSE;
    }
    s.pos = 0;
}

s_data_t* signal_get_prev_i(gint i)
{
    return &s.data[((i==0) ? (s.len-1) : i-1)];
}

s_data_t* signal_get_prev()
{
    return signal_get_prev_i(s.pos);
}

s_data_t* signal_get()
{
    return &s.data[s.pos];
}

s_data_t* signal_get_i(gint i)
{
    return &s.data[i];
}

s_data_t* signal_get_next_i(gint i)
{
    return &s.data[((i==(s.len-1)) ? 0 : i+1)];
}

gfloat signal_level(gfloat value)
{
    if(tuner.mode == MODE_FM)
    {
        switch(conf.signal_unit)
        {
        case UNIT_DBM:
            return value - 120;

        case UNIT_DBUV:
            return value - 11.25;

        default:
        case UNIT_DBF:
            return value;
        }
    }
    return value;
}

