#include <gtk/gtk.h>
#include <string.h>
#include <glib/gprintf.h>
#include <math.h>
#include "gui.h"
#include "connection.h"
#include "graph.h"
#include "menu.h"
#include "settings.h"
#include "clipboard.h"
#include "keyboard.h"

void gui_init()
{
    gdk_color_parse("#FFFFFF", &gui.colors.background);
    gdk_color_parse("#DDDDDD", &gui.colors.grey);
    gdk_color_parse("#000000", &gui.colors.black);
    gdk_color_parse("#EE4000", &gui.colors.stereo);

    gui.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(gui.window), 3);
    gtk_window_set_title(GTK_WINDOW(gui.window), "XDR-GTK");
    gtk_window_set_resizable(GTK_WINDOW(gui.window), FALSE);
    gtk_widget_modify_bg(gui.window, GTK_STATE_NORMAL, &gui.colors.background);
    gui.clipboard = gtk_widget_get_clipboard(gui.window, GDK_SELECTION_CLIPBOARD);

    gui.box = gtk_vbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(gui.window), gui.box);

    gui.box_header = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box), gui.box_header);

    gui.box_gui = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box), gui.box_gui);

    gui.box_left = gtk_vbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(gui.box_gui), gui.box_left);

    gui.box_right = gtk_vbox_new(FALSE, 1);
    gtk_container_add(GTK_CONTAINER(gui.box_gui), gui.box_right);

    // ----------------

    gui.volume = gtk_volume_button_new();
    gui.volume_adj = gtk_scale_button_get_adjustment(GTK_SCALE_BUTTON(gui.volume));
    gtk_adjustment_configure(gui.volume_adj, 1, 0, 1, 0.1, 0.1, 0);
    gtk_widget_set_can_focus(GTK_WIDGET(gui.volume), FALSE);
    g_signal_connect(gui.volume, "value-changed", G_CALLBACK(tty_change_volume), NULL);
    g_signal_connect(gui.volume, "button-press-event", G_CALLBACK(volume_click), NULL);
#ifndef G_OS_WIN32
    g_object_set(G_OBJECT(gui.volume), "size", GTK_ICON_SIZE_MENU, NULL);
#endif
    gtk_container_add(GTK_CONTAINER(gui.box_header), gui.volume);

    gui.event_band = gtk_event_box_new();
    gui.l_band = gtk_label_new(NULL);
    gtk_container_add(GTK_CONTAINER(gui.event_band), gui.l_band);
    gtk_widget_modify_font(gui.l_band, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 20"));
    gtk_box_pack_start(GTK_BOX(gui.box_header), gui.event_band, TRUE, FALSE, 5);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(gui.event_band), FALSE);
    g_signal_connect(gui.event_band, "button-press-event", G_CALLBACK(gui_mode_toggle), NULL);

    gui.event_freq = gtk_event_box_new();
    gui.l_freq = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_freq, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 20"));
    gtk_container_add(GTK_CONTAINER(gui.event_freq), gui.l_freq);
    gtk_box_pack_start(GTK_BOX(gui.box_header), gui.event_freq, TRUE, FALSE, 8);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(gui.event_freq), FALSE);
    g_signal_connect(gui.event_freq, "button-press-event", G_CALLBACK(clipboard_full), NULL);

    gui.event_pi = gtk_event_box_new();
    gui.l_pi = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_pi, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 20"));
    gtk_container_add(GTK_CONTAINER(gui.event_pi), gui.l_pi);
    gtk_box_pack_start(GTK_BOX(gui.box_header), gui.event_pi, TRUE, FALSE, 12);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(gui.event_pi), FALSE);
    g_signal_connect(gui.event_pi, "button-press-event", G_CALLBACK(clipboard_pi), NULL);

    gui.event_ps = gtk_event_box_new();
    gui.l_ps = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_ps, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 20"));
    gtk_container_add(GTK_CONTAINER(gui.event_ps), gui.l_ps);
    gtk_box_pack_start(GTK_BOX(gui.box_header), gui.event_ps, TRUE, FALSE, 5);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(gui.event_ps), FALSE);
    g_signal_connect(gui.event_ps, "button-press-event", G_CALLBACK(clipboard_str), ps_data);

    // ----------------

    gui.box_left_tune = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box_left), gui.box_left_tune);

    gui.b_tune_back = gtk_button_new_with_label(" B ");
    gtk_box_pack_start(GTK_BOX(gui.box_left_tune), gui.b_tune_back, TRUE, TRUE, 0);
    g_signal_connect(gui.b_tune_back, "button-press-event", G_CALLBACK(tune_gui_back), NULL);
    gtk_widget_set_tooltip_text(gui.b_tune_back, "Tune back to the previous frequency");

    gui.e_freq = gtk_entry_new_with_max_length(7);
    gtk_entry_set_width_chars(GTK_ENTRY(gui.e_freq), 10);
    gtk_widget_modify_font(gui.e_freq, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 10"));
    gtk_box_pack_start(GTK_BOX(gui.box_left_tune), gui.e_freq, FALSE, FALSE, 0);

    gui.b_tune_reset = gtk_button_new_with_label("R");
    gtk_widget_add_events(gui.b_tune_reset, GDK_SCROLL_MASK);
    gtk_box_pack_start(GTK_BOX(gui.box_left_tune), gui.b_tune_reset, TRUE, TRUE, 0);
    g_signal_connect(gui.b_tune_reset, "button-press-event", G_CALLBACK(tune_gui_round), NULL);
    gtk_widget_set_tooltip_text(gui.b_tune_reset, "Round to the nearest 100kHz");

    gchar label[5];
    gint i;
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

    gui.box_left_signal = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box_left), gui.box_left_signal);

    gui.graph = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(gui.box_left_signal), gui.graph, TRUE, TRUE, 0);
    // or
    gui.p_signal = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(gui.box_left_signal), gui.p_signal, TRUE, TRUE, 0);

    // ----------------

    gui.box_left_indicators = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box_left), gui.box_left_indicators);

    gui.l_st = gtk_label_new("ST");
    gtk_widget_modify_font(gui.l_st, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 16"));
    gtk_widget_modify_fg(GTK_WIDGET(gui.l_st), GTK_STATE_NORMAL, &gui.colors.grey);
    gtk_misc_set_alignment(GTK_MISC(gui.l_st), 0, 0);
    gtk_widget_set_tooltip_text(gui.l_st, "19kHz stereo subcarrier indicator (>4kHz injection)");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_st, TRUE, TRUE,  3);

    gui.l_rds = gtk_label_new("RDS");
    gtk_widget_modify_font(gui.l_rds, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 16"));
    gtk_widget_modify_fg(GTK_WIDGET(gui.l_rds), GTK_STATE_NORMAL, &gui.colors.grey);
    gtk_misc_set_alignment(GTK_MISC(gui.l_rds), 0, 0);
    gtk_widget_set_tooltip_text(gui.l_rds, "RDS PI indicator (timeout configurable in settings)");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_rds, TRUE, TRUE,  3);

    gui.l_tp = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_tp, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 16"));
    gtk_widget_set_tooltip_text(gui.l_tp, "RDS Traffic Programme flag");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_tp, TRUE, TRUE, 3);

    gui.l_ta = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_ta, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 16"));
    gtk_widget_set_tooltip_text(gui.l_ta, "RDS Traffic Announcement flag");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_ta, TRUE, TRUE, 3);

    gui.l_ms = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_ms, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 16"));
    gtk_widget_set_tooltip_text(gui.l_ms, "RDS Music/Speech flag");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_ms, TRUE, TRUE, 3);

    gui.l_pty = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_pty, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 14"));
    gtk_misc_set_alignment(GTK_MISC(gui.l_pty), 1, 0.5);
    gtk_widget_set_tooltip_text(gui.l_pty, "RDS Programme Type (PTY)");
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_pty, TRUE, TRUE, 3);

    gui.l_sig = gtk_label_new(NULL);
    gtk_widget_modify_font(gui.l_sig, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 14"));
    gtk_misc_set_alignment(GTK_MISC(gui.l_sig), 1, 0.5);
    gtk_box_pack_start(GTK_BOX(gui.box_left_indicators), gui.l_sig, TRUE, TRUE, 2);
    gtk_widget_set_tooltip_text(gui.l_sig, "max / current signal level");

    // ----------------

    gui.box_left_settings1 = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box_left), gui.box_left_settings1);

    gui.menu = menu_create();
    gui.b_menu = gtk_button_new_with_label("Menu");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.b_menu, TRUE, TRUE, 0);
    g_signal_connect_swapped(gui.b_menu, "event", G_CALLBACK(menu_popup), gui.menu);

    gui.c_ant = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_ant), "Ant A");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_ant), "Ant B");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_ant), "Ant C");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_ant), "Ant D");
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_ant), 0);
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.c_ant, TRUE, TRUE, 2);
    g_signal_connect(G_OBJECT(gui.c_ant), "changed", G_CALLBACK(tty_change_ant), NULL);

    gui.l_agc = gtk_label_new("AGC threshold:");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.l_agc, TRUE, TRUE, 0);

    gui.c_agc = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_agc), "highest");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_agc), "high");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_agc), "medium");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_agc), "low");
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_agc), conf.agc);
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.c_agc, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(gui.c_agc), "changed", G_CALLBACK(tty_change_agc), NULL);

    gui.x_rf = gtk_check_button_new_with_label("RF +6dB");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings1), gui.x_rf, TRUE, TRUE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_rf), conf.rfgain);
    g_signal_connect(gui.x_rf, "button-press-event", G_CALLBACK(gui_toggle_gain), NULL);

    // ----------------

    gui.box_left_settings2 = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gui.box_left), gui.box_left_settings2);

    gui.l_deemph = gtk_label_new("De-emphasis:");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.l_deemph, TRUE, TRUE, 0);

    gui.c_deemph = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_deemph), "50 us");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_deemph), "75 us");
    gtk_combo_box_append_text(GTK_COMBO_BOX(gui.c_deemph), "off");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.c_deemph, TRUE, TRUE, 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_deemph), conf.deemphasis);
    g_signal_connect(G_OBJECT(gui.c_deemph), "changed", G_CALLBACK(tty_change_deemphasis), NULL);

    gui.l_bw = gtk_label_new("Bandwidth:");
    gtk_widget_set_tooltip_text(gui.l_bw, "FIR digital filter -3dB bandwidth");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.l_bw, TRUE, TRUE, 0);

    gui.c_bw = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.c_bw, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(gui.c_bw), "changed", G_CALLBACK(tty_change_bandwidth), NULL);

    gui.x_if = gtk_check_button_new_with_label("IF +6dB");
    gtk_box_pack_start(GTK_BOX(gui.box_left_settings2), gui.x_if, TRUE, TRUE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_if), conf.ifgain);
    g_signal_connect(gui.x_if, "button-press-event", G_CALLBACK(gui_toggle_gain), NULL);

    // ----------------

    gui.l_af = gtk_label_new("  AF:  ");
    gtk_widget_modify_font(gui.l_af, pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 11"));
    gtk_box_pack_start(GTK_BOX(gui.box_right), gui.l_af, FALSE, FALSE, 3);

    gui.af = gtk_list_store_new(1, G_TYPE_STRING);
    GtkWidget *view = gtk_tree_view_new();
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "AF", renderer, "text", 0, NULL);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(gui.af));
    g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), "changed", G_CALLBACK(tune_gui_af), NULL);
    gtk_widget_set_can_focus(GTK_WIDGET(view), FALSE);
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
        gtk_widget_modify_font(gui.l_rt[i], pango_font_description_from_string("Bitstream Vera Sans Mono, monospace 9"));
        gtk_misc_set_alignment(GTK_MISC(gui.l_rt[i]), 0.0, 0.5);
        gtk_label_set_width_chars(GTK_LABEL(gui.l_rt[i]), 66);
        gtk_container_add(GTK_CONTAINER(gui.event_rt[i]), gui.l_rt[i]);
        gtk_event_box_set_visible_window(GTK_EVENT_BOX(gui.event_rt[i]), FALSE);
        gtk_box_pack_start(GTK_BOX(gui.box), gui.event_rt[i], TRUE, TRUE, 0);
        g_signal_connect(gui.event_rt[i], "button-press-event", G_CALLBACK(clipboard_str), rt_data[i]);
    }

    // ----------------

    gui.l_status = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(gui.box), gui.l_status, TRUE, TRUE, 0);

    if(conf.mode == MODE_FM)
    {
        gui_mode_FM();
    }
    else if(conf.mode == MODE_AM)
    {
        gui_mode_AM();
    }

    gui_clear(NULL);
    gtk_widget_show_all(gui.window);
    g_signal_connect(gui.window, "destroy", G_CALLBACK(gui_quit), NULL);
    g_signal_connect(gui.window, "key-press-event", G_CALLBACK(keyboard), NULL);
    gui.status_timeout = g_timeout_add(1000, (GSourceFunc)gui_update_clock, (gpointer)gui.l_status);
    graph_init();
}

void gui_quit()
{
    if(thread)
    {
        xdr_write("X");
        g_usleep(25000);
        thread = 0;
        g_usleep(25000);
    }
    gtk_main_quit();
}

void dialog_error(gchar* msg)
{
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(gui.window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

gboolean gui_update_status(gpointer nothing)
{
    if(stereo != rssi[rssi_pos].stereo)
    {
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_st), GTK_STATE_NORMAL, rssi[rssi_pos].stereo?&gui.colors.stereo:&gui.colors.grey);
        stereo = !stereo;
    }

    if(rds != rssi[rssi_pos].rds)
    {
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_rds), GTK_STATE_NORMAL, rssi[rssi_pos].rds?&conf.color_rds:&gui.colors.grey);
        rds = !rds;
    }

    if((rssi_pos % 3) == 0) // slower signal level label refresh
    {
        gchar *s, *s_m, *s_m2;
        if(conf.mode == MODE_FM)
        {
            switch(conf.signal_unit)
            {
            case UNIT_DBM:
                s = g_markup_printf_escaped("<span color=\"#777777\">%4.0f/</span>%4.0fdBm", max_signal-120, rssi[rssi_pos].value-120);
                break;

            case UNIT_DBUV:
                s = g_markup_printf_escaped("<span color=\"#777777\">%3.0f/</span>%3.0f dBuV", max_signal-11.25, rssi[rssi_pos].value-11.25);
                break;

            case UNIT_S:
                s_m = s_meter(max_signal);
                s_m2 = s_meter(rssi[rssi_pos].value);
                s = g_markup_printf_escaped("<span color=\"#777777\"> %5s/</span>%5s", s_m, s_m2);
                g_free(s_m);
                g_free(s_m2);
                break;

            case UNIT_DBF:
            default:
                s = g_markup_printf_escaped("<span color=\"#777777\"> %3.0f/</span>%3.0f dBf", max_signal, rssi[rssi_pos].value);
                break;
            }
        }
        else
        {
            s = g_markup_printf_escaped("<span color=\"#777777\">     %3.0f/</span>%3.0f", max_signal, rssi[rssi_pos].value);
        }
        gtk_label_set_markup(GTK_LABEL(gui.l_sig), s);
        g_free(s);
    }
    if(conf.signal_display == SIGNAL_GRAPH)
    {
        gtk_widget_queue_draw(gui.graph);
    }
    else if(conf.signal_display == SIGNAL_BAR)
    {
        gfloat sig = (rssi[rssi_pos].value>75?75:rssi[rssi_pos].value);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gui.p_signal), (sig/75.0));
    }
    return FALSE;
}

gboolean gui_clear(gpointer freq)
{
    if(freq == NULL)
    {
        gtk_label_set_text(GTK_LABEL(gui.l_freq), "       ");
        gtk_label_set_text(GTK_LABEL(gui.l_sig), "            ");
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(gui.l_freq), (gchar*)freq);
        g_free(freq);
    }
    gui_clear_rds();

    return FALSE;
}

void gui_clear_rds()
{
    gtk_label_set_text(GTK_LABEL(gui.l_pi), "     ");
    gtk_label_set_text(GTK_LABEL(gui.l_ps), "          ");
    gtk_label_set_text(GTK_LABEL(gui.l_pty), "        ");
    gtk_label_set_text(GTK_LABEL(gui.l_tp), "  ");
    gtk_label_set_text(GTK_LABEL(gui.l_ta), "  ");
    gtk_label_set_text(GTK_LABEL(gui.l_ms), "  ");

    gtk_label_set_text(GTK_LABEL(gui.l_rt[0]), " ");
    gtk_label_set_text(GTK_LABEL(gui.l_rt[1]), " ");

    g_sprintf(ps_data, "%8s", "");
    g_sprintf(rt_data[0], "%64s", "");
    g_sprintf(rt_data[1], "%64s", "");

    gtk_list_store_clear(GTK_LIST_STORE(gui.af));

    gtk_widget_modify_fg(GTK_WIDGET(gui.l_st), GTK_STATE_NORMAL, &gui.colors.grey);
    stereo = FALSE;

    gtk_widget_modify_fg(GTK_WIDGET(gui.l_rds), GTK_STATE_NORMAL, &gui.colors.grey);
    rds = FALSE;

    rds_timer = 0;
    pi = prevpi = prevpty = prevtp = prevta = prevms = rds_reset_timer = -1;
    ps_available = FALSE;
}

gboolean gui_update_ps(gpointer nothing)
{
    gchar *markup = g_markup_printf_escaped("<span color=\"#C8C8C8\">[</span>%s<span color=\"#C8C8C8\">]</span>", ps_data);
    gtk_label_set_markup(GTK_LABEL(gui.l_ps), markup);
    g_free(markup);
    return FALSE;
}

gboolean gui_update_rt(gpointer flag)
{
    gchar *markup = g_markup_printf_escaped("<span color=\"#C8C8C8\">[</span>%s<span color=\"#C8C8C8\">]</span>", rt_data[GPOINTER_TO_INT(flag)]);
    gtk_label_set_markup(GTK_LABEL(gui.l_rt[GPOINTER_TO_INT(flag)]), markup);
    g_free(markup);
    return FALSE;
}

gboolean gui_update_pi(gpointer isOK)
{
    gchar pi_text[6];
    if(GPOINTER_TO_INT(isOK))
        g_snprintf(pi_text, 6, "%04X ", pi);
    else
        g_snprintf(pi_text, 6, "%04X?", pi);
    if(pi != -1)
    {
        gtk_label_set_text(GTK_LABEL(gui.l_pi), pi_text);
    }
    return FALSE;
}

gboolean gui_update_ptytp(gpointer nothing)
{
    static gchar* ptys_eu[] = {"None", "News", "Affairs", "Info", "Sport", "Educate", "Drama", "Culture", "Science", "Varied", "Pop M", "Rock M", "Easy M", "Light M", "Classics", "Other M", "Weather", "Finance", "Children", "Social", "Religion", "Phone In", "Travel", "Leisure", "Jazz", "Country", "Nation M", "Oldies", "Folk M", "Document", "TEST", "Alarm !"};
    static gchar* ptys_usa[] = {"None", "News", "Inform", "Sports", "Talk", "Rock", "Cls Rock", "Adlt Hit", "Soft Rck", "Top 40", "Country", "Oldies", "Soft", "Nostalga", "Jazz", "Classicl", "R & B", "Soft R&B", "Language", "Rel Musc", "Rel Talk", "Persnlty", "Public", "College", "N/A", "N/A", "N/A", "N/A", "N/A", "Weather", "Test", "ALERT!"};

    if(prevpty != -1)
    {
        gchar tmp[15];
        g_sprintf(tmp, "%-8s", (conf.rds_pty ? ptys_usa[prevpty] : ptys_eu[prevpty]));
        gtk_label_set_text(GTK_LABEL(gui.l_pty), tmp);
    }

    if(prevtp == 1)
    {
        gtk_label_set_text(GTK_LABEL(gui.l_tp), "TP");
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_tp), GTK_STATE_NORMAL, &gui.colors.black);
    }
    else if(prevtp == 0)
    {
        gtk_label_set_text(GTK_LABEL(gui.l_tp), "TP");
        gtk_label_set_text(GTK_LABEL(gui.l_ta), "  ");
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_tp), GTK_STATE_NORMAL, &gui.colors.grey);
    }

    return FALSE;
}

gboolean gui_update_tams(gpointer nothing)
{
    if(prevtp == 1 && prevta == 1)
    {
        gtk_label_set_text(GTK_LABEL(gui.l_ta), "TA");
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_ta), GTK_STATE_NORMAL, &gui.colors.black);
    }
    else if(prevtp == 1)
    {
        gtk_label_set_text(GTK_LABEL(gui.l_ta), "TA");
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_ta), GTK_STATE_NORMAL, &gui.colors.grey);
    }

    gchar *markup;
    if(prevms == 1)
    {
        markup = g_markup_printf_escaped("M<span color=\"#DDDDDD\">S</span>");
        gtk_label_set_markup(GTK_LABEL(gui.l_ms), markup);
        g_free(markup);
    }
    else if(prevms == 0)
    {
        markup = g_markup_printf_escaped("<span color=\"#DDDDDD\">M</span>S");
        gtk_label_set_markup(GTK_LABEL(gui.l_ms), markup);
        g_free(markup);
    }

    return FALSE;
}

gboolean gui_update_af(gpointer af_new_freq)
{
    GtkTreeIter iter;

    // if new frequency is found on the AF list, gui_af_check() will free it and set the pointer to NULL
    gtk_tree_model_foreach(GTK_TREE_MODEL(gui.af), (GtkTreeModelForeachFunc)gui_af_check, &af_new_freq);

    if(af_new_freq)
    {
        gtk_list_store_append(gui.af, &iter);
        gtk_list_store_set(gui.af, &iter, 0, (gchar*)af_new_freq, -1);
        g_free(af_new_freq);
    }

    return FALSE;
}

gboolean gui_clear_power_off(gpointer nothing)
{
    gui_clear(NULL);
    graph_clear();
    if(conf.signal_display == SIGNAL_GRAPH)
    {
        gtk_widget_queue_draw(gui.graph);
    }
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gui.p_signal), 0);

    gtk_widget_set_sensitive(gui.menu_items.connect, TRUE);
    gtk_menu_item_set_label(GTK_MENU_ITEM(gui.menu_items.connect), "Connect");
    gtk_window_set_title(GTK_WINDOW(gui.window), "XDR-GTK");
    return FALSE;
}

gboolean gui_af_check(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer* newfreq)
{
    gchar *cfreq;
    gtk_tree_model_get(model, iter, 0, &cfreq, -1);
    if(!strcmp(cfreq,*newfreq))
    {
        g_free(*newfreq);
        *newfreq = NULL;
        g_free(cfreq);
        // frequency is already on the list, stop searching.
        return TRUE;
    }
    g_free(cfreq);
    return FALSE;
}

void gui_mode_toggle(GtkWidget *widget, GdkEventButton *event, gpointer step)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
        if(conf.mode != MODE_FM)
        {
            xdr_write("M0");
            gui_mode_FM();
        }
        else
        {
            xdr_write("M1");
            gui_mode_AM();
        }
    }
}

gboolean gui_mode_FM()
{
    conf.mode = MODE_FM;
    settings_write();
    gtk_label_set_text(GTK_LABEL(gui.l_band), "FM");
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tty_change_bandwidth), NULL);
    gui_fill_bandwidths(gui.c_bw, TRUE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_bw), 29);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tty_change_bandwidth), NULL);
    gtk_widget_set_sensitive(gui.c_deemph, TRUE);
    return FALSE;
}

gboolean gui_mode_AM()
{
    conf.mode = MODE_AM;
    settings_write();
    rds_timer = 0;
    gtk_label_set_text(GTK_LABEL(gui.l_band), "AM");
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tty_change_bandwidth), NULL);
    gui_fill_bandwidths(gui.c_bw, FALSE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_bw), 16);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tty_change_bandwidth), NULL);
    gtk_widget_set_sensitive(gui.c_deemph, FALSE);
    return FALSE;
}

void gui_fill_bandwidths(GtkWidget* combo, gboolean auto_mode)
{
    GtkTreeModel *store = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    gtk_list_store_clear(GTK_LIST_STORE(store));

    switch(conf.mode)
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
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "auto");
}

void tty_change_bandwidth()
{
    gchar buffer[10];
    g_snprintf(buffer, sizeof(buffer), "F%d", filters[gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_bw))]);
    xdr_write(buffer);
}

void tty_change_deemphasis()
{
    gint deemphasis = gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_deemph));
    switch(deemphasis)
    {
    case 0:
        xdr_write("D0");
        break;
    case 1:
        xdr_write("D1");
        break;
    case 2:
        xdr_write("D2");
        break;
    }
    conf.deemphasis = deemphasis;
    settings_write();
}

void tty_change_volume(GtkScaleButton *widget, gdouble volume, gpointer data)
{
    gchar tmp[10];
    gint val = (exp(volume)-1)/(M_E-1) * 2047;
    g_snprintf(tmp, sizeof(tmp), "Y%d", val);
    xdr_write(tmp);
}

void tty_change_ant()
{
    gint ant = gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_ant));
    switch(ant)
    {
    case 0:
        xdr_write("Z0");
        break;
    case 1:
        xdr_write("Z1");
        break;
    case 2:
        xdr_write("Z2");
        break;
    case 3:
        xdr_write("Z3");
        break;
    }
}

void tty_change_agc()
{
    gint agc = gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_agc));
    switch(agc)
    {
    case 0:
        xdr_write("A0");
        break;
    case 1:
        xdr_write("A1");
        break;
    case 2:
        xdr_write("A2");
        break;
    case 3:
        xdr_write("A3");
        break;
    }

    conf.agc = agc;
    settings_write();
}

void tty_change_gain()
{
    conf.rfgain = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.x_rf));
    conf.ifgain = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.x_if));

    gchar cmd[4];
    g_snprintf(cmd, sizeof(cmd), "G%d%d", conf.rfgain, conf.ifgain);
    xdr_write(cmd);

    settings_write();
}

void tune_gui_back(GtkWidget *widget, GdkEventButton *event, gpointer nothing)
{
    if(event->type == GDK_BUTTON_PRESS)
    {
        tune(prevfreq);
    }
}

void tune_gui_round(GtkWidget *widget, GdkEventButton *event, gpointer nothing)
{
    if(event->type == GDK_BUTTON_PRESS)
    {
        tune_r(freq);
    }
}

void tune_gui_step_click(GtkWidget *widget, GdkEventButton *event, gpointer step)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) // right click, tune down
    {
        tune(freq-(GPOINTER_TO_INT(step)));
    }
    else if(event->type == GDK_BUTTON_PRESS && event->button == 1) // left click, tune up
    {
        tune(freq+(GPOINTER_TO_INT(step)));
    }
}

void tune_gui_step_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer step)
{
    if(event->direction)
    {
        tune(freq-(GPOINTER_TO_INT(step)));
    }
    else
    {
        tune(freq+(GPOINTER_TO_INT(step)));
    }
}

gboolean gui_toggle_gain(GtkWidget *widget, GdkEventButton *event, gpointer nothing)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) // right click, toggle both RF and IF
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_rf), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.x_rf)));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_if), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.x_if)));
        tty_change_gain();
        return TRUE;
    }
    else if(event->type == GDK_BUTTON_PRESS && event->button == 1) // left click, toggle
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
        tty_change_gain();
        return TRUE;
    }
    return FALSE;
}


gboolean gui_update_clock(gpointer label)
{
    gchar buff[30], buff2[50];
    time_t tt = time(NULL);
    if(conf.utc)
    {
        strftime(buff, sizeof(buff), "%d-%m-%Y %H:%M:%S UTC", gmtime(&tt));
    }
    else
    {
        strftime(buff, sizeof(buff), "%d-%m-%Y %H:%M:%S LT", localtime(&tt));
    }

    // network connection
    if(online > 0)
    {
        g_snprintf(buff2, sizeof(buff2), "Online: %d  |  %s", online, buff);
        gtk_label_set_text(GTK_LABEL(label), buff2);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(label), buff);
    }

    // reset RDS data after timeout
    if(conf.rds_reset && rds_reset_timer != -1)
    {
        if((g_get_real_time() - rds_reset_timer) > (conf.rds_reset_timeout*1000000))
        {
            gui_clear_rds();
        }
    }
    return TRUE;
}

void tune_gui_af(GtkTreeSelection *ts, gpointer nothing)
{
    GtkTreeModel *list_store;
    GtkTreeIter iter;

    if(!gtk_tree_selection_get_selected(ts, &list_store, &iter))
    {
        return;
    }

    GValue value = {0,};
    gtk_tree_model_get_value(list_store, &iter, 0, &value);
    gchar *v = (gchar*)g_strdup_value_contents(&value);
    g_value_unset(&value);

    if(v)
    {
        size_t i;
        for(i=0; i<strlen(v); i++)
        {
            if(v[i] == '.' || v[i] == ',')
            {
                v[i] = v[i+1];
                v[i+1] = 0x00;
                break;
            }
        }
        tune(g_ascii_strtoll(v+1, NULL, 10)*100);
    }
    g_free(v);
}

gboolean volume_click(GtkWidget *widget, GdkEventButton *event)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) // right click
    {
        if(gtk_scale_button_get_value(GTK_SCALE_BUTTON(widget)) > 0)
        {
            gtk_scale_button_set_value(GTK_SCALE_BUTTON(widget), 0.0);
        }
        else
        {
            gtk_scale_button_set_value(GTK_SCALE_BUTTON(widget), 1.0);
        }
        return TRUE;
    }
    return FALSE;
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
