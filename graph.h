#ifndef XDR_GRAPH_H_
#define XDR_GRAPH_H_

#define GRAPH_OFFSET    23
#define GRAPH_FONT_SIZE 12
#define GRAPH_OFFSET_V (GRAPH_FONT_SIZE/2)

void graph_init();
gboolean graph_draw(GtkWidget*, GdkEventExpose*, gpointer);
void graph_resize();
gboolean graph_click(GtkWidget*, GdkEventButton*, gpointer);
void signal_display();
#endif

