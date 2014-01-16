#include <gtk/gtk.h>
#include "gui.h"
#include "settings.h"
#include "graph.h"
#include "menu.h"

void settings_read()
{
    GKeyFile *keyfile = g_key_file_new();
    GError *error = NULL;
    gsize length;
    gint i, *p;

    if(!g_key_file_load_from_file(keyfile, CONF_FILE, G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        dialog_error("Configuration file not found.\nUsing default settings.");
        g_error_free(error);
        g_key_file_free(keyfile);

        conf.network = 0;
#ifdef G_OS_WIN32
        conf.serial = g_strdup("COM3");
#else
        conf.serial = g_strdup("ttyUSB0");
#endif
        conf.host = g_strdup("localhost");
        conf.port = 7373;

        conf.mode = MODE_FM;
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

        conf.rds_timeout = 7;
        conf.rds_discard = FALSE;
        conf.rds_pty = 0;
        conf.rds_info_error = 1;
        conf.rds_data_error = 1;

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

        settings_write();
        return;
    }

    conf.network = g_key_file_get_integer(keyfile, "connection", "network", NULL);
    conf.serial = g_key_file_get_string(keyfile, "connection", "serial", NULL);
    conf.host = g_key_file_get_string(keyfile, "connection", "host", NULL);
    conf.port = g_key_file_get_integer(keyfile, "connection", "port", NULL);

    conf.mode = g_key_file_get_integer(keyfile, "tuner", "mode", NULL);
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

    conf.rds_timeout = g_key_file_get_integer(keyfile, "settings", "rds_timeout", NULL);
    conf.rds_discard = g_key_file_get_boolean(keyfile, "settings", "rds_discard", NULL);
    conf.rds_pty = g_key_file_get_integer(keyfile, "settings", "rds_pty", NULL);
    conf.rds_info_error = g_key_file_get_integer(keyfile, "settings", "rds_info_error", NULL);
    conf.rds_data_error = g_key_file_get_integer(keyfile, "settings", "rds_data_error", NULL);

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
    g_key_file_set_string(keyfile, "connection", "host", conf.host);
    g_key_file_set_integer(keyfile, "connection", "port", conf.port);

    g_key_file_set_integer(keyfile, "tuner", "mode", conf.mode);
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
    g_key_file_set_integer(keyfile, "settings", "rds_pty", conf.rds_pty);
    g_key_file_set_integer(keyfile, "settings", "rds_info_error", conf.rds_info_error);
    g_key_file_set_integer(keyfile, "settings", "rds_data_error", conf.rds_data_error);

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

    tmp = g_key_file_to_data(keyfile, &length, &error);
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
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

    GtkWidget *hbox_main = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(content), hbox_main);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(hbox_main), notebook);

    // ----------

    GtkWidget *page_interface = gtk_vbox_new(FALSE, 5);

    GtkWidget *table_signal = gtk_table_new(8, 2, TRUE);
    gtk_container_add(GTK_CONTAINER(page_interface), table_signal);

    row = 0;
    GtkWidget *l_display = gtk_label_new("Signal level:");
    gtk_table_attach(GTK_TABLE(table_signal), l_display, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_display = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_display), "text only");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_display), "graph");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_display), "bar");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_display), conf.signal_display);
    gtk_table_attach(GTK_TABLE(table_signal), c_display, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_unit = gtk_label_new("Signal unit:");
    gtk_table_attach(GTK_TABLE(table_signal), l_unit, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_unit = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "dBf");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "dBm");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "dBuV");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "S-meter");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_unit), conf.signal_unit);
    gtk_table_attach(GTK_TABLE(table_signal), c_unit, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_height = gtk_label_new("Graph height:");
    gtk_table_attach(GTK_TABLE(table_signal), l_height, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(conf.graph_height, 50.0, 300.0, 5.0, 10.0, 0.0);
    GtkWidget *s_height = gtk_spin_button_new(adj, 0, 0);
    gtk_table_attach(GTK_TABLE(table_signal), s_height, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_mono = gtk_label_new("Mono color:");
    gtk_table_attach(GTK_TABLE(table_signal), l_mono, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_mono = gtk_color_button_new_with_color(&conf.color_mono);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_mono), "Mono");
    gtk_table_attach(GTK_TABLE(table_signal), c_mono, 1, 2, row, row+1, 0, 0, 0, 0);

    row++;
    GtkWidget *l_stereo = gtk_label_new("Stereo color:");
    gtk_table_attach(GTK_TABLE(table_signal), l_stereo, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_stereo = gtk_color_button_new_with_color(&conf.color_stereo);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_stereo), "Stereo");
    gtk_table_attach(GTK_TABLE(table_signal), c_stereo, 1, 2, row, row+1, 0, 0, 0, 0);

    row++;
    GtkWidget *l_rds = gtk_label_new("RDS color:");
    gtk_table_attach(GTK_TABLE(table_signal), l_rds, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_rds = gtk_color_button_new_with_color(&conf.color_rds);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_rds), "RDS");
    gtk_table_attach(GTK_TABLE(table_signal), c_rds, 1, 2, row, row+1, 0, 0, 0, 0);

    row++;
    GtkWidget *x_avg = gtk_check_button_new_with_label("Signal level averaging");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_avg), conf.signal_avg);
    gtk_table_attach(GTK_TABLE(table_signal), x_avg, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_grid = gtk_check_button_new_with_label("Show grid");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_grid), conf.show_grid);
    gtk_table_attach(GTK_TABLE(table_signal), x_grid, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *x_utc = gtk_check_button_new_with_label("UTC time");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_utc), conf.utc);
    gtk_table_attach(GTK_TABLE(table_signal), x_utc, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_replace = gtk_check_button_new_with_label("Replace spaces with _ (clipboard)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_replace), conf.replace_spaces);
    gtk_table_attach(GTK_TABLE(table_signal), x_replace, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *page_interface_label = gtk_label_new("Interface");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_interface, page_interface_label);

    // ...

    GtkWidget *page_rds = gtk_vbox_new(FALSE, 5);

    GtkWidget *table_rds = gtk_table_new(6, 2, TRUE);
    gtk_container_add(GTK_CONTAINER(page_rds), table_rds);

    row = 0;
    GtkWidget *l_indicator = gtk_label_new("Indicator timeout:");
    gtk_table_attach(GTK_TABLE(table_rds), l_indicator, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkAdjustment *adj2 = (GtkAdjustment*)gtk_adjustment_new(conf.rds_timeout, 2.0, 30.0, 2.0, 200.0, 0.0);
    GtkWidget *s_indicator = gtk_spin_button_new(adj2, 0, 0);
    gtk_table_attach(GTK_TABLE(table_rds), s_indicator, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_discard = gtk_check_button_new_with_label("Discard RDS data after timeout");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_discard), conf.rds_discard);
    gtk_table_attach(GTK_TABLE(table_rds), x_discard, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_pty = gtk_label_new("PTYs:");
    gtk_table_attach(GTK_TABLE(table_rds), l_pty, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_pty = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_pty), "European (RDS)");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_pty), "USA (RBDS)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_pty), conf.rds_pty);
    gtk_table_attach(GTK_TABLE(table_rds), c_pty, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rds_error = gtk_label_new("PS & RT error correction:");
    gtk_table_attach(GTK_TABLE(table_rds), l_rds_error, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rds_info_error = gtk_label_new("Block 1 (position):");
    gtk_table_attach(GTK_TABLE(table_rds), l_rds_info_error, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *c_rds_info_error = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_info_error), "no errors");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_info_error), "max 2-bit correction");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_info_error), "max 5-bit correction");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_rds_info_error), conf.rds_info_error);
    gtk_table_attach(GTK_TABLE(table_rds), c_rds_info_error, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rds_data_error = gtk_label_new("Block 3/4 (data):");
    gtk_table_attach(GTK_TABLE(table_rds), l_rds_data_error, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *c_rds_data_error = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_data_error), "no errors");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_data_error), "max 2-bit correction");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rds_data_error), "max 5-bit correction");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_rds_data_error), conf.rds_data_error);
    gtk_table_attach(GTK_TABLE(table_rds), c_rds_data_error, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *page_rds_label = gtk_label_new("RDS");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_rds, page_rds_label);

    // ...

    GtkWidget *page_ant = gtk_vbox_new(FALSE, 5);

    GtkWidget *table_ant = gtk_table_new(5, 5, FALSE);
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

    // ----------

    GtkWidget *vbox_right = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(hbox_main), vbox_right);

    GtkWidget *frame_presets = gtk_frame_new("Presets [kHz]");
    gtk_container_add(GTK_CONTAINER(vbox_right), frame_presets);
    GtkWidget *table_presets = gtk_table_new(PRESETS, 2, FALSE);
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
    // workaround for windows to keep dialog over main window
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gui.menu_items.alwaysontop)))
    {
        gtk_window_set_keep_above(GTK_WINDOW(gui.window), FALSE);
    }
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gui.menu_items.alwaysontop)))
    {
        gtk_window_set_keep_above(GTK_WINDOW(gui.window), TRUE);
    }
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

        conf.rds_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_indicator));
        conf.rds_discard = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_discard));
        conf.rds_pty = gtk_combo_box_get_active(GTK_COMBO_BOX(c_pty));
        conf.rds_info_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_rds_info_error));
        conf.rds_data_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_rds_data_error));
        for(i=0; i<PRESETS; i++)
        {
            conf.presets[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_presets[i]));
        }

        conf.ant_switching = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_ant));
        for(i=0; i<ANTENNAS; i++)
        {
            conf.ant_start[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_ant_start[i]));
            conf.ant_stop[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_ant_stop[i]));
        }

        settings_write();
        graph_resize();
        signal_display();
    }
    gtk_widget_destroy(dialog);
}
