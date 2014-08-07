#ifndef XDR_GRAPH_H_
#define XDR_GRAPH_H_

#define GRAPH_OFFSET    23
#define GRAPH_OFFSET_V   6
#define GRAPH_FONT_SIZE 12

void graph_init();
gboolean graph_draw(GtkWidget*, GdkEventExpose*, gpointer);
void graph_resize();
void graph_click(GtkWidget*, GdkEventButton*, gpointer);
void signal_display();
gfloat signal_level(gfloat);
#endif

