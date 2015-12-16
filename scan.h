#ifndef XDR_SCAN_H_
#define XDR_SCAN_H_

typedef struct scan_node
{
    gint freq;
    gfloat signal;
} scan_node_t;

typedef struct scan_data
{
    scan_node_t* signals;
    gint len;
    gint min;
    gint max;
} scan_data_t;

typedef struct scan
{
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *box_settings;

    GtkWidget *box_buttons;
    GtkWidget *b_start;
    GtkWidget *b_continuous;
    GtkWidget *b_relative;
    GtkWidget *b_peakhold;
    GtkWidget *b_hold;

    GtkWidget *box_from;
    GtkWidget *l_from;
    GtkWidget *s_start;

    GtkWidget *box_to;
    GtkWidget *l_to;
    GtkWidget *s_end;

    GtkWidget *box_step;
    GtkWidget *l_step;
    GtkWidget *s_step;

    GtkWidget *box_bw;
    GtkWidget *l_bw;
    GtkWidget *d_bw;

    GtkWidget *box_presets;
    GtkWidget *b_ccir;
    GtkWidget *b_oirt;
    GtkWidget *b_marks;
    GtkWidget *marks_menu;

    GtkWidget *view;

    gboolean active;
    gboolean locked;
    gboolean motion_tuning;
    gint focus;
    scan_data_t* data;
    scan_data_t* peak;
    scan_data_t* hold;
} scan_t;

scan_t scan;

void scan_init();
void scan_dialog();
GtkWidget* scan_dialog_menu();
void scan_destroy(GtkWidget*, gpointer);
gboolean scan_window_event(GtkWidget*, gpointer);
void scan_toggle(GtkWidget*, gpointer);
void scan_peakhold(GtkWidget*, gpointer);
void scan_hold(GtkWidget*, gpointer);
void scan_lock(gboolean);

gboolean scan_redraw(GtkWidget*, GdkEventExpose*, gpointer);
void scan_draw_spectrum(cairo_t*, scan_data_t*, gint, gint, gdouble, gdouble, gdouble);
void scan_draw_mark(cairo_t*, gint, gint, gint);
const gchar* scan_format_frequency(gint);

gboolean scan_update(gpointer);
void scan_check_finished();

gboolean scan_click(GtkWidget*, GdkEventButton*, gpointer);
gboolean scan_motion(GtkWidget*, GdkEventMotion*, gpointer);
void scan_ccir(GtkWidget*, gpointer);
void scan_oirt(GtkWidget*, gpointer);
gboolean scan_marks(GtkWidget*, GdkEventButton*);
void scan_marks_add(GtkMenuItem*, gpointer);
void scan_marks_clear(GtkMenuItem*, gpointer);

scan_data_t* scan_data_copy(scan_data_t*);
void scan_data_free(scan_data_t*);
#endif
