#ifndef XDR_UI_H_
#define XDR_UI_H_
#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#define PATH_SEP "\\"
#define LOG_NL "\r\n"
#else
#define PATH_SEP "/"
#define LOG_NL "\n"
#endif

#define UI_ALPHA_INSENSITIVE "25"

#define AF_LIST_STORE_ID   0
#define AF_LIST_STORE_FREQ 1

#define UI_COLOR_STEREO      "#EE4000"

typedef struct ui
{
    GtkWidget *window;
    gchar window_title[100];
    GdkCursor *click_cursor;
    GtkListStore *af_model;

    GtkWidget *frame;
    GtkWidget *margin;
    GtkWidget *box;
    GtkWidget *box_header;
    GtkWidget *box_ui;
    GtkWidget *box_left;
    GtkWidget *box_left_tune;
    GtkWidget *box_left_interference;
    GtkWidget *box_left_signal;
    GtkWidget *box_left_indicators;
    GtkWidget *box_left_settings1;
    GtkWidget *box_buttons;
    GtkWidget *box_left_settings2;
    GtkWidget *box_rotator;

    GtkWidget *volume;
    GtkWidget *squelch;
    GtkWidget *event_band, *l_band;
    GtkWidget *event_freq, *l_freq;
    GtkWidget *event_pi, *l_pi;
    GtkWidget *event_ps, *l_ps;

    GtkWidget *b_scan;
    GtkWidget *b_tune_back, *b_tune_back_label;
    GtkWidget *e_freq;
    GtkWidget *b_tune_reset;

    GtkAdjustment *adj_align;
    GtkWidget *hs_align;

    GtkWidget *p_cci, *p_aci;
    GtkWidget *l_cci, *l_aci;

    GtkWidget *graph, *p_signal;

    GtkWidget *event_st, *l_st;
    GtkWidget *l_rds;
    GtkWidget *l_tp;
    GtkWidget *l_ta;
    GtkWidget *l_ms;
    GtkWidget *l_pty;
    GtkWidget *event_sig, *l_sig;

    GtkWidget *b_connect;
    GtkWidget *b_pattern;
    GtkWidget *b_settings;
    GtkWidget *b_scheduler;
    GtkWidget *b_rdsspy;
    GtkWidget *b_ontop;

    GtkWidget *c_ant;
    GtkListStore *ant;

    GtkWidget *b_cw, *b_cw_label;
    GtkWidget *b_ccw, *b_ccw_label;
    GtkWidget *l_agc, *c_agc;
    GtkWidget *x_rf;

    GtkWidget *l_deemph, *c_deemph;
    GtkWidget *l_bw, *c_bw;
    GtkWidget *x_if;

    GtkWidget *event_rt[2], *l_rt[2];

    guint status_timeout;
    guint title_timeout;
    GtkWidget *l_status;

    GtkWidget *box_af;
    GtkWidget *l_af_title;
    GtkWidget *af_view;
    GtkWidget *af_scroll;

    gboolean autoscroll;
} ui_t;

extern ui_t ui;

void ui_init();

GtkWidget* ui_bandwidth_new();
void ui_bandwidth_fill(GtkWidget*, gboolean);
gboolean ui_update_title(gpointer);

void ui_dialog(GtkWidget*, GtkMessageType, gchar*, gchar*, ...);
void connect_button(gboolean);
gint ui_antenna_switch(gint);
gint ui_antenna_id(gint);
void ui_antenna_update();
void ui_toggle_ps_mode();
void ui_toggle_rt_mode();
void ui_screenshot();
void ui_activate();
void ui_rotator_button_swap();
void ui_status(gint, gchar*, ...);
void ui_decorations(gboolean);
gboolean ui_dialog_confirm_disconnect();

#endif

