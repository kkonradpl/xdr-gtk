#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "ui.h"
#include "tuner-scan.h"
#include "ui-tuner-set.h"
#include "conf.h"
#include "ui-input.h"
#include "scan.h"
#include "ui-signal.h"
#include "tuner.h"
#include "settings.h"
#include "scheduler.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

#define MAP(val, in_min, in_max, out_min, out_max) ((val - in_min) * (out_max - out_min) / (gdouble)(in_max - in_min) + out_min)
#define SCAN_ADD_COLOR(x, y, z, a) cairo_pattern_add_color_stop_rgba(x, y, (((z) & 0xFF0000) >> 16) / 255.0, (((z) & 0xFF00) >> 8) / 255.0, ((z) & 0xFF) / 255.0, (a))

#define SCAN_FONT "DejaVu Sans Mono"

#define SCAN_FONT_SCALE_SIZE 12
#define SCAN_POINT_WIDTH 2.5

#define SCAN_OFFSET_LEFT 30
#define SCAN_OFFSET_RIGHT 20
#define SCAN_OFFSET_TOP (SCAN_FONT_SCALE_SIZE/2 + 2)
#define SCAN_OFFSET_BOTTOM (SCAN_FONT_SCALE_SIZE+5)

#define SCAN_SCALE_LINE_LENGTH      5
#define SCAN_MIN_SAMPLES            2
#define SCAN_MAX_SAMPLES          700
#define SCAN_DEFAULT_MIN_LEVEL    3.0
#define SCAN_DEFAULT_MAX_LEVEL   84.0
#define SCAN_SPECTRUM_ALPHA_PEAK 0.45
#define SCAN_SPECTRUM_ALPHA      1.00
#define SCAN_SPECTRUM_ALPHA_HOLD 1.00
#define SCAN_GRID_ALPHA          0.50
#define SCAN_MARK_TUNED_WIDTH     3.0
#define SCAN_MARK_LABEL_MARGIN    2.0

#define SCAN_REDRAW_DELAY         500

typedef struct scan
{
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *box_settings;

    GtkWidget *box_buttons;
    GtkWidget *b_start;
    GtkWidget *b_continuous;
    GtkWidget *b_relative;
    GtkWidget *b_peakhold;
    GtkWidget *b_hold;

    GtkWidget *grid;
    GtkWidget *l_from;
    GtkWidget *s_start;
    GtkWidget *l_to;
    GtkWidget *s_end;
    GtkWidget *l_step;
    GtkWidget *s_step;
    GtkWidget *l_bw;
    GtkWidget *d_bw;

    GtkWidget *box_other;
    GtkWidget *b_prev;
    GtkWidget *b_next;
    GtkWidget *b_menu_presets;
    GtkWidget *b_menu_main;

    GtkWidget *b_view;
    GtkWidget *l_view;

    GtkWidget *canvas;

    gboolean active;
    gboolean locked;
    gboolean motion_tuning;
    gint focus;
    tuner_scan_t *data;
    tuner_scan_t *peak;
    tuner_scan_t *hold;
    gint64 last_redraw;
    gint queue_redraw;
} scan_t;

typedef struct scan_ext_pairs
{
    gdouble start;
    gdouble end;
} scan_ext_t;

static scan_t scan;
static const double grid_pattern[] = {1.0, 1.0};
static const gint grid_pattern_len = 2;

static const gdouble color_steps[] =
{
    0.00,
    0.05,
    0.35,
    0.60,
    0.90,
    1.00
};

static const uint32_t color_data[] =
{
    0xff0080,
    0xff0000,
    0xd0d000,
    0x00e000,
    0x4080e0,
    0x0030e0
};

static const gint scan_colors = sizeof(color_data) / sizeof(uint32_t);


static void scan_destroy(GtkWidget*, gpointer);
static gboolean scan_window_event(GtkWidget*, gpointer);
static void scan_lock(gboolean);
static GtkWidget* scan_menu_main();
static GtkWidget* scan_menu_presets();
static void scan_toggle(GtkWidget*, gpointer);
static void scan_peakhold(GtkWidget*, gpointer);
static void scan_hold(GtkWidget*, gpointer);
static void scan_prev(GtkWidget*, gpointer);
static void scan_next(GtkWidget*, gpointer);
static gboolean scan_menu(GtkWidget*, GdkEventButton*, gpointer);
static void scan_menu_update_toggled(GtkCheckMenuItem*, gpointer);
static void scan_menu_clear(GtkCheckMenuItem*, gpointer);
static void scan_menu_tuned_toggled(GtkCheckMenuItem*, gpointer);
static void scan_menu_marks_add(GtkMenuItem*, gpointer);
static void scan_menu_marks_add_custom(GtkMenuItem*, gpointer);
static gboolean scan_menu_marks_add_custom_key(GtkWidget*, GdkEventKey*, gpointer);
static void scan_menu_marks_scheduler(GtkMenuItem*, gpointer);
static gboolean scan_menu_marks_scheduler_key(GtkWidget*, GdkEventKey*, gpointer);
static void scan_menu_marks_clear(GtkMenuItem*, gpointer);
static void scan_menu_preset_ccir(GtkMenuItem*, gpointer);
static void scan_menu_preset_oirt(GtkMenuItem*, gpointer);
static void scan_marks_add(gint);
static void scan_view(GtkWidget*, gpointer);
static gboolean scan_redraw(GtkWidget*, cairo_t*, gpointer);
static void scan_draw_spectrum(cairo_t*, tuner_scan_t*, gint, gint, gdouble, gdouble, gdouble);
static void scan_draw_scale(cairo_t*, gint, gint, gdouble, gdouble);
static void scan_draw_mark(cairo_t*, gint, gint, gint, gboolean, GSList**);
static gboolean scan_click(GtkWidget*, GdkEventButton*, gpointer);
static gboolean scan_motion(GtkWidget*, GdkEventMotion*, gpointer);
static gboolean scan_leave(GtkWidget*, GdkEvent*, gpointer);
static const gchar* scan_format_frequency(gint);
static gboolean scan_timeout_redraw(gpointer);

void
scan_init()
{
    scan.window = NULL;
    scan.active = FALSE;
    scan.locked = FALSE;
    scan.motion_tuning = FALSE;
    scan.focus = -1;
    scan.data = NULL;
    scan.peak = NULL;
    scan.hold = NULL;
    scan.last_redraw = 0;
    scan.queue_redraw = 0;
}

void
scan_dialog()
{
    if(scan.window)
    {
        gtk_window_present(GTK_WINDOW(scan.window));
        return;
    }

    scan.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#ifdef G_OS_WIN32
    g_signal_connect(scan.window, "realize", G_CALLBACK(win32_realize), NULL);
#endif

    gtk_window_set_title(GTK_WINDOW(scan.window), "Spectral scan");
    gtk_window_set_icon_name(GTK_WINDOW(scan.window), "xdr-gtk-scan");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(scan.window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(scan.window), 2);
    gtk_window_set_transient_for(GTK_WINDOW(scan.window), GTK_WINDOW(ui.window));
    gtk_window_set_default_size(GTK_WINDOW(scan.window), conf.scan_width, conf.scan_height);
    gtk_widget_add_events(GTK_WIDGET(scan.window), GDK_CONFIGURE);
    if(conf.restore_position && conf.scan_x >= 0 && conf.scan_y >= 0)
        gtk_window_move(GTK_WINDOW(scan.window), conf.scan_x, conf.scan_y);

    scan.box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_container_add(GTK_CONTAINER(scan.window), scan.box);

    scan.box_settings = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    gtk_box_pack_start(GTK_BOX(scan.box), scan.box_settings, FALSE, FALSE, 0);

    scan.box_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(scan.box_settings), scan.box_buttons, FALSE, FALSE, 0);

    scan.b_start = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(scan.b_start), gtk_image_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_LARGE_TOOLBAR));
    gtk_widget_set_name(scan.b_start, "small-button");
    g_signal_connect(scan.b_start, "clicked", G_CALLBACK(scan_toggle), NULL);
    gtk_box_pack_start(GTK_BOX(scan.box_buttons), scan.b_start, TRUE, TRUE, 0);

    scan.b_continuous = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(scan.b_continuous, "Continous scanning");
    gtk_button_set_image(GTK_BUTTON(scan.b_continuous), gtk_image_new_from_icon_name("view-refresh", GTK_ICON_SIZE_LARGE_TOOLBAR));
    gtk_widget_set_name(scan.b_continuous, "small-button");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_continuous), conf.scan_continuous);
    gtk_box_pack_start(GTK_BOX(scan.box_buttons), scan.b_continuous, TRUE, TRUE, 0);

    scan.b_relative = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(scan.b_relative, "Relative scale");
    gtk_button_set_image(GTK_BUTTON(scan.b_relative), gtk_image_new_from_icon_name("zoom-fit-best", GTK_ICON_SIZE_LARGE_TOOLBAR));
    gtk_widget_set_name(scan.b_relative, "small-button");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_relative), conf.scan_relative);
    gtk_box_pack_start(GTK_BOX(scan.box_buttons), scan.b_relative, TRUE, TRUE, 0);

    scan.b_peakhold = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(scan.b_peakhold, "Peak hold");
    gtk_button_set_image(GTK_BUTTON(scan.b_peakhold), gtk_image_new_from_icon_name("go-top", GTK_ICON_SIZE_LARGE_TOOLBAR));
    gtk_widget_set_name(scan.b_peakhold, "small-button");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_peakhold), conf.scan_peakhold);
    g_signal_connect(scan.b_peakhold, "clicked", G_CALLBACK(scan_peakhold), NULL);
    gtk_box_pack_start(GTK_BOX(scan.box_buttons), scan.b_peakhold, TRUE, TRUE, 0);

    scan.b_hold = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(scan.b_hold, "Hold");
    gtk_button_set_image(GTK_BUTTON(scan.b_hold), gtk_image_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_LARGE_TOOLBAR));
    gtk_widget_set_name(scan.b_hold, "small-button");
    if(scan.hold)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_hold), TRUE);

    g_signal_connect(scan.b_hold, "clicked", G_CALLBACK(scan_hold), NULL);
    gtk_box_pack_start(GTK_BOX(scan.box_buttons), scan.b_hold, TRUE, TRUE, 0);



    scan.grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(scan.grid), 1);
    gtk_grid_set_column_spacing(GTK_GRID(scan.grid), 15);
    gtk_grid_set_row_homogeneous(GTK_GRID(scan.grid), TRUE);
    gtk_box_pack_start(GTK_BOX(scan.box_settings), scan.grid, FALSE, FALSE, 0);

    gint grid_pos = 0;

    scan.l_from = gtk_label_new("From:");
    gtk_label_set_xalign(GTK_LABEL(scan.l_from), 0.0);
    gtk_grid_attach(GTK_GRID(scan.grid), scan.l_from, 0, grid_pos, 1, 1);

    scan.s_start = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.scan_start, TUNER_FREQ_MIN, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
    gtk_grid_attach(GTK_GRID(scan.grid), scan.s_start, 1, grid_pos, 1, 1);

    scan.l_to = gtk_label_new("To:");
    gtk_label_set_xalign(GTK_LABEL(scan.l_to), 0.0);
    gtk_grid_attach(GTK_GRID(scan.grid), scan.l_to, 0, ++grid_pos, 1, 1);
    scan.s_end = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.scan_end, TUNER_FREQ_MIN, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
    gtk_grid_attach(GTK_GRID(scan.grid), scan.s_end, 1, grid_pos, 1, 1);


    scan.l_step = gtk_label_new("Step:");
    gtk_label_set_xalign(GTK_LABEL(scan.l_step), 0.0);
    gtk_grid_attach(GTK_GRID(scan.grid), scan.l_step, 0, ++grid_pos, 1, 1);
    scan.s_step = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.scan_step, 5.0, 1000.0, 5.0, 5.0, 0.0)), 0, 0);
    gtk_grid_attach(GTK_GRID(scan.grid), scan.s_step, 1, grid_pos, 1, 1);

    scan.l_bw = gtk_label_new("BW:");
    gtk_label_set_xalign(GTK_LABEL(scan.l_bw), 0.0);
    gtk_grid_attach(GTK_GRID(scan.grid), scan.l_bw, 0, ++grid_pos, 1, 1);

    scan.d_bw = ui_bandwidth_new();
    ui_bandwidth_fill(scan.d_bw, FALSE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(scan.d_bw), 13);
    gtk_grid_attach(GTK_GRID(scan.grid), scan.d_bw, 1, grid_pos, 1, 1);
    if(conf.scan_bw >= 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(scan.d_bw), conf.scan_bw);

    scan.box_other = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(scan.box_settings), scan.box_other, FALSE, FALSE, 0);

    scan.b_prev = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(scan.b_prev), gtk_image_new_from_icon_name("go-previous", GTK_ICON_SIZE_MENU));
    gtk_widget_set_name(scan.b_prev, "small-button");
    gtk_box_pack_start(GTK_BOX(scan.box_other), scan.b_prev, TRUE, TRUE, 0);
    g_signal_connect(scan.b_prev, "clicked", G_CALLBACK(scan_prev), NULL);

    scan.b_next = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(scan.b_next), gtk_image_new_from_icon_name("go-next", GTK_ICON_SIZE_MENU));
    gtk_widget_set_name(scan.b_next, "small-button");
    gtk_box_pack_start(GTK_BOX(scan.box_other), scan.b_next, TRUE, TRUE, 0);
    g_signal_connect(scan.b_next, "clicked", G_CALLBACK(scan_next), NULL);

    scan.b_menu_presets = gtk_button_new_with_label(" P ");
    gtk_widget_set_name(scan.b_menu_presets, "small-button");
    gtk_box_pack_start(GTK_BOX(scan.box_other), scan.b_menu_presets, FALSE, FALSE, 0);
    g_signal_connect(scan.b_menu_presets, "event", G_CALLBACK(scan_menu), scan_menu_presets());

    scan.b_menu_main = gtk_button_new_with_label(" ... ");
    gtk_widget_set_name(scan.b_menu_main, "small-button");
    gtk_box_pack_start(GTK_BOX(scan.box_other), scan.b_menu_main, FALSE, FALSE, 0);
    g_signal_connect(scan.b_menu_main, "event", G_CALLBACK(scan_menu), scan_menu_main());

    scan.b_view = gtk_button_new();
    scan.l_view = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(scan.l_view), "<span size=\"x-small\">&lt;</span>");
    gtk_style_context_add_class(gtk_widget_get_style_context(scan.b_view), "xdr-smallest-button");
    gtk_container_add(GTK_CONTAINER(scan.b_view), scan.l_view);
    gtk_box_pack_start(GTK_BOX(scan.box), scan.b_view, FALSE, FALSE, 0);
    g_signal_connect(scan.b_view, "clicked", G_CALLBACK(scan_view), NULL);

    scan.canvas = gtk_drawing_area_new();
    gtk_widget_add_events(scan.canvas, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);
    gtk_widget_set_size_request(scan.canvas, 500, 100);
    gtk_box_pack_start(GTK_BOX(scan.box), scan.canvas, TRUE, TRUE, 0);

    g_signal_connect(scan.canvas, "draw", G_CALLBACK(scan_redraw), NULL);
    g_signal_connect_swapped(scan.b_relative, "clicked", G_CALLBACK(gtk_widget_queue_draw), scan.canvas);
    g_signal_connect(scan.canvas, "button-press-event", G_CALLBACK(scan_click), NULL);
    g_signal_connect(scan.canvas, "button-release-event", G_CALLBACK(scan_click), NULL);
    g_signal_connect(scan.canvas, "motion-notify-event", G_CALLBACK(scan_motion), NULL);
    g_signal_connect(scan.canvas, "leave-notify-event", G_CALLBACK(scan_leave), NULL);
    g_signal_connect(scan.window, "configure-event", G_CALLBACK(scan_window_event), NULL);
    g_signal_connect(scan.window, "key-press-event", G_CALLBACK(keyboard_press), GINT_TO_POINTER(TRUE));
    g_signal_connect(scan.window, "key-release-event", G_CALLBACK(keyboard_release), NULL);
    g_signal_connect(scan.window, "destroy", G_CALLBACK(scan_destroy), NULL);

    if(scan.active)
        scan_lock(TRUE);

    gtk_widget_show_all(scan.window);
    gtk_window_set_transient_for(GTK_WINDOW(scan.window), NULL);
}

static void
scan_destroy(GtkWidget *widget,
             gpointer   user_data)
{
    conf.scan_continuous = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan.b_continuous));
    conf.scan_relative = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan.b_relative));
    conf.scan_peakhold = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan.b_peakhold));
    conf.scan_start = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(scan.s_start));
    conf.scan_end = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(scan.s_end));
    conf.scan_step = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(scan.s_step));
    conf.scan_bw = gtk_combo_box_get_active(GTK_COMBO_BOX(scan.d_bw));
    gtk_widget_destroy(widget);
    scan.window = NULL;
}

static gboolean
scan_window_event(GtkWidget *widget,
                  gpointer   data)
{
    gtk_window_get_position(GTK_WINDOW(scan.window), &conf.scan_x, &conf.scan_y);
    gtk_window_get_size(GTK_WINDOW(scan.window), &conf.scan_width, &conf.scan_height);
    return FALSE;
}

static void
scan_lock(gboolean value)
{
    gboolean sensitive = !value;
    gtk_widget_set_sensitive(scan.b_continuous, sensitive);
    gtk_widget_set_sensitive(scan.s_start, sensitive);
    gtk_widget_set_sensitive(scan.s_end, sensitive);
    gtk_widget_set_sensitive(scan.s_step, sensitive);
    gtk_widget_set_sensitive(scan.d_bw, sensitive);
    gtk_widget_set_sensitive(scan.b_prev, sensitive);
    gtk_widget_set_sensitive(scan.b_next, sensitive);
    gtk_widget_set_sensitive(scan.b_menu_presets, sensitive);
    gtk_button_set_image(GTK_BUTTON(scan.b_start),
                         gtk_image_new_from_icon_name((value?"media-playback-stop":"media-playback-start"), GTK_ICON_SIZE_LARGE_TOOLBAR));
    scan.locked = value;
}

static GtkWidget*
scan_menu_main()
{
    GtkWidget *menu = gtk_menu_new();

    GtkWidget *title_main = gtk_menu_item_new_with_label("Spectral scan");
    gtk_widget_set_sensitive(title_main, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), title_main);

    GtkWidget *update = gtk_check_menu_item_new_with_label("Update interactively");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(update), conf.scan_update);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), update);
    g_signal_connect(update, "toggled", G_CALLBACK(scan_menu_update_toggled), NULL);

    GtkWidget *clear_graph = gtk_menu_item_new_with_label("Clear the scan");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), clear_graph);
    g_signal_connect(clear_graph, "activate", G_CALLBACK(scan_menu_clear), NULL);

    GtkWidget *title_marks = gtk_menu_item_new_with_label("Frequency marks");
    gtk_widget_set_sensitive(title_marks, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), title_marks);

    GtkWidget *mark_tuned = gtk_check_menu_item_new_with_label("Mark tuned frequency");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mark_tuned), conf.scan_mark_tuned);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mark_tuned);
    g_signal_connect(mark_tuned, "toggled", G_CALLBACK(scan_menu_tuned_toggled), NULL);

    GtkWidget *add_100k = gtk_menu_item_new_with_label("Add every 100 kHz");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), add_100k);
    g_signal_connect(add_100k, "activate", G_CALLBACK(scan_menu_marks_add), GINT_TO_POINTER(100));

    GtkWidget *add_1M = gtk_menu_item_new_with_label("Add every 1 MHz");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), add_1M);
    g_signal_connect(add_1M, "activate", G_CALLBACK(scan_menu_marks_add), GINT_TO_POINTER(1000));

    GtkWidget *add_custom = gtk_menu_item_new_with_label("Add every ...");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), add_custom);
    g_signal_connect(add_custom, "activate", G_CALLBACK(scan_menu_marks_add_custom), NULL);

    GtkWidget *append_scheduler = gtk_menu_item_new_with_label("Add marks to scheduler");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), append_scheduler);
    g_signal_connect(append_scheduler, "activate", G_CALLBACK(scan_menu_marks_scheduler), NULL);

    GtkWidget *clear_visible = gtk_menu_item_new_with_label("Clear visible marks");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), clear_visible);
    g_signal_connect(clear_visible, "activate", G_CALLBACK(scan_menu_marks_clear), GINT_TO_POINTER(FALSE));

    GtkWidget *clear_all = gtk_menu_item_new_with_label("Clear all marks");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), clear_all);
    g_signal_connect(clear_all, "activate", G_CALLBACK(scan_menu_marks_clear), GINT_TO_POINTER(TRUE));

    gtk_widget_show_all(menu);
    return menu;
}

static GtkWidget*
scan_menu_presets()
{
    GtkWidget *menu = gtk_menu_new();

    GtkWidget *title_presets = gtk_menu_item_new_with_label("Presets");
    gtk_widget_set_sensitive(title_presets, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), title_presets);

    GtkWidget *preset_ccir = gtk_menu_item_new_with_label("CCIR (87500 - 108000 / 100)");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), preset_ccir);
    g_signal_connect(preset_ccir, "activate", G_CALLBACK(scan_menu_preset_ccir), NULL);

    GtkWidget *preset_oirt = gtk_menu_item_new_with_label("OIRT (65750 - 74000 / 30)");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), preset_oirt);
    g_signal_connect(preset_oirt, "activate", G_CALLBACK(scan_menu_preset_oirt), NULL);

    gtk_widget_show_all(menu);
    return menu;
}

static void
scan_toggle(GtkWidget *widget,
            gpointer   user_data)
{
    gint start, stop, step, bw;
    gboolean continuous;
    gchar buff[100];
    gint samples;
    gint offset;
    gint antenna;

    if(!tuner.thread)
        return;

    if(scan.active)
    {
        /* Try to cancel current scan */
        tuner_write(tuner.thread, "");
        gtk_widget_set_sensitive(scan.b_start, FALSE);
    }
    else
    {
        start = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(scan.s_start));
        stop = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(scan.s_end));
        step = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(scan.s_step));
        bw = gtk_combo_box_get_active(GTK_COMBO_BOX(scan.d_bw));
        continuous = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan.b_continuous));

        if(stop < start)
        {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_start), stop);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_end), start);
            start = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(scan.s_start));
            stop = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(scan.s_end));
        }

        samples = (stop - start)/(gdouble)step;
        if(samples < SCAN_MIN_SAMPLES || samples > SCAN_MAX_SAMPLES)
        {
            ui_dialog(scan.window,
                      GTK_MESSAGE_INFO,
                      "Spectral scan",
                      (samples < SCAN_MIN_SAMPLES) ? "The selected sample count is too low." : "The selected sample count is too large.");
            return;
        }

        antenna = ui_antenna_id(start);
        offset = tuner.offset[antenna];
        g_snprintf(buff, sizeof(buff),
                   "Sa%d\nSb%d\nSc%d\nSf%d\nSz%d\nS%s",
                   start+offset, stop+offset, step, tuner_filter_from_index(bw), antenna, (continuous?"m":""));

        tuner_write(tuner.thread, buff);
        scan_lock(TRUE);
        gtk_widget_set_sensitive(scan.b_start, FALSE);
    }

}

static void
scan_peakhold(GtkWidget *widget,
              gpointer   user_data)
{
    if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
        if(scan.peak)
        {
            tuner_scan_free(scan.peak);
            scan.peak = NULL;
            scan_force_redraw();
        }
    }
}

static void
scan_hold(GtkWidget *widget,
          gpointer   user_data)
{
    gboolean redraw = FALSE;

    if(scan.hold)
    {
        tuner_scan_free(scan.hold);
        scan.hold = NULL;
        redraw = TRUE;
    }

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
        if(scan.data)
        {
            scan.hold = tuner_scan_copy(scan.data);
            redraw = TRUE;
        }
        else
        {
            g_signal_handlers_block_by_func(G_OBJECT(scan.b_hold), GINT_TO_POINTER(scan_hold), NULL);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_hold), FALSE);
            g_signal_handlers_unblock_by_func(G_OBJECT(scan.b_hold), GINT_TO_POINTER(scan_hold), NULL);
        }
    }

    if(redraw)
        scan_force_redraw();
}

static void
scan_prev(GtkWidget *widget,
          gpointer   data)
{
    GList *ptr;
    gint freq_min, freq_max, freq_curr;
    gint prev, last, value;

    if(!scan.data)
        return;

    freq_min = scan.data->signals[0].freq;
    freq_max = scan.data->signals[scan.data->len-1].freq;
    freq_curr = tuner_get_freq();
    prev = 0;
    last = 0;

    for(ptr = conf.scan_marks; ptr; ptr = ptr->next)
    {
        value = GPOINTER_TO_INT(ptr->data);
        if(value >= freq_min && value <= freq_max)
        {
            if(value < freq_curr)
                prev = value;
            else
            {
                if(prev)
                    break;
                last = value;
            }
        }
    }

    if(prev)
        tuner_set_frequency(prev);
    else if(last)
        tuner_set_frequency(last);
}

static void
scan_next(GtkWidget *widget,
          gpointer   data)
{
    GList *ptr;
    gint freq_min, freq_max, freq_curr;
    gint next, first, value;

    if(!scan.data)
        return;

    freq_min = scan.data->signals[0].freq;
    freq_max = scan.data->signals[scan.data->len-1].freq;
    freq_curr = tuner_get_freq();
    next = 0;
    first = 0;

    for(ptr = conf.scan_marks; ptr; ptr=ptr->next)
    {
        value = GPOINTER_TO_INT(ptr->data);
        if(value >= freq_min && value <= freq_max)
        {
            if(!first)
                first = value;
            if(value > freq_curr)
            {
                next = value;
                break;
            }
        }
    }

    if(next)
        tuner_set_frequency(next);
    else if(first)
        tuner_set_frequency(first);
}

static gboolean
scan_menu(GtkWidget      *widget,
          GdkEventButton *event,
          gpointer        userdata)
{
    GtkMenu *menu = GTK_MENU(userdata);
    if(event->type == GDK_BUTTON_PRESS)
    {
        gtk_menu_popup_at_widget(menu, widget, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, (GdkEvent*)event);
        return TRUE;
    }
    return FALSE;
}

static void
scan_menu_update_toggled(GtkCheckMenuItem *item,
                         gpointer          user_data)
{
    conf.scan_update = gtk_check_menu_item_get_active(item);
}

static void
scan_menu_clear(GtkCheckMenuItem *item,
                gpointer          user_data)
{
    if(scan.data)
    {
        tuner_scan_free(scan.data);
        scan.data = NULL;
        if(scan.hold)
        {
            tuner_scan_free(scan.hold);
            scan.hold = NULL;
            g_signal_handlers_block_by_func(G_OBJECT(scan.b_hold), GINT_TO_POINTER(scan_hold), NULL);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_hold), FALSE);
            g_signal_handlers_unblock_by_func(G_OBJECT(scan.b_hold), GINT_TO_POINTER(scan_hold), NULL);
        }
        if(scan.peak)
        {
            tuner_scan_free(scan.peak);
            scan.peak = NULL;
        }
        scan_force_redraw();
    }
}

static void
scan_menu_tuned_toggled(GtkCheckMenuItem *item,
                        gpointer          user_data)
{
    conf.scan_mark_tuned = gtk_check_menu_item_get_active(item);
    scan_force_redraw();
}

static void
scan_menu_marks_add(GtkMenuItem *menuitem,
                    gpointer     user_data)
{
    gint step = GPOINTER_TO_INT(user_data);
    scan_marks_add(step);
}

static void
scan_menu_marks_add_custom(GtkMenuItem *menuitem,
                           gpointer     user_data)
{
    GtkWidget *dialog, *spin_button;
    gint response;
    gint step;

    dialog = gtk_message_dialog_new(GTK_WINDOW(scan.window),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_OK_CANCEL,
                                    "Custom step [kHz]:");
#ifdef G_OS_WIN32
    g_signal_connect(dialog, "realize", G_CALLBACK(win32_realize), NULL);
#endif
    gtk_window_set_title(GTK_WINDOW(dialog), "Frequency marks");
    spin_button = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(2000.0, 5.0, 100000.0, 1.0, 2.0, 0.0)), 0, 0);
    gtk_container_add(GTK_CONTAINER(gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog))), spin_button);
    g_signal_connect(dialog, "key-press-event", G_CALLBACK(scan_menu_marks_add_custom_key), spin_button);

    gtk_widget_show_all(dialog);
#ifdef G_OS_WIN32
    response = win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    response = gtk_dialog_run(GTK_DIALOG(dialog));
#endif
    if (response == GTK_RESPONSE_OK)
    {
        step = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_button));
        scan_marks_add(step);
    }
    gtk_widget_destroy(dialog);
}

static gboolean
scan_menu_marks_add_custom_key(GtkWidget   *widget,
                               GdkEventKey *event,
                               gpointer     button)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    if(current == GDK_KEY_Return)
    {
        gtk_spin_button_update(GTK_SPIN_BUTTON(button));
        gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_OK);
        return TRUE;
    }
    return FALSE;
}

static void
scan_menu_marks_scheduler(GtkMenuItem *menuitem,
                          gpointer     user_data)
{
    GtkWidget *dialog, *spin_button;
    GList *ptr;
    gint response;
    gint freq_min;
    gint freq_max;
    gint value;
    gint count;
    gint freq_time;

    if(!scan.data)
        goto nothing_to_do;

    freq_min = scan.data->signals[0].freq;
    freq_max = scan.data->signals[scan.data->len-1].freq;
    count = 0;

    for(ptr = conf.scan_marks; ptr; ptr = ptr->next)
    {
        value = GPOINTER_TO_INT(ptr->data);
        if(value >= freq_min && value <= freq_max)
            count++;
    }

    if(!count)
        goto nothing_to_do;

    dialog = gtk_message_dialog_new(GTK_WINDOW(scan.window),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_OK_CANCEL,
                                    "Time for each frequency [s]:");
#ifdef G_OS_WIN32
    g_signal_connect(dialog, "realize", G_CALLBACK(win32_realize), NULL);
#endif
    gtk_window_set_title(GTK_WINDOW(dialog), "Scheduler");
    spin_button = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.scheduler_default_timeout, 1.0, 99999.0, 1.0, 2.0, 0.0)), 0, 0);
    gtk_container_add(GTK_CONTAINER(gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog))), spin_button);
    g_signal_connect(dialog, "key-press-event", G_CALLBACK(scan_menu_marks_scheduler_key), spin_button);

    gtk_widget_show_all(dialog);
#ifdef G_OS_WIN32
    response = win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    response = gtk_dialog_run(GTK_DIALOG(dialog));
#endif
    if (response == GTK_RESPONSE_OK)
    {
        freq_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_button));
        scheduler_stop();
        conf.scheduler_freqs = g_realloc_n(conf.scheduler_freqs, sizeof(gint), conf.scheduler_n+count);
        conf.scheduler_timeouts = g_realloc_n(conf.scheduler_timeouts, sizeof(gint), conf.scheduler_n+count);

        for(ptr = conf.scan_marks; ptr; ptr = ptr->next)
        {
            value = GPOINTER_TO_INT(ptr->data);
            if(value >= freq_min && value <= freq_max)
            {
                conf.scheduler_freqs[conf.scheduler_n] = value;
                conf.scheduler_timeouts[conf.scheduler_n] = freq_time;
                conf.scheduler_n++;
            }
        }

        gtk_widget_destroy(dialog);
        settings_dialog(SETTINGS_TAB_SCHEDULER);
    }
    else
    {
        gtk_widget_destroy(dialog);
    }
    return;

    nothing_to_do:
        ui_dialog(scan.window, GTK_MESSAGE_INFO, "Spectral scan", "There are no visible marks that can be added to the scheduler.");
}

static gboolean
scan_menu_marks_scheduler_key(GtkWidget   *widget,
                              GdkEventKey *event,
                              gpointer     button)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    if(current == GDK_KEY_Return)
    {
        gtk_spin_button_update(GTK_SPIN_BUTTON(button));
        gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_OK);
        return TRUE;
    }
    return FALSE;
}


static void
scan_menu_marks_clear(GtkMenuItem *menuitem,
                      gpointer     user_data)
{
    gboolean all = GPOINTER_TO_INT(user_data);
    if(all)
    {
        conf_uniq_int_list_clear(&conf.scan_marks);
    }
    else if(scan.data)
    {
        conf_uniq_int_list_clear_range(&conf.scan_marks,
                                       scan.data->signals[0].freq,
                                       scan.data->signals[scan.data->len-1].freq);
    }
    scan_force_redraw();
}

static void
scan_menu_preset_ccir(GtkMenuItem *menuitem,
                      gpointer     user_data)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_start), 87500.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_end), 108000.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_step), 100.0);
}

static void
scan_menu_preset_oirt(GtkMenuItem *menuitem,
                      gpointer     user_data)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_start), 65750.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_end), 74000.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_step), 30.0);
}

static void
scan_marks_add(gint step)
{
    gint min, max, curr;
    if(scan.data)
    {
        min = ceil(scan.data->signals[0].freq / (gdouble)step)*step;
        max = floor(scan.data->signals[scan.data->len-1].freq / (gdouble)step)*step;
        for(curr=min; curr<=max; curr+=step)
            conf_uniq_int_list_add(&conf.scan_marks, curr);
        scan_force_redraw();
    }
}

static void
scan_view(GtkWidget *widget,
          gpointer   data)
{
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(widget));
    gboolean visible = gtk_widget_get_visible(scan.box_settings);
    gtk_label_set_markup(GTK_LABEL(label),
                         visible ? "<span size=\"x-small\">&gt;</span>" : "<span size=\"x-small\">&lt;</span>");
    gtk_widget_set_visible(scan.box_settings, !visible);
}

static gboolean
scan_redraw(GtkWidget *widget,
            cairo_t   *cr,
            gpointer   user_data)
{
    cairo_text_extents_t extents;
    cairo_pattern_t *gradient;
    gint width, height;
    gdouble max, min, step;
    gdouble x_focus, y_focus, point_radius;
    gchar text[50];
    GList *l;
    GSList *exts = NULL;
    gint i;

    width = gtk_widget_get_allocated_width(widget) - SCAN_OFFSET_LEFT - SCAN_OFFSET_RIGHT;
    height = gtk_widget_get_allocated_height(widget) - SCAN_OFFSET_TOP - SCAN_OFFSET_BOTTOM;
    cairo_select_font_face(cr, SCAN_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_line_width(cr, 1.0);

    /* Clear the screen */
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    /* Give up if no data is available */
    if(!scan.data || scan.data->len < 2)
        return FALSE;

    step = width/(gdouble)(scan.data->len - 1);
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan.b_relative)))
    {
        if(scan.peak && scan.hold)
        {
            min = MIN(signal_level(scan.peak->min), signal_level(scan.hold->min));
            max = MAX(signal_level(scan.peak->max), signal_level(scan.hold->max));
        }
        else if(scan.peak)
        {
            min = signal_level(scan.peak->min);
            max = signal_level(scan.peak->max);
        }
        else if(scan.hold)
        {
            min = MIN(signal_level(scan.data->min), signal_level(scan.hold->min));
            max = MAX(signal_level(scan.data->max), signal_level(scan.hold->max));
        }
        else
        {
            min = signal_level(scan.data->min);
            max = signal_level(scan.data->max);
        }
    }
    else
    {
        min = signal_level(SCAN_DEFAULT_MIN_LEVEL);
        max = signal_level(SCAN_DEFAULT_MAX_LEVEL);
    }

    /* Draw left vertical and bottom horizontal line  */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr, SCAN_OFFSET_LEFT-0.5, SCAN_OFFSET_TOP+0.5);
    cairo_line_to(cr, SCAN_OFFSET_LEFT-0.5, SCAN_OFFSET_TOP+height+0.5);
    cairo_line_to(cr, SCAN_OFFSET_LEFT-0.5+width+0.5, SCAN_OFFSET_TOP+height+0.5);
    cairo_stroke(cr);

    /* Draw signal scale */
    scan_draw_scale(cr, width, height, min, max);

    /* Draw the spectrum (peak) */
    if(scan.peak)
    {
        gradient = cairo_pattern_create_linear(0.0, 0.0, 0.0, height);
        
        for(i=0; i<scan_colors; i++)
            SCAN_ADD_COLOR(gradient, color_steps[i], color_data[i], SCAN_SPECTRUM_ALPHA_PEAK);
        
        scan_draw_spectrum(cr, scan.peak, width, height, step, min, max);
        cairo_set_source(cr, gradient);
        cairo_close_path(cr);
        cairo_fill(cr);
        cairo_pattern_destroy(gradient);
    }

    /* Draw the spectrum (current) */
    gradient = cairo_pattern_create_linear(0.0, 0.0, 0.0, height);
    
    for(i=0; i<scan_colors; i++)
        SCAN_ADD_COLOR(gradient, color_steps[i], color_data[i], SCAN_SPECTRUM_ALPHA);
    
    scan_draw_spectrum(cr, scan.data, width, height, step, min, max);
    cairo_set_source(cr, gradient);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_pattern_destroy(gradient);

    /* Draw the spectrum (hold) */
    if(scan.hold)
    {
        cairo_save(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
        scan_draw_spectrum(cr, scan.hold, width, height, step, min, max);
        cairo_close_path(cr);
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

    /* Draw each marked frequency */
    cairo_save(cr);
    cairo_set_dash(cr, grid_pattern, grid_pattern_len, 0);
    for(l=conf.scan_marks; l; l=l->next)
    {
        gint freq = GPOINTER_TO_INT(l->data);
        if(freq >= scan.data->signals[0].freq &&
           freq <= scan.data->signals[scan.data->len-1].freq)
        {
            scan_draw_mark(cr, width, height, freq, FALSE, &exts);
        }
    }
    cairo_restore(cr);

    /* Mark currently tuned frequency */
    if(conf.scan_mark_tuned &&
       tuner.thread &&
       tuner_get_freq() >= scan.data->signals[0].freq &&
       tuner_get_freq() <= scan.data->signals[scan.data->len-1].freq)
    {
        scan_draw_mark(cr, width, height, tuner_get_freq(), TRUE, &exts);
    }

    if(scan.focus >= 0 && scan.focus < scan.data->len)
    {
        /* Mark the selected position */
        x_focus = SCAN_OFFSET_LEFT+step*scan.focus;
        y_focus = SCAN_OFFSET_TOP+height - MAP(signal_level(scan.data->signals[scan.focus].signal), min, max, 0.0, height);
        point_radius = (height > 200 ? 5.0 : height / 40.0);

        cairo_save(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
        cairo_set_line_width(cr, SCAN_POINT_WIDTH);
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
        cairo_arc(cr, x_focus, y_focus, point_radius, 0.0, 2.0 * M_PI);
        cairo_stroke(cr);
        cairo_restore(cr);

        /* Show selected frequency and signal */
        g_snprintf(text, sizeof(text), "%s %d%s",
                   scan_format_frequency(scan.data->signals[scan.focus].freq),
                   (gint)ceil(signal_level(scan.data->signals[scan.focus].signal)),
                   signal_unit());

        cairo_select_font_face(cr, SCAN_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, round(height > 300 ? 20.0 : (2/45.0 * height + 20/3.0)));
        cairo_text_extents(cr, text, &extents);
        x_focus -= (extents.width/2.0 + extents.x_bearing);
        y_focus -= (extents.height + point_radius + SCAN_POINT_WIDTH);

        /* Fit the label within a spectrum view size */
        if(x_focus <= SCAN_OFFSET_LEFT)
            x_focus = SCAN_OFFSET_LEFT;
        if(x_focus+extents.width > SCAN_OFFSET_LEFT+width)
            x_focus = SCAN_OFFSET_LEFT + width - extents.width;
        if(y_focus < SCAN_OFFSET_TOP)
            y_focus += extents.height + 2*(point_radius + SCAN_POINT_WIDTH);

        /* Draw a transparent background for text */
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
        cairo_rectangle(cr, x_focus - 1.0,
                            y_focus - 1.0,
                            extents.width + extents.x_bearing + 2.0,
                            extents.height + 2.0);
        cairo_fill(cr);

        /* Draw a text */
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_move_to(cr, round(x_focus), round(y_focus-extents.y_bearing));
        cairo_show_text(cr, text);
        cairo_stroke(cr);
    }

    scan.last_redraw = g_get_monotonic_time() / 1000;
    if(scan.queue_redraw)
    {
        g_source_remove(scan.queue_redraw);
        scan.queue_redraw = 0;
    }

    g_slist_free_full(exts, g_free);
    return FALSE;
}

static void
scan_draw_spectrum(cairo_t      *cr,
                   tuner_scan_t *data,
                   gint          width,
                   gint          height,
                   gdouble       step,
                   gdouble       min,
                   gdouble       max)
{
    gdouble x_src, x_control, x_dest;
    gdouble y_src, y_dest;
    gint i;
    gdouble sample;

    sample = signal_level(data->signals[0].signal);
    if(sample > max)
        sample = max;
    if(sample < min)
        sample = min;

    x_src = 0.0;
    x_dest = 0.0;
    y_src = height - MAP(sample, min, max, 0.0, height);
    cairo_move_to(cr, SCAN_OFFSET_LEFT+x_src, SCAN_OFFSET_TOP+y_src);

    for(i=1; i<data->len; i++)
    {
        sample = signal_level(data->signals[i].signal);
        if(sample > max)
            sample = max;
        if(sample < min)
            sample = min;
        sample = MAP(sample, min, max, 0.0, height);
        x_dest += step;
        x_control = x_src + (x_dest - x_src) / 2.0;
        y_dest = height - sample;
        cairo_curve_to(cr, SCAN_OFFSET_LEFT+x_control, SCAN_OFFSET_TOP+y_src,
                           SCAN_OFFSET_LEFT+x_control, SCAN_OFFSET_TOP+y_dest,
                           SCAN_OFFSET_LEFT+x_dest, SCAN_OFFSET_TOP+y_dest);
        x_src = x_dest;
        y_src = y_dest;
    }

    cairo_line_to(cr, SCAN_OFFSET_LEFT+width, SCAN_OFFSET_TOP+height);
    cairo_line_to(cr, SCAN_OFFSET_LEFT, SCAN_OFFSET_TOP+height);
}

static void
scan_draw_scale(cairo_t *cr,
                gint     width,
                gint     height,
                gdouble  min,
                gdouble  max)
{
    gint scale_points = ceil(max) - floor(min);
    gdouble step = height / (gdouble)scale_points;
    gint current_value = ceil(max);
    gdouble position = SCAN_OFFSET_TOP+0.5;
    gdouble last_position = SCAN_FONT_SCALE_SIZE * -1.0;
    gchar text[16];
    gdouble x_pos;

    cairo_save(cr);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_font_size(cr, SCAN_FONT_SCALE_SIZE);

    while(position <= SCAN_OFFSET_TOP+height)
    {
        if(last_position + SCAN_FONT_SCALE_SIZE + 1.0 < position)
        {
            /* Draw a tick */
            cairo_move_to(cr, SCAN_OFFSET_LEFT-0.5-SCAN_SCALE_LINE_LENGTH/2.0, position);
            cairo_line_to(cr, SCAN_OFFSET_LEFT-0.5+SCAN_SCALE_LINE_LENGTH/2.0, position);
            cairo_stroke(cr);

            /* Draw a grid */
            cairo_save(cr);
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, SCAN_GRID_ALPHA);
            cairo_set_dash(cr, grid_pattern, grid_pattern_len, 0);
            cairo_move_to(cr, SCAN_OFFSET_LEFT-0.5+SCAN_SCALE_LINE_LENGTH/2.0, position);
            cairo_line_to(cr, SCAN_OFFSET_LEFT-0.5+width+0.5, position);
            cairo_stroke(cr);
            cairo_restore(cr);

            g_snprintf(text, sizeof(text), "%3d", current_value);
            x_pos = (strlen(text) >= 4 ? -1.0 : 0.0);
            cairo_move_to(cr, round(x_pos), round(position+SCAN_FONT_SCALE_SIZE/2.0-1.0));
            cairo_show_text(cr, text);
            last_position = position;
        }
        current_value--;
        position += step;
    }
    cairo_restore(cr);
}

static void
scan_draw_mark(cairo_t   *cr,
               gint       width,
               gint       height,
               gint       freq,
               gboolean   current,
               GSList   **exts)
{
    cairo_text_extents_t extents;
    const gchar *freq_text;
    scan_ext_t *ext;
    GSList *it;
    gdouble x, lstart, lend;
    gboolean label;

    /* Current frequency has no label */
    label = !current;

    x  = freq - scan.data->signals[0].freq;
    x /= (gdouble)(scan.data->signals[scan.data->len-1].freq - scan.data->signals[0].freq);
    x *= width;

    /* Check whether the label overlaps any existing one */
    cairo_set_font_size(cr, SCAN_FONT_SCALE_SIZE);
    freq_text = scan_format_frequency(freq);
    cairo_text_extents(cr, freq_text, &extents);
    lstart = SCAN_OFFSET_LEFT+x-(extents.width/2.0 + extents.x_bearing);
    lend = SCAN_OFFSET_LEFT+x+(extents.width/2.0 + extents.x_bearing);
    for(it=*exts; it; it=it->next)
    {
        ext = (scan_ext_t*)it->data;
        if((lstart >= ext->start && lstart <= ext->end) ||
           (lend >= ext->start && lend <= ext->end))
        {
            label = FALSE;
            break;
        }
    }

    if(label)
    {
        /* Draw a frequency label */
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_move_to(cr, round(lstart), round(SCAN_OFFSET_TOP+SCAN_OFFSET_BOTTOM+height-3.0));
        cairo_show_text(cr, freq_text);
        cairo_stroke(cr);

        /* Add label extents to the list */
        ext = g_malloc(sizeof(scan_ext_t));
        ext->start = lstart - SCAN_MARK_LABEL_MARGIN;
        ext->end = lend + SCAN_MARK_LABEL_MARGIN;
        *exts = g_slist_append(*exts, ext);
    }


    if(current)
    {
        /* Set different width for the currently tuned frequency */
        cairo_set_line_width(cr, SCAN_MARK_TUNED_WIDTH);
    }

    /* Draw a vertical line */
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, SCAN_GRID_ALPHA);
    cairo_move_to(cr, SCAN_OFFSET_LEFT+x, SCAN_OFFSET_TOP);
    cairo_line_to(cr, SCAN_OFFSET_LEFT+x, SCAN_OFFSET_TOP+height+(label?4.0:0.0));
    cairo_stroke(cr);
}

static gboolean
scan_click(GtkWidget      *widget,
           GdkEventButton *event,
           gpointer        user_data)
{
    if(scan.data)
    {
        /* Left click */
        if(event->type == GDK_BUTTON_PRESS && event->button == 1)
        {
            scan.motion_tuning = TRUE;
            if(scan.focus >= 0 && scan.focus < scan.data->len)
                tuner_set_frequency(scan.data->signals[scan.focus].freq);
        }
        else if(event->type == GDK_BUTTON_RELEASE && event->button == 1)
        {
            scan.motion_tuning = FALSE;
        }
        /* Right click */
        else if(event->type == GDK_BUTTON_PRESS && event->button == 3 &&
                scan.focus >= 0 && scan.focus < scan.data->len)
        {
            conf_uniq_int_list_toggle(&conf.scan_marks, scan.data->signals[scan.focus].freq);
            scan_force_redraw();
        }
    }
    return FALSE;
}

static gboolean
scan_motion(GtkWidget      *widget,
            GdkEventMotion *event,
            gpointer        user_data)
{
    gdouble width = gtk_widget_get_allocated_width(widget) - SCAN_OFFSET_LEFT - SCAN_OFFSET_RIGHT;
    gdouble x = event->x - 0.5 - SCAN_OFFSET_LEFT;
    gint current_focus;

    if(scan.data)
    {
        current_focus = round(x/(width/(gdouble)(scan.data->len - 1)));
        if(current_focus >= 0 && current_focus < scan.data->len)
        {
            if(scan.motion_tuning && tuner_get_freq() != scan.data->signals[current_focus].freq)
                tuner_set_frequency(scan.data->signals[current_focus].freq);
        }
        else
        {
            current_focus = -1;
        }

        if(scan.focus != current_focus)
        {
            scan.focus = current_focus;
            scan_force_redraw();
        }
    }
    return TRUE;
}

static gboolean
scan_leave(GtkWidget *widget,
           GdkEvent  *event,
           gpointer   user_data)
{
    if(scan.focus != -1)
    {
        scan.focus = -1;
        scan_force_redraw();
    }
    return TRUE;
}

static const gchar*
scan_format_frequency(gint freq)
{
    static gchar buff[10];
    gint length, i;

    g_snprintf(buff, sizeof(buff), "%.3f", freq/1000.0);
    length = strlen(buff);

    for(i=length-1; i>0; i--)
    {
        if(buff[i] == '0' && buff[i-1] != '.')
            buff[i] = '\0';
        else
            break;
    }
    return buff;
}

void
scan_update(tuner_scan_t *data_new)
{
    gboolean peakhold = (scan.window && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan.b_peakhold)));
    gint i;

    scan.active = TRUE;

    if(scan.data)
    {
        if(peakhold &&
           scan.data->len == data_new->len &&
           scan.data->signals[0].freq == data_new->signals[0].freq &&
           scan.data->signals[scan.data->len-1].freq == data_new->signals[data_new->len-1].freq)
        {
            if(!scan.peak)
                scan.peak = scan.data;
            else
                tuner_scan_free(scan.data);
        }
        else
        {
            tuner_scan_free(scan.data);
            if(scan.peak)
            {
                tuner_scan_free(scan.peak);
                scan.peak = NULL;
            }
        }
    }

    scan.data = data_new;

    if(scan.hold &&
       !(scan.hold->len == data_new->len &&
         scan.hold->signals[0].freq == data_new->signals[0].freq &&
         scan.hold->signals[scan.hold->len-1].freq == data_new->signals[data_new->len-1].freq))
    {
        tuner_scan_free(scan.hold);
        scan.hold = NULL;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_hold), FALSE);
    }

    if(scan.peak)
    {
        for(i=0; i<data_new->len; i++)
            if(data_new->signals[i].signal > scan.peak->signals[i].signal)
                scan.peak->signals[i].signal = data_new->signals[i].signal;

        if(data_new->max > scan.peak->max)
            scan.peak->max = data_new->max;

        if(data_new->min < scan.peak->min)
            scan.peak->min = data_new->min;
    }

    if(scan.window)
    {
        if(!scan.locked)
            scan_lock(TRUE);

        scan_force_redraw();
        gtk_widget_set_sensitive(scan.b_start, TRUE);
    }
}

void
scan_update_value(gint   freq,
                  gfloat val)
{
    gint found = -1;
    gint i;

    if(scan.active)
    {
        scan.active = FALSE;
        if(scan.window)
        {
            gtk_widget_set_sensitive(scan.b_start, TRUE);
            scan_lock(FALSE);
        }
    }

    if(!conf.scan_update || !scan.window || !scan.data)
        return;

    for(i=0; i<scan.data->len; i++)
    {
        if(scan.data->signals[i].freq == freq)
        {
            scan.data->signals[i].signal = val;
            found = i;
            if(val > scan.data->max)
                scan.data->max = ceil(val);
            if(val < scan.data->min)
                scan.data->min = floor(val);
            break;
        }
    }

    if(found == -1)
        return;

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan.b_peakhold)) &&
       !scan.peak)
    {
        scan.peak = tuner_scan_copy(scan.data);
    }

    if(scan.peak)
    {
        if(scan.peak->signals[found].signal < val)
        {
            scan.peak->signals[found].signal = val;
            if(val > scan.peak->max)
                scan.peak->max = ceil(val);
        }
        if(val < scan.peak->min)
            scan.peak->min = floor(val);
    }

    /* Queue plot redraw */
    if(scan.queue_redraw)
        return;

    i = g_get_monotonic_time() / 1000 - scan.last_redraw;
    if(i >= SCAN_REDRAW_DELAY)
        scan_force_redraw();
    else
        scan.queue_redraw = g_timeout_add(SCAN_REDRAW_DELAY-i, scan_timeout_redraw, NULL);
}

void
scan_try_toggle(gboolean continous)
{
    gboolean continous_pressed;
    if(!scan.window)
        return;
    if(!gtk_widget_get_sensitive(scan.b_start))
        return;
    if(!scan.active)
    {
        continous_pressed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan.b_continuous));
        if((continous && !continous_pressed) ||
           (!continous && continous_pressed))
           gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_continuous), continous);
    }
    gtk_button_clicked(GTK_BUTTON(scan.b_start));
}

void
scan_try_prev()
{
    if(!scan.window)
        return;
    if(!gtk_widget_get_sensitive(scan.b_prev))
        return;
    gtk_button_clicked(GTK_BUTTON(scan.b_prev));
}

void
scan_try_next()
{
    if(!scan.window)
        return;
    if(!gtk_widget_get_sensitive(scan.b_next))
        return;
    gtk_button_clicked(GTK_BUTTON(scan.b_next));
}

void
scan_force_redraw()
{
    if(scan.window)
        gtk_widget_queue_draw(scan.canvas);
}

static gboolean
scan_timeout_redraw(gpointer data)
{
    scan.queue_redraw = 0;
    scan_force_redraw();
    return FALSE;
}
