#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "gui.h"
#include "settings.h"
#include "graph.h"
#include "rdsspy.h"
#include "stationlist.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

void settings_read()
{
    GKeyFile *keyfile = g_key_file_new();
    GError *error = NULL;
    gsize length;
    gint i, *p;

    if(!g_key_file_load_from_file(keyfile, CONF_FILE, G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "Configuration file not found.\nUsing default settings");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        g_key_file_free(keyfile);

        conf.network = 0;
        conf.serial = NULL;
#ifdef G_OS_WIN32
        conf.serial = g_strdup("COM3");
#else
        conf.serial = g_strdup("ttyUSB0");
#endif
        conf.host = g_new(char*, 2);
        conf.host[0] = g_strdup("localhost");
        conf.host[1] = NULL;

        conf.port = 7373;
        conf.password = g_strdup("");

        conf.rfgain = 0;
        conf.ifgain = 0;
        conf.agc = 1;
        conf.deemphasis = 0;

        conf.presets[0] = 87600;
        conf.presets[1] = 89400;
        conf.presets[2] = 91300;
        conf.presets[3] = 93100;
        conf.presets[4] = 95000;
        conf.presets[5] = 96800;
        conf.presets[6] = 98600;
        conf.presets[7] = 100500;
        conf.presets[8] = 102400;
        conf.presets[9] = 104200;
        conf.presets[10] = 106100;
        conf.presets[11] = 107900;

        conf.signal_display = SIGNAL_GRAPH;
        conf.signal_unit = UNIT_DBF;
        conf.graph_height = 100;
        gdk_color_parse("#B5B5FF", &conf.color_mono);
        gdk_color_parse("#8080FF", &conf.color_stereo);
        gdk_color_parse("#3333FF", &conf.color_rds);
        conf.signal_avg = FALSE;
        conf.show_grid = TRUE;
        conf.utc = TRUE;
        conf.replace_spaces = TRUE;
        conf.alignment = FALSE;
        conf.stationlist = FALSE;
        conf.stationlist_client = 9030;
        conf.stationlist_server = 9031;

        conf.rds_timeout = 2;
        conf.rds_discard = FALSE;
        conf.rds_reset = FALSE;
        conf.rds_reset_timeout = 60;
        conf.rds_pty = 0;
        conf.rds_info_error = 2;
        conf.rds_data_error = 1;
        conf.rds_ps_progressive = FALSE;
        conf.rds_spy_port = 7376;
        conf.rds_spy_auto = FALSE;
        conf.rds_spy_run = FALSE;
        conf.rds_spy_command = g_strdup("");

        conf.scan_width = 950;
        conf.scan_height = 250;
        conf.scan_start = 87500;
        conf.scan_end = 108000;
        conf.scan_step = 100;
        conf.scan_bw = 12;
        conf.scan_relative = FALSE;

        conf.ant_switching = FALSE;

        conf.pattern_size = 600;
        conf.pattern_fill = TRUE;
        conf.pattern_avg = FALSE;

        conf.key_tune_up = GDK_Right;
        conf.key_tune_down = GDK_Left;
        conf.key_tune_up_5 = GDK_Up;
        conf.key_tune_down_5 = GDK_Down;
        conf.key_tune_up_1000 = GDK_Page_Up;
        conf.key_tune_down_1000 = GDK_Page_Down;
        conf.key_tune_back = GDK_B;
        conf.key_reset = GDK_R;
        conf.key_screen = GDK_S;
        conf.key_bw_up = GDK_KEY_bracketright;
        conf.key_bw_down = GDK_KEY_bracketleft;
        conf.key_bw_auto = GDK_KEY_backslash;
        conf.key_rotate_cw = GDK_KEY_Home;
        conf.key_rotate_ccw = GDK_KEY_End;

        settings_write();
        return;
    }

    conf.network = g_key_file_get_integer(keyfile, "connection", "network", NULL);
    conf.serial = settings_read_string(g_key_file_get_string(keyfile, "connection", "serial", NULL));
    conf.host = g_key_file_get_string_list(keyfile, "connection", "host", NULL, NULL);
    conf.port = g_key_file_get_integer(keyfile, "connection", "port", NULL);
    conf.password = settings_read_string(g_key_file_get_string(keyfile, "connection", "password", NULL));

    conf.rfgain = g_key_file_get_boolean(keyfile, "tuner", "rfgain", NULL);
    conf.ifgain = g_key_file_get_boolean(keyfile, "tuner", "ifgain", NULL);
    conf.agc = g_key_file_get_integer(keyfile, "tuner", "agc", NULL);
    conf.deemphasis = g_key_file_get_integer(keyfile, "tuner", "deemphasis", NULL);

    p = g_key_file_get_integer_list(keyfile, "settings", "presets", &length, NULL);
    for(i=0; i<PRESETS; i++)
    {
        if(i<length)
        {
            conf.presets[i] = p[i];
        }
        else
        {
            conf.presets[i] = 0;
        }
    }
    g_free(p);

    conf.signal_display = g_key_file_get_integer(keyfile, "settings", "signal_display", NULL);
    conf.signal_unit = g_key_file_get_integer(keyfile, "settings", "signal_unit", NULL);
    conf.graph_height = g_key_file_get_integer(keyfile, "settings", "graph_height", NULL);
    gdk_color_parse(g_key_file_get_string(keyfile, "settings", "color_mono", NULL), &conf.color_mono);
    gdk_color_parse(g_key_file_get_string(keyfile, "settings", "color_stereo", NULL), &conf.color_stereo);
    gdk_color_parse(g_key_file_get_string(keyfile, "settings", "color_rds", NULL), &conf.color_rds);
    conf.signal_avg = g_key_file_get_boolean(keyfile, "settings", "signal_avg", NULL);
    conf.show_grid = g_key_file_get_boolean(keyfile, "settings", "show_grid", NULL);
    conf.utc = g_key_file_get_boolean(keyfile, "settings", "utc", NULL);
    conf.replace_spaces = g_key_file_get_boolean(keyfile, "settings", "replace_spaces", NULL);
    conf.alignment = g_key_file_get_boolean(keyfile, "settings", "alignment", NULL);
    conf.stationlist = g_key_file_get_boolean(keyfile, "settings", "stationlist", NULL);
    conf.stationlist_client = g_key_file_get_integer(keyfile, "settings", "stationlist_client", NULL);
    conf.stationlist_server = g_key_file_get_integer(keyfile, "settings", "stationlist_server", NULL);

    conf.rds_timeout = g_key_file_get_integer(keyfile, "settings", "rds_timeout", NULL);
    conf.rds_discard = g_key_file_get_boolean(keyfile, "settings", "rds_discard", NULL);
    conf.rds_reset = g_key_file_get_boolean(keyfile, "settings", "rds_reset", NULL);
    conf.rds_reset_timeout = g_key_file_get_integer(keyfile, "settings", "rds_reset_timeout", NULL);
    conf.rds_pty = g_key_file_get_integer(keyfile, "settings", "rds_pty", NULL);
    conf.rds_info_error = g_key_file_get_integer(keyfile, "settings", "rds_info_error", NULL);
    conf.rds_data_error = g_key_file_get_integer(keyfile, "settings", "rds_data_error", NULL);
    conf.rds_ps_progressive = g_key_file_get_boolean(keyfile, "settings", "rds_ps_progressive", NULL);
    conf.rds_spy_port = g_key_file_get_integer(keyfile, "settings", "rds_spy_port", NULL);
    conf.rds_spy_auto = g_key_file_get_boolean(keyfile, "settings", "rds_spy_auto", NULL);
    conf.rds_spy_run = g_key_file_get_boolean(keyfile, "settings", "rds_spy_run", NULL);
    conf.rds_spy_command = settings_read_string(g_key_file_get_string(keyfile, "settings", "rds_spy_command", NULL));

    conf.scan_width = g_key_file_get_integer(keyfile, "scan", "scan_width", NULL);
    conf.scan_height = g_key_file_get_integer(keyfile, "scan", "scan_height", NULL);
    conf.scan_start = g_key_file_get_integer(keyfile, "scan", "scan_start", NULL);
    conf.scan_end = g_key_file_get_integer(keyfile, "scan", "scan_end", NULL);
    conf.scan_step = g_key_file_get_integer(keyfile, "scan", "scan_step", NULL);
    conf.scan_bw = g_key_file_get_integer(keyfile, "scan", "scan_bw", NULL);
    conf.scan_relative = g_key_file_get_boolean(keyfile, "scan", "scan_relative", NULL);

    conf.ant_switching = g_key_file_get_boolean(keyfile, "ant", "ant_switching", NULL);
    p = g_key_file_get_integer_list(keyfile, "ant", "ant_start", &length, NULL);
    for(i=0; i<ANTENNAS; i++)
    {
        if(i<length)
        {
            conf.ant_start[i] = p[i];
        }
        else
        {
            conf.ant_start[i] = 0;
        }
    }
    g_free(p);
    p = g_key_file_get_integer_list(keyfile, "ant", "ant_stop", &length, NULL);
    for(i=0; i<ANTENNAS; i++)
    {
        if(i<length)
        {
            conf.ant_stop[i] = p[i];
        }
        else
        {
            conf.ant_stop[i] = 0;
        }
    }
    g_free(p);

    conf.pattern_size = g_key_file_get_integer(keyfile, "pattern", "pattern_size", NULL);
    conf.pattern_fill = g_key_file_get_boolean(keyfile, "pattern", "pattern_fill", NULL);
    conf.pattern_avg = g_key_file_get_boolean(keyfile, "pattern", "pattern_avg", NULL);

    conf.key_tune_up = g_key_file_get_integer(keyfile, "keyboard", "key_tune_up", NULL);
    conf.key_tune_down = g_key_file_get_integer(keyfile, "keyboard", "key_tune_down", NULL);
    conf.key_tune_up_5 = g_key_file_get_integer(keyfile, "keyboard", "key_tune_up_5", NULL);
    conf.key_tune_down_5 = g_key_file_get_integer(keyfile, "keyboard", "key_tune_down_5", NULL);
    conf.key_tune_up_1000 = g_key_file_get_integer(keyfile, "keyboard", "key_tune_up_1000", NULL);
    conf.key_tune_down_1000 = g_key_file_get_integer(keyfile, "keyboard", "key_tune_down_1000", NULL);
    conf.key_tune_back = g_key_file_get_integer(keyfile, "keyboard", "key_tune_back", NULL);
    conf.key_reset = g_key_file_get_integer(keyfile, "keyboard", "key_reset", NULL);
    conf.key_screen = g_key_file_get_integer(keyfile, "keyboard", "key_screen", NULL);
    conf.key_bw_up = g_key_file_get_integer(keyfile, "keyboard", "key_bw_up", NULL);
    conf.key_bw_down = g_key_file_get_integer(keyfile, "keyboard", "key_bw_down", NULL);
    conf.key_bw_auto = g_key_file_get_integer(keyfile, "keyboard", "key_bw_auto", NULL);
    conf.key_rotate_cw = g_key_file_get_integer(keyfile, "keyboard", "key_rotate_cw", NULL);
    conf.key_rotate_ccw = g_key_file_get_integer(keyfile, "keyboard", "key_rotate_ccw", NULL);

    g_key_file_free(keyfile);
}

void settings_write()
{
    GKeyFile *keyfile = g_key_file_new();
    GError *error = NULL;
    gsize length;
    gchar *tmp;

    g_key_file_set_integer(keyfile, "connection", "network", conf.network);
    g_key_file_set_string(keyfile, "connection", "serial", conf.serial);
    g_key_file_set_string_list(keyfile, "connection", "host", (const gchar* const*)conf.host, g_strv_length(conf.host));
    g_key_file_set_integer(keyfile, "connection", "port", conf.port);
    g_key_file_set_string(keyfile, "connection", "password", conf.password);

    g_key_file_set_boolean(keyfile, "tuner", "rfgain", conf.rfgain);
    g_key_file_set_boolean(keyfile, "tuner", "ifgain", conf.ifgain);
    g_key_file_set_integer(keyfile, "tuner", "agc", conf.agc);
    g_key_file_set_integer(keyfile, "tuner", "deemphasis", conf.deemphasis);

    g_key_file_set_integer_list(keyfile, "settings", "presets", conf.presets, PRESETS);
    g_key_file_set_integer(keyfile, "settings", "signal_display", conf.signal_display);
    g_key_file_set_integer(keyfile, "settings", "signal_unit", conf.signal_unit);
    g_key_file_set_integer(keyfile, "settings", "graph_height", conf.graph_height);
    g_key_file_set_boolean(keyfile, "settings", "signal_avg", conf.signal_avg);
    g_key_file_set_boolean(keyfile, "settings", "show_grid", conf.show_grid);
    g_key_file_set_boolean(keyfile, "settings", "utc", conf.utc);
    g_key_file_set_boolean(keyfile, "settings", "replace_spaces", conf.replace_spaces);
    g_key_file_set_boolean(keyfile, "settings", "alignment", conf.alignment);
    g_key_file_set_boolean(keyfile, "settings", "stationlist", conf.stationlist);
    g_key_file_set_integer(keyfile, "settings", "stationlist_client", conf.stationlist_client);
    g_key_file_set_integer(keyfile, "settings", "stationlist_server", conf.stationlist_server);

    tmp = gdk_color_to_string(&conf.color_mono);
    g_key_file_set_string(keyfile, "settings", "color_mono", tmp);
    g_free(tmp);

    tmp = gdk_color_to_string(&conf.color_stereo);
    g_key_file_set_string(keyfile, "settings", "color_stereo", tmp);
    g_free(tmp);

    tmp = gdk_color_to_string(&conf.color_rds);
    g_key_file_set_string(keyfile, "settings", "color_rds", tmp);
    g_free(tmp);

    g_key_file_set_integer(keyfile, "settings", "rds_timeout", conf.rds_timeout);
    g_key_file_set_boolean(keyfile, "settings", "rds_discard", conf.rds_discard);
    g_key_file_set_boolean(keyfile, "settings", "rds_reset", conf.rds_reset);
    g_key_file_set_integer(keyfile, "settings", "rds_reset_timeout", conf.rds_reset_timeout);
    g_key_file_set_integer(keyfile, "settings", "rds_pty", conf.rds_pty);
    g_key_file_set_integer(keyfile, "settings", "rds_info_error", conf.rds_info_error);
    g_key_file_set_integer(keyfile, "settings", "rds_data_error", conf.rds_data_error);
    g_key_file_set_boolean(keyfile, "settings", "rds_ps_progressive", conf.rds_ps_progressive);
    g_key_file_set_integer(keyfile, "settings", "rds_spy_port", conf.rds_spy_port);
    g_key_file_set_boolean(keyfile, "settings", "rds_spy_auto", conf.rds_spy_auto);
    g_key_file_set_boolean(keyfile, "settings", "rds_spy_run", conf.rds_spy_run);
    g_key_file_set_string(keyfile, "settings", "rds_spy_command", conf.rds_spy_command);

    g_key_file_set_integer(keyfile, "scan", "scan_width", conf.scan_width);
    g_key_file_set_integer(keyfile, "scan", "scan_height", conf.scan_height);
    g_key_file_set_integer(keyfile, "scan", "scan_start", conf.scan_start);
    g_key_file_set_integer(keyfile, "scan", "scan_end", conf.scan_end);
    g_key_file_set_integer(keyfile, "scan", "scan_step", conf.scan_step);
    g_key_file_set_integer(keyfile, "scan", "scan_bw", conf.scan_bw);
    g_key_file_set_boolean(keyfile, "scan", "scan_relative", conf.scan_relative);

    g_key_file_set_boolean(keyfile, "ant", "ant_switching", conf.ant_switching);
    g_key_file_set_integer_list(keyfile, "ant", "ant_start", conf.ant_start, ANTENNAS);
    g_key_file_set_integer_list(keyfile, "ant", "ant_stop", conf.ant_stop, ANTENNAS);

    g_key_file_set_integer(keyfile, "pattern", "pattern_size", conf.pattern_size);
    g_key_file_set_boolean(keyfile, "pattern", "pattern_fill", conf.pattern_fill);
    g_key_file_set_boolean(keyfile, "pattern", "pattern_avg", conf.pattern_avg);

    g_key_file_set_integer(keyfile, "keyboard", "key_tune_up", conf.key_tune_up);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_down", conf.key_tune_down);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_up_5", conf.key_tune_up_5);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_down_5", conf.key_tune_down_5);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_up_1000", conf.key_tune_up_1000);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_down_1000", conf.key_tune_down_1000);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_back", conf.key_tune_back);
    g_key_file_set_integer(keyfile, "keyboard", "key_reset", conf.key_reset);
    g_key_file_set_integer(keyfile, "keyboard", "key_screen", conf.key_screen);
    g_key_file_set_integer(keyfile, "keyboard", "key_bw_up", conf.key_bw_up);
    g_key_file_set_integer(keyfile, "keyboard", "key_bw_down", conf.key_bw_down);
    g_key_file_set_integer(keyfile, "keyboard", "key_bw_auto", conf.key_bw_auto);
    g_key_file_set_integer(keyfile, "keyboard", "key_rotate_cw", conf.key_rotate_cw);
    g_key_file_set_integer(keyfile, "keyboard", "key_rotate_ccw", conf.key_rotate_ccw);

    if(!(tmp = g_key_file_to_data(keyfile, &length, &error)))
    {
        dialog_error("Unable to generate the configuration file.");
        g_error_free(error);
        error = NULL;
    }
    g_key_file_free(keyfile);

    FILE *f = fopen(CONF_FILE, "w");
    if (f == NULL)
    {
        dialog_error("Unable to save the configuration file.");
    }
    else
    {
        fwrite(tmp, 1, length, f);
        fclose(f);
    }

    g_free(tmp);
}

void settings_dialog()
{
    gint row;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Settings", GTK_WINDOW(gui.window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    gtk_window_set_icon_name(GTK_WINDOW(dialog), "xdr-gtk-settings");
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

    GtkWidget *hbox_main = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(content), hbox_main);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(hbox_main), notebook);

    // ----------

    GtkWidget *page_interface = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_interface), 4);

    GtkWidget *table_signal = gtk_table_new(13, 2, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(table_signal), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table_signal), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table_signal), 4);
    gtk_container_add(GTK_CONTAINER(page_interface), table_signal);

    row = 0;
    GtkWidget *l_display = gtk_label_new("Signal level:");
    gtk_misc_set_alignment(GTK_MISC(l_display), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_signal), l_display, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_display = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_display), "text only");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_display), "graph");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_display), "bar");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_display), conf.signal_display);
    gtk_table_attach(GTK_TABLE(table_signal), c_display, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_unit = gtk_label_new("Signal unit:");
    gtk_misc_set_alignment(GTK_MISC(l_unit), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_signal), l_unit, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_unit = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "dBf");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "dBm");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "dBuV");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "S-meter");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_unit), conf.signal_unit);
    gtk_table_attach(GTK_TABLE(table_signal), c_unit, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_height = gtk_label_new("Graph height [px]:");
    gtk_misc_set_alignment(GTK_MISC(l_height), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_signal), l_height, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(conf.graph_height, 50.0, 300.0, 5.0, 10.0, 0.0);
    GtkWidget *s_height = gtk_spin_button_new(adj, 0, 0);
    gtk_table_attach(GTK_TABLE(table_signal), s_height, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_colors = gtk_label_new("Graph colors:");
    gtk_misc_set_alignment(GTK_MISC(l_colors), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_signal), l_colors, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *box_colors = gtk_hbox_new(FALSE, 5);
    GtkWidget *c_mono = gtk_color_button_new_with_color(&conf.color_mono);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_mono), "Mono color");
    gtk_widget_set_tooltip_text(c_mono, "Mono color");
    gtk_box_pack_start(GTK_BOX(box_colors), c_mono, TRUE, TRUE, 0);

    GtkWidget *c_stereo = gtk_color_button_new_with_color(&conf.color_stereo);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_stereo), "Stereo color");
    gtk_widget_set_tooltip_text(c_stereo, "Stereo color");
    gtk_box_pack_start(GTK_BOX(box_colors), c_stereo, TRUE, TRUE, 0);

    GtkWidget *c_rds = gtk_color_button_new_with_color(&conf.color_rds);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_rds), "RDS color");
    gtk_widget_set_tooltip_text(c_rds, "RDS color");
    gtk_box_pack_start(GTK_BOX(box_colors), c_rds, TRUE, TRUE, 0);

    gtk_table_attach(GTK_TABLE(table_signal), box_colors, 1, 2, row, row+1, 0, 0, 0, 0);

    row++;
    GtkWidget *x_avg = gtk_check_button_new_with_label("Signal level averaging");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_avg), conf.signal_avg);
    gtk_table_attach(GTK_TABLE(table_signal), x_avg, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_grid = gtk_check_button_new_with_label("Show grid");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_grid), conf.show_grid);
    gtk_table_attach(GTK_TABLE(table_signal), x_grid, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_utc = gtk_check_button_new_with_label("UTC time");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_utc), conf.utc);
    gtk_table_attach(GTK_TABLE(table_signal), x_utc, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_replace = gtk_check_button_new_with_label("Replace spaces with _ (clipboard)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_replace), conf.replace_spaces);
    gtk_table_attach(GTK_TABLE(table_signal), x_replace, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_alignment = gtk_check_button_new_with_label("Show alignment");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_alignment), conf.alignment);
    gtk_table_attach(GTK_TABLE(table_signal), x_alignment, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *hs_sl = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(table_signal), hs_sl, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_stationlist = gtk_check_button_new_with_label("StationList");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_stationlist), conf.stationlist);
    gtk_table_attach(GTK_TABLE(table_signal), x_stationlist, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_stationlist_client = gtk_label_new("Client UDP Port:");
    gtk_misc_set_alignment(GTK_MISC(l_stationlist_client), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_signal), l_stationlist_client, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkAdjustment *adj_sl_c = (GtkAdjustment*)gtk_adjustment_new(conf.stationlist_client, 1024.0, 65535.0, 1.0, 10.0, 0.0);
    GtkWidget *s_stationlist_client = gtk_spin_button_new(adj_sl_c, 0, 0);
    gtk_table_attach(GTK_TABLE(table_signal), s_stationlist_client, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_stationlist_server = gtk_label_new("Server UDP Port:");
    gtk_misc_set_alignment(GTK_MISC(l_stationlist_server), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_signal), l_stationlist_server, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkAdjustment *adj_sl_s = (GtkAdjustment*)gtk_adjustment_new(conf.stationlist_server, 1024.0, 65535.0, 1.0, 10.0, 0.0);
    GtkWidget *s_stationlist_server = gtk_spin_button_new(adj_sl_s, 0, 0);
    gtk_table_attach(GTK_TABLE(table_signal), s_stationlist_server, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);


    GtkWidget *page_interface_label = gtk_label_new("Interface");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_interface, page_interface_label);

    // ...

    GtkWidget *page_rds = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_rds), 4);

    GtkWidget *table_rds = gtk_table_new(13, 2, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(table_rds), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table_rds), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table_rds), 4);
    gtk_container_add(GTK_CONTAINER(page_rds), table_rds);

    row = 0;
    GtkWidget *l_indicator = gtk_label_new("Indicator timeout:");
    gtk_misc_set_alignment(GTK_MISC(l_indicator), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_rds), l_indicator, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkAdjustment *adj2 = (GtkAdjustment*)gtk_adjustment_new(conf.rds_timeout, 2.0, 30.0, 1.0, 2.0, 0.0);
    GtkWidget *s_indicator = gtk_spin_button_new(adj2, 0, 0);
    gtk_table_attach(GTK_TABLE(table_rds), s_indicator, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_discard = gtk_check_button_new_with_label("Discard RDS data after indicator timeout");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_discard), conf.rds_discard);
    gtk_table_attach(GTK_TABLE(table_rds), x_discard, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_rds_reset = gtk_check_button_new_with_label("Reset RDS after: [s]");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_rds_reset), conf.rds_reset);
    gtk_table_attach(GTK_TABLE(table_rds), x_rds_reset, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkAdjustment *adj3 = (GtkAdjustment*)gtk_adjustment_new(conf.rds_reset_timeout, 1.0, 10000.0, 10.0, 60.0, 0.0);
    GtkWidget *s_reset = gtk_spin_button_new(adj3, 0, 0);
    gtk_table_attach(GTK_TABLE(table_rds), s_reset, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_pty = gtk_label_new("PTY set:");
    gtk_misc_set_alignment(GTK_MISC(l_pty), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_rds), l_pty, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_pty = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_pty), "RDS/EU");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_pty), "RBDS/US");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_pty), conf.rds_pty);
    gtk_table_attach(GTK_TABLE(table_rds), c_pty, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *hs_err = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(table_rds), hs_err, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rds_error = gtk_label_new("PS & RT error correction:");
    gtk_table_attach(GTK_TABLE(table_rds), l_rds_error, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rds_info_error = gtk_label_new("Group Type / Position:");
    gtk_misc_set_alignment(GTK_MISC(l_rds_info_error), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_rds), l_rds_info_error, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *c_rds_info_error = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_info_error), "no errors");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_info_error), "max 2-bit");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_info_error), "max 5-bit");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_rds_info_error), conf.rds_info_error);
    gtk_table_attach(GTK_TABLE(table_rds), c_rds_info_error, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rds_data_error = gtk_label_new("Data:");
    gtk_misc_set_alignment(GTK_MISC(l_rds_data_error), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_rds), l_rds_data_error, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *c_rds_data_error = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_data_error), "no errors");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_data_error), "max 2-bit");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_data_error), "max 5-bit");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_rds_data_error), conf.rds_data_error);
    gtk_table_attach(GTK_TABLE(table_rds), c_rds_data_error, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_psprog = gtk_check_button_new_with_label("PS progressive correction");
    gtk_widget_set_tooltip_text(x_psprog, "Replace characters in the RDS PS only with lower or the same error level. This also overrides the error correction to 5/5bit. Useful for static RDS PS. For a quick toggle, click a right mouse button on the displayed RDS PS.");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_psprog), conf.rds_ps_progressive);
    gtk_table_attach(GTK_TABLE(table_rds), x_psprog, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *hs_rdsspy = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(table_rds), hs_rdsspy, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rdsspy_port = gtk_label_new("RDS Spy Link TCP/IP port:");
    gtk_misc_set_alignment(GTK_MISC(l_rdsspy_port), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_rds), l_rdsspy_port, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkAdjustment *adj4 = (GtkAdjustment*)gtk_adjustment_new(conf.rds_spy_port, 1024.0, 65535.0, 1.0, 10.0, 0.0);
    GtkWidget *s_rdsspy_port = gtk_spin_button_new(adj4, 0, 0);
    gtk_table_attach(GTK_TABLE(table_rds), s_rdsspy_port, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_rdsspy_auto = gtk_check_button_new_with_label("RDS Spy Link auto-start");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_rdsspy_auto), conf.rds_spy_auto);
    gtk_table_attach(GTK_TABLE(table_rds), x_rdsspy_auto, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_rdsspy_run = gtk_check_button_new_with_label("RDS Spy auto-start:");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_rdsspy_run), conf.rds_spy_run);
    gtk_table_attach(GTK_TABLE(table_rds), x_rdsspy_run, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_rdsspy_command = gtk_file_chooser_button_new("RDS Spy Executable File", GTK_FILE_CHOOSER_ACTION_OPEN);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "RDS Spy Executable File (rdsspy.exe)");
    gtk_file_filter_add_pattern(filter, "rdsspy.exe");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(c_rdsspy_command), filter);
    GtkFileFilter* filter2 = gtk_file_filter_new();
    gtk_file_filter_set_name(filter2, "All files");
    gtk_file_filter_add_pattern(filter2, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(c_rdsspy_command), filter2);
    if(strlen(conf.rds_spy_command))
    {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(c_rdsspy_command), conf.rds_spy_command);
    }
    gtk_table_attach(GTK_TABLE(table_rds), c_rdsspy_command, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *page_rds_label = gtk_label_new("RDS");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_rds, page_rds_label);

    // ...

    GtkWidget *page_ant = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_ant), 4);

    GtkWidget *table_ant = gtk_table_new(5, 2, FALSE);
    gtk_table_set_homogeneous(GTK_TABLE(table_ant), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table_ant), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table_ant), 4);
    gtk_container_add(GTK_CONTAINER(page_ant), table_ant);
    GtkAdjustment *tmp_adj;

    row = 0;
    GtkWidget *x_ant = gtk_check_button_new_with_label("Automatic antenna switching");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_ant), conf.ant_switching);
    gtk_table_attach(GTK_TABLE(table_ant), x_ant, 0, 5, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *l_ant_label[ANTENNAS];
    l_ant_label[0] = gtk_label_new("Ant A:");
    l_ant_label[1] = gtk_label_new("Ant B:");
    l_ant_label[2] = gtk_label_new("Ant C:");
    l_ant_label[3] = gtk_label_new("Ant D:");

    GtkWidget *s_ant_start[ANTENNAS];
    GtkWidget *s_ant_stop[ANTENNAS];
    gint i;
    for(i=0; i<ANTENNAS; i++)
    {
        row++;
        gtk_table_attach(GTK_TABLE(table_ant), l_ant_label[i], 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
        tmp_adj = (GtkAdjustment*)gtk_adjustment_new(conf.ant_start[i], 0.0, 200000.0, 100.0, 200.0, 0.0);
        s_ant_start[i] = gtk_spin_button_new(tmp_adj, 0, 0);
        gtk_table_attach(GTK_TABLE(table_ant), s_ant_start[i], 1, 2, row, row+1, 0, 0, 0, 0);
        GtkWidget *l_ant_ = gtk_label_new("-");
        gtk_table_attach(GTK_TABLE(table_ant), l_ant_, 2, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
        tmp_adj = (GtkAdjustment*)gtk_adjustment_new(conf.ant_stop[i], 0.0, 200000.0, 100.0, 200.0, 0.0);
        s_ant_stop[i] = gtk_spin_button_new(tmp_adj, 0, 0);
        gtk_table_attach(GTK_TABLE(table_ant), s_ant_stop[i], 3, 4, row, row+1, 0, 0, 0, 0);
        GtkWidget *l_ant_k = gtk_label_new("kHz");
        gtk_table_attach(GTK_TABLE(table_ant), l_ant_k, 4, 5, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    }

    GtkWidget *page_ant_label = gtk_label_new("Antenna");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_ant, page_ant_label);

    // ...

    GtkWidget *page_key = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_key), 4);

    GtkWidget *table_key = gtk_table_new(14, 2, FALSE);
    gtk_table_set_homogeneous(GTK_TABLE(table_key), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table_key), 1);
    gtk_table_set_col_spacings(GTK_TABLE(table_key), 0);
    gtk_container_add(GTK_CONTAINER(page_key), table_key);

    row = 0;

    GtkWidget *l_tune_up = gtk_label_new("Tune up");
    gtk_misc_set_alignment(GTK_MISC(l_tune_up), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_up, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_up = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_up));
    g_signal_connect(b_tune_up, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_up, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_tune_down = gtk_label_new("Tune down");
    gtk_misc_set_alignment(GTK_MISC(l_tune_down), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_down, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_down = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_down));
    g_signal_connect(b_tune_down, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_down, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_tune_up_5 = gtk_label_new("Tune +5 kHz");
    gtk_misc_set_alignment(GTK_MISC(l_tune_up_5), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_up_5, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_up_5 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_up_5));
    g_signal_connect(b_tune_up_5, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_up_5, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_tune_down_5 = gtk_label_new("Tune -5 kHz");
    gtk_misc_set_alignment(GTK_MISC(l_tune_down_5), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_down_5, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_down_5 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_down_5));
    g_signal_connect(b_tune_down_5, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_down_5, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_tune_up_1000 = gtk_label_new("Tune +1 MHz");
    gtk_misc_set_alignment(GTK_MISC(l_tune_up_1000), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_up_1000, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_up_1000 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_up_1000));
    g_signal_connect(b_tune_up_1000, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_up_1000, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_tune_down_1000 = gtk_label_new("Tune -1 MHz");
    gtk_misc_set_alignment(GTK_MISC(l_tune_down_1000), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_down_1000, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_down_1000 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_down_1000));
    g_signal_connect(b_tune_down_1000, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_down_1000, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_tune_back = gtk_label_new("Tune back");
    gtk_misc_set_alignment(GTK_MISC(l_tune_back), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_back, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_back = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_back));
    g_signal_connect(b_tune_back, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_back, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_reset = gtk_label_new("Reset frequency");
    gtk_misc_set_alignment(GTK_MISC(l_reset), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_reset, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_reset = gtk_button_new_with_label(gdk_keyval_name(conf.key_reset));
    g_signal_connect(b_reset, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_reset, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_screen = gtk_label_new("Screenshot");
    gtk_misc_set_alignment(GTK_MISC(l_screen), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_screen, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_screen = gtk_button_new_with_label(gdk_keyval_name(conf.key_screen));
    g_signal_connect(b_screen, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_screen, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_bw_up = gtk_label_new("Increase bandwidth");
    gtk_misc_set_alignment(GTK_MISC(l_bw_up), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_bw_up, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_bw_up = gtk_button_new_with_label(gdk_keyval_name(conf.key_bw_up));
    g_signal_connect(b_bw_up, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_bw_up, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_bw_down = gtk_label_new("Decrease bandwidth");
    gtk_misc_set_alignment(GTK_MISC(l_bw_down), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_bw_down, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_bw_down = gtk_button_new_with_label(gdk_keyval_name(conf.key_bw_down));
    g_signal_connect(b_bw_down, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_bw_down, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_bw_auto = gtk_label_new("Adaptive bandwidth");
    gtk_misc_set_alignment(GTK_MISC(l_bw_auto), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_bw_auto, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_bw_auto = gtk_button_new_with_label(gdk_keyval_name(conf.key_bw_auto));
    g_signal_connect(b_bw_auto, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_bw_auto, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_rotate_cw = gtk_label_new("Rotate CW");
    gtk_misc_set_alignment(GTK_MISC(l_rotate_cw), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_rotate_cw, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_rotate_cw = gtk_button_new_with_label(gdk_keyval_name(conf.key_rotate_cw));
    g_signal_connect(b_rotate_cw, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_rotate_cw, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *l_rotate_ccw = gtk_label_new("Rotate CCW");
    gtk_misc_set_alignment(GTK_MISC(l_rotate_ccw), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_rotate_ccw, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_rotate_ccw = gtk_button_new_with_label(gdk_keyval_name(conf.key_rotate_ccw));
    g_signal_connect(b_rotate_ccw, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_rotate_ccw, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    row++;

    GtkWidget *page_key_label = gtk_label_new("Keyboard");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_key, page_key_label);

    // ----------

    GtkWidget *frame_presets = gtk_frame_new("Presets [kHz]");;
    gtk_container_add(GTK_CONTAINER(hbox_main), frame_presets);
    GtkWidget *table_presets = gtk_table_new(PRESETS, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table_presets), 3);
    gtk_table_set_row_spacings(GTK_TABLE(table_presets), 1);
    gtk_table_set_col_spacings(GTK_TABLE(table_presets), 2);
    gtk_container_add(GTK_CONTAINER(frame_presets), table_presets);

    GtkWidget* s_presets[PRESETS];

    gchar tmp[10];
    for(i=0; i<PRESETS; i++)
    {
        g_snprintf(tmp, sizeof(tmp), "F%d:", i+1);
        GtkWidget *l_preset = gtk_label_new(tmp);
        gtk_table_attach(GTK_TABLE(table_presets), l_preset, 0, 1, i, i+1, 0, 0, 0, 0);
        GtkAdjustment *adj_pr = (GtkAdjustment*)gtk_adjustment_new(conf.presets[i], 100.0, 200000.0, 100.0, 200.0, 0.0);
        s_presets[i] = gtk_spin_button_new(adj_pr, 0, 0);
        gtk_table_attach(GTK_TABLE(table_presets), s_presets[i], 1, 2, i, i+1, 0, 0, 0, 0);
    }

    // ----------

    gtk_widget_show_all(dialog);
    gint result;
#ifdef G_OS_WIN32
    result = win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    result = gtk_dialog_run(GTK_DIALOG(dialog));
#endif

    if(result == GTK_RESPONSE_ACCEPT)
    {
        conf.signal_display = gtk_combo_box_get_active(GTK_COMBO_BOX(c_display));
        conf.signal_unit = gtk_combo_box_get_active(GTK_COMBO_BOX(c_unit));
        conf.graph_height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_height));
        gtk_color_button_get_color(GTK_COLOR_BUTTON(c_mono), &conf.color_mono);
        gtk_color_button_get_color(GTK_COLOR_BUTTON(c_stereo), &conf.color_stereo);
        gtk_color_button_get_color(GTK_COLOR_BUTTON(c_rds), &conf.color_rds);
        conf.show_grid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_grid));
        conf.signal_avg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_avg));
        conf.utc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_utc));
        conf.replace_spaces = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_replace));
        conf.alignment = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_alignment));

        if(conf.alignment)
        {
            gtk_widget_show(gui.hs_align);
        }
        else
        {
            gtk_widget_hide(gui.hs_align);
        }

        conf.stationlist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_stationlist));
        conf.stationlist_client = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_stationlist_client));
        conf.stationlist_server = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_stationlist_server));

        stationlist_stop();
        if(conf.stationlist)
        {
            stationlist_init();
        }

        conf.rds_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_indicator));
        conf.rds_discard = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_discard));
        conf.rds_reset = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_rds_reset));
        conf.rds_reset_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_reset));
        conf.rds_pty = gtk_combo_box_get_active(GTK_COMBO_BOX(c_pty));
        conf.rds_info_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_rds_info_error));
        conf.rds_data_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_rds_data_error));
        conf.rds_ps_progressive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_psprog));

        // stop RDS Spy server if the port has been changed
        gint tmp_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_rdsspy_port));
        if(rdsspy_is_up() && conf.rds_spy_port != tmp_port)
        {
            rdsspy_stop();
        }
        conf.rds_spy_port = tmp_port;
        conf.rds_spy_auto = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_rdsspy_auto));
        conf.rds_spy_run = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_rdsspy_run));
        if(conf.rds_spy_command)
        {
            g_free(conf.rds_spy_command);
        }
        conf.rds_spy_command = settings_read_string(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(c_rdsspy_command)));

        conf.ant_switching = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_ant));
        for(i=0; i<ANTENNAS; i++)
        {
            conf.ant_start[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_ant_start[i]));
            conf.ant_stop[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_ant_stop[i]));
        }

        for(i=0; i<PRESETS; i++)
        {
            conf.presets[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_presets[i]));
        }

        conf.key_tune_up = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_up)));
        conf.key_tune_down = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_down)));
        conf.key_tune_up_5 = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_up_5)));
        conf.key_tune_down_5 = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_down_5)));
        conf.key_tune_up_1000 = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_up_1000)));
        conf.key_tune_down_1000 = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_down_1000)));
        conf.key_tune_back = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_back)));
        conf.key_reset = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_reset)));
        conf.key_screen = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_screen)));
        conf.key_bw_up = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_bw_up)));
        conf.key_bw_down = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_bw_down)));
        conf.key_bw_auto = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_bw_auto)));
        conf.key_rotate_cw = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_rotate_cw)));
        conf.key_rotate_ccw = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_rotate_ccw)));

        settings_write();
        graph_resize();
        signal_display();
    }
    gtk_widget_destroy(dialog);
}

void settings_key(GtkWidget *widget, GdkEventButton *event, gpointer nothing)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Key", GTK_WINDOW(gui.window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    g_signal_connect(dialog, "key-press-event", G_CALLBACK(settings_key_press), widget);
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);

    gchar *text = g_markup_printf_escaped("Current key: <b>%s</b>\n\nPress any key to change...\n", gtk_button_get_label(GTK_BUTTON(widget)));
    GtkWidget *desc = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(desc), text);
    gtk_container_add(GTK_CONTAINER(content), desc);
    g_free(text);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
}

gboolean settings_key_press(GtkWidget* widget, GdkEventKey* event, gpointer button)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    gtk_button_set_label(GTK_BUTTON(button), gdk_keyval_name(current));
    gtk_widget_destroy(widget);
    return TRUE;
}

void settings_update_string(gchar** ptr, const gchar* str)
{
    if(*ptr != NULL)
    {
        g_free(*ptr);
    }

    *ptr = g_strdup((str ? str : ""));
}

gchar* settings_read_string(gchar* str)
{
    return str ? str : g_strdup("");
}

void settings_add_host(const gchar* host)
{
    gchar** hosts;
    gint i, j;

    if(host == NULL)
        return;

    hosts = g_new0(char*, HOST_HISTORY_LEN+1);
    hosts[0] = g_strdup(host);

    for(i=0, j=1; (j<HOST_HISTORY_LEN && conf.host[i]); i++)
    {
        if(g_ascii_strcasecmp(host, conf.host[i]))
        {
            hosts[j++] = g_strdup(conf.host[i]);
        }
    }
    g_strfreev(conf.host);
    conf.host = hosts;
}
