#ifndef XDR_SCAN_H_
#define XDR_SCAN_H_

typedef struct
{
    gint freq;
    gint signal;
} scan_node;

typedef struct
{
    gint len;
    gint max;
    gint min;
    scan_node* signals;
    scan_node* peak;
} scan_data;

typedef struct
{
    gboolean window;
    gboolean active;
    scan_data* data;
    GtkWidget* dialog;
    GtkWidget* b_start;
    GtkWidget* b_continuous;
    GtkWidget* b_tune;
    GtkWidget* b_relative;
    GtkWidget* b_close;

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
} scan_struct;

scan_struct scan;

void scan_dialog();
void scan_toggle(GtkWidget*, scan_struct*);
void scan_continuous(GtkWidget*, scan_struct*);
void scan_tune(GtkWidget*, scan_struct*);
void scan_relative(GtkWidget*);
void scan_destroy(GtkWidget*, gpointer);
gboolean scan_redraw(GtkWidget*, GdkEventExpose*, scan_struct*);
void scan_init(scan_struct*);
gboolean scan_update(gpointer);
gboolean scan_click(GtkWidget*, GdkEventButton*, scan_struct*);
gboolean scan_motion(GtkWidget*, GdkEventMotion*, scan_struct*);
void scan_resize(GtkWidget*, gpointer);
#endif
