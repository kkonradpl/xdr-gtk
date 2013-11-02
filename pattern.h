#ifndef XDR_PATTERN_H_
#define XDR_PATTERN_H_

#define PATTERN_OFFSET 25
#define PATTERN_ARCS 36
#define DEG2RAD(DEG) ((DEG)*((M_PI)/(180.0)))

typedef struct pattern_rssi
{
    gfloat sig;
    struct pattern_rssi *next;
} pattern_rssi;

typedef struct
{
    volatile gboolean active;

    gint freq;
    gint count;
    gfloat peak;
    gint peak_i;
    gint rotate_i;

    GtkWidget* dialog;
    GtkWidget* b_start;
    GtkWidget* b_load;
    GtkWidget* b_save;
    GtkWidget* b_save_img;
    GtkWidget* b_close;

    GtkWidget* l_size;
    GtkWidget* s_size;
    GtkWidget* l_title;
    GtkWidget* e_title;

    GtkWidget* image;

    GtkWidget* b_rotate_ll;
    GtkWidget* b_rotate_l;
    GtkWidget* b_rotate_peak;
    GtkWidget* b_rotate_r;
    GtkWidget* b_rotate_rr;
    GtkWidget* fill;
    GtkWidget* avg;

    pattern_rssi *head;
    pattern_rssi *tail;
} pattern_struct;

pattern_struct pattern;

void pattern_dialog();
gboolean gui_pattern(gpointer);
void gui_pattern_destroy(gpointer);
gboolean draw_pattern(GtkWidget*, GdkEventExpose*, gpointer);

void pattern_init(gint);
void pattern_push(gfloat);
void pattern_start(GtkWidget*, gpointer);
void pattern_load(GtkWidget*, gpointer);
void pattern_save(GtkWidget*, gpointer);
void pattern_save_img(GtkWidget*, gpointer);
void pattern_resize();
gboolean pattern_update();
void pattern_clear();
gfloat pattern_log_signal(gfloat);
void pattern_rotate(GtkWidget*, gpointer);

#endif
