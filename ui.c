#include <gtk/gtk.h>
#include <string.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <math.h>
#include "ui.h"
#include "ui-audio.h"
#include "tuner.h"
#include "ui-tuner-set.h"
#include "ui-connect.h"
#include "tuner.h"
#include "ui-signal.h"
#include "settings.h"
#include "conf.h"
#include "ui-tuner-update.h"
#include "ui-input.h"
#include "scan.h"
#include "pattern.h"
#include "rdsspy.h"
#include "version.h"
#include "scheduler.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

static const char rc_string[] = "style \"small-button-style\"\n"
                                "{\n"
                                    "GtkButton::inner-border = { 0, 0, 0, 0 }\n"
                                 "}\n"
                                 "widget \"*.small-button\" style\n\"small-button-style\"\n"
                                 "\n"
                                 "style \"smaller-button-style\"\n"
                                 "{\n"
                                    "GtkButton::inner-border = { 0, 0, 0, 0 }\n"
                                    "GtkWidget::focus-padding = 0\n"
                                    "GtkWidget::focus-line-width = 0\n"
                                 "}\n"
                                 "widget \"*.smaller-button\" style\n\"smaller-button-style\"\n"
                                 "\n"
                                 "style \"ci-progress-style\"\n"
                                 "{\n"
                                     "GtkProgressBar::min-horizontal-bar-height = 15\n"
                                     "GtkProgressBar::yspacing = 0\n"
                                     "GtkProgressBar::xspacing = 0\n"
                                 "}\n"
                                 "widget \"*.ci-progress\" style\n\"ci-progress-style\"\n";

static void ui_destroy();
static gboolean ui_delete_event(GtkWidget*, GdkEvent*, gpointer);
static gboolean ui_window_event(GtkWidget*, gpointer);
static void tune_ui_back(GtkWidget*, gpointer);
static void tune_ui_round(GtkWidget*, gpointer);
static void tune_ui_step_click(GtkWidget*, GdkEventButton*, gpointer);
static void tune_ui_step_scroll(GtkWidget*, GdkEventScroll*, gpointer);
static gboolean ui_toggle_gain(GtkWidget*, GdkEventButton*, gpointer);
static gboolean ui_update_clock(gpointer);
static void tune_ui_af(GtkTreeSelection*, gpointer);
static void window_on_top(GtkToggleButton*);
static void ui_toggle_band(GtkWidget*, GdkEventButton*, gpointer);
static void ui_st_click(GtkWidget*, GdkEventButton*, gpointer);
static void ui_sig_click(GtkWidget*, GdkEventButton*, gpointer);
static gboolean ui_sig_click_key(GtkWidget*, GdkEventKey*, gpointer);
static gboolean ui_cursor(GtkWidget *widget, GdkEvent  *event, gpointer cursor);
static gboolean signal_tooltip(GtkWidget*, gint, gint, gboolean, GtkTooltip*, gpointer);
static void ui_af_autoscroll(GtkWidget*, GtkAllocation*, gpointer);

void
ui_init()
{
    GtkListStore *model;
    GtkCellRenderer *renderer;

    gtk_rc_parse_string(rc_string);
    gdk_color_parse(UI_COLOR_BACKGROUND, &ui.colors.background);
    gdk_color_parse(UI_COLOR_FOREGROUND, &ui.colors.foreground);
    gdk_color_parse(UI_COLOR_INSENSITIVE, &ui.colors.insensitive);
    gdk_color_parse(UI_COLOR_STEREO, &ui.colors.stereo);
    gdk_color_parse(UI_COLOR_ACTION, &ui.colors.action);
    gdk_color_parse(UI_COLOR_ACTION2, &ui.colors.action2);
    ui.click_cursor = gdk_cursor_new(GDK_HAND2);

    ui.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(ui.window), GTK_WIN_POS_CENTER);
    if(conf.restore_position && conf.win_x >= 0 && conf.win_y >= 0)
        gtk_window_move(GTK_WINDOW(ui.window), conf.win_x, conf.win_y);
    gtk_window_set_title(GTK_WINDOW(ui.window), APP_NAME);
    gtk_window_set_default_icon_name(APP_ICON);
    gtk_container_set_border_width(GTK_CONTAINER(ui.window), 0);
    gtk_widget_modify_bg(ui.window, GTK_STATE_NORMAL, &ui.colors.background);

    PangoFontDescription *font_header = pango_font_description_from_string("DejaVu Sans Mono, monospace 20");
    PangoFontDescription *font_status = pango_font_description_from_string("DejaVu Sans Mono, monospace 14");
    PangoFontDescription *font_entry  = pango_font_description_from_string("DejaVu Sans Mono, monospace 10");
    PangoFontDescription *font_af     = pango_font_description_from_string("DejaVu Sans Mono, monospace 11");
    PangoFontDescription *font_rt     = pango_font_description_from_string("DejaVu Sans Mono, monospace 9");

    ui.frame = gtk_event_box_new();
    gtk_container_set_border_width(GTK_CONTAINER(ui.frame), 0);
    gtk_container_add(GTK_CONTAINER(ui.window), ui.frame);

    ui.margin = gtk_event_box_new();
    gtk_container_set_border_width(GTK_CONTAINER(ui.margin), 1);
    gtk_widget_modify_bg(ui.margin, GTK_STATE_NORMAL, &ui.colors.background);
    gtk_container_add(GTK_CONTAINER(ui.frame), ui.margin);

    ui.box = gtk_vbox_new(FALSE, 2);
    gtk_container_set_border_width(GTK_CONTAINER(ui.box), 1);
    gtk_container_add(GTK_CONTAINER(ui.margin), ui.box);
    ui.box_header = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ui.box), ui.box_header);
    ui.box_ui = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ui.box), ui.box_ui);
    ui.box_left = gtk_vbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(ui.box_ui), ui.box_left, TRUE, TRUE, 0);
    ui.box_right = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ui.box_ui), ui.box_right, FALSE, FALSE, 0);

    // ----------------
    ui.volume = volume_init(conf.volume);
    gtk_box_pack_start(GTK_BOX(ui.box_header), ui.volume, FALSE, FALSE, 0);

    ui.squelch = squelch_init(0);
    gtk_box_pack_start(GTK_BOX(ui.box_header), ui.squelch, FALSE, FALSE, 0);

    ui.event_band = gtk_event_box_new();
    ui.l_band = gtk_label_new(NULL);
    gtk_container_add(GTK_CONTAINER(ui.event_band), ui.l_band);
    gtk_widget_modify_font(ui.l_band, font_header);
    gtk_box_pack_start(GTK_BOX(ui.box_header), ui.event_band, TRUE, FALSE, 4);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(ui.event_band), FALSE);
    g_signal_connect(ui.event_band, "enter-notify-event", G_CALLBACK(ui_cursor), ui.click_cursor);
    g_signal_connect(ui.event_band, "leave-notify-event", G_CALLBACK(ui_cursor), NULL);
    g_signal_connect(ui.event_band, "button-press-event", G_CALLBACK(ui_toggle_band), NULL);

    ui.event_freq = gtk_event_box_new();
    ui.l_freq = gtk_label_new(NULL);
    gtk_widget_modify_font(ui.l_freq, font_header);
    gtk_misc_set_alignment(GTK_MISC(ui.l_freq), 1.0, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(ui.l_freq), 7);
    gtk_container_add(GTK_CONTAINER(ui.event_freq), ui.l_freq);
    gtk_box_pack_start(GTK_BOX(ui.box_header), ui.event_freq, TRUE, FALSE, 6);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(ui.event_freq), FALSE);
    g_signal_connect(ui.event_freq, "button-press-event", G_CALLBACK(mouse_freq), NULL);

    ui.event_pi = gtk_event_box_new();
    ui.l_pi = gtk_label_new(NULL);
    gtk_widget_modify_font(ui.l_pi, font_header);
    gtk_misc_set_alignment(GTK_MISC(ui.l_pi), 0.0, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(ui.l_pi), 5);
    gtk_container_add(GTK_CONTAINER(ui.event_pi), ui.l_pi);
    gtk_box_pack_start(GTK_BOX(ui.box_header), ui.event_pi, TRUE, FALSE, 5);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(ui.event_pi), FALSE);
    g_signal_connect(ui.event_pi, "button-press-event", G_CALLBACK(mouse_pi), NULL);

    ui.event_ps = gtk_event_box_new();
    ui.l_ps = gtk_label_new(NULL);
    gtk_widget_modify_font(ui.l_ps, font_header);
    gtk_misc_set_alignment(GTK_MISC(ui.l_ps), 0.0, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(ui.l_ps), 10);
    gtk_container_add(GTK_CONTAINER(ui.event_ps), ui.l_ps);
    gtk_box_pack_start(GTK_BOX(ui.box_header), ui.event_ps, TRUE, FALSE, 0);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(ui.event_ps), FALSE);
    g_signal_connect(ui.event_ps, "button-press-event", G_CALLBACK(mouse_ps), NULL);

    // ----------------

    ui.box_left_tune = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ui.box_left), ui.box_left_tune, FALSE, FALSE, 0);

    ui.b_scan = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(ui.b_scan), gtk_image_new_from_icon_name("xdr-gtk-scan", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(ui.b_scan), FALSE);
    gtk_widget_set_name(ui.b_scan, "small-button");
    gtk_widget_set_tooltip_text(ui.b_scan, "Spectral scan");
    gtk_box_pack_start(GTK_BOX(ui.box_left_tune), ui.b_scan, FALSE, FALSE, 0);
    g_signal_connect(ui.b_scan, "clicked", G_CALLBACK(scan_dialog), NULL);

    ui.b_tune_back = gtk_button_new_with_label("↔");
    gtk_button_set_focus_on_click(GTK_BUTTON(ui.b_tune_back), FALSE);
    gtk_widget_modify_font(ui.b_tune_back, font_header);
    gtk_box_pack_start(GTK_BOX(ui.box_left_tune), ui.b_tune_back, TRUE, TRUE, 0);
    g_signal_connect(ui.b_tune_back, "clicked", G_CALLBACK(tune_ui_back), NULL);
    gtk_widget_set_tooltip_text(ui.b_tune_back, "Tune back to the previous frequency");

    ui.e_freq = gtk_entry_new_with_max_length(7);
    gtk_entry_set_width_chars(GTK_ENTRY(ui.e_freq), 8);
    gtk_widget_modify_font(ui.e_freq, font_entry);
    gtk_box_pack_start(GTK_BOX(ui.box_left_tune), ui.e_freq, FALSE, FALSE, 0);

    ui.b_tune_reset = gtk_button_new_with_label("R");
    gtk_button_set_focus_on_click(GTK_BUTTON(ui.b_tune_reset), FALSE);
    gtk_widget_set_name(ui.b_tune_reset, "small-button");
    gtk_box_pack_start(GTK_BOX(ui.box_left_tune), ui.b_tune_reset, TRUE, TRUE, 0);
    g_signal_connect(ui.b_tune_reset, "clicked", G_CALLBACK(tune_ui_round), NULL);
    gtk_widget_set_tooltip_text(ui.b_tune_reset, "Reset the frequency to a nearest channel");

    gchar label[5];
    size_t i;
    const gint steps[] = {5, 9, 10, 30, 50, 100, 200, 300};
    const size_t steps_n = sizeof(steps)/sizeof(gint);
    GtkWidget *b_tune[steps_n];
    for(i=0; i<steps_n; i++)
    {
        g_snprintf(label, 5, "%d", steps[i]);
        b_tune[i] = gtk_button_new_with_label(label);
        gtk_box_pack_start(GTK_BOX(ui.box_left_tune), b_tune[i], TRUE, TRUE, 0);
        g_signal_connect(b_tune[i], "button-press-event", G_CALLBACK(tune_ui_step_click), GINT_TO_POINTER(steps[i]));
        g_signal_connect(b_tune[i], "scroll-event", G_CALLBACK(tune_ui_step_scroll), GINT_TO_POINTER(steps[i]));
    }

    // ----------------

    ui.adj_align = gtk_adjustment_new(0.0, 0.0, 127.0, 0.5, 1.0, 0);
    ui.hs_align = gtk_hscale_new(GTK_ADJUSTMENT(ui.adj_align));
    gtk_scale_set_digits(GTK_SCALE(ui.hs_align), 0);
    gtk_scale_set_value_pos(GTK_SCALE(ui.hs_align), GTK_POS_RIGHT);
    g_signal_connect(G_OBJECT(ui.adj_align), "value-changed", G_CALLBACK(tuner_set_alignment), NULL);
    gtk_box_pack_start(GTK_BOX(ui.box_left), ui.hs_align, TRUE, TRUE, 0);

    // ----------------
    ui.box_left_interference = gtk_hbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(ui.box_left), ui.box_left_interference);

    ui.p_cci = gtk_progress_bar_new();
    gtk_widget_set_name(ui.p_cci, "ci-progress");
    gtk_widget_modify_bg(ui.p_cci, GTK_STATE_PRELIGHT, &ui.colors.action);
    gtk_widget_modify_fg(ui.p_cci, GTK_STATE_PRELIGHT, &ui.colors.foreground);
    gtk_box_pack_start(GTK_BOX(ui.box_left_interference), ui.p_cci, TRUE, TRUE, 0);

    ui.p_aci = gtk_progress_bar_new();
    gtk_widget_set_name(ui.p_aci, "ci-progress");
    gtk_widget_modify_bg(ui.p_aci, GTK_STATE_PRELIGHT, &ui.colors.action2);
    gtk_widget_modify_fg(ui.p_aci, GTK_STATE_PRELIGHT, &ui.colors.foreground);
    gtk_box_pack_start(GTK_BOX(ui.box_left_interference), ui.p_aci, TRUE, TRUE, 0);

    // ----------------

    ui.box_left_signal = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ui.box_left), ui.box_left_signal);

    ui.graph = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(ui.box_left_signal), ui.graph, TRUE, TRUE, 0);
    // or
    ui.p_signal = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(ui.box_left_signal), ui.p_signal, TRUE, TRUE, 0);

    // ----------------

    ui.box_left_indicators = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ui.box_left), ui.box_left_indicators, FALSE, FALSE, 0);

    ui.event_st = gtk_event_box_new();
    ui.l_st = gtk_label_new("ST");
    gtk_container_add(GTK_CONTAINER(ui.event_st), ui.l_st);
    gtk_widget_modify_font(ui.l_st, font_status);
    gtk_widget_modify_fg(GTK_WIDGET(ui.l_st), GTK_STATE_NORMAL, &ui.colors.insensitive);
    gtk_misc_set_alignment(GTK_MISC(ui.l_st), 0, 0.5);
    gtk_widget_set_tooltip_text(ui.l_st, "19kHz stereo pilot indicator");
    gtk_box_pack_start(GTK_BOX(ui.box_left_indicators), ui.event_st, TRUE, TRUE,  3);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(ui.event_st), FALSE);
    gtk_widget_set_events(ui.event_st, GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(ui.event_st, "enter-notify-event", G_CALLBACK(ui_cursor), ui.click_cursor);
    g_signal_connect(ui.event_st, "leave-notify-event", G_CALLBACK(ui_cursor), NULL);
    g_signal_connect(ui.event_st, "button-press-event", G_CALLBACK(ui_st_click), NULL);


    ui.l_rds = gtk_label_new("RDS");
    gtk_widget_modify_font(ui.l_rds, font_status);
    gtk_widget_modify_fg(GTK_WIDGET(ui.l_rds), GTK_STATE_NORMAL, &ui.colors.insensitive);
    gtk_misc_set_alignment(GTK_MISC(ui.l_rds), 0, 0.5);
    gtk_widget_set_tooltip_text(ui.l_rds, "RDS PI indicator");
    gtk_box_pack_start(GTK_BOX(ui.box_left_indicators), ui.l_rds, TRUE, TRUE,  3);

    ui.l_tp = gtk_label_new(NULL);
    gtk_widget_modify_font(ui.l_tp, font_status);
    gtk_widget_set_tooltip_text(ui.l_tp, "RDS Traffic Programme flag");
    gtk_box_pack_start(GTK_BOX(ui.box_left_indicators), ui.l_tp, TRUE, TRUE, 3);

    ui.l_ta = gtk_label_new(NULL);
    gtk_widget_modify_font(ui.l_ta, font_status);
    gtk_widget_set_tooltip_text(ui.l_ta, "RDS Traffic Announcement flag");
    gtk_box_pack_start(GTK_BOX(ui.box_left_indicators), ui.l_ta, TRUE, TRUE, 3);

    ui.l_ms = gtk_label_new(NULL);
    gtk_widget_modify_font(ui.l_ms, font_status);
    gtk_widget_set_tooltip_text(ui.l_ms, "RDS Music/Speech flag");
    gtk_box_pack_start(GTK_BOX(ui.box_left_indicators), ui.l_ms, TRUE, TRUE, 3);

    ui.l_pty = gtk_label_new(NULL);
    gtk_widget_modify_font(ui.l_pty, font_status);
    gtk_misc_set_alignment(GTK_MISC(ui.l_pty), 0.0, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(ui.l_pty), 8);
    gtk_widget_set_tooltip_text(ui.l_pty, "RDS Programme Type (PTY)");
    gtk_box_pack_start(GTK_BOX(ui.box_left_indicators), ui.l_pty, TRUE, TRUE, 3);

    ui.event_sig = gtk_event_box_new();
    ui.l_sig = gtk_label_new(NULL);
    gtk_container_add(GTK_CONTAINER(ui.event_sig), ui.l_sig);
    gtk_widget_modify_font(ui.l_sig, font_status);
    gtk_misc_set_alignment(GTK_MISC(ui.l_sig), 1, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(ui.l_sig), 12);
    gtk_widget_set_tooltip_text(ui.l_sig, "max / current signal level");
    gtk_box_pack_start(GTK_BOX(ui.box_left_indicators), ui.event_sig, TRUE, TRUE, 2);
    g_signal_connect(ui.l_sig, "query-tooltip", G_CALLBACK(signal_tooltip), NULL);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(ui.event_sig), FALSE);
    gtk_widget_set_events(ui.event_sig, GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(ui.event_sig, "enter-notify-event", G_CALLBACK(ui_cursor), ui.click_cursor);
    g_signal_connect(ui.event_sig, "leave-notify-event", G_CALLBACK(ui_cursor), NULL);
    g_signal_connect(ui.event_sig, "button-press-event", G_CALLBACK(ui_sig_click), NULL);

    // ----------------

    ui.box_left_settings1 = gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(ui.box_left), ui.box_left_settings1);

    ui.box_buttons = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ui.box_left_settings1), ui.box_buttons, FALSE, FALSE, 0);

    ui.b_connect = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(ui.b_connect), gtk_image_new_from_stock(GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(ui.b_connect), FALSE);
    gtk_widget_set_name(ui.b_connect, "small-button");
    gtk_widget_set_tooltip_text(ui.b_connect, "Connect");
    gtk_box_pack_start(GTK_BOX(ui.box_buttons), ui.b_connect, FALSE, FALSE, 0);
    g_signal_connect(ui.b_connect, "toggled", G_CALLBACK(connection_toggle), NULL);

    ui.b_pattern = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(ui.b_pattern), gtk_image_new_from_icon_name("xdr-gtk-pattern", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(ui.b_pattern), FALSE);
    gtk_widget_set_name(ui.b_pattern, "small-button");
    gtk_widget_set_tooltip_text(ui.b_pattern, "Antenna pattern");
    gtk_box_pack_start(GTK_BOX(ui.box_buttons), ui.b_pattern, FALSE, FALSE, 0);
    g_signal_connect(ui.b_pattern, "clicked", G_CALLBACK(pattern_dialog), NULL);

    ui.b_settings = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(ui.b_settings), gtk_image_new_from_icon_name("xdr-gtk-settings", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(ui.b_settings), FALSE);
    gtk_widget_set_name(ui.b_settings, "small-button");
    gtk_widget_set_tooltip_text(ui.b_settings, "Settings");
    gtk_box_pack_start(GTK_BOX(ui.box_buttons), ui.b_settings, FALSE, FALSE, 0);
    g_signal_connect_swapped(ui.b_settings, "clicked", G_CALLBACK(settings_dialog), GINT_TO_POINTER(SETTINGS_TAB_DEFAULT));

    ui.b_scheduler = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(ui.b_scheduler), gtk_image_new_from_icon_name("xdr-gtk-scheduler", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(ui.b_scheduler), FALSE);
    gtk_widget_set_name(ui.b_scheduler, "small-button");
    gtk_widget_set_tooltip_text(ui.b_scheduler, "Scheduler");
    gtk_box_pack_start(GTK_BOX(ui.box_buttons), ui.b_scheduler, FALSE, FALSE, 0);
    g_signal_connect(ui.b_scheduler, "toggled", G_CALLBACK(scheduler_toggle), NULL);

    ui.b_rdsspy = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(ui.b_rdsspy), gtk_image_new_from_icon_name("xdr-gtk-rdsspy", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(ui.b_rdsspy), FALSE);
    gtk_widget_set_name(ui.b_rdsspy, "small-button");
    gtk_widget_set_tooltip_text(ui.b_rdsspy, "RDS Spy Link");
    gtk_box_pack_start(GTK_BOX(ui.box_buttons), ui.b_rdsspy, FALSE, FALSE, 0);
    g_signal_connect(ui.b_rdsspy, "toggled", G_CALLBACK(rdsspy_toggle), NULL);

    ui.b_ontop = gtk_toggle_button_new();
    ui.b_ontop_icon = gtk_image_new_from_icon_name("xdr-gtk-top", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(ui.b_ontop), ui.b_ontop_icon);
    gtk_button_set_focus_on_click(GTK_BUTTON(ui.b_ontop), FALSE);
    gtk_widget_set_name(ui.b_ontop, "small-button");
    gtk_widget_set_tooltip_text(ui.b_ontop, "Stay on top");
    gtk_box_pack_start(GTK_BOX(ui.box_buttons), ui.b_ontop, FALSE, FALSE, 0);
    g_signal_connect(ui.b_ontop, "toggled", G_CALLBACK(window_on_top), NULL);

    ui.l_agc = gtk_label_new("AGC threshold:");
    gtk_box_pack_start(GTK_BOX(ui.box_left_settings1), ui.l_agc, TRUE, TRUE, 0);

    ui.c_agc = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(ui.c_agc), "highest");
    gtk_combo_box_append_text(GTK_COMBO_BOX(ui.c_agc), "high");
    gtk_combo_box_append_text(GTK_COMBO_BOX(ui.c_agc), "medium");
    gtk_combo_box_append_text(GTK_COMBO_BOX(ui.c_agc), "low");
    gtk_combo_box_set_active(GTK_COMBO_BOX(ui.c_agc), conf.agc);
    gtk_box_pack_start(GTK_BOX(ui.box_left_settings1), ui.c_agc, TRUE, TRUE, 0);
    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(ui.c_agc), FALSE);
    g_signal_connect(G_OBJECT(ui.c_agc), "changed", G_CALLBACK(tuner_set_agc), NULL);

    ui.x_rf = gtk_check_button_new_with_label("RF+");
    gtk_box_pack_start(GTK_BOX(ui.box_left_settings1), ui.x_rf, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.x_rf), conf.rfgain);
    g_signal_connect(ui.x_rf, "button-press-event", G_CALLBACK(ui_toggle_gain), NULL);

    // ----------------

    ui.box_left_settings2 = gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(ui.box_left), ui.box_left_settings2);

    ui.l_deemph = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(ui.box_left_settings2), ui.l_deemph, FALSE, FALSE, 0);

    ui.c_deemph = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(ui.c_deemph), "50 us");
    gtk_combo_box_append_text(GTK_COMBO_BOX(ui.c_deemph), "75 us");
    gtk_combo_box_append_text(GTK_COMBO_BOX(ui.c_deemph), "0 us");
    gtk_box_pack_start(GTK_BOX(ui.box_left_settings2), ui.c_deemph, TRUE, TRUE, 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(ui.c_deemph), conf.deemphasis);
    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(ui.c_deemph), FALSE);
    g_signal_connect(G_OBJECT(ui.c_deemph), "changed", G_CALLBACK(tuner_set_deemphasis), NULL);

    ui.ant = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
    gtk_list_store_insert_with_values(ui.ant, NULL, -1, 0, "Ant A", 1, (conf.ant_count>=1), -1);
    gtk_list_store_insert_with_values(ui.ant, NULL, -1, 0, "Ant B", 1, (conf.ant_count>=2), -1);
    gtk_list_store_insert_with_values(ui.ant, NULL, -1, 0, "Ant C", 1, (conf.ant_count>=3), -1);
    gtk_list_store_insert_with_values(ui.ant, NULL, -1, 0, "Ant D", 1, (conf.ant_count>=4), -1);
    GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(ui.ant), NULL);
    gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(filter), 1);

    ui.c_ant = gtk_combo_box_new_with_model(filter);
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ui.c_ant), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ui.c_ant), renderer, "text", 0, NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(ui.c_ant), 0);
    gtk_box_pack_start(GTK_BOX(ui.box_left_settings2), ui.c_ant, TRUE, TRUE, 0);
    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(ui.c_ant), FALSE);
    g_signal_connect(G_OBJECT(ui.c_ant), "changed", G_CALLBACK(tuner_set_antenna), NULL);

    ui.box_rotator = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(ui.box_left_settings2), ui.box_rotator, TRUE, TRUE, 0);

    ui.b_cw = gtk_toggle_button_new();
    ui.b_cw_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(ui.b_cw_label), "  <b>↻</b>  ");
    gtk_container_add(GTK_CONTAINER(ui.b_cw), ui.b_cw_label);
    gtk_box_pack_start(GTK_BOX(ui.box_rotator), ui.b_cw, TRUE, TRUE, 0);
    gtk_widget_set_tooltip_text(ui.b_cw, "Rotate antenna clockwise");
    gtk_button_set_focus_on_click(GTK_BUTTON(ui.b_cw), FALSE);
    g_signal_connect_swapped(ui.b_cw, "toggled", G_CALLBACK(tuner_set_rotator), GINT_TO_POINTER(1));

    ui.b_ccw = gtk_toggle_button_new();
    ui.b_ccw_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(ui.b_ccw_label), "  <b>↺</b>  ");
    gtk_container_add(GTK_CONTAINER(ui.b_ccw), ui.b_ccw_label);
    gtk_box_pack_start(GTK_BOX(ui.box_rotator), ui.b_ccw, TRUE, TRUE, 0);
    gtk_widget_set_tooltip_text(ui.b_ccw, "Rotate antenna counterclockwise");
    gtk_button_set_focus_on_click(GTK_BUTTON(ui.b_ccw), FALSE);
    g_signal_connect_swapped(ui.b_ccw, "toggled", G_CALLBACK(tuner_set_rotator), GINT_TO_POINTER(2));

    ui.l_bw = gtk_label_new("BW:");
    gtk_widget_set_tooltip_text(ui.l_bw, "FIR digital filter -3dB bandwidth");
    gtk_box_pack_start(GTK_BOX(ui.box_left_settings2), ui.l_bw, TRUE, TRUE, 0);

    ui.c_bw = ui_bandwidth_new();
    gtk_box_pack_start(GTK_BOX(ui.box_left_settings2), ui.c_bw, TRUE, TRUE, 0);
    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(ui.c_bw), FALSE);
    g_signal_connect(G_OBJECT(ui.c_bw), "changed", G_CALLBACK(tuner_set_bandwidth), NULL);

    ui.x_if = gtk_check_button_new_with_label("IF+");
    gtk_box_pack_start(GTK_BOX(ui.box_left_settings2), ui.x_if, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.x_if), conf.ifgain);
    g_signal_connect(ui.x_if, "button-press-event", G_CALLBACK(ui_toggle_gain), NULL);

    // ----------------

    ui.l_af = gtk_label_new("  AF:  ");
    gtk_widget_modify_font(ui.l_af, font_af);
    gtk_box_pack_start(GTK_BOX(ui.box_right), ui.l_af, FALSE, FALSE, 3);

    model = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
    ui.af_list = gtk_tree_view_new();
    gtk_tree_view_set_model(GTK_TREE_VIEW(ui.af_list), GTK_TREE_MODEL(model));
    g_object_unref(model);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(ui.af_list), -1, "ID", gtk_cell_renderer_text_new(), "text", AF_LIST_STORE_ID, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(ui.af_list), -1, "FREQ", gtk_cell_renderer_text_new(), "text", AF_LIST_STORE_FREQ, NULL);
    gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(ui.af_list), AF_LIST_STORE_ID), FALSE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui.af_list), FALSE);
    gtk_widget_modify_base(ui.af_list, GTK_STATE_NORMAL, &ui.colors.background);
    g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui.af_list)), "changed", G_CALLBACK(tune_ui_af), NULL);
    ui.autoscroll = FALSE;
    g_signal_connect(GTK_TREE_VIEW(ui.af_list), "size-allocate", G_CALLBACK(ui_af_autoscroll), NULL);
    gtk_widget_set_can_focus(GTK_WIDGET(ui.af_list), FALSE);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), AF_LIST_STORE_ID, GTK_SORT_ASCENDING);

    ui.af_box = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ui.af_box), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(ui.af_box), ui.af_list);
    gtk_box_pack_start(GTK_BOX(ui.box_right), ui.af_box, TRUE, TRUE, 0);

    // ----------------

    for(i=0; i<2; i++)
    {
        ui.event_rt[i] = gtk_event_box_new();
        ui.l_rt[i] = gtk_label_new(NULL);
        gtk_widget_modify_font(ui.l_rt[i], font_rt);
        gtk_misc_set_alignment(GTK_MISC(ui.l_rt[i]), 0.0, 0.5);
        gtk_label_set_width_chars(GTK_LABEL(ui.l_rt[i]), 66);
        gtk_container_add(GTK_CONTAINER(ui.event_rt[i]), ui.l_rt[i]);
        gtk_event_box_set_visible_window(GTK_EVENT_BOX(ui.event_rt[i]), FALSE);
        gtk_box_pack_start(GTK_BOX(ui.box), ui.event_rt[i], TRUE, TRUE, 0);
        g_signal_connect(ui.event_rt[i], "button-press-event", G_CALLBACK(mouse_rt), tuner.rds_rt[i]);
    }

    // ----------------

    ui.l_status = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(ui.box), ui.l_status, TRUE, TRUE, 0);

    pango_font_description_free(font_status);
    pango_font_description_free(font_header);
    pango_font_description_free(font_entry);
    pango_font_description_free(font_af);
    pango_font_description_free(font_rt);

    tuner.volume = lround(gtk_scale_button_get_value(GTK_SCALE_BUTTON(ui.volume)));
    tuner.squelch = lround(gtk_scale_button_get_value(GTK_SCALE_BUTTON(ui.squelch)));
    tuner.daa = lround(gtk_adjustment_get_value(GTK_ADJUSTMENT(ui.adj_align)));
    tuner.agc = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_agc));
    tuner.deemphasis = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_deemph));
    tuner.antenna = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_ant));
    tuner.rfgain = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.x_rf));
    tuner.ifgain = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.x_if));
    tuner_clear_all();
    tuner.mode = MODE_FM;
    tuner.filter = -1;
    for(i=0; i<ANT_COUNT; i++)
        tuner.offset[i] = conf.ant_offset[i];
    ui_update_mode();
    gtk_combo_box_set_active(GTK_COMBO_BOX(ui.c_bw),
                             gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(ui.c_bw))), NULL) - 1);

    ui_update_service();

    scan_init();

    ui_decorations(!conf.hide_decorations);
    gtk_widget_show_all(ui.window);

    if(!conf.ant_show_alignment)
        gtk_widget_hide(ui.hs_align);
    if(conf.hide_interference)
        gtk_widget_hide(ui.box_left_interference);
    if(conf.hide_radiotext)
    {
        gtk_widget_hide(ui.l_rt[0]);
        gtk_widget_hide(ui.l_rt[1]);
    }
    if(conf.hide_statusbar)
        gtk_widget_hide(ui.l_status);
    ui_rotator_button_swap();
    ui_antenna_showhide();

    gtk_widget_add_events(GTK_WIDGET(ui.window), GDK_CONFIGURE);
    g_signal_connect(ui.window, "configure-event", G_CALLBACK(ui_window_event), NULL);
    g_signal_connect(ui.window, "key-press-event", G_CALLBACK(keyboard_press), NULL);
    g_signal_connect(ui.window, "key-release-event", G_CALLBACK(keyboard_release), NULL);
    g_signal_connect(ui.window, "button-press-event", G_CALLBACK(mouse_window), GTK_WINDOW(ui.window));
    g_signal_connect(ui.window, "delete-event", G_CALLBACK(ui_delete_event), NULL);
    g_signal_connect(ui.window, "destroy", G_CALLBACK(ui_destroy), NULL);
    ui.status_timeout = g_timeout_add(1000, (GSourceFunc)ui_update_clock, (gpointer)ui.l_status);

    signal_init();

    if(conf.auto_connect)
        connection_dialog(TRUE);
}

static void
ui_destroy()
{
    if(tuner.thread)
    {
        tuner_write(tuner.thread, "X");
        g_usleep(25000);
        tuner_thread_cancel(tuner.thread);
        g_usleep(25000);
    }
    gtk_main_quit();
}

static gboolean
ui_delete_event(GtkWidget *widget,
                GdkEvent  *event,
                gpointer   user_data)
{
    GtkWidget *dialog;
    gint response;

    conf_write();

    if(!tuner.thread || !conf.disconnect_confirm)
        return FALSE;

    dialog = gtk_message_dialog_new(GTK_WINDOW(ui.window),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO,
                                    "Tuner is currently connected.\nAre you sure you want to quit?");
    gtk_window_set_title(GTK_WINDOW(dialog), APP_NAME);
#ifdef G_OS_WIN32
    response = win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    response = gtk_dialog_run(GTK_DIALOG(dialog));
#endif
    gtk_widget_destroy(dialog);
    return (response != GTK_RESPONSE_YES);
}

static gboolean
ui_window_event(GtkWidget *widget,
                gpointer   data)
{
    gtk_window_get_position(GTK_WINDOW(ui.window), &conf.win_x, &conf.win_y);
    return FALSE;
}

GtkWidget*
ui_bandwidth_new()
{
    GtkListStore *model;
    GtkCellRenderer *renderer;
    GtkWidget *widget;

    model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
    widget = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget), renderer,
                                   "text", 0,
                                   "sensitive", 1,
                                   NULL);
    return widget;
}

void
ui_bandwidth_fill(GtkWidget *widget,
                  gboolean   auto_mode)
{
    GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(widget)));
    GtkTreeIter iter;
    gchar str[10];
    gint i;

    gtk_list_store_clear(GTK_LIST_STORE(store));

    for(i=0; i<tuner_filter_count()-1; i++)
    {
        if(tuner.mode == MODE_FM)
        {
            g_snprintf(str, sizeof(str), "%d kHz", tuner_filter_bw_from_index(i)/1000);
        }
        else if(tuner.mode == MODE_AM)
        {
            g_snprintf(str, sizeof(str), "%.1f%skHz",
                     tuner_filter_bw_from_index(i)/1000.0,
                     (tuner_filter_bw_from_index(i)/1000.0 < 10.0) ? " " : "");
        }

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, str, 1, TRUE, -1);
    }

    if(auto_mode)
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, (tuner.mode == MODE_FM ? "auto" : "default"),
                           1, (tuner.mode == MODE_FM),
                           -1);
    }
}

static void
tune_ui_back(GtkWidget *widget,
             gpointer   nothing)
{
    tuner_set_frequency_prev();
}

static void
tune_ui_round(GtkWidget *widget,
              gpointer   nothing)
{
    tuner_modify_frequency(TUNER_FREQ_MODIFY_RESET);
}

static void
tune_ui_step_click(GtkWidget      *widget,
                   GdkEventButton *event,
                   gpointer        step)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) // right click, tune down
        tuner_set_frequency(tuner_get_freq()-(GPOINTER_TO_INT(step)));
    else if(event->type == GDK_BUTTON_PRESS && event->button == 1) // left click, tune up
        tuner_set_frequency(tuner_get_freq()+(GPOINTER_TO_INT(step)));
}

static void
tune_ui_step_scroll(GtkWidget      *widget,
                    GdkEventScroll *event,
                    gpointer        step)
{
    if(event->direction)
        tuner_set_frequency(tuner_get_freq()-(GPOINTER_TO_INT(step)));
    else
        tuner_set_frequency(tuner_get_freq()+(GPOINTER_TO_INT(step)));
}

static gboolean
ui_toggle_gain(GtkWidget      *widget,
               GdkEventButton *event,
               gpointer        nothing)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) // right click, toggle both RF and IF
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.x_rf), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.x_rf)));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.x_if), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.x_if)));
        tuner_set_gain();
        return TRUE;
    }
    else if(event->type == GDK_BUTTON_PRESS && event->button == 1) // left click, toggle
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
        tuner_set_gain();
        return TRUE;
    }
    return FALSE;
}

static gboolean
ui_update_clock(gpointer label)
{
    gchar buff[20], buff2[100];
    time_t tt = time(NULL);
    strftime(buff, sizeof(buff), "%d-%m-%Y %H:%M:%S", (conf.utc ? gmtime(&tt) : localtime(&tt)));

    // network connection
    if(tuner.online_guests)
    {
        g_snprintf(buff2, sizeof(buff2),
                   "Online: %d (+%d)  |  %s %s%s",
                   tuner.online,
                   tuner.online_guests,
                   buff,
                   (conf.utc ? "UTC" : "LT"),
                   (tuner.guest ? "  <b>(guest)</b>" : ""));
    }
    else if(tuner.online)
    {
        g_snprintf(buff2, sizeof(buff2),
                   "Online: %d  |  %s %s%s",
                   tuner.online,
                   buff,
                   (conf.utc ? "UTC" : "LT"),
                   (tuner.guest ? "  <b>(guest)</b>" : ""));
    }
    else
    {
        g_snprintf(buff2, sizeof(buff2),
                   "%s %s",
                   buff,
                   (conf.utc ? "UTC" : "LT"));
    }

    gtk_label_set_markup(GTK_LABEL(label), buff2);

    ui.status_timeout = g_timeout_add(1000, (GSourceFunc)ui_update_clock, (gpointer)ui.l_status);
    return FALSE;
}

static void
tune_ui_af(GtkTreeSelection *ts,
           gpointer          nothing)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint n = 0;

    if(!gtk_tree_selection_get_selected(ts, &model, &iter))
    {
        return;
    }
    gtk_tree_model_get(model, &iter, 0, &n, -1);

    if(n)
    {
        tuner_set_frequency(87500+n*100);
    }
}

void
ui_dialog(GtkWidget      *window,
          GtkMessageType  icon,
          gchar          *title,
          gchar          *format,
          ...)
{
    GtkWidget *dialog;
    va_list args;
    gchar *msg;

    va_start(args, format);
    msg = g_markup_vprintf_escaped(format, args);
    va_end(args);
    dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                    GTK_DIALOG_MODAL,
                                    icon,
                                    GTK_BUTTONS_CLOSE,
                                    NULL);
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), msg);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    if(!window)
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
#ifdef G_OS_WIN32
    win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    gtk_dialog_run(GTK_DIALOG(dialog));
#endif
    gtk_widget_destroy(dialog);
    g_free(msg);
}

static void
window_on_top(GtkToggleButton *item)
{
    gboolean state = gtk_toggle_button_get_active(item);
    gtk_window_set_keep_above(GTK_WINDOW(ui.window), state);
}

void
connect_button(gboolean is_active)
{
    g_signal_handlers_block_by_func(G_OBJECT(ui.b_connect), GINT_TO_POINTER(connection_toggle), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.b_connect), is_active);
    g_signal_handlers_unblock_by_func(G_OBJECT(ui.b_connect), GINT_TO_POINTER(connection_toggle), NULL);
    gtk_widget_set_tooltip_text(ui.b_connect, (is_active ? "Disconnect" : "Connect"));
}

gint
ui_antenna_switch(gint newfreq)
{
    gint current = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_ant));
    gint i;

    if(current < 0 || current > ANT_COUNT)
        current = 0;

    if(!conf.ant_auto_switch)
        return current;

    i = ui_antenna_id(newfreq);
    if(current == i)
        return i;

    gtk_combo_box_set_active(GTK_COMBO_BOX(ui.c_ant), i);
    return i;
}

gint
ui_antenna_id(gint newfreq)
{
    gint current = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_ant));
    gint i;

    if(current < 0 || current > ANT_COUNT)
        current = 0;

    if(!conf.ant_auto_switch)
        return current;

    for(i=0; i<conf.ant_count; i++)
    {
        if(newfreq >= conf.ant_start[i] && newfreq <= conf.ant_stop[i])
            return i;
    }

    return current;
}

static void
ui_toggle_band(GtkWidget      *widget,
               GdkEventButton *event,
               gpointer        step)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 1)
        tuner_set_mode((tuner.mode != MODE_FM) ? MODE_FM : MODE_AM);
}

static void
ui_st_click(GtkWidget      *widget,
            GdkEventButton *event,
            gpointer        step)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
        if(tuner.mode == MODE_FM)
            tuner_set_stereo_test();
    }
    else if(event->type == GDK_BUTTON_PRESS && event->button == 3)
        tuner_set_forced_mono(!tuner.forced_mono);
}

static void
ui_sig_click(GtkWidget      *widget,
             GdkEventButton *event,
             gpointer        step)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
        GtkWidget *dialog, *message;
        GtkWidget *box, *spinbutton, *label, *checkbox;
        gint interval;
        gboolean fast_mode;

        dialog = gtk_message_dialog_new(GTK_WINDOW(ui.window),
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_QUESTION,
                                        GTK_BUTTONS_OK_CANCEL,
                                        "Custom interval [ms]:");
        gtk_window_set_title(GTK_WINDOW(dialog), "Signal sampling");
        message = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog));

        box = gtk_vbox_new(FALSE, 2);
        gtk_box_pack_start(GTK_BOX(message), box, FALSE, FALSE, 0);

        spinbutton = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1000.0, 1.0, 2.0, 0.0)), 0, 0);
        g_signal_connect(dialog, "key-press-event", G_CALLBACK(ui_sig_click_key), spinbutton);
        gtk_box_pack_start(GTK_BOX(box), spinbutton, FALSE, FALSE, 0);

        label = gtk_label_new("(use zero for default)");
        gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

        checkbox = gtk_check_button_new_with_label("Fast signal detector");
        gtk_box_pack_start(GTK_BOX(box), checkbox, FALSE, FALSE, 2);

        gtk_widget_show_all(dialog);
        if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
        {
            interval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton));
            fast_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox));
            tuner_set_sampling_interval(interval, fast_mode);
        }
        gtk_widget_destroy(dialog);
    }
}

static gboolean
ui_sig_click_key(GtkWidget   *widget,
                 GdkEventKey *event,
                 gpointer     button)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    if(current == GDK_Return)
    {
        gtk_spin_button_update(GTK_SPIN_BUTTON(button));
        gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_OK);
        return TRUE;
    }
    return FALSE;
}

static gboolean
ui_cursor(GtkWidget *widget,
          GdkEvent  *event,
          gpointer   cursor)
{
    gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
    return FALSE;
}

void
ui_antenna_showhide()
{
    GtkTreeIter iter;

    if(conf.ant_count == 1)
    {
        gtk_widget_hide(ui.c_ant);
        gtk_label_set_text(GTK_LABEL(ui.l_deemph), "De-emphasis:");
        gtk_widget_show(ui.l_deemph);
    }
    else
    {
        gtk_widget_show(ui.c_ant);
        gtk_widget_hide(ui.l_deemph);
    }

    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ui.ant), &iter))
        gtk_list_store_set(ui.ant, &iter, 1, (conf.ant_count>=1), -1);

    if(gtk_tree_model_iter_next(GTK_TREE_MODEL(ui.ant), &iter))
        gtk_list_store_set(ui.ant, &iter, 1, (conf.ant_count>=2), -1);

    if(gtk_tree_model_iter_next(GTK_TREE_MODEL(ui.ant), &iter))
        gtk_list_store_set(ui.ant, &iter, 1, (conf.ant_count>=3), -1);

    if(gtk_tree_model_iter_next(GTK_TREE_MODEL(ui.ant), &iter))
        gtk_list_store_set(ui.ant, &iter, 1, (conf.ant_count>=4), -1);
}

static gboolean
signal_tooltip(GtkWidget  *label,
               gint        x,
               gint        y,
               gboolean    keyboard_mode,
               GtkTooltip *tooltip,
               gpointer    user_data)
{
    const gchar *unit = signal_unit();
    gchar *str;

    if(!tuner.signal_samples)
        return FALSE;

    str = g_markup_printf_escaped("average signal: <b>%.1f%s%s</b> (%d samples)",
                                  signal_level(tuner.signal_sum/(gdouble)tuner.signal_samples),
                                  (strlen(unit) ? " " : ""),
                                  unit,
                                  tuner.signal_samples);

    gtk_tooltip_set_markup(tooltip, str);
    g_free(str);
    return TRUE;
}

void
ui_toggle_ps_mode()
{
    conf.rds_ps_progressive = !conf.rds_ps_progressive;
    if(tuner.rds_ps_avail)
        ui_update_ps(FALSE);
}

void
ui_screenshot()
{
    static gchar *default_path = "." PATH_SEP "screenshots";
    gchar t[20], *filename;
    GdkPixmap *pixmap;
    GdkPixbuf *pixbuf;
    gint width, height;
    gchar *directory;
    time_t tt = time(NULL);
    GError *err = NULL;
    strftime(t, sizeof(t), "%Y%m%d-%H%M%S", conf.utc?gmtime(&tt):localtime(&tt));

    if(conf.screen_dir && strlen(conf.screen_dir))
    {
        directory = conf.screen_dir;
    }
    else
    {
        g_mkdir(default_path, 0755);
        directory = default_path;
    }

    if(tuner.rds_pi >= 0)
    {
        filename = g_strdup_printf("%s" PATH_SEP "%s-%d-%04X.png", directory, t, tuner_get_freq(), tuner.rds_pi);
    }
    else
    {
        filename = g_strdup_printf("%s" PATH_SEP "%s-%d.png", directory, t, tuner_get_freq());
    }

    /* HACK: refresh window to avoid icons disappearing */
    gtk_widget_queue_draw(ui.window);
    gdk_window_process_all_updates();

    pixmap = gtk_widget_get_snapshot(ui.window, NULL);
    gdk_pixmap_get_size(pixmap, &width, &height);
    pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 1, 1, 0, 0, width-2, height-2);
    if(!gdk_pixbuf_save(pixbuf, filename, "png", &err, NULL))
    {
        ui_dialog(ui.window,
                  GTK_MESSAGE_ERROR,
                  "Screenshot",
                  "%s\n\nCheck selected screenshot directory in settings.",
                  err->message);
        g_error_free(err);
    }
    g_object_unref(G_OBJECT(pixmap));
    g_object_unref(G_OBJECT(pixbuf));
    g_free(filename);
}

void
ui_activate()
{
#ifdef G_OS_WIN32
    win32_grab_focus(GTK_WINDOW(ui.window));
#else
    gtk_window_present(GTK_WINDOW(ui.window));
#endif
}

void
ui_rotator_button_swap()
{
    if(conf.ant_swap_rotator)
    {
        gtk_box_reorder_child(GTK_BOX(ui.box_rotator), ui.b_cw, 1);
        gtk_box_reorder_child(GTK_BOX(ui.box_rotator), ui.b_ccw, 0);
    }
    else
    {
        gtk_box_reorder_child(GTK_BOX(ui.box_rotator), ui.b_cw, 0);
        gtk_box_reorder_child(GTK_BOX(ui.box_rotator), ui.b_ccw, 1);
    }
}

void
ui_status(gint   duration,
          gchar *format,
          ...)
{
    va_list args;
    gchar *msg;
    va_start(args, format);
    msg = g_strdup_vprintf(format, args);
    va_end(args);
    gtk_label_set_markup(GTK_LABEL(ui.l_status), msg);
    g_free(msg);

    g_source_remove(ui.status_timeout);
    ui.status_timeout = g_timeout_add(duration, (GSourceFunc)ui_update_clock, (gpointer)ui.l_status);
}

void
ui_decorations(gboolean state)
{
    gtk_window_set_decorated(GTK_WINDOW(ui.window), state);
    gtk_window_set_resizable(GTK_WINDOW(ui.window), FALSE);
    gtk_widget_modify_bg(ui.frame, GTK_STATE_NORMAL, (state?&ui.colors.background:&ui.colors.insensitive));
}

gboolean
ui_dialog_confirm_disconnect()
{
    GtkWidget *dialog;
    gint response;
    dialog = gtk_message_dialog_new(GTK_WINDOW(ui.window),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO,
                                    "Are you sure you want to disconnect?");
    gtk_window_set_title(GTK_WINDOW(dialog), APP_NAME);
#ifdef G_OS_WIN32
    response = win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    response = gtk_dialog_run(GTK_DIALOG(dialog));
#endif
    gtk_widget_destroy(dialog);
    return (response == GTK_RESPONSE_YES);
}

static void
ui_af_autoscroll(GtkWidget     *widget,
                 GtkAllocation *allocation,
                 gpointer       user_data)
{
    GtkWidget *parent = gtk_widget_get_parent(widget);
    GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(parent));
    if(ui.autoscroll)
    {
        gtk_adjustment_set_value(adj, gtk_adjustment_get_lower(adj));
        ui.autoscroll = FALSE;
    }
}
