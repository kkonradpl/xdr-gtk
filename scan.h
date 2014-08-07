#ifndef XDR_SCAN_H_
#define XDR_SCAN_H_

typedef struct scan_node
{
    gint freq;
    gfloat signal;
} scan_node_t;

typedef struct scan_data
{
    gint len;
    gfloat max;
    gfloat min;
    scan_node_t* signals;
    scan_node_t* peak;
} scan_data_t;

typedef struct scan
{
    gboolean window;
    gboolean active;
    scan_data_t* data;
    GtkWidget* dialog;
    GtkWidget* b_start;
    GtkWidget* b_continuous;
    GtkWidget* b_tune;
    GtkWidget* b_relative;

    GtkWidget* b_ccir;
    GtkWidget* b_oirt;
    GtkWidget* l_frange;
    GtkWidget* e_fstart;
    GtkWidget* l_frange_;
    GtkWidget* e_fstop;
    GtkWidget* l_fstop_kHz;

    GtkWidget* l_fstep;
    GtkWidget* s_fstep;
    GtkWidget* l_fstep_kHz;

    GtkWidget* l_bw;
    GtkWidget* d_bw;

    GtkWidget* image;
    GtkWidget* label;

    gint focus;
} scan_t;

scan_t scan;

void scan_dialog();
void scan_toggle(GtkWidget*, scan_t*);
void scan_continuous(GtkWidget*, scan_t*);
void scan_tune(GtkWidget*, scan_t*);
void scan_relative(GtkWidget*);
void scan_destroy(GtkWidget*, gpointer);
gboolean scan_redraw(GtkWidget*, GdkEventExpose*, scan_t*);
void scan_init(scan_t*);
gboolean scan_update(gpointer);
gboolean scan_click(GtkWidget*, GdkEventButton*, scan_t*);
gboolean scan_motion(GtkWidget*, GdkEventMotion*, scan_t*);
void scan_resize(GtkWidget*, gpointer);
void scan_ccir(GtkWidget*, gpointer);
void scan_oirt(GtkWidget*, gpointer);

#endif
