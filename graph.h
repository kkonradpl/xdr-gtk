#ifndef XDR_GRAPH_H_
#define XDR_GRAPH_H_

#define GRAPH_OFFSET 23
#define GRAPH_HALF_FONT 6

typedef struct
{
    gfloat value;
    gboolean rds;
    gboolean stereo;
} rssi_struct;

rssi_struct *rssi;
gint rssi_len;
gint rssi_pos;

GdkColor graph_color_border;
GdkColor graph_color_scale;
GdkColor graph_color_grid;
GdkColor graph_color_font;

void graph_init();
gboolean graph_draw(GtkWidget*, GdkEventExpose*, gpointer);
void graph_resize();
void graph_click(GtkWidget*, GdkEventButton*, gpointer);
void graph_clear();
void signal_display();
#endif

