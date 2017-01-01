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

#define AF_LIST_STORE_ID   0
#define AF_LIST_STORE_FREQ 1

#define UI_COLOR_BACKGROUND  "#FFFFFF"
#define UI_COLOR_FOREGROUND  "#000000"
#define UI_COLOR_INSENSITIVE "#C8C8C8"
#define UI_COLOR_STEREO      "#EE4000"
#define UI_COLOR_ACTION      "#FF9999"
#define UI_COLOR_ACTION2     "#FFCF99"

typedef struct ui_colors
{
    GdkColor background;
    GdkColor foreground;
    GdkColor insensitive;
    GdkColor stereo;
    GdkColor action;
    GdkColor action2;
} ui_colors_t;

typedef struct ui
{
    GtkWidget *window;
    gchar window_title[100];
    ui_colors_t colors;
    GdkCursor *click_cursor;

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
    GtkWidget *box_right;

    GtkWidget *volume;
    GtkWidget *squelch;
    GtkWidget *event_band, *l_band;
    GtkWidget *event_freq, *l_freq;
    GtkWidget *event_pi, *l_pi;
    GtkWidget *event_ps, *l_ps;

    GtkWidget *b_scan;
    GtkWidget *b_tune_back;
    GtkWidget *e_freq;
    GtkWidget *b_tune_reset;

    GtkObject *adj_align;
    GtkWidget *hs_align;

    GtkWidget *p_cci, *p_aci;

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
    GtkWidget *b_ontop, *b_ontop_icon;

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
    GtkWidget *l_status;

    GtkWidget *l_af;
    GtkWidget *af_list;
    GtkWidget *af_box;

    gboolean autoscroll;
} ui_t;

ui_t ui;

void ui_init();

GtkWidget* ui_bandwidth_new();
void ui_bandwidth_fill(GtkWidget*, gboolean);

void ui_dialog(GtkWidget*, GtkMessageType, gchar*, gchar*, ...);
void connect_button(gboolean);
void ui_antenna_switch(gint);
void ui_antenna_showhide();
void ui_toggle_ps_mode();
void ui_screenshot();
void ui_activate();
void ui_rotator_button_swap();
void ui_status(gint, gchar*, ...);
void ui_decorations(gboolean);
gboolean ui_dialog_confirm_disconnect();

#endif

