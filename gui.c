#include <gtk/gtk.h>
#include <string.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <math.h>
#include "gui.h"
#include "gui-audio.h"
#include "gui-tuner.h"
#include "gui-connect.h"
#include "gui-update.h"
#include "tuner.h"
#include "graph.h"
#include "settings.h"
#include "clipboard.h"
#include "keyboard.h"
#include "scan.h"
#include "pattern.h"
#include "sig.h"
#include "rdsspy.h"
#include "version.h"
#include "scheduler.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

const char rc_string[] = "style \"small-button-style\"\n"
                         "{\n"
                             "GtkWidget::focus-padding = 0\n"
                             "GtkWidget::focus-line-width = 0\n"
                         "}\n"
                         "widget \"*.small-button\" style\n\"small-button-style\"\n";

void gui_init()
{
    gtk_rc_parse_string(rc_string);
    gdk_color_parse("#FFFFFF", &gui.colors.background);
    gdk_color_parse("#DDDDDD", &gui.colors.grey);
    gdk_color_parse("#000000", &gui.colors.black);
    gdk_color_parse("#EE4000", &gui.colors.stereo);
    gdk_color_parse("#FF8888", &gui.colors.action);
    gui.click_cursor = gdk_cursor_new(GDK_HAND2);

    gui.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(gui.window), GTK_WIN_POS_CENTER);
    if(conf.restore_pos)
    {
        gtk_window_move(GTK_WINDOW(gui.window), conf.win_x, conf.win_y);
    }
    gtk_window_set_title(GTK_WINDOW(gui.window), APP_NAME);
    gtk_window_set_resizable(GTK_WINDOW(gui.window), FALSE);
    gui.clipboard = gtk_widget_get_clipboard(gui.window, GDK_SELECTION_CLIPBOARD);
    gtk_window_set_default_icon_name(APP_ICON);
    gtk_window_set_decorated(GTK_WINDOW(gui.window), !conf.hide_decorations);
    gtk_container_set_border_width(GTK_CONTAINER(gui.window), 0);
    gtk_widget_modify_bg(gui.window, GTK_STATE_NORMAL, &gui.colors.background);

    PangoFontDescription *font_header = pango_font_description_from_string("DejaVu Sans Mono, monospace 20");
    PangoFontDescription *font_status = pango_font_description_from_string("DejaVu Sans Mono, monospace 14");
    PangoFontDescription *font_entry  = pango_font_description_from_string("DejaVu Sans Mono, monospace 10");
    PangoFontDescription *font_af     = pango_font_description_from_string("DejaVu Sans Mono, monospace 11");
    PangoFontDescription *font_rt     = pango_font_description_from_string("DejaVu Sans Mono, monospace 9");

    gui.frame = gtk_event_box_new();
    gtk_container_set_border_width(GTK_CONTAINER(gui.frame), 0);
    gtk_widget_modify_bg(gui.frame, GTK_STATE_NORMAL, (conf.hide_decorations?&gui.colors.grey:&gui.colors.background));
    gtk_container_add(GTK_CONTAINER(gui.window), gui.frame);

    gui.margin = gtk_event_box_new();
    gtk_container_set_border_width(GTK_CONTAINER(gui.margin), 1);
    gtk_widget_modify_bg(gui.margin, GTK_STATE_NORMAL, &gui.colors.background);
    gtk_container_add(GTK_CONTAINER(gui.frame), gui.margin);

    gui.box = gtk_vbox_new(FALSE, 2);
    gtk_container_set_border_width(GTK_CONTAINER(gui.box), 1);
    gtk_container_add(GTK_CONTAINER(gui.margin), gui.box);
    gui.box_header = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box), gui.box_header);
    gui.box_gui = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box), gui.box_gui);
    gui.box_left = gtk_vbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(gui.box_gui), gui.box_left);
    gui.box_right = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box_gui), gui.box_right);

    // ----------------

    gui.volume = volume_init();
    gtk_box_pack_start(GTK_BOX(gui.box_header), gui.volume, FALSE, FALSE, 0);

    gui.squelch = squelch_init();
    gtk_box_pack_start(GTK_BOX(gui.box_header), gui.squelch, FALSE, FALSE, 0);

    gui.event_band = gtk_event_box_new();
    gui.l_band = gtk_label_new(NULL);
    gtk_container_add(GTK_CONTAINER(gui.event_band), gui.l_band);
    gtk_widget_modify_font(gui.l_band, font_header);
    gtk_box_pack_start(GTK_BOX(gui.box_header), gui.event_band, TRUE, FALSE, 4);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(gui.event_band), FALSE);
    g_signal_connect(gui.event_band, "enter-notify-event", G_CALLBACK(gui_cursor), gui.click_cursor);
    g_signal_connect(gui.event_band, "leave-notify-event", G_CALLBACK(gui_cursor), NULL);
    g_signal_connect(gui.event_band, "button-press-event", G_CALLBACK(gui_toggle_band), NULL);

    gui.event_freq = gtk_event_box_new();
    gui.l_freq = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_freq, font_header);
    gtk_misc_set_alignment(GTK_MISC(gui.l_freq), 1.0, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(gui.l_freq), 7);
    gtk_container_add(GTK_CONTAINER(gui.event_freq), gui.l_freq);
    gtk_box_pack_start(GTK_BOX(gui.box_header), gui.event_freq, TRUE, FALSE, 6);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(gui.event_freq), FALSE);
    g_signal_connect(gui.event_freq, "button-press-event", G_CALLBACK(clipboard_full), NULL);

    gui.event_pi = gtk_event_box_new();
    gui.l_pi = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_pi, font_header);
    gtk_misc_set_alignment(GTK_MISC(gui.l_pi), 0.0, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(gui.l_pi), 5);
    gtk_container_add(GTK_CONTAINER(gui.event_pi), gui.l_pi);
    gtk_box_pack_start(GTK_BOX(gui.box_header), gui.event_pi, TRUE, FALSE, 5);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(gui.event_pi), FALSE);
    g_signal_connect(gui.event_pi, "button-press-event", G_CALLBACK(clipboard_pi), NULL);

    gui.event_ps = gtk_event_box_new();
    gui.l_ps = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_ps, font_header);
    gtk_misc_set_alignment(GTK_MISC(gui.l_ps), 0.0, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(gui.l_ps), 10);
    gtk_container_add(GTK_CONTAINER(gui.event_ps), gui.l_ps);
    gtk_box_pack_start(GTK_BOX(gui.box_header), gui.event_ps, TRUE, FALSE, 0);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(gui.event_ps), FALSE);
    g_signal_connect(gui.event_ps, "button-press-event", G_CALLBACK(clipboard_ps), NULL);

    // ----------------

    gui.box_left_tune = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gui.box_left), gui.box_left_tune, FALSE, FALSE, 0);

    gui.b_scan = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(gui.b_scan), gtk_image_new_from_icon_name("xdr-gtk-scan", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(gui.b_scan), FALSE);
    gtk_widget_set_name(gui.b_scan, "small-button");
    gtk_widget_set_tooltip_text(gui.b_scan, "Spectral scan");
    gtk_box_pack_start(GTK_BOX(gui.box_left_tune), gui.b_scan, FALSE, FALSE, 0);
    g_signal_connect(gui.b_scan, "clicked", G_CALLBACK(scan_dialog), NULL);

    gui.b_tune_back = gtk_button_new_with_label("↔");
    gtk_button_set_focus_on_click(GTK_BUTTON(gui.b_tune_back), FALSE);
    gtk_widget_modify_font(gui.b_tune_back, font_header);
    gtk_box_pack_start(GTK_BOX(gui.box_left_tune), gui.b_tune_back, TRUE, TRUE, 0);
    g_signal_connect(gui.b_tune_back, "clicked", G_CALLBACK(tune_gui_back), NULL);
    gtk_widget_set_tooltip_text(gui.b_tune_back, "Tune back to the previous frequency");

    gui.e_freq = gtk_entry_new_with_max_length(7);
    gtk_entry_set_width_chars(GTK_ENTRY(gui.e_freq), 8);
    gtk_widget_modify_font(gui.e_freq, font_entry);
    gtk_box_pack_start(GTK_BOX(gui.box_left_tune), gui.e_freq, FALSE, FALSE, 0);

    gui.b_tune_reset = gtk_button_new_with_label("R");
    gtk_button_set_focus_on_click(GTK_BUTTON(gui.b_tune_reset), FALSE);
    gtk_widget_set_name(gui.b_tune_reset, "small-button");
    gtk_box_pack_start(GTK_BOX(gui.box_left_tune), gui.b_tune_reset, TRUE, TRUE, 0);
    g_signal_connect(gui.b_tune_reset, "clicked", G_CALLBACK(tune_gui_round), NULL);
    gtk_widget_set_tooltip_text(gui.b_tune_reset, "Reset the frequency to a nearest channel");

    gchar label[5];
    size_t i;
    const gint steps[] = {5, 9, 10, 30, 50, 100, 200, 300};
    const size_t steps_n = sizeof(steps)/sizeof(gint);
    GtkWidget *b_tune[steps_n];
    for(i=0; i<steps_n; i++)
    {
        g_snprintf(label, 5, "%d", steps[i]);
        b_tune[i] = gtk_button_new_with_label(label);
        gtk_box_pack_start(GTK_BOX(gui.box_left_tune), b_tune[i], TRUE, TRUE, 0);
        g_signal_connect(b_tune[i], "button-press-event", G_CALLBACK(tune_gui_step_click), GINT_TO_POINTER(steps[i]));
        g_signal_connect(b_tune[i], "scroll-event", G_CALLBACK(tune_gui_step_scroll), GINT_TO_POINTER(steps[i]));
    }

    // ----------------

    gui.adj_align = gtk_adjustment_new(0.0, 0.0, 127.0, 0.5, 1.0, 0);
    gui.hs_align = gtk_hscale_new(GTK_ADJUSTMENT(gui.adj_align));
    gtk_scale_set_digits(GTK_SCALE(gui.hs_align), 0);
    gtk_scale_set_value_pos(GTK_SCALE(gui.hs_align), GTK_POS_RIGHT);
    g_signal_connect(G_OBJECT(gui.adj_align), "value-changed", G_CALLBACK(tuner_set_alignment), NULL);
    gtk_box_pack_start(GTK_BOX(gui.box_left), gui.hs_align, TRUE, TRUE, 0);

    // ----------------

    gui.box_left_signal = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box_left), gui.box_left_signal);

    gui.graph = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(gui.box_left_signal), gui.graph, TRUE, TRUE, 0);
    // or
    gui.p_signal = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(gui.box_left_signal), gui.p_signal, TRUE, TRUE, 0);

    // ----------------

    gui.box_left_indicators = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gui.box_left), gui.box_left_indicators, FALSE, FALSE, 0);
    //gtk_container_add(GTK_CONTAINER(gui.box_left), gui.box_left_indicators);

    gui.event_st = gtk_event_box_new();
    gui.l_st = gtk_label_new("ST");
    gtk_container_add(GTK_CONTAINER(gui.event_st), gui.l_st);
    gtk_widget_modify_font(gui.l_st, font_status);
    gtk_widget_modify_fg(GTK_WIDGET(gui.l_st), GTK_STATE_NORMAL, &gui.colors.grey);
    gtk_misc_set_alignment(GTK_MISC(gui.l_st), 0, 0.5);
    gtk_widget_set_tooltip_text(gui.l_st, "19kHz stereo pilot indicator");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.event_st, TRUE, TRUE,  3);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(gui.event_st), FALSE);
    gtk_widget_set_events(gui.event_st, GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(gui.event_st, "enter-notify-event", G_CALLBACK(gui_cursor), gui.click_cursor);
    g_signal_connect(gui.event_st, "leave-notify-event", G_CALLBACK(gui_cursor), NULL);
    g_signal_connect(gui.event_st, "button-press-event", G_CALLBACK(gui_st_click), NULL);


    gui.l_rds = gtk_label_new("RDS");
    gtk_widget_modify_font(gui.l_rds, font_status);
    gtk_widget_modify_fg(GTK_WIDGET(gui.l_rds), GTK_STATE_NORMAL, &gui.colors.grey);
    gtk_misc_set_alignment(GTK_MISC(gui.l_rds), 0, 0.5);
    gtk_widget_set_tooltip_text(gui.l_rds, "RDS PI indicator");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_rds, TRUE, TRUE,  3);

    gui.l_tp = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_tp, font_status);
    gtk_widget_set_tooltip_text(gui.l_tp, "RDS Traffic Programme flag");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_tp, TRUE, TRUE, 3);

    gui.l_ta = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_ta, font_status);
    gtk_widget_set_tooltip_text(gui.l_ta, "RDS Traffic Announcement flag");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_ta, TRUE, TRUE, 3);

    gui.l_ms = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_ms, font_status);
    gtk_widget_set_tooltip_text(gui.l_ms, "RDS Music/Speech flag");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_ms, TRUE, TRUE, 3);

    gui.l_pty = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_pty, font_status);
    gtk_misc_set_alignment(GTK_MISC(gui.l_pty), 0.0, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(gui.l_pty), 8);
    gtk_widget_set_tooltip_text(gui.l_pty, "RDS Programme Type (PTY)");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_pty, TRUE, TRUE, 3);

    gui.l_sig = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_sig, font_status);
    gtk_misc_set_alignment(GTK_MISC(gui.l_sig), 1, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(gui.l_sig), 12);
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_sig, TRUE, TRUE, 2);
    gtk_widget_set_tooltip_text(gui.l_sig, "max / current signal level");
    g_signal_connect(gui.l_sig, "query-tooltip", G_CALLBACK(signal_tooltip), NULL);

    // ----------------

    gui.box_left_settings1 = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box_left), gui.box_left_settings1);

    gui.b_connect = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(gui.b_connect), gtk_image_new_from_stock(GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(gui.b_connect), FALSE);
    gtk_widget_set_name(gui.b_connect, "small-button");
    gtk_widget_set_tooltip_text(gui.b_connect, "Connect");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.b_connect, FALSE, FALSE, 0);
    g_signal_connect(gui.b_connect, "toggled", G_CALLBACK(connection_toggle), NULL);

    gui.b_pattern = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(gui.b_pattern), gtk_image_new_from_icon_name("xdr-gtk-pattern", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(gui.b_pattern), FALSE);
    gtk_widget_set_name(gui.b_pattern, "small-button");
    gtk_widget_set_tooltip_text(gui.b_pattern, "Antenna pattern");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.b_pattern, FALSE, FALSE, 0);
    g_signal_connect(gui.b_pattern, "clicked", G_CALLBACK(pattern_dialog), NULL);

    gui.b_settings = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(gui.b_settings), gtk_image_new_from_icon_name("xdr-gtk-settings", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(gui.b_settings), FALSE);
    gtk_widget_set_name(gui.b_settings, "small-button");
    gtk_widget_set_tooltip_text(gui.b_settings, "Settings");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.b_settings, FALSE, FALSE, 0);
    g_signal_connect(gui.b_settings, "clicked", G_CALLBACK(settings_dialog), NULL);

    gui.b_scheduler = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(gui.b_scheduler), gtk_image_new_from_icon_name("xdr-gtk-scheduler", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(gui.b_scheduler), FALSE);
    gtk_widget_set_name(gui.b_scheduler, "small-button");
    gtk_widget_set_tooltip_text(gui.b_scheduler, "Scheduler");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.b_scheduler, FALSE, FALSE, 0);
    g_signal_connect(gui.b_scheduler, "toggled", G_CALLBACK(scheduler_toggle), NULL);

    gui.b_rdsspy = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(gui.b_rdsspy), gtk_image_new_from_icon_name("xdr-gtk-rdsspy", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_focus_on_click(GTK_BUTTON(gui.b_rdsspy), FALSE);
    gtk_widget_set_name(gui.b_rdsspy, "small-button");
    gtk_widget_set_tooltip_text(gui.b_rdsspy, "RDS Spy Link");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.b_rdsspy, FALSE, FALSE, 0);
    g_signal_connect(gui.b_rdsspy, "toggled", G_CALLBACK(rdsspy_toggle), NULL);

    gui.b_ontop = gtk_toggle_button_new();
    gui.b_ontop_icon = gtk_image_new_from_icon_name("xdr-gtk-top", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(gui.b_ontop), gui.b_ontop_icon);
    gtk_button_set_focus_on_click(GTK_BUTTON(gui.b_ontop), FALSE);
    gtk_widget_set_name(gui.b_ontop, "small-button");
    gtk_widget_set_tooltip_text(gui.b_ontop, "Stay on top");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.b_ontop, FALSE, FALSE, 0);
    g_signal_connect(gui.b_ontop, "toggled", G_CALLBACK(window_on_top), NULL);

    gui.l_agc = gtk_label_new("AGC threshold:");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.l_agc, TRUE, TRUE, 0);

    gui.c_agc = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_agc), "highest");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_agc), "high");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_agc), "medium");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_agc), "low");
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_agc), conf.agc);
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.c_agc, TRUE, TRUE, 0);
    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(gui.c_agc), FALSE);
    g_signal_connect(G_OBJECT(gui.c_agc), "changed", G_CALLBACK(tuner_set_agc), NULL);

    gui.x_rf = gtk_check_button_new_with_label("RF+");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.x_rf, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_rf), conf.rfgain);
    g_signal_connect(gui.x_rf, "button-press-event", G_CALLBACK(gui_toggle_gain), NULL);

    // ----------------

    gui.box_left_settings2 = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box_left), gui.box_left_settings2);

    gui.l_deemph = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.l_deemph, FALSE, FALSE, 0);

    gui.c_deemph = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_deemph), "50 us");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_deemph), "75 us");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_deemph), "0 us");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.c_deemph, TRUE, TRUE, 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_deemph), conf.deemphasis);
    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(gui.c_deemph), FALSE);
    g_signal_connect(G_OBJECT(gui.c_deemph), "changed", G_CALLBACK(tuner_set_deemphasis), NULL);

    gui.ant = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
    gtk_list_store_insert_with_values(gui.ant, NULL, -1, 0, "Ant A", 1, (conf.ant_count>=1), -1);
    gtk_list_store_insert_with_values(gui.ant, NULL, -1, 0, "Ant B", 1, (conf.ant_count>=2), -1);
    gtk_list_store_insert_with_values(gui.ant, NULL, -1, 0, "Ant C", 1, (conf.ant_count>=3), -1);
    gtk_list_store_insert_with_values(gui.ant, NULL, -1, 0, "Ant D", 1, (conf.ant_count>=4), -1);
    GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(gui.ant), NULL);
    gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(filter), 1);

    gui.c_ant = gtk_combo_box_new_with_model(filter);
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(gui.c_ant), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(gui.c_ant), renderer, "text", 0, NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_ant), 0);
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.c_ant, TRUE, TRUE, 4);
    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(gui.c_ant), FALSE);
    g_signal_connect(G_OBJECT(gui.c_ant), "changed", G_CALLBACK(tuner_set_antenna), NULL);

    gui.b_cw = gtk_toggle_button_new();
    gui.b_cw_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(gui.b_cw_label), " <b>↻</b> ");
    gtk_container_add(GTK_CONTAINER(gui.b_cw), gui.b_cw_label);
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.b_cw, TRUE, TRUE, 0);
    gtk_widget_set_tooltip_text(gui.b_cw, "Rotate antenna clockwise");
    gtk_button_set_focus_on_click(GTK_BUTTON(gui.b_cw), FALSE);
    g_signal_connect(gui.b_cw, "realize", G_CALLBACK(gui_rotator_button_realized), NULL);
    g_signal_connect_swapped(gui.b_cw, "toggled", G_CALLBACK(tuner_set_rotator), GINT_TO_POINTER(1));

    gui.b_ccw = gtk_toggle_button_new();
    gui.b_ccw_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(gui.b_ccw_label), " <b>↺</b> ");
    gtk_container_add(GTK_CONTAINER(gui.b_ccw), gui.b_ccw_label);
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.b_ccw, TRUE, TRUE, 0);
    gtk_widget_set_tooltip_text(gui.b_ccw, "Rotate antenna counterclockwise");
    gtk_button_set_focus_on_click(GTK_BUTTON(gui.b_ccw), FALSE);
    g_signal_connect_swapped(gui.b_ccw, "toggled", G_CALLBACK(tuner_set_rotator), GINT_TO_POINTER(2));

    gui.l_bw = gtk_label_new("BW:");
    gtk_widget_set_tooltip_text(gui.l_bw, "FIR digital filter -3dB bandwidth");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.l_bw, TRUE, TRUE, 0);

    gui.c_bw = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.c_bw, TRUE, TRUE, 0);
    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(gui.c_bw), FALSE);
    g_signal_connect(G_OBJECT(gui.c_bw), "changed", G_CALLBACK(tuner_set_bandwidth), NULL);

    gui.x_if = gtk_check_button_new_with_label("IF+");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.x_if, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_if), conf.ifgain);
    g_signal_connect(gui.x_if, "button-press-event", G_CALLBACK(gui_toggle_gain), NULL);

    // ----------------

    gui.l_af = gtk_label_new("  AF:  ");
    gtk_widget_modify_font(gui.l_af, font_af);
    gtk_box_pack_start(GTK_BOX(gui.box_right), gui.l_af, FALSE, FALSE, 3);

    gui.af = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
    GtkWidget *view = gtk_tree_view_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "ID", gtk_cell_renderer_text_new(), "text", AF_LIST_STORE_ID, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "FREQ", gtk_cell_renderer_text_new(), "text", AF_LIST_STORE_FREQ, NULL);
    gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(view), AF_LIST_STORE_ID), FALSE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(gui.af));
    g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), "changed", G_CALLBACK(tune_gui_af), NULL);
    gtk_widget_set_can_focus(GTK_WIDGET(view), FALSE);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(gui.af), AF_LIST_STORE_ID, GTK_SORT_ASCENDING);
    g_object_unref(gui.af);

    gui.af_box = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gui.af_box), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(gui.af_box), view);
    gtk_box_pack_start(GTK_BOX(gui.box_right), gui.af_box, TRUE, TRUE, 0);

    // ----------------

    for(i=0; i<2; i++)
    {
        gui.event_rt[i] = gtk_event_box_new();
        gui.l_rt[i] = gtk_label_new(NULL);
        gtk_widget_modify_font(gui.l_rt[i], font_rt);
        gtk_misc_set_alignment(GTK_MISC(gui.l_rt[i]), 0.0, 0.5);
        gtk_label_set_width_chars(GTK_LABEL(gui.l_rt[i]), 66);
        gtk_container_add(GTK_CONTAINER(gui.event_rt[i]), gui.l_rt[i]);
        gtk_event_box_set_visible_window(GTK_EVENT_BOX(gui.event_rt[i]), FALSE);
        gtk_box_pack_start(GTK_BOX(gui.box), gui.event_rt[i], TRUE, TRUE, 0);
        g_signal_connect(gui.event_rt[i], "button-press-event", G_CALLBACK(clipboard_rt), tuner.rt[i]);
    }

    // ----------------

    gui.l_status = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(gui.box), gui.l_status, TRUE, TRUE, 0);

    pango_font_description_free(font_status);
    pango_font_description_free(font_header);
    pango_font_description_free(font_entry);
    pango_font_description_free(font_af);
    pango_font_description_free(font_rt);

    gui_mode_FM();
    tuner.online = 0;
    tuner.guest = FALSE;
    tuner.freq = conf.init_freq;
    tuner.thread = FALSE;
    tuner.ready = FALSE;
    tuner.filter = -1;

    pattern.window = NULL;
    scan_init();

    gui_clear(NULL);
    gtk_widget_show_all(gui.window);
    if(!conf.alignment)
    {
        gtk_widget_hide(gui.hs_align);
    }
    if(conf.hide_status)
    {
        gtk_widget_hide(gui.l_status);
    }

    gui_antenna_showhide();

    g_signal_connect(gui.window, "destroy", G_CALLBACK(gui_destroy), NULL);
    g_signal_connect(gui.window, "delete-event", G_CALLBACK(gui_delete_event), NULL);
    g_signal_connect(gui.window, "key-press-event", G_CALLBACK(keyboard_press), NULL);
    g_signal_connect(gui.window, "key-release-event", G_CALLBACK(keyboard_release), NULL);
    g_signal_connect(gui.window, "button-press-event", G_CALLBACK(window_click), GTK_WINDOW(gui.window));
    gtk_widget_add_events(GTK_WIDGET(gui.window), GDK_CONFIGURE);
    g_signal_connect(gui.window, "configure-event", G_CALLBACK(gui_window_event), NULL);
    gui.status_timeout = g_timeout_add(1000, (GSourceFunc)gui_update_clock, (gpointer)gui.l_status);
    graph_init();

    if(conf.autoconnect)
    {
        connection_dialog(TRUE);
    }
}

void gui_destroy()
{
    if(tuner.thread)
    {
        tuner_poweroff();
        g_usleep(25000);
        tuner.thread = 0;
        g_usleep(25000);
    }
    gtk_main_quit();
}

gboolean gui_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    GtkWidget *dialog;
    gint response;
    settings_write();
    if(!tuner.thread || !conf.disconnect_confirm)
        return FALSE;

    dialog = gtk_message_dialog_new(GTK_WINDOW(gui.window),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO,
                                    "Tuner is currently connected.\nAre you sure you want to quit?");
    gtk_window_set_title(GTK_WINDOW(dialog), APP_NAME);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return (response != GTK_RESPONSE_YES);
}

void gui_clear()
{
    gtk_label_set_text(GTK_LABEL(gui.l_freq), " ");
    gtk_label_set_text(GTK_LABEL(gui.l_sig),  " ");
    gui_clear_rds();
}

gboolean gui_clear_rds()
{
    gint i;

    gtk_label_set_text(GTK_LABEL(gui.l_pi),  " ");
    gtk_label_set_text(GTK_LABEL(gui.l_ps),  " ");
    gtk_label_set_text(GTK_LABEL(gui.l_pty), " ");
    gtk_label_set_text(GTK_LABEL(gui.l_tp),  "  ");
    gtk_label_set_text(GTK_LABEL(gui.l_ta),  "  ");
    gtk_label_set_text(GTK_LABEL(gui.l_ms),  "  ");
    gtk_label_set_text(GTK_LABEL(gui.l_rt[0]), " ");
    gtk_label_set_text(GTK_LABEL(gui.l_rt[1]), " ");
    gtk_list_store_clear(GTK_LIST_STORE(gui.af));
    gtk_widget_modify_fg(GTK_WIDGET(gui.l_st), GTK_STATE_NORMAL, &gui.colors.grey);
    gtk_widget_modify_fg(GTK_WIDGET(gui.l_rds), GTK_STATE_NORMAL, &gui.colors.grey);

    s.stereo = FALSE;
    s.rds = FALSE;
    s.rds_reset_timer = -1;

    tuner.pi = tuner.pi_checked = tuner.pty = tuner.tp = tuner.ta = tuner.ms = tuner.ecc = -1;
    tuner.ps_avail = FALSE;
    tuner.rds_timer = 0;

    g_sprintf(tuner.ps, "%8s", "");
    for(i=0; i<8; i++)
    {
        tuner.ps_err[i] = 0xFF;
    }

    g_sprintf(tuner.rt[0], "%64s", "");
    g_sprintf(tuner.rt[1], "%64s", "");

    return FALSE;
}

gboolean gui_mode_FM()
{
    tuner.mode = MODE_FM;
    gtk_label_set_text(GTK_LABEL(gui.l_band), "FM");
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tuner_set_bandwidth), NULL);
    gui_fill_bandwidths(gui.c_bw, TRUE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_bw), 29);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tuner_set_bandwidth), NULL);
    gtk_widget_set_sensitive(gui.c_deemph, TRUE);
    return FALSE;
}

gboolean gui_mode_AM()
{
    tuner.mode = MODE_AM;
    tuner.rds_timer = 0;
    gtk_label_set_text(GTK_LABEL(gui.l_band), "AM");
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tuner_set_bandwidth), NULL);
    gui_fill_bandwidths(gui.c_bw, FALSE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_bw), 16);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tuner_set_bandwidth), NULL);
    gtk_widget_set_sensitive(gui.c_deemph, FALSE);
    return FALSE;
}

void gui_fill_bandwidths(GtkWidget* combo, gboolean auto_mode)
{
    GtkTreeModel *store = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    gtk_list_store_clear(GTK_LIST_STORE(store));

    switch(tuner.mode)
    {
    case MODE_FM:
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "309 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "298 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "281 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "263 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "246 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "229 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "211 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "194 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "177 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "159 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "142 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "125 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "108 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "95 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "90 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "83 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "73 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "63 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "55 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "48 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "42 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "36 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "32 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "27 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "24 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "20 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "17 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "15 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "9 kHz");
        break;

    case MODE_AM:
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "38.6kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "37.3kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "35.1kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "32.9kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "30.8kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "28.6kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "26.4kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "24.3kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "22.1kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "19.9kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "17.8kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "15.6kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "13.5kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "11.8kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "11.3kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "10.4kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "9.1 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "7.9 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "6.9 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "6.0 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "5.2 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "4.6 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "3.9 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "3.4 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "2.9 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "2.5 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "2.2 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "1.9 kHz");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "1.1 kHz");
        break;
    }

    if(auto_mode)
    {
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "auto");
    }
}

void tune_gui_back(GtkWidget *widget, gpointer nothing)
{
    tuner_set_frequency(tuner.prevfreq);
}

void tune_gui_round(GtkWidget *widget, gpointer nothing)
{
    tuner_modify_frequency(FREQ_MODIFY_RESET);
}

void tune_gui_step_click(GtkWidget *widget, GdkEventButton *event, gpointer step)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) // right click, tune down
    {
        tuner_set_frequency(tuner.freq-(GPOINTER_TO_INT(step)));
    }
    else if(event->type == GDK_BUTTON_PRESS && event->button == 1) // left click, tune up
    {
        tuner_set_frequency(tuner.freq+(GPOINTER_TO_INT(step)));
    }
}

void tune_gui_step_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer step)
{
    if(event->direction)
    {
        tuner_set_frequency(tuner.freq-(GPOINTER_TO_INT(step)));
    }
    else
    {
        tuner_set_frequency(tuner.freq+(GPOINTER_TO_INT(step)));
    }
}

gboolean gui_toggle_gain(GtkWidget *widget, GdkEventButton *event, gpointer nothing)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) // right click, toggle both RF and IF
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_rf), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.x_rf)));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_if), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.x_if)));
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


gboolean gui_update_clock(gpointer label)
{
    gchar buff[20], buff2[100];
    time_t tt = time(NULL);
    strftime(buff, sizeof(buff), "%d-%m-%Y %H:%M:%S", (conf.utc ? gmtime(&tt) : localtime(&tt)));

    // network connection
    if(tuner.online)
    {
        if(tuner.guest)
        {
            g_snprintf(buff2, sizeof(buff2), "Online: %d  |  %s %s  <i>(guest)</i>", tuner.online, buff, (conf.utc ? "UTC" : "LT"));
        }
        else
        {
            g_snprintf(buff2, sizeof(buff2), "Online: %d  |  %s %s", tuner.online, buff, (conf.utc ? "UTC" : "LT"));
        }
    }
    else
    {
        g_snprintf(buff2, sizeof(buff2), "%s %s", buff, (conf.utc ? "UTC" : "LT"));
    }

    gtk_label_set_markup(GTK_LABEL(label), buff2);

    // reset RDS data after timeout
    if(conf.rds_reset && s.rds_reset_timer != -1)
    {
        if((g_get_real_time() - s.rds_reset_timer) > (conf.rds_reset_timeout*1000000))
        {
            gui_clear_rds();
        }
    }

    gui.status_timeout = g_timeout_add(1000, (GSourceFunc)gui_update_clock, (gpointer)gui.l_status);
    return FALSE;
}

void tune_gui_af(GtkTreeSelection *ts, gpointer nothing)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint n;

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

void dialog_error(gchar* title, gchar* format, ...)
{
    GtkWidget* dialog;
    va_list args;
    gchar *msg;

    va_start(args, format);
    msg = g_strdup_vprintf(format, args);
    va_end(args);
    dialog = gtk_message_dialog_new(GTK_WINDOW(gui.window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, msg);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
#ifdef G_OS_WIN32
    win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    gtk_dialog_run(GTK_DIALOG(dialog));
#endif
    gtk_widget_destroy(dialog);
    g_free(msg);
}

gchar* s_meter(gfloat val)
{
    gint s, plus = 0;

    if(val >= 85.2)
    {
        s = 9;
        plus = 40;
    }
    if(val >= 75.2)
    {
        s = 9;
        plus = 30;
    }
    else if(val >= 65.2)
    {
        s = 9;
        plus = 20;
    }
    else if(val >= 55.2)
    {
        s = 9;
        plus = 10;
    }
    else if(val >= 45.2)
    {
        s = 9;
    }
    else if(val >= 39.2)
    {
        s = 8;
    }
    else if(val >= 33.2)
    {
        s = 7;
    }
    else if(val >= 27.2)
    {
        s = 6;
    }
    else if(val >= 21.2)
    {
        s = 5;
    }
    else if(val >= 15.2)
    {
        s = 4;
    }
    else if(val >= 9.23)
    {
        s = 3;
    }
    else if(val >= 3.23)
    {
        s = 2;
    }
    else if(val >= -2.77)
    {
        s = 1;
    }
    else
    {
        s = 0;
    }

    if(plus)
    {
        return g_strdup_printf("S%d+%d", s, plus);
    }
    else
    {
        return g_strdup_printf("S%d", s);
    }
}

void window_on_top(GtkToggleButton *item)
{
    gboolean state = gtk_toggle_button_get_active(item);
    gtk_window_set_keep_above(GTK_WINDOW(gui.window), state);
}

void connect_button(gboolean is_active)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.b_connect), GINT_TO_POINTER(connection_toggle), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_connect), is_active);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.b_connect), GINT_TO_POINTER(connection_toggle), NULL);
    gtk_widget_set_tooltip_text(gui.b_connect, (is_active ? "Disconnect" : "Connect"));
}

void gui_antenna_switch(gint newfreq)
{
    gint i;
    if(conf.ant_switching)
    {
        for(i=0; i<conf.ant_count; i++)
        {
            if(newfreq >= conf.ant_start[i] && newfreq <= conf.ant_stop[i])
            {
                if(gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_ant)) == i)
                {
                    break;
                }
                gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_ant), i);
                break;
            }
        }
    }
}

void gui_toggle_band(GtkWidget *widget, GdkEventButton *event, gpointer step)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
        if(tuner.mode != MODE_FM)
        {
            gui_mode_FM();
            tuner_set_mode();
        }
        else
        {
            gui_mode_AM();
            tuner_set_mode();
        }
    }
}

void gui_st_click(GtkWidget *widget, GdkEventButton *event, gpointer step)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
        if(tuner.mode == MODE_FM)
        {
            tuner_st_test();
        }
    }
}

gboolean gui_cursor(GtkWidget *widget, GdkEvent  *event, gpointer cursor)
{
    gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
    return FALSE;
}

void gui_antenna_showhide()
{
    GtkTreeIter iter;

    if(conf.ant_count == 1)
    {
        gtk_widget_hide(gui.c_ant);
        gtk_label_set_text(GTK_LABEL(gui.l_deemph), "De-emphasis:");
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(gui.l_deemph), "");
        gtk_widget_show(gui.c_ant);
    }

    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gui.ant), &iter))
    {
        gtk_list_store_set(gui.ant, &iter, 1, (conf.ant_count>=1), -1);
    }

    if(gtk_tree_model_iter_next(GTK_TREE_MODEL(gui.ant), &iter))
    {
        gtk_list_store_set(gui.ant, &iter, 1, (conf.ant_count>=2), -1);
    }

    if(gtk_tree_model_iter_next(GTK_TREE_MODEL(gui.ant), &iter))
    {
        gtk_list_store_set(gui.ant, &iter, 1, (conf.ant_count>=3), -1);
    }

    if(gtk_tree_model_iter_next(GTK_TREE_MODEL(gui.ant), &iter))
    {
        gtk_list_store_set(gui.ant, &iter, 1, (conf.ant_count>=4), -1);
    }
}

gboolean signal_tooltip(GtkWidget *label, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data)
{
    gdouble avg;
    gchar *str, *s_m;
    if(!s.samples)
    {
        return FALSE;
    }
    avg = s.sum / (gdouble)s.samples;
    if(tuner.mode == MODE_FM)
    {
        switch(conf.signal_unit)
        {
        case UNIT_DBM:
            str = g_markup_printf_escaped("average signal: <b>%.1f dBm</b> (%d samples)", avg-120, s.samples);
            break;

        case UNIT_DBUV:
            str = g_markup_printf_escaped("average signal: <b>%.1f dBµV</b> (%d samples)", avg-11.25, s.samples);
            break;

        case UNIT_S:
            s_m = s_meter(avg);
            str = g_markup_printf_escaped("average signal: <b>%s</b> (%d samples)", s_m, s.samples);
            g_free(s_m);
            break;

        case UNIT_DBF:
        default:
            str = g_markup_printf_escaped("average signal: <b>%.1f dBf</b> (%d samples)", avg, s.samples);
            break;
        }
    }
    else
    {
        str = g_markup_printf_escaped("average signal: <b>%.1f</b> (%d samples)", avg, s.samples);
    }
    gtk_tooltip_set_markup(tooltip, str);
    g_free(str);
    return TRUE;
}

void gui_toggle_ps_mode()
{
    conf.rds_ps_progressive = !conf.rds_ps_progressive;
    if(tuner.ps_avail)
    {
        gui_update_ps(NULL);
    }
}

void gui_screenshot()
{
    static gchar* default_path = "./screenshots";
    gchar t[20], *filename;
    GdkPixmap *pixmap;
    GdkPixbuf *pixbuf;
    gint width, height;
    gchar* directory;
    time_t tt = time(NULL);
    GError* err = NULL;
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

    if(tuner.pi != -1)
    {
        filename = g_strdup_printf("%s/%s-%d-%04X.png", directory, t, tuner.freq, tuner.pi);
    }
    else
    {
        filename = g_strdup_printf("%s/%s-%d.png", directory, t, tuner.freq);
    }

    /* HACK: refresh window to avoid icons disappearing */
    gtk_widget_queue_draw(gui.window);
    while(gtk_events_pending())
    {
        gtk_main_iteration();
    }

    pixmap = gtk_widget_get_snapshot(gui.window, NULL);
    gdk_pixmap_get_size(pixmap, &width, &height);
    pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 1, 1, 0, 0, width-2, height-2);
    if(!gdk_pixbuf_save(pixbuf, filename, "png", &err, NULL))
    {
        dialog_error("Screenshot", "%s\n\nCheck selected screenshots directory in settings.", err->message);
        g_error_free(err);
    }
    g_object_unref(G_OBJECT(pixmap));
    g_object_unref(G_OBJECT(pixbuf));
    g_free(filename);
}

void gui_activate()
{
#ifdef G_OS_WIN32
    win32_grab_focus(GTK_WINDOW(gui.window));
#else
    gtk_window_present(GTK_WINDOW(gui.window));
#endif
}

gboolean gui_window_event(GtkWidget* widget, gpointer data)
{
    gtk_window_get_position(GTK_WINDOW(gui.window), &conf.win_x, &conf.win_y);
    return FALSE;
}

void gui_rotator_button_realized(GtkWidget *widget)
{
    GtkStyle *style = gtk_widget_get_style(widget);
    if(style)
    {
        gui.colors.prelight = style->bg[GTK_STATE_PRELIGHT];
        gtk_widget_modify_bg(gui.b_cw, GTK_STATE_ACTIVE, &gui.colors.prelight);
        gtk_widget_modify_bg(gui.b_ccw, GTK_STATE_ACTIVE, &gui.colors.prelight);
    }
}

void gui_rotator_button_swap()
{
    if(conf.swap_rotator)
    {
        gtk_box_reorder_child(GTK_BOX(gui.box_left_settings2), gui.b_cw, 4);
        gtk_box_reorder_child(GTK_BOX(gui.box_left_settings2), gui.b_ccw, 3);
    }
    else
    {
        gtk_box_reorder_child(GTK_BOX(gui.box_left_settings2), gui.b_cw, 3);
        gtk_box_reorder_child(GTK_BOX(gui.box_left_settings2), gui.b_ccw, 4);
    }
}

void gui_status(gint duration, gchar* format, ...)
{
    va_list args;
    gchar *msg;
    va_start(args, format);
    msg = g_strdup_vprintf(format, args);
    va_end(args);

    g_source_remove(gui.status_timeout);
    gtk_label_set_markup(GTK_LABEL(gui.l_status), msg);
    gui.status_timeout = g_timeout_add(duration, (GSourceFunc)gui_update_clock, (gpointer)gui.l_status);

    g_free(msg);
}
