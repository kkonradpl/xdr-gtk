#include <gtk/gtk.h>
#include <math.h>
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

    s.sum += level;
    s.samples++;
}

void signal_separator()
{
    if(!isnan(signal_get()->value) && signal_get()->value < 0)
        return;
    s.pos = (s.pos + 1)%s.len;
    signal_get()->value = -1;
    signal_get()->stereo = FALSE;
    signal_get()->rds = FALSE;
}

void signal_clear()
{
    gint i;
    for(i=0; i<s.len; i++)
    {
        s.data[i].value = NAN;
        s.data[i].rds = FALSE;
        s.data[i].stereo = FALSE;
    }
    s.pos = 0;
    s.sum = 0;
    s.samples = 0;
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
            return value - 120 + conf.signal_offset;

        case UNIT_DBUV:
            return value - 11.25 + conf.signal_offset;

        default:
        case UNIT_DBF:
            return value + conf.signal_offset;
        }
    }
    return value;
}

const gchar* signal_level_unit()
{
    static const gchar dBuV[] = "dBµV";
    static const gchar dBf[] = "dBf";
    static const gchar dBm[] = "dBm";
    static const gchar unknown[] = "";

    if(tuner.mode == MODE_FM)
    {
        switch(conf.signal_unit)
        {
        case UNIT_DBM:
            return dBm;

        case UNIT_DBUV:
            return dBuV;

        default:
        case UNIT_DBF:
            return dBf;
        }
    }
    return unknown;
}
