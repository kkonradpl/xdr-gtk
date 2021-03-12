#ifndef XDR_TUNER_SCAN_H_
#define XDR_TUNER_SCAN_H_

typedef struct tuner_scan_node
{
    gint freq;
    gfloat signal;
} tuner_scan_node_t;

typedef struct tuner_scan
{
    tuner_scan_node_t* signals;
    gint len;
    gint min;
    gint max;
} tuner_scan_t;

tuner_scan_t* tuner_scan_parse(gchar*);
tuner_scan_t* tuner_scan_copy(tuner_scan_t*);
void tuner_scan_free(tuner_scan_t*);

#endif
