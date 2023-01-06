#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "ui.h"
#include "conf.h"
#include "ui-signal.h"
#include "rdsspy.h"
#include "version.h"
#include "log.h"
#include "scheduler.h"
#include "settings.h"
#include "stationlist.h"
#include "tuner.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

#define SCHEDULER_LIST_STORE_FREQ    0
#define SCHEDULER_LIST_STORE_TIMEOUT 1

static GtkWidget *dialog, *notebook;

/* Interface page */
static GtkWidget *page_interface;
static GtkWidget *grid_interface;
static GtkWidget *l_init_freq, *s_init_freq, *l_init_freq_unit;
static GtkWidget *l_event, *c_event;
static GtkWidget *x_utc, *x_autoconnect, *x_fmstep, *x_amstep;
static GtkWidget *x_disconnect_confirm, *x_auto_reconnect, *x_grab_focus;

/* Appearance page */
static GtkWidget *page_appearance;
static GtkWidget *grid_appearance;
static GtkWidget *x_hide_decorations, *x_hide_interference, *x_hide_radiotext, *x_hide_status, *x_restore_pos;
static GtkWidget *x_title_tuner_info, *x_accessibility, *x_horizontal_af, *x_dark_theme;

/* Signal page */
static GtkWidget *page_signal;
static GtkWidget *grid_signal;
static GtkWidget *l_signal_offset, *s_signal_offset, *l_signal_offset_unit;
static GtkWidget *l_unit, *c_unit;
static GtkWidget *l_display, *c_display;
static GtkWidget *l_signal_mode, *c_signal_mode;
static GtkWidget *l_colors, *box_colors, *c_mono, *c_stereo, *c_rds;
static GtkWidget *l_colors_dark, *box_colors_dark, *c_mono_dark, *c_stereo_dark, *c_rds_dark;
static GtkWidget *l_height, *s_height, *l_height_unit;
static GtkWidget *x_scroll, *x_avg, *x_grid;

/* RDS page */
static GtkWidget *page_rds;
static GtkWidget *grid_rds;
static GtkWidget *l_pty, *c_pty;
static GtkWidget *x_rds_reset, *l_rds_timeout, *s_rds_timeout;
static GtkWidget *l_ps_error, *l_ps_info_error, *c_ps_info_error, *l_ps_data_error, *c_ps_data_error;
static GtkWidget *x_psprog, *l_rt_error, *l_rt_info_error, *c_rt_info_error, *l_rt_data_error, *c_rt_data_error;

/* Antenna page */
static GtkWidget *page_ant;
static GtkWidget *grid_ant;
static GtkWidget *x_alignment, *x_swap_rotator;
static GtkWidget *l_ant_count, *c_ant_count;
static GtkWidget *x_ant_clear_rds, *x_ant_switching;
static GtkWidget *s_ant_start[ANT_COUNT], *s_ant_stop[ANT_COUNT];
static GtkWidget *l_ant_name, *l_ant_offset;
static GtkWidget *e_ant_name[ANT_COUNT], *s_ant_offset[ANT_COUNT];

/* Logs page */
static GtkWidget *page_logs, *l_fmlist;
static GtkWidget *grid_logs;
static GtkWidget *l_rdsspy_port, *s_rdsspy_port;
static GtkWidget *x_rdsspy_auto, *x_rdsspy_run, *c_rdsspy_command;
static GtkWidget *x_stationlist, *l_stationlist_port, *s_stationlist_port;
static GtkWidget *x_rds_logging, *x_replace;
static GtkWidget *l_log_dir, *c_log_dir_dialog, *c_log_dir;
static GtkWidget *l_screen_dir, *c_screen_dir_dialog, *c_screen_dir;

/* Keyboard page */
static GtkWidget *page_key;
static GtkWidget *grid_key;
static GtkWidget *l_tune_up, *b_tune_up;
static GtkWidget *l_tune_down, *b_tune_down;
static GtkWidget *l_tune_up_5, *b_tune_up_5;
static GtkWidget *l_tune_down_5, *b_tune_down_5;
static GtkWidget *l_tune_up_1000, *b_tune_up_1000;
static GtkWidget *l_tune_down_1000, *b_tune_down_1000;
static GtkWidget *l_tune_back, *b_tune_back;
static GtkWidget *l_reset, *b_reset;
static GtkWidget *l_screen, *b_screen;
static GtkWidget *l_bw_up, *b_bw_up;
static GtkWidget *l_bw_down, *b_bw_down;
static GtkWidget *l_bw_auto, *b_bw_auto;
static GtkWidget *l_rotate_cw, *b_rotate_cw;
static GtkWidget *l_rotate_ccw, *b_rotate_ccw;
static GtkWidget *l_switch_ant, *b_switch_ant;
static GtkWidget *l_key_ps_mode, *b_key_ps_mode;
static GtkWidget *l_scan_toggle, *b_key_scan_toggle;
static GtkWidget *l_scan_prev, *b_key_scan_prev;
static GtkWidget *l_scan_next, *b_key_scan_next;
static GtkWidget *l_stereo_toggle, *b_key_stereo_toggle;
static GtkWidget *l_mode_toggle, *b_key_mode_toggle;

/* Presets page */
static GtkWidget *page_presets, *presets_wrapper;
static GtkWidget *grid_presets;
static GtkWidget *s_presets[PRESETS];
static GtkWidget *l_presets_unit, *l_presets_info;

/* Scheduler page */
static GtkWidget *page_scheduler, *scheduler_treeview, *scheduler_scroll;
static GtkListStore *scheduler_liststore;
static GtkCellRenderer *scheduler_renderer_freq, *scheduler_renderer_timeout;
static GtkWidget *scheduler_add_box, *s_scheduler_freq, *s_scheduler_timeout;
static GtkWidget *b_scheduler_add, *scheduler_right_align, *scheduler_right_box;
static GtkWidget *b_scheduler_remove, *b_scheduler_clear;

/* About page */
static GtkWidget *page_about;
static GtkWidget *xdr_gtk_img, *xdr_gtk_title, *xdr_gtk_info;
static GtkWidget *xdr_gtk_copyright, *xdr_gtk_link;


static void settings_key(GtkWidget*, GdkEventButton*, gpointer);
static gboolean settings_key_press(GtkWidget*, GdkEventKey*, gpointer);
static void settings_scheduler_load();
static void settings_scheduler_store();
static gboolean settings_scheduler_store_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer*);
static void settings_scheduler_add(GtkWidget*, gpointer);
static void settings_scheduler_edit(GtkCellRendererText*, gchar*, gchar*, gpointer);
static void settings_scheduler_remove(GtkWidget*, gpointer);
static gboolean settings_scheduler_key(GtkWidget*, GdkEventKey*, gpointer);
static gboolean settings_scheduler_add_key(GtkWidget*, GdkEventKey*, gpointer);
static gboolean settings_scheduler_add_key_idle(gpointer);


void
settings_dialog(gint tab_num)
{
    gint row, i;
    gchar tmp[10];

    dialog = gtk_dialog_new_with_buttons("Settings",
                                         GTK_WINDOW(ui.window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Save", GTK_RESPONSE_ACCEPT,
                                         "_Cancel", GTK_RESPONSE_REJECT,
                                         NULL);

#ifdef G_OS_WIN32
    g_signal_connect(dialog, "realize", G_CALLBACK(win32_realize), NULL);
#endif

    gtk_window_set_icon_name(GTK_WINDOW(dialog), "xdr-gtk-settings");
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);
    gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), notebook);

    /* Interface page */
    page_interface = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_interface), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_interface, gtk_label_new("Interface"));
    gtk_container_child_set(GTK_CONTAINER(notebook), page_interface, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    grid_interface = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(grid_interface), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(grid_interface), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid_interface), 4);
    gtk_container_add(GTK_CONTAINER(page_interface), GTK_WIDGET(grid_interface));

    row = 0;
    l_init_freq = gtk_label_new("Initial frequency:");
    gtk_label_set_xalign(GTK_LABEL(l_init_freq), 0.0);
    gtk_grid_attach(GTK_GRID(grid_interface), l_init_freq, 0, row, 1, 1);

    s_init_freq = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.initial_freq, TUNER_FREQ_MIN, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
    gtk_grid_attach(GTK_GRID(grid_interface), s_init_freq, 1, row, 1, 1);

    l_init_freq_unit = gtk_label_new("kHz");
    gtk_grid_attach(GTK_GRID(grid_interface), l_init_freq_unit, 2, row, 1, 1);

    row++;
    l_event = gtk_label_new("External event:");
    gtk_label_set_xalign(GTK_LABEL(l_event), 0.0);
    gtk_grid_attach(GTK_GRID(grid_interface), l_event, 0, row, 1, 1);
    c_event = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_event), "no action");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_event), "grab focus");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_event), "screenshot");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_event), conf.event_action);
    gtk_grid_attach(GTK_GRID(grid_interface), c_event, 1, row, 2, 1);

    row++;
    x_utc = gtk_check_button_new_with_label("Use UTC instead of local time");
    gtk_widget_set_hexpand(x_utc, TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_utc), conf.utc);
    gtk_grid_attach(GTK_GRID(grid_interface), x_utc, 0, row, 3, 1);

    row++;
    x_autoconnect = gtk_check_button_new_with_label("Automatically connect on startup");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_autoconnect), conf.auto_connect);
    gtk_grid_attach(GTK_GRID(grid_interface), x_autoconnect, 0, row, 3, 1);

    row++;
    x_fmstep = gtk_check_button_new_with_label("10 kHz FM fine-tuning (instead of 5 kHz)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_fmstep), conf.fm_10k_steps);
    gtk_grid_attach(GTK_GRID(grid_interface), x_fmstep, 0, row, 3, 1);

    row++;
    x_amstep = gtk_check_button_new_with_label("10 kHz MW tuning (instead of 9 kHz)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_amstep), conf.mw_10k_steps);
    gtk_grid_attach(GTK_GRID(grid_interface), x_amstep, 0, row, 3, 1);

    row++;
    x_disconnect_confirm = gtk_check_button_new_with_label("Show disconnect confirmation");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_disconnect_confirm), conf.disconnect_confirm);
    gtk_grid_attach(GTK_GRID(grid_interface), x_disconnect_confirm, 0, row, 3, 1);

    row++;
    x_auto_reconnect = gtk_check_button_new_with_label("Connect again automatically");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_auto_reconnect), conf.auto_reconnect);
    gtk_grid_attach(GTK_GRID(grid_interface), x_auto_reconnect, 0, row, 3, 1);

    row++;
    x_grab_focus = gtk_check_button_new_with_label("Grab focus on frequency change");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_grab_focus), conf.grab_focus);
    gtk_grid_attach(GTK_GRID(grid_interface), x_grab_focus, 0, row, 3, 1);


    /* Interface page */
    page_appearance = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_appearance), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_appearance, gtk_label_new("Appearance"));
    gtk_container_child_set(GTK_CONTAINER(notebook), page_appearance, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    grid_appearance = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(grid_appearance), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(grid_appearance), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid_appearance), 4);
    gtk_container_add(GTK_CONTAINER(page_appearance), GTK_WIDGET(grid_appearance));

    row = 0;
#ifdef G_OS_WIN32
    x_hide_decorations = gtk_check_button_new_with_label("Hide window decorations (restart required for restore)");
#else
    x_hide_decorations = gtk_check_button_new_with_label("Hide window decorations");
#endif
    gtk_widget_set_hexpand(x_hide_decorations, TRUE);
    gtk_widget_set_tooltip_text(x_hide_decorations, "When no window decorations are displayed, one still can:\n- Move window with Shift + mouse drag.\n- Close application with Alt + F4.");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_hide_decorations), conf.hide_decorations);
    gtk_grid_attach(GTK_GRID(grid_appearance), x_hide_decorations, 0, row, 3, 1);

    row++;
    x_hide_interference = gtk_check_button_new_with_label("Hide interference detectors (CCI, ACI)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_hide_interference), conf.hide_interference);
    gtk_grid_attach(GTK_GRID(grid_appearance), x_hide_interference, 0, row, 3, 1);

    row++;
    x_hide_radiotext = gtk_check_button_new_with_label("Hide RadioText");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_hide_radiotext), conf.hide_radiotext);
    gtk_grid_attach(GTK_GRID(grid_appearance), x_hide_radiotext, 0, row, 3, 1);

    row++;
    x_hide_status = gtk_check_button_new_with_label("Hide statusbar (with clock)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_hide_status), conf.hide_statusbar);
    gtk_grid_attach(GTK_GRID(grid_appearance), x_hide_status, 0, row, 3, 1);

    row++;
    x_restore_pos = gtk_check_button_new_with_label("Restore window positions on screen");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_restore_pos), conf.restore_position);
    gtk_grid_attach(GTK_GRID(grid_appearance), x_restore_pos, 0, row, 3, 1);

    row++;
    x_title_tuner_info = gtk_check_button_new_with_label("Display tuner info in application title");
    gtk_widget_set_tooltip_text(x_title_tuner_info, "Ctrl + 1: Freq/Signal/PI\nCtrl + 2: PS\nCtrl + 3: RT 1\nCtrl + 4: RT 2\nCtrl + 5: PTY\nCtrl + 6: AF list");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_title_tuner_info), conf.title_tuner_info);
    gtk_grid_attach(GTK_GRID(grid_appearance), x_title_tuner_info, 0, row, 3, 1);

    row++;
    x_accessibility = gtk_check_button_new_with_label("Accessibility enhancements");
    gtk_widget_set_tooltip_text(x_accessibility, "Useful for screen readers. Indicators are removed instead of being grayed out.");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_accessibility), conf.accessibility);
    gtk_grid_attach(GTK_GRID(grid_appearance), x_accessibility, 0, row, 3, 1);

    row++;
    x_horizontal_af = gtk_check_button_new_with_label("AFs in a horizontal list (restart required)");
    gtk_widget_set_tooltip_text(x_horizontal_af, "Another feature useful for screen readers.");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_horizontal_af), conf.horizontal_af);
    gtk_grid_attach(GTK_GRID(grid_appearance), x_horizontal_af, 0, row, 3, 1);

    row++;
    x_dark_theme = gtk_check_button_new_with_label("Dark theme mode (restart required)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_dark_theme), conf.dark_theme);
    gtk_grid_attach(GTK_GRID(grid_appearance), x_dark_theme, 0, row, 3, 1);


    /* Signal page */
    page_signal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_signal), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_signal, gtk_label_new("Signal"));
    gtk_container_child_set(GTK_CONTAINER(notebook), page_signal, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    grid_signal = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(grid_signal), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(grid_signal), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid_signal), 4);
    gtk_container_add(GTK_CONTAINER(page_signal), GTK_WIDGET(grid_signal));

    row = 0;
    l_unit = gtk_label_new("Signal unit:");
    gtk_label_set_xalign(GTK_LABEL(l_unit), 0.0);
    gtk_grid_attach(GTK_GRID(grid_signal), l_unit, 0, row, 1, 1);
    c_unit = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_unit), "dBf");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_unit), "dBm");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_unit), "dBuV");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_unit), conf.signal_unit);
    gtk_grid_attach(GTK_GRID(grid_signal), c_unit, 1, row, 2, 1);

    row++;
    l_display = gtk_label_new("Signal display:");
    gtk_label_set_xalign(GTK_LABEL(l_display), 0.0);
    gtk_grid_attach(GTK_GRID(grid_signal), l_display, 0, row, 1, 1);
    c_display = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_display), "text only");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_display), "graph");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_display), "bar");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_display), conf.signal_display);
    gtk_grid_attach(GTK_GRID(grid_signal), c_display, 1, row, 2, 1);

    row++;
    l_signal_mode = gtk_label_new("Frequency change:");
    gtk_label_set_xalign(GTK_LABEL(l_signal_mode), 0.0);
    gtk_grid_attach(GTK_GRID(grid_signal), l_signal_mode, 0, row, 1, 1);
    c_signal_mode = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_signal_mode), "no action");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_signal_mode), "reset graph");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_signal_mode), conf.signal_mode);
    gtk_grid_attach(GTK_GRID(grid_signal), c_signal_mode, 1, row, 2, 1);

    row++;
    l_colors = gtk_label_new("Graph colors:");
    gtk_label_set_xalign(GTK_LABEL(l_colors), 0.0);
    gtk_grid_attach(GTK_GRID(grid_signal), l_colors, 0, row, 1, 1);

    box_colors = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    c_mono = gtk_color_button_new_with_rgba(&conf.color_mono);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_mono), "Mono color");
    gtk_widget_set_tooltip_text(c_mono, "Mono color");
    gtk_box_pack_start(GTK_BOX(box_colors), c_mono, TRUE, TRUE, 0);

    c_stereo = gtk_color_button_new_with_rgba(&conf.color_stereo);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_stereo), "Stereo color");
    gtk_widget_set_tooltip_text(c_stereo, "Stereo color");
    gtk_box_pack_start(GTK_BOX(box_colors), c_stereo, TRUE, TRUE, 0);

    c_rds = gtk_color_button_new_with_rgba(&conf.color_rds);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_rds), "RDS color");
    gtk_widget_set_tooltip_text(c_rds, "RDS color");
    gtk_box_pack_start(GTK_BOX(box_colors), c_rds, TRUE, TRUE, 0);

    gtk_grid_attach(GTK_GRID(grid_signal), box_colors, 1, row, 2, 1);

    row++;
    l_colors_dark = gtk_label_new("Graph colors (dark):");
    gtk_label_set_xalign(GTK_LABEL(l_colors_dark), 0.0);
    gtk_grid_attach(GTK_GRID(grid_signal), l_colors_dark, 0, row, 1, 1);

    box_colors_dark = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    c_mono_dark = gtk_color_button_new_with_rgba(&conf.color_mono_dark);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_mono_dark), "Mono color");
    gtk_widget_set_tooltip_text(c_mono_dark, "Mono color");
    gtk_box_pack_start(GTK_BOX(box_colors_dark), c_mono_dark, TRUE, TRUE, 0);

    c_stereo_dark = gtk_color_button_new_with_rgba(&conf.color_stereo_dark);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_stereo_dark), "Stereo color");
    gtk_widget_set_tooltip_text(c_stereo_dark, "Stereo color");
    gtk_box_pack_start(GTK_BOX(box_colors_dark), c_stereo_dark, TRUE, TRUE, 0);

    c_rds_dark = gtk_color_button_new_with_rgba(&conf.color_rds_dark);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_rds_dark), "RDS color");
    gtk_widget_set_tooltip_text(c_rds_dark, "RDS color");
    gtk_box_pack_start(GTK_BOX(box_colors_dark), c_rds_dark, TRUE, TRUE, 0);

    gtk_grid_attach(GTK_GRID(grid_signal), box_colors_dark, 1, row, 2, 1);

    row++;
    l_height = gtk_label_new("Graph height:");
    gtk_label_set_xalign(GTK_LABEL(l_height), 0.0);
    gtk_grid_attach(GTK_GRID(grid_signal), l_height, 0, row, 1, 1);
    s_height = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.signal_height, 50.0, 300.0, 5.0, 10.0, 0.0)), 0, 0);
    gtk_grid_attach(GTK_GRID(grid_signal), s_height, 1, row, 1, 1);
    l_height_unit = gtk_label_new("px");
    gtk_grid_attach(GTK_GRID(grid_signal), l_height_unit, 2, row, 1, 1);

    row++;
    l_signal_offset = gtk_label_new("Level offset:");
    gtk_label_set_xalign(GTK_LABEL(l_signal_offset), 0.0);
    gtk_grid_attach(GTK_GRID(grid_signal), l_signal_offset, 0, row, 1, 1);
    s_signal_offset = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.signal_offset, -50.0, +50.0, 1.0, 2.0, 0.0)), 0, 1);
    gtk_grid_attach(GTK_GRID(grid_signal), s_signal_offset, 1, row, 1, 1);
    l_signal_offset_unit = gtk_label_new("dB");
    gtk_grid_attach(GTK_GRID(grid_signal), l_signal_offset_unit, 2, row, 1, 1);

    row++;
    x_scroll = gtk_check_button_new_with_label("Tune with mouse scroll over signal graph");
    gtk_widget_set_hexpand(x_scroll, TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_scroll), conf.signal_scroll);
    gtk_grid_attach(GTK_GRID(grid_signal), x_scroll, 0, row, 3, 1);

    row++;
    x_avg = gtk_check_button_new_with_label("Average the signal level");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_avg), conf.signal_avg);
    gtk_grid_attach(GTK_GRID(grid_signal), x_avg, 0, row, 3, 1);

    row++;
    x_grid = gtk_check_button_new_with_label("Draw grid");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_grid), conf.signal_grid);
    gtk_grid_attach(GTK_GRID(grid_signal), x_grid, 0, row, 3, 1);

   /* RDS page */
    page_rds = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_rds), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_rds, gtk_label_new("RDS"));

    grid_rds = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(grid_rds), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(grid_rds), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid_rds), 4);
    gtk_container_add(GTK_CONTAINER(page_rds), GTK_WIDGET(grid_rds));

    row = 0;
    l_pty = gtk_label_new("PTY set:");
    gtk_label_set_xalign(GTK_LABEL(l_pty), 0.0);
    gtk_grid_attach(GTK_GRID(grid_rds), l_pty, 0, row, 1, 1);
    c_pty = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_pty), "RDS/EU");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_pty), "RBDS/US");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_pty), conf.rds_pty_set);
    gtk_grid_attach(GTK_GRID(grid_rds), c_pty, 1, row, 1, 1);

    row++;
    x_rds_reset = gtk_check_button_new_with_label("Reset RDS after specified timeout");
    gtk_widget_set_hexpand(x_rds_reset, TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_rds_reset), conf.rds_reset);
    gtk_grid_attach(GTK_GRID(grid_rds), x_rds_reset, 0, row, 2, 1);

    row++;
    l_rds_timeout = gtk_label_new("RDS timeout [s]:");
    gtk_label_set_xalign(GTK_LABEL(l_rds_timeout), 0.0);
    gtk_grid_attach(GTK_GRID(grid_rds), l_rds_timeout, 0, row, 1, 1);
    s_rds_timeout = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.rds_reset_timeout, 1.0, 10000.0, 10.0, 60.0, 0.0)), 0, 0);
    gtk_grid_attach(GTK_GRID(grid_rds), s_rds_timeout, 1, row, 1, 1);

    row++;
    gtk_grid_attach(GTK_GRID(grid_rds), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row, 2, 1);

    row++;
    l_ps_error = gtk_label_new("RDS PS error correction:");
    gtk_grid_attach(GTK_GRID(grid_rds), l_ps_error, 0, row, 2, 1);

    row++;
    l_ps_info_error = gtk_label_new("Group Type / Position:");
    gtk_label_set_xalign(GTK_LABEL(l_ps_info_error), 0.0);
    gtk_grid_attach(GTK_GRID(grid_rds), l_ps_info_error, 0, row, 1, 1);

    c_ps_info_error = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_ps_info_error), "no errors");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_ps_info_error), "max 2-bit");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_ps_info_error), "max 5-bit");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_ps_info_error), conf.rds_ps_info_error);
    gtk_grid_attach(GTK_GRID(grid_rds), c_ps_info_error, 1, row, 1, 1);

    row++;
    l_ps_data_error = gtk_label_new("Data:");
    gtk_label_set_xalign(GTK_LABEL(l_ps_data_error), 0.0);
    gtk_grid_attach(GTK_GRID(grid_rds), l_ps_data_error, 0, row, 1, 1);

    c_ps_data_error = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_ps_data_error), "no errors");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_ps_data_error), "max 2-bit");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_ps_data_error), "max 5-bit");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_ps_data_error), conf.rds_ps_data_error);
    gtk_grid_attach(GTK_GRID(grid_rds), c_ps_data_error, 1, row, 1, 1);

    row++;
    x_psprog = gtk_check_button_new_with_label("PS progressive correction");
    gtk_widget_set_tooltip_text(x_psprog, "Replace characters in the RDS PS only with lower or the same error level. This also overrides the error correction to 5/5bit. Useful for static RDS PS. For a quick toggle, click a right mouse button on the displayed RDS PS.");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_psprog), conf.rds_ps_progressive);
    gtk_grid_attach(GTK_GRID(grid_rds), x_psprog, 0, row, 2, 1);

    row++;
    gtk_grid_attach(GTK_GRID(grid_rds), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row, 2, 1);

    row++;
    l_rt_error = gtk_label_new("Radio Text error correction:");
    gtk_grid_attach(GTK_GRID(grid_rds), l_rt_error, 0, row, 2, 1);

    row++;
    l_rt_info_error = gtk_label_new("Group Type / Position:");
    gtk_label_set_xalign(GTK_LABEL(l_rt_info_error), 0.0);
    gtk_grid_attach(GTK_GRID(grid_rds), l_rt_info_error, 0, row, 1, 1);

    c_rt_info_error = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_rt_info_error), "no errors");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_rt_info_error), "max 2-bit");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_rt_info_error), "max 5-bit");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_rt_info_error), conf.rds_rt_info_error);
    gtk_grid_attach(GTK_GRID(grid_rds), c_rt_info_error, 1, row, 1, 1);

    row++;
    l_rt_data_error = gtk_label_new("Data:");
    gtk_label_set_xalign(GTK_LABEL(l_rt_data_error), 0.0);
    gtk_grid_attach(GTK_GRID(grid_rds), l_rt_data_error, 0, row, 1, 1);

    c_rt_data_error = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_rt_data_error), "no errors");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_rt_data_error), "max 2-bit");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_rt_data_error), "max 5-bit");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_rt_data_error), conf.rds_rt_data_error);
    gtk_grid_attach(GTK_GRID(grid_rds), c_rt_data_error, 1, row, 1, 1);


    /* Antenna page */
    page_ant = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_ant), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_ant, gtk_label_new("Antenna"));

    grid_ant = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(grid_ant), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(grid_ant), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid_ant), 4);
    gtk_container_add(GTK_CONTAINER(page_ant), GTK_WIDGET(grid_ant));

    row = 0;
    x_alignment = gtk_check_button_new_with_label("Show antenna input alignment");
    gtk_widget_set_hexpand(x_alignment, TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_alignment), conf.ant_show_alignment);
    gtk_grid_attach(GTK_GRID(grid_ant), x_alignment, 0, row, 5, 1);

    row++;
    x_swap_rotator = gtk_check_button_new_with_label("Swap antenna rotator buttons");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_swap_rotator), conf.ant_swap_rotator);
    gtk_grid_attach(GTK_GRID(grid_ant), x_swap_rotator, 0, row, 5, 1);

    row++;
    gtk_grid_attach(GTK_GRID(grid_ant), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row, 5, 1);

    row++;
    l_ant_count = gtk_label_new("Antenna switch count:");
    gtk_label_set_xalign(GTK_LABEL(l_ant_count), 0.0);
    gtk_grid_attach(GTK_GRID(grid_ant), l_ant_count, 0, row, 3, 1);
    c_ant_count = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_ant_count), "hidden");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_ant_count), "2");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_ant_count), "3");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_ant_count), "4");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_ant_count), conf.ant_count-1);
    gtk_grid_attach(GTK_GRID(grid_ant), c_ant_count, 3, row, 2, 1);

    row++;
    x_ant_clear_rds = gtk_check_button_new_with_label("Reset RDS data after switching");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_ant_clear_rds), conf.ant_clear_rds);
    gtk_grid_attach(GTK_GRID(grid_ant), x_ant_clear_rds, 0, row, 5, 1);

    row++;
    x_ant_switching = gtk_check_button_new_with_label("Automatic antenna switching");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_ant_switching), conf.ant_auto_switch);
    gtk_grid_attach(GTK_GRID(grid_ant), x_ant_switching, 0, row, 5, 1);

    for(i=0; i<ANT_COUNT; i++)
    {
        row++;
        g_snprintf(tmp, sizeof(tmp), "Range %c:", 'A'+i);
        gtk_grid_attach(GTK_GRID(grid_ant), gtk_label_new(tmp), 0, row, 1, 1);
        s_ant_start[i] = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.ant_start[i], 0.0, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
        gtk_grid_attach(GTK_GRID(grid_ant), s_ant_start[i], 1, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid_ant), gtk_label_new(" - "), 2, row, 1, 1);
        s_ant_stop[i] = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.ant_stop[i], 0.0, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
        gtk_grid_attach(GTK_GRID(grid_ant), s_ant_stop[i], 3, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid_ant), gtk_label_new("kHz"), 4, row, 1, 1);
    }

    row++;
    gtk_grid_attach(GTK_GRID(grid_ant), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row, 5, 1);

    row++;
    l_ant_name = gtk_label_new("Antenna name:");
    gtk_grid_attach(GTK_GRID(grid_ant), l_ant_name, 1, row, 1, 1);
    l_ant_offset = gtk_label_new("Frequency offset:");
    gtk_grid_attach(GTK_GRID(grid_ant), l_ant_offset, 3, row, 1, 1);

    for(i=0; i<ANT_COUNT; i++)
    {
        row++;
        g_snprintf(tmp, sizeof(tmp), "Ant %c:", 'A'+i);
        gtk_grid_attach(GTK_GRID(grid_ant), gtk_label_new(tmp), 0, row, 1, 1);

        e_ant_name[i] = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(e_ant_name[i]), conf.ant_name[i]);
        gtk_entry_set_max_length(GTK_ENTRY(e_ant_name[i]), 10);
        gtk_entry_set_width_chars(GTK_ENTRY(e_ant_name[i]), 10);
        gtk_grid_attach(GTK_GRID(grid_ant), e_ant_name[i], 1, row, 1, 1);

        s_ant_offset[i] = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.ant_offset[i], -100000.0, 1000000.0, 100.0, 200.0, 0.0)), 0, 0);
        gtk_grid_attach(GTK_GRID(grid_ant), s_ant_offset[i], 3, row, 1, 1);

        gtk_grid_attach(GTK_GRID(grid_ant), gtk_label_new("kHz"), 4, row, 1, 1);
    }

    /* Logs page */
    page_logs = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_logs), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_logs, gtk_label_new("Logs"));

    grid_logs = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(grid_logs), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(grid_logs), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid_logs), 4);
    gtk_container_add(GTK_CONTAINER(page_logs), GTK_WIDGET(grid_logs));

    row = 0;
    l_fmlist = gtk_label_new(NULL);
    gtk_widget_set_hexpand(l_fmlist, TRUE);
    gtk_label_set_markup(GTK_LABEL(l_fmlist), "See <a href=\"https://fmlist.org\">FMLIST</a>, the worldwide FM radio\n station list. Report to it what stations\n you receive at home and from far away.");
    gtk_label_set_justify(GTK_LABEL(l_fmlist), GTK_JUSTIFY_CENTER);
#ifdef G_OS_WIN32
    g_signal_connect(l_fmlist, "activate-link", G_CALLBACK(win32_uri), NULL);
#endif
    gtk_grid_attach(GTK_GRID(grid_logs), l_fmlist, 0, row, 2, 1);

    row++;
    gtk_grid_attach(GTK_GRID(grid_logs), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row, 2, 1);

    row++;
    l_rdsspy_port = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(l_rdsspy_port), "<a href=\"https://rdsspy.com/\">RDS Spy</a> ASCII G Port:");
#ifdef G_OS_WIN32
    g_signal_connect(l_rdsspy_port, "activate-link", G_CALLBACK(win32_uri), NULL);
#endif
    gtk_label_set_xalign(GTK_LABEL(l_rdsspy_port), 0.0);
    gtk_grid_attach(GTK_GRID(grid_logs), l_rdsspy_port, 0, row, 1, 1);
    s_rdsspy_port = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.rdsspy_port, 1024.0, 65535.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_grid_attach(GTK_GRID(grid_logs), s_rdsspy_port, 1, row, 1, 1);

    row++;
    x_rdsspy_auto = gtk_check_button_new_with_label("Enable RDS Spy server on startup");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_rdsspy_auto), conf.rdsspy_auto);
    gtk_grid_attach(GTK_GRID(grid_logs), x_rdsspy_auto, 0, row, 2, 1);

    row++;
    x_rdsspy_run = gtk_check_button_new_with_label("Automatic RDS Spy start (rdsspy.exe):");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_rdsspy_run), conf.rdsspy_run);
    gtk_grid_attach(GTK_GRID(grid_logs), x_rdsspy_run, 0, row, 2, 1);

    row++;
    c_rdsspy_command = gtk_file_chooser_button_new("RDS Spy Executable File", GTK_FILE_CHOOSER_ACTION_OPEN);
#ifdef G_OS_WIN32
    g_signal_connect(c_rdsspy_command, "realize", G_CALLBACK(win32_realize), NULL);
#endif
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "RDS Spy Executable File (rdsspy.exe)");
    gtk_file_filter_add_pattern(filter, "rdsspy.exe");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(c_rdsspy_command), filter);
    GtkFileFilter* filter2 = gtk_file_filter_new();
    gtk_file_filter_set_name(filter2, "All files");
    gtk_file_filter_add_pattern(filter2, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(c_rdsspy_command), filter2);
    if(strlen(conf.rdsspy_exec))
    {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(c_rdsspy_command), conf.rdsspy_exec);
    }
    gtk_grid_attach(GTK_GRID(grid_logs), c_rdsspy_command, 0, row, 2, 1);

    row++;
    gtk_grid_attach(GTK_GRID(grid_logs), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row, 2, 1);

    row++;
    x_stationlist = gtk_check_button_new_with_label("Enable SRCP (StationList)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_stationlist), conf.srcp);
    gtk_grid_attach(GTK_GRID(grid_logs), x_stationlist, 0, row, 2, 1);

    row++;
    l_stationlist_port = gtk_label_new("SRCP Radio UDP Port:");
    gtk_label_set_xalign(GTK_LABEL(l_stationlist_port), 0.0);
    gtk_grid_attach(GTK_GRID(grid_logs), l_stationlist_port, 0, row, 1, 1);
    s_stationlist_port = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.srcp_port, 1025.0, 65535.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_grid_attach(GTK_GRID(grid_logs), s_stationlist_port, 1, row, 1, 1);

    row++;
    gtk_grid_attach(GTK_GRID(grid_logs), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row, 2, 1);

    row++;
    x_rds_logging = gtk_check_button_new_with_label("Enable simple RDS logging");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_rds_logging), conf.rds_logging);
    gtk_grid_attach(GTK_GRID(grid_logs), x_rds_logging, 0, row, 2, 1);

    row++;
    x_replace = gtk_check_button_new_with_label("Replace spaces with underscores");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_replace), conf.replace_spaces);
    gtk_grid_attach(GTK_GRID(grid_logs), x_replace, 0, row, 2, 1);

    row++;
    l_log_dir = gtk_label_new("Log directory:");
    gtk_label_set_xalign(GTK_LABEL(l_log_dir), 0.0);
    gtk_grid_attach(GTK_GRID(grid_logs), l_log_dir, 0, row, 1, 1);
    c_log_dir_dialog = gtk_file_chooser_dialog_new("Log directory", NULL,
                                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   "_Open", GTK_RESPONSE_ACCEPT,
                                                   NULL);
#ifdef G_OS_WIN32
    g_signal_connect(c_log_dir_dialog, "realize", G_CALLBACK(win32_realize), NULL);
#endif

    gtk_window_set_keep_above(GTK_WINDOW(c_log_dir_dialog), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.b_ontop)));
    c_log_dir = gtk_file_chooser_button_new_with_dialog(c_log_dir_dialog);
    if(strlen(conf.log_dir))
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(c_log_dir), conf.log_dir);
    gtk_file_chooser_set_action(GTK_FILE_CHOOSER(c_log_dir_dialog), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER); /* HACK */
    gtk_grid_attach(GTK_GRID(grid_logs), c_log_dir, 1, row, 1, 1);

    row++;
    l_screen_dir = gtk_label_new("Screenshot directory:");
    gtk_label_set_xalign(GTK_LABEL(l_screen_dir), 0.0);
    gtk_grid_attach(GTK_GRID(grid_logs), l_screen_dir, 0, row, 1, 1);
    c_screen_dir_dialog = gtk_file_chooser_dialog_new("Screenshot directory", NULL,
                                                      GTK_FILE_CHOOSER_ACTION_OPEN,
                                                      "_Cancel", GTK_RESPONSE_CANCEL,
                                                      "_Open", GTK_RESPONSE_ACCEPT,
                                                      NULL);
#ifdef G_OS_WIN32
    g_signal_connect(c_screen_dir_dialog, "realize", G_CALLBACK(win32_realize), NULL);
#endif
    gtk_window_set_keep_above(GTK_WINDOW(c_screen_dir_dialog), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.b_ontop)));
    c_screen_dir = gtk_file_chooser_button_new_with_dialog(c_screen_dir_dialog);
    if(strlen(conf.screen_dir))
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(c_screen_dir), conf.screen_dir);
    gtk_file_chooser_set_action(GTK_FILE_CHOOSER(c_screen_dir_dialog), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER); /* HACK */
    gtk_grid_attach(GTK_GRID(grid_logs), c_screen_dir, 1, row, 1, 1);

    /* Keyboard page */
    page_key = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(page_key), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_key, gtk_label_new("Keyboard"));

    grid_key = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(grid_key), 4);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid_key), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(grid_key), 1);
    gtk_grid_set_column_spacing(GTK_GRID(grid_key), 0);
    gtk_container_add(GTK_CONTAINER(page_key), GTK_WIDGET(grid_key));

    row = 0;
    l_tune_up = gtk_label_new("Tune up");
    gtk_widget_set_hexpand(l_tune_up, TRUE);
    gtk_label_set_xalign(GTK_LABEL(l_tune_up), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_tune_up, 0, row, 1, 1);
    b_tune_up = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_up));
    gtk_widget_set_hexpand(b_tune_up, TRUE);
    g_signal_connect(b_tune_up, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_tune_up, 1, row, 1, 1);

    row++;
    l_tune_down = gtk_label_new("Tune down");
    gtk_label_set_xalign(GTK_LABEL(l_tune_down), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_tune_down, 0, row, 1, 1);
    b_tune_down = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_down));
    g_signal_connect(b_tune_down, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_tune_down, 1, row, 1, 1);

    row++;
    l_tune_up_5 = gtk_label_new("Fine tune up");
    gtk_label_set_xalign(GTK_LABEL(l_tune_up_5), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_tune_up_5, 0, row, 1, 1);
    b_tune_up_5 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_fine_up));
    g_signal_connect(b_tune_up_5, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_tune_up_5, 1, row, 1, 1);

    row++;
    l_tune_down_5 = gtk_label_new("Fine tune down");
    gtk_label_set_xalign(GTK_LABEL(l_tune_down_5), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_tune_down_5, 0, row, 1, 1);
    b_tune_down_5 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_fine_down));
    g_signal_connect(b_tune_down_5, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_tune_down_5, 1, row, 1, 1);

    row++;
    l_tune_up_1000 = gtk_label_new("Tune +1 MHz");
    gtk_label_set_xalign(GTK_LABEL(l_tune_up_1000), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_tune_up_1000, 0, row, 1, 1);
    b_tune_up_1000 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_jump_up));
    g_signal_connect(b_tune_up_1000, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_tune_up_1000, 1, row, 1, 1);

    row++;
    l_tune_down_1000 = gtk_label_new("Tune -1 MHz");
    gtk_label_set_xalign(GTK_LABEL(l_tune_down_1000), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_tune_down_1000, 0, row, 1, 1);
    b_tune_down_1000 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_jump_down));
    g_signal_connect(b_tune_down_1000, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_tune_down_1000, 1, row, 1, 1);

    row++;
    l_tune_back = gtk_label_new("Tune back");
    gtk_label_set_xalign(GTK_LABEL(l_tune_back), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_tune_back, 0, row, 1, 1);
    b_tune_back = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_back));
    g_signal_connect(b_tune_back, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_tune_back, 1, row, 1, 1);

    row++;
    l_reset = gtk_label_new("Reset frequency");
    gtk_label_set_xalign(GTK_LABEL(l_reset), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_reset, 0, row, 1, 1);
    b_reset = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_reset));
    g_signal_connect(b_reset, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_reset, 1, row, 1, 1);

    row++;
    l_screen = gtk_label_new("Screenshot");
    gtk_label_set_xalign(GTK_LABEL(l_screen), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_screen, 0, row, 1, 1);
    b_screen = gtk_button_new_with_label(gdk_keyval_name(conf.key_screenshot));
    g_signal_connect(b_screen, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_screen, 1, row, 1, 1);

    row++;
    l_bw_up = gtk_label_new("Increase bandwidth");
    gtk_label_set_xalign(GTK_LABEL(l_bw_up), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_bw_up, 0, row, 1, 1);
    b_bw_up = gtk_button_new_with_label(gdk_keyval_name(conf.key_bw_up));
    g_signal_connect(b_bw_up, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_bw_up, 1, row, 1, 1);

    row++;
    l_bw_down = gtk_label_new("Decrease bandwidth");
    gtk_label_set_xalign(GTK_LABEL(l_bw_down), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_bw_down, 0, row, 1, 1);
    b_bw_down = gtk_button_new_with_label(gdk_keyval_name(conf.key_bw_down));
    g_signal_connect(b_bw_down, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_bw_down, 1, row, 1, 1);

    row++;
    l_bw_auto = gtk_label_new("Adaptive bandwidth");
    gtk_label_set_xalign(GTK_LABEL(l_bw_auto), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_bw_auto, 0, row, 1, 1);
    b_bw_auto = gtk_button_new_with_label(gdk_keyval_name(conf.key_bw_auto));
    g_signal_connect(b_bw_auto, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_bw_auto, 1, row, 1, 1);

    row++;
    l_rotate_cw = gtk_label_new("Rotate CW");
    gtk_label_set_xalign(GTK_LABEL(l_rotate_cw), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_rotate_cw, 0, row, 1, 1);
    b_rotate_cw = gtk_button_new_with_label(gdk_keyval_name(conf.key_rotate_cw));
    g_signal_connect(b_rotate_cw, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_rotate_cw, 1, row, 1, 1);

    row++;
    l_rotate_ccw = gtk_label_new("Rotate CCW");
    gtk_label_set_xalign(GTK_LABEL(l_rotate_ccw), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_rotate_ccw, 0, row, 1, 1);
    b_rotate_ccw = gtk_button_new_with_label(gdk_keyval_name(conf.key_rotate_ccw));
    g_signal_connect(b_rotate_ccw, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_rotate_ccw, 1, row, 1, 1);

    row++;
    l_switch_ant = gtk_label_new("Switch antenna");
    gtk_label_set_xalign(GTK_LABEL(l_switch_ant), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_switch_ant, 0, row, 1, 1);
    b_switch_ant = gtk_button_new_with_label(gdk_keyval_name(conf.key_switch_antenna));
    g_signal_connect(b_switch_ant, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_switch_ant, 1, row, 1, 1);

    row++;
    l_key_ps_mode = gtk_label_new("Toggle RDS PS mode");
    gtk_label_set_xalign(GTK_LABEL(l_key_ps_mode), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_key_ps_mode, 0, row, 1, 1);
    b_key_ps_mode = gtk_button_new_with_label(gdk_keyval_name(conf.key_rds_ps_mode));
    g_signal_connect(b_key_ps_mode, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_key_ps_mode, 1, row, 1, 1);

    row++;
    l_scan_toggle = gtk_label_new("Toggle spectral scan");
    gtk_label_set_xalign(GTK_LABEL(l_scan_toggle), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_scan_toggle, 0, row, 1, 1);
    b_key_scan_toggle = gtk_button_new_with_label(gdk_keyval_name(conf.key_scan_toggle));
    g_signal_connect(b_key_scan_toggle, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_key_scan_toggle, 1, row, 1, 1);

    row++;
    l_scan_prev = gtk_label_new("Switch to previous scan mark");
    gtk_label_set_xalign(GTK_LABEL(l_scan_prev), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_scan_prev, 0, row, 1, 1);
    b_key_scan_prev = gtk_button_new_with_label(gdk_keyval_name(conf.key_scan_prev));
    g_signal_connect(b_key_scan_prev, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_key_scan_prev, 1, row, 1, 1);

    row++;
    l_scan_next = gtk_label_new("Switch to next scan mark");
    gtk_label_set_xalign(GTK_LABEL(l_scan_next), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_scan_next, 0, row, 1, 1);
    b_key_scan_next = gtk_button_new_with_label(gdk_keyval_name(conf.key_scan_next));
    g_signal_connect(b_key_scan_next, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_key_scan_next, 1, row, 1, 1);

    row++;
    l_stereo_toggle = gtk_label_new("Stereo toggle");
    gtk_label_set_xalign(GTK_LABEL(l_stereo_toggle), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_stereo_toggle, 0, row, 1, 1);
    b_key_stereo_toggle = gtk_button_new_with_label(gdk_keyval_name(conf.key_stereo_toggle));
    g_signal_connect(b_key_stereo_toggle, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_key_stereo_toggle, 1, row, 1, 1);

    row++;
    l_mode_toggle = gtk_label_new("Mode toggle");
    gtk_label_set_xalign(GTK_LABEL(l_mode_toggle), 0.0);
    gtk_grid_attach(GTK_GRID(grid_key), l_mode_toggle, 0, row, 1, 1);
    b_key_mode_toggle = gtk_button_new_with_label(gdk_keyval_name(conf.key_mode_toggle));
    g_signal_connect(b_key_mode_toggle, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_grid_attach(GTK_GRID(grid_key), b_key_mode_toggle, 1, row, 1, 1);


    /* Presets page */
    page_presets = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(page_presets), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_presets, gtk_label_new("Presets"));

    presets_wrapper = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(page_presets), presets_wrapper, TRUE, TRUE, 0);

    grid_presets = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(grid_presets), 3);
    gtk_grid_set_row_spacing(GTK_GRID(grid_presets), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid_presets), 4);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid_presets), FALSE);
    gtk_box_pack_start(GTK_BOX(presets_wrapper), GTK_WIDGET(grid_presets), TRUE, FALSE, 0);

    for(i=0; i<PRESETS; i++)
    {
        g_snprintf(tmp, sizeof(tmp), "F%d:", i+1);
        gtk_grid_attach(GTK_GRID(grid_presets), gtk_label_new(tmp), (i>=6)?2:0, i%6, 1, 1);
        s_presets[i] = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.presets[i], TUNER_FREQ_MIN, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
        gtk_grid_attach(GTK_GRID(grid_presets), s_presets[i], (i>=6)?3:1, i%6, 1, 1);
    }

    l_presets_unit = gtk_label_new("All frequencies are in kHz.");
    gtk_label_set_justify(GTK_LABEL(l_presets_unit), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(page_presets), l_presets_unit, TRUE, FALSE, 0);

    l_presets_info = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(l_presets_info), "Tune to a frequency - press <b>F1</b>...<b>F12</b>.\nStore a frequency - <b>Shift+F1</b>...<b>F12</b>.");
    gtk_label_set_justify(GTK_LABEL(l_presets_info), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(page_presets), l_presets_info, TRUE, FALSE, 0);


    /* Scheduler page */
    page_scheduler = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(page_scheduler), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_scheduler, gtk_label_new("Scheduler"));

    scheduler_liststore = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_INT);
    scheduler_treeview = gtk_tree_view_new();
    scheduler_renderer_freq = gtk_cell_renderer_text_new();
    scheduler_renderer_timeout = gtk_cell_renderer_text_new();
    g_object_set(scheduler_renderer_freq, "editable", TRUE, NULL);
    g_object_set(scheduler_renderer_timeout, "editable", TRUE, NULL);
    g_signal_connect(scheduler_renderer_freq, "edited", G_CALLBACK(settings_scheduler_edit), GINT_TO_POINTER(SCHEDULER_LIST_STORE_FREQ));
    g_signal_connect(scheduler_renderer_timeout, "edited", G_CALLBACK(settings_scheduler_edit), GINT_TO_POINTER(SCHEDULER_LIST_STORE_TIMEOUT));
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(scheduler_treeview), -1, "Freq [kHz]", scheduler_renderer_freq, "text", SCHEDULER_LIST_STORE_FREQ, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(scheduler_treeview), -1, "Time [s]", scheduler_renderer_timeout, "text", SCHEDULER_LIST_STORE_TIMEOUT, NULL);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(scheduler_treeview), TRUE);
    g_signal_connect(scheduler_treeview, "key-press-event", G_CALLBACK(settings_scheduler_key), NULL);
    gtk_tree_view_set_model(GTK_TREE_VIEW(scheduler_treeview), GTK_TREE_MODEL(scheduler_liststore));
    g_object_unref(scheduler_liststore);
    scheduler_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scheduler_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scheduler_scroll), scheduler_treeview);
    gtk_box_pack_start(GTK_BOX(page_scheduler), scheduler_scroll, TRUE, TRUE, 0);

    scheduler_add_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_hexpand(scheduler_add_box, TRUE);
    gtk_box_pack_start(GTK_BOX(page_scheduler), scheduler_add_box, FALSE, FALSE, 0);

    s_scheduler_freq = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(87500.0, TUNER_FREQ_MIN, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
    gtk_widget_set_tooltip_text(s_scheduler_freq, "Frequency [kHz]");
    g_signal_connect(s_scheduler_freq, "key-press-event", G_CALLBACK(settings_scheduler_add_key), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_add_box), s_scheduler_freq, FALSE, FALSE, 0);

    s_scheduler_timeout = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.scheduler_default_timeout, 1.0, 99999.0, 1.0, 2.0, 0.0)), 0, 0);
    gtk_widget_set_tooltip_text(s_scheduler_timeout, "Time [s]");
    g_signal_connect(s_scheduler_timeout, "key-press-event", G_CALLBACK(settings_scheduler_add_key), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_add_box), s_scheduler_timeout, FALSE, FALSE, 0);

    b_scheduler_add = gtk_button_new_with_label("Add");
    gtk_button_set_image(GTK_BUTTON(b_scheduler_add), gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_always_show_image(GTK_BUTTON(b_scheduler_add), TRUE);
    g_signal_connect(b_scheduler_add, "clicked", G_CALLBACK(settings_scheduler_add), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_add_box), b_scheduler_add, TRUE, TRUE, 0);

    scheduler_right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(scheduler_right_align), scheduler_right_box);

    b_scheduler_remove = gtk_button_new();
    gtk_widget_set_tooltip_text(b_scheduler_remove, "Remove");
    gtk_button_set_image(GTK_BUTTON(b_scheduler_remove), gtk_image_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON));
    g_signal_connect(b_scheduler_remove, "clicked", G_CALLBACK(settings_scheduler_remove), scheduler_treeview);
    gtk_box_pack_start(GTK_BOX(scheduler_right_box), b_scheduler_remove, FALSE, FALSE, 0);

    b_scheduler_clear = gtk_button_new_with_label("Clear");
    gtk_button_set_image(GTK_BUTTON(b_scheduler_clear), gtk_image_new_from_icon_name("edit-clear", GTK_ICON_SIZE_BUTTON));
    g_signal_connect_swapped(b_scheduler_clear, "clicked", G_CALLBACK(gtk_list_store_clear), scheduler_liststore);
    gtk_box_pack_start(GTK_BOX(scheduler_right_box), b_scheduler_clear, FALSE, FALSE, 0);

    settings_scheduler_load();


    /* About page */
    page_about = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_about, gtk_label_new("About..."));

    xdr_gtk_img = gtk_image_new_from_icon_name(APP_ICON, GTK_ICON_SIZE_DIALOG);
    gtk_image_set_pixel_size(GTK_IMAGE(xdr_gtk_img), 128);
    gtk_box_pack_start(GTK_BOX(page_about), xdr_gtk_img, TRUE, FALSE, 10);

    xdr_gtk_title = gtk_label_new(NULL);
    gtk_label_set_justify(GTK_LABEL(xdr_gtk_title), GTK_JUSTIFY_CENTER);
    gtk_label_set_markup(GTK_LABEL(xdr_gtk_title), "<span size=\"xx-large\"><b>" APP_NAME "</b></span>\n<span size=\"x-large\"><b>" APP_VERSION "</b></span>");
    gtk_box_pack_start(GTK_BOX(page_about), xdr_gtk_title, TRUE, FALSE, 0);

    xdr_gtk_info = gtk_label_new(NULL);
    gtk_label_set_justify(GTK_LABEL(xdr_gtk_info), GTK_JUSTIFY_CENTER);
    gtk_label_set_markup(GTK_LABEL(xdr_gtk_info), "<span size=\"larger\">User interface for XDR-F1HD tuner\nwith <a href=\"https://fmdx.pl/xdr-i2c\">XDR-I2C</a> modification.</span>");
#ifdef G_OS_WIN32
    g_signal_connect(xdr_gtk_info, "activate-link", G_CALLBACK(win32_uri), NULL);
#endif
    gtk_box_pack_start(GTK_BOX(page_about), xdr_gtk_info, TRUE, FALSE, 5);

    xdr_gtk_link = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(xdr_gtk_link), "<span size=\"larger\"><a href=\"https://fmdx.pl/xdr-gtk/\">https://fmdx.pl/xdr-gtk/</a></span>");
#ifdef G_OS_WIN32
    g_signal_connect(xdr_gtk_link, "activate-link", G_CALLBACK(win32_uri), NULL);
#endif
    gtk_box_pack_start(GTK_BOX(page_about), xdr_gtk_link, TRUE, FALSE, 10);

    xdr_gtk_copyright = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(xdr_gtk_copyright), "Copyright © 2012-2022  Konrad Kosmatka\n\n<span size=\"smaller\">Icon by Marek Farkaš (Noobish)</span>");
    gtk_label_set_justify(GTK_LABEL(xdr_gtk_copyright), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(page_about), xdr_gtk_copyright, TRUE, FALSE, 5);

    gtk_widget_show_all(dialog);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), tab_num);
#ifdef G_OS_WIN32
    i = win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    i = gtk_dialog_run(GTK_DIALOG(dialog));
#endif
    if(i != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dialog);
        return;
    }

    /* Interface page */
    conf.initial_freq = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_init_freq));
    conf.event_action = gtk_combo_box_get_active(GTK_COMBO_BOX(c_event));
    conf.utc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_utc));
    conf.auto_connect = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_autoconnect));
    conf.fm_10k_steps = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_fmstep));
    conf.mw_10k_steps = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_amstep));
    conf.disconnect_confirm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_disconnect_confirm));
    conf.auto_reconnect = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_auto_reconnect));
    conf.grab_focus = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_grab_focus));

    /* Appearance page */
    conf.hide_decorations = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_hide_decorations));
#ifdef G_OS_WIN32
    if (conf.hide_decorations)
        ui_decorations(FALSE);
#else
    ui_decorations(!conf.hide_decorations);
#endif
    conf.hide_interference = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_hide_interference));
    if(conf.hide_interference)
        gtk_widget_hide(ui.box_left_interference);
    else
        gtk_widget_show(ui.box_left_interference);
    conf.hide_statusbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_hide_status));
    if(conf.hide_statusbar)
        gtk_widget_hide(ui.l_status);
    else
        gtk_widget_show(ui.l_status);
    conf.hide_radiotext = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_hide_radiotext));
    if(conf.hide_radiotext)
    {
        gtk_widget_hide(ui.l_rt[0]);
        gtk_widget_hide(ui.l_rt[1]);
    }
    else
    {
        gtk_widget_show(ui.l_rt[0]);
        gtk_widget_show(ui.l_rt[1]);
    }
    conf.restore_position = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_restore_pos));
    conf.title_tuner_info = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_title_tuner_info));
    conf.accessibility = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_accessibility));
    conf.horizontal_af = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_horizontal_af));
    conf.dark_theme = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_dark_theme));

    /* Signal page */
    conf.signal_unit = gtk_combo_box_get_active(GTK_COMBO_BOX(c_unit));
    conf.signal_display = gtk_combo_box_get_active(GTK_COMBO_BOX(c_display));
    signal_display();
    conf.signal_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(c_signal_mode));
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(c_mono), &conf.color_mono);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(c_stereo), &conf.color_stereo);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(c_rds), &conf.color_rds);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(c_mono_dark), &conf.color_mono_dark);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(c_stereo_dark), &conf.color_stereo_dark);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(c_rds_dark), &conf.color_rds_dark);
    conf.signal_height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_height));
    signal_resize();
    conf.signal_offset = gtk_spin_button_get_value(GTK_SPIN_BUTTON(s_signal_offset));
    conf.signal_scroll = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_scroll));
    conf.signal_avg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_avg));
    conf.signal_grid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_grid));

    /* RDS page */
    conf.rds_pty_set = gtk_combo_box_get_active(GTK_COMBO_BOX(c_pty));
    conf.rds_reset = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_rds_reset));
    conf.rds_reset_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_rds_timeout));
    conf.rds_ps_info_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_ps_info_error));
    conf.rds_ps_data_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_ps_data_error));
    conf.rds_ps_progressive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_psprog));
    conf.rds_rt_info_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_rt_info_error));
    conf.rds_rt_data_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_rt_data_error));

    /* Antenna page */
    conf.ant_show_alignment = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_alignment));
    if(conf.ant_show_alignment)
        gtk_widget_show(ui.hs_align);
    else
        gtk_widget_hide(ui.hs_align);
    conf.ant_swap_rotator = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_swap_rotator));
    ui_rotator_button_swap();
    conf.ant_count = gtk_combo_box_get_active(GTK_COMBO_BOX(c_ant_count))+1;
    conf.ant_clear_rds = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_ant_clear_rds));
    conf.ant_auto_switch = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_ant_switching));
    for(i=0; i<ANT_COUNT; i++)
    {
        conf.ant_start[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_ant_start[i]));
        conf.ant_stop[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_ant_stop[i]));
        conf.ant_offset[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_ant_offset[i]));
        g_free(conf.ant_name[i]);
        conf.ant_name[i] = g_strdup(gtk_entry_get_text(GTK_ENTRY(e_ant_name[i])));
        tuner_set_offset(i, conf.ant_offset[i]);
    }
    ui_antenna_update();

    /* Logs page */
    i = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_rdsspy_port));
    if(rdsspy_is_up() && conf.rdsspy_port != i)
        rdsspy_stop();
    conf.rdsspy_port = i;

    conf.rdsspy_auto = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_rdsspy_auto));
    conf.rdsspy_run = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_rdsspy_run));
    conf_update_string(&conf.rdsspy_exec, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(c_rdsspy_command)));
    conf.srcp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_stationlist));
    conf.srcp_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_stationlist_port));
    stationlist_stop();
    if(conf.srcp)
        stationlist_init();
    if(conf.rds_logging)
        log_cleanup();
    conf.rds_logging = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_rds_logging));
    conf.replace_spaces = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_replace));
    conf_update_string(&conf.log_dir, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(c_log_dir)));
    conf_update_string(&conf.screen_dir, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(c_screen_dir)));

    /* Keyboard page */
    conf.key_tune_up = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_up)));
    conf.key_tune_down = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_down)));
    conf.key_tune_fine_up = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_up_5)));
    conf.key_tune_fine_down = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_down_5)));
    conf.key_tune_jump_up = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_up_1000)));
    conf.key_tune_jump_down = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_down_1000)));
    conf.key_tune_back = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_back)));
    conf.key_tune_reset = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_reset)));
    conf.key_screenshot = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_screen)));
    conf.key_bw_up = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_bw_up)));
    conf.key_bw_down = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_bw_down)));
    conf.key_bw_auto = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_bw_auto)));
    conf.key_rotate_cw = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_rotate_cw)));
    conf.key_rotate_ccw = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_rotate_ccw)));
    conf.key_switch_antenna = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_switch_ant)));
    conf.key_rds_ps_mode = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_key_ps_mode)));
    conf.key_scan_toggle = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_key_scan_toggle)));
    conf.key_scan_prev = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_key_scan_prev)));
    conf.key_scan_next = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_key_scan_next)));
    conf.key_stereo_toggle = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_key_stereo_toggle)));
    conf.key_mode_toggle = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_key_mode_toggle)));

    /* Presets page */
    for(i=0; i<PRESETS; i++)
        conf.presets[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_presets[i]));

    /* Scheduler page */
    settings_scheduler_store();
    scheduler_stop();
    conf.scheduler_default_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_scheduler_timeout));

    gtk_widget_destroy(dialog);
    conf_write();
}

static void
settings_key(GtkWidget      *widget,
             GdkEventButton *event,
             gpointer        nothing)
{
    GtkWidget *dialog, *desc;
    gchar *text;

    dialog = gtk_dialog_new_with_buttons("Key",
                                         GTK_WINDOW(ui.window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Cancel", GTK_RESPONSE_REJECT,
                                         NULL);
#ifdef G_OS_WIN32
    g_signal_connect(dialog, "realize", G_CALLBACK(win32_realize), NULL);
#endif

    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);

    g_signal_connect(dialog, "key-press-event", G_CALLBACK(settings_key_press), widget);
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);

    text = g_markup_printf_escaped("Current key: <b>%s</b>\n\nPress any key to change...\n", gtk_button_get_label(GTK_BUTTON(widget)));
    desc = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(desc), text);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), desc);
    g_free(text);

    gtk_widget_show_all(dialog);
#ifdef G_OS_WIN32
    win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    gtk_dialog_run(GTK_DIALOG(dialog));
#endif
}

static gboolean
settings_key_press(GtkWidget   *widget,
                   GdkEventKey *event,
                   gpointer     button)
{
    /* Ignore shift key */
    if(event->keyval == GDK_KEY_Shift_L ||
       event->keyval == GDK_KEY_Shift_R)
        return TRUE;

    gtk_button_set_label(GTK_BUTTON(button),
                         gdk_keyval_name(gdk_keyval_to_upper(event->keyval)));

    gtk_widget_destroy(widget);
    return TRUE;
}

static void
settings_scheduler_load()
{
    GtkTreeIter iter;
    int i;
    for(i=0; i<conf.scheduler_n; i++)
    {
        gtk_list_store_append(scheduler_liststore, &iter);
        gtk_list_store_set(scheduler_liststore, &iter,
                           SCHEDULER_LIST_STORE_FREQ, conf.scheduler_freqs[i],
                           SCHEDULER_LIST_STORE_TIMEOUT, conf.scheduler_timeouts[i],
                           -1);
    }
}

static void
settings_scheduler_store()
{
    gint i;
    g_free(conf.scheduler_freqs);
    g_free(conf.scheduler_timeouts);
    conf.scheduler_n = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(scheduler_liststore), NULL);
    if(!conf.scheduler_n)
    {
        conf.scheduler_freqs = NULL;
        conf.scheduler_timeouts = NULL;
        return;
    }
    conf.scheduler_freqs = g_new(gint, conf.scheduler_n);
    conf.scheduler_timeouts = g_new(gint, conf.scheduler_n);
    i = 0;
    gtk_tree_model_foreach(GTK_TREE_MODEL(scheduler_liststore), (GtkTreeModelForeachFunc)settings_scheduler_store_foreach, &i);
}

static gboolean
settings_scheduler_store_foreach(GtkTreeModel *model,
                                 GtkTreePath  *path,
                                 GtkTreeIter  *iter,
                                 gpointer     *data)
{
    gint *i = (gint*)data;
    gtk_tree_model_get(model, iter,
                       SCHEDULER_LIST_STORE_FREQ, &conf.scheduler_freqs[*i],
                       SCHEDULER_LIST_STORE_TIMEOUT, &conf.scheduler_timeouts[*i],
                       -1);
    (*i)++;
    return FALSE;
}

static void
settings_scheduler_add(GtkWidget *widget,
                       gpointer   nothing)
{
    GtkTreeIter iter;
    gint freq = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_scheduler_freq));
    gint timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_scheduler_timeout));
    gtk_list_store_append(scheduler_liststore, &iter);
    gtk_list_store_set(scheduler_liststore, &iter,
                       SCHEDULER_LIST_STORE_FREQ, freq,
                       SCHEDULER_LIST_STORE_TIMEOUT, timeout,
                       -1);
}

static void
settings_scheduler_edit(GtkCellRendererText *cell,
                        gchar               *path_string,
                        gchar               *new_text,
                        gpointer             col)
{
    GtkTreeIter iter;
    gint new_value = atoi(new_text);
    gint column = GPOINTER_TO_INT(col);

    if(column == SCHEDULER_LIST_STORE_FREQ && (new_value < TUNER_FREQ_MIN || new_value > TUNER_FREQ_MAX))
        return;
    if(column == SCHEDULER_LIST_STORE_TIMEOUT && new_value < 1)
        return;

    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(scheduler_liststore), &iter, path_string);
    gtk_list_store_set(scheduler_liststore, &iter, col, new_value, -1);
}

static void
settings_scheduler_remove(GtkWidget *widget,
                          gpointer   treeview)
{
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
}

static gboolean
settings_scheduler_key(GtkWidget   *widget,
                       GdkEventKey *event,
                       gpointer     data)
{
    if(event->keyval == GDK_KEY_Delete)
    {
        gtk_button_clicked(GTK_BUTTON(b_scheduler_remove));
        return TRUE;
    }
    return FALSE;
}

static gboolean
settings_scheduler_add_key(GtkWidget   *widget,
                           GdkEventKey *event,
                           gpointer     button)
{
    if(event->keyval == GDK_KEY_Return)
    {
        /* Let the spin button update its value before adding an entry */
        g_idle_add(settings_scheduler_add_key_idle, b_scheduler_add);
    }
    return FALSE;
}

static gboolean
settings_scheduler_add_key_idle(gpointer ptr)
{
    gtk_button_clicked(GTK_BUTTON(ptr));
    return FALSE;
}
