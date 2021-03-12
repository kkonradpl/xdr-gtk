#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "tuner-scan.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

tuner_scan_t*
tuner_scan_parse(gchar *msg)
{
    tuner_scan_t *scan;
    gchar *ptr, *freq, *value;
    gint n = 0;
    gint i;

    if(!msg)
        return NULL;

    for(i=0; i<strlen(msg); i++)
        if(msg[i] == ',')
            n++;

    if(!n)
        return NULL;

    scan = g_new(tuner_scan_t, 1);
    scan->min = G_MAXINT;
    scan->max = G_MININT;
    scan->signals = g_new(tuner_scan_node_t, n);

    i = 0;
    while((ptr = strsep(&msg, ",")) && i < n)
    {
        freq = strsep(&ptr, "=");
        value = strsep(&ptr, "=");
        if(freq && value)
        {
            scan->signals[i].freq = atoi(freq);
            scan->signals[i].signal = g_ascii_strtod(value, NULL);
            if(scan->signals[i].signal > scan->max)
                scan->max = ceil(scan->signals[i].signal);
            if(scan->signals[i].signal < scan->min)
                scan->min = floor(scan->signals[i].signal);
            i++;
        }
    }

    if(!i)
    {
        tuner_scan_free(scan);
        return NULL;
    }

    scan->len = i;
    return scan;
}

tuner_scan_t*
tuner_scan_copy(tuner_scan_t *data)
{
    tuner_scan_t *copy;
    gint i;

    copy = g_new(tuner_scan_t, 1);
    copy->len = data->len;
    copy->max = data->max;
    copy->min = data->min;
    copy->signals = g_new(tuner_scan_node_t, data->len);
    for(i=0;i<data->len;i++)
    {
        copy->signals[i].freq = data->signals[i].freq;
        copy->signals[i].signal = data->signals[i].signal;
    }
    return copy;
}

void
tuner_scan_free(tuner_scan_t *data)
{
    g_free(data->signals);
    g_free(data);
}
