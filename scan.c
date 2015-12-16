#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gui.h"
#include "gui-tuner.h"
#include "settings.h"
#include "keyboard.h"
#include "scan.h"
#include "sig.h"
#include "tuner.h"

#define MAP(val, in_min, in_max, out_min, out_max) ((val - in_min) * (out_max - out_min) / (gdouble)(in_max - in_min) + out_min)
#define SCAN_ADD_COLOR(x, y, z, a) cairo_pattern_add_color_stop_rgba(x, y, (((z) & 0xFF0000) >> 16) / 255.0, (((z) & 0xFF00) >> 8) / 255.0, ((z) & 0xFF) / 255.0, (a))

#define SCAN_FONT "DejaVu Sans Mono"
#define SCAN_POINT_WIDTH 2.5

#define SCAN_OFFSET_LEFT 5
#define SCAN_OFFSET_RIGHT 5
#define SCAN_OFFSET_TOP 5
#define SCAN_OFFSET_BOTTOM 17

#define SCAN_MIN_SAMPLES 2
#define SCAN_MAX_SAMPLES 700
#define SCAN_DEFAULT_MIN_LEVEL 0.0
#define SCAN_DEFAULT_MAX_LEVEL 85.0

#define SCAN_SPECTRUM_ALPHA_PEAK 0.40
#define SCAN_SPECTRUM_ALPHA      1.00
#define SCAN_SPECTRUM_ALPHA_HOLD 1.00

void scan_init()
{
    scan.window = NULL;
    scan.active = FALSE;
    scan.locked = FALSE;
    scan.motion_tuning = FALSE;
    scan.focus = -1;
    scan.data = NULL;
    scan.peak = NULL;
    scan.hold = NULL;
}

void scan_dialog()
{
    if(scan.window)
    {
        gtk_window_present(GTK_WINDOW(scan.window));
        return;
    }

    scan.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(scan.window), "Spectral scan");
    gtk_window_set_icon_name(GTK_WINDOW(scan.window), "xdr-gtk-scan");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(scan.window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(scan.window), 2);
    gtk_window_set_transient_for(GTK_WINDOW(scan.window), GTK_WINDOW(gui.window));
    gtk_window_set_default_size(GTK_WINDOW(scan.window), conf.scan_width, conf.scan_height);
    gtk_widget_add_events(GTK_WIDGET(scan.window), GDK_CONFIGURE);
    if(conf.restore_pos)
    {
        gtk_window_move(GTK_WINDOW(scan.window), conf.scan_x, conf.scan_y);
    }

    scan.box = gtk_hbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(scan.window), scan.box);

    scan.box_settings = gtk_vbox_new(FALSE, 1);
    gtk_box_pack_start(GTK_BOX(scan.box), scan.box_settings, FALSE, FALSE, 0);


    scan.box_buttons = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(scan.box_settings), scan.box_buttons, FALSE, FALSE, 0);

    scan.b_start = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(scan.b_start), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_name(scan.b_start, "small-button");
    g_signal_connect(scan.b_start, "clicked", G_CALLBACK(scan_toggle), NULL);
    gtk_box_pack_start(GTK_BOX(scan.box_buttons), scan.b_start, TRUE, TRUE, 0);

    scan.b_continuous = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(scan.b_continuous, "Continous scanning");
    gtk_button_set_image(GTK_BUTTON(scan.b_continuous), gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_name(scan.b_continuous, "small-button");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_continuous), conf.scan_continuous);
    gtk_box_pack_start(GTK_BOX(scan.box_buttons), scan.b_continuous, FALSE, FALSE, 0);

    scan.b_relative = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(scan.b_relative, "Relative scale");
    gtk_button_set_image(GTK_BUTTON(scan.b_relative), gtk_image_new_from_stock(GTK_STOCK_ZOOM_FIT, GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_name(scan.b_relative, "small-button");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_relative), conf.scan_relative);
    gtk_box_pack_start(GTK_BOX(scan.box_buttons), scan.b_relative, FALSE, FALSE, 0);

    scan.b_peakhold = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(scan.b_peakhold, "Peak hold");
    gtk_button_set_image(GTK_BUTTON(scan.b_peakhold), gtk_image_new_from_stock(GTK_STOCK_GOTO_TOP, GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_name(scan.b_peakhold, "small-button");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_peakhold), conf.scan_peakhold);
    g_signal_connect(scan.b_peakhold, "clicked", G_CALLBACK(scan_peakhold), NULL);
    gtk_box_pack_start(GTK_BOX(scan.box_buttons), scan.b_peakhold, FALSE, FALSE, 0);

    scan.b_hold = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(scan.b_hold, "Hold");
    gtk_button_set_image(GTK_BUTTON(scan.b_hold), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_name(scan.b_hold, "small-button");
    if(scan.hold)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_hold), TRUE);
    }
    g_signal_connect(scan.b_hold, "clicked", G_CALLBACK(scan_hold), NULL);
    gtk_box_pack_start(GTK_BOX(scan.box_buttons), scan.b_hold, FALSE, FALSE, 0);


    scan.box_from = gtk_hbox_new(FALSE, 1);
    gtk_box_pack_start(GTK_BOX(scan.box_settings), scan.box_from, FALSE, FALSE, 0);

    scan.l_from = gtk_label_new("From:");
    gtk_misc_set_alignment(GTK_MISC(scan.l_from), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(scan.box_from), scan.l_from, TRUE, TRUE, 2);
    scan.s_start = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.scan_start, TUNER_FREQ_MIN, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
    gtk_box_pack_start(GTK_BOX(scan.box_from), scan.s_start, FALSE, FALSE, 0);

    scan.box_to = gtk_hbox_new(FALSE, 1);
    gtk_box_pack_start(GTK_BOX(scan.box_settings), scan.box_to, FALSE, FALSE, 0);

    scan.l_to = gtk_label_new("To:");
    gtk_misc_set_alignment(GTK_MISC(scan.l_to), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(scan.box_to), scan.l_to, TRUE, TRUE, 2);
    scan.s_end = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.scan_end, TUNER_FREQ_MIN, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
    gtk_box_pack_start(GTK_BOX(scan.box_to), scan.s_end, FALSE, FALSE, 0);


    scan.box_step = gtk_hbox_new(FALSE, 1);
    gtk_box_pack_start(GTK_BOX(scan.box_settings), scan.box_step, FALSE, FALSE, 0);

    scan.l_step = gtk_label_new("Step:");
    gtk_misc_set_alignment(GTK_MISC(scan.l_step), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(scan.box_step), scan.l_step, TRUE, TRUE, 2);
    scan.s_step = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.scan_step, 5.0, 1000.0, 5.0, 5.0, 0.0)), 0, 0);
    gtk_box_pack_start(GTK_BOX(scan.box_step), scan.s_step, TRUE, TRUE, 0);


    scan.box_bw = gtk_hbox_new(FALSE, 1);
    gtk_box_pack_start(GTK_BOX(scan.box_settings), scan.box_bw, FALSE, FALSE, 0);

    scan.l_bw = gtk_label_new("BW:");
    gtk_misc_set_alignment(GTK_MISC(scan.l_bw), 0.0, 0.5);
    scan.d_bw = gtk_combo_box_new_text();
    gui_fill_bandwidths(scan.d_bw, FALSE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(scan.d_bw), 13);
    gtk_box_pack_start(GTK_BOX(scan.box_bw), scan.l_bw, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(scan.box_bw), scan.d_bw, FALSE, FALSE, 0);
    if(conf.scan_bw >= 0)
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(scan.d_bw), conf.scan_bw);
    }

    scan.box_presets = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(scan.box_settings), scan.box_presets, FALSE, FALSE, 0);

    scan.b_ccir = gtk_button_new_with_label("CCIR");
    gtk_widget_set_name(scan.b_ccir, "small-button");
    gtk_box_pack_start(GTK_BOX(scan.box_presets), scan.b_ccir, TRUE, TRUE, 0);
    g_signal_connect(scan.b_ccir, "clicked", G_CALLBACK(scan_ccir), NULL);

    scan.b_oirt = gtk_button_new_with_label("OIRT");
    gtk_widget_set_name(scan.b_oirt, "small-button");
    gtk_box_pack_start(GTK_BOX(scan.box_presets), scan.b_oirt, TRUE, TRUE, 0);
    g_signal_connect(scan.b_oirt, "clicked", G_CALLBACK(scan_oirt), NULL);

    scan.marks_menu = scan_dialog_menu();
    scan.b_marks = gtk_button_new_with_label(" M ");
    gtk_widget_set_name(scan.b_marks, "small-button");
    gtk_box_pack_start(GTK_BOX(scan.box_presets), scan.b_marks, FALSE, FALSE, 0);
    g_signal_connect_swapped(scan.b_marks, "event", G_CALLBACK(scan_marks), scan.marks_menu);


    scan.view = gtk_drawing_area_new();
    gtk_widget_add_events(scan.view, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    gtk_widget_set_size_request(scan.view, 640, -1);
    gtk_box_pack_start(GTK_BOX(scan.box), scan.view, TRUE, TRUE, 0);

    g_signal_connect(scan.view, "expose-event", G_CALLBACK(scan_redraw), NULL);
    g_signal_connect_swapped(scan.b_relative, "clicked", G_CALLBACK(gtk_widget_queue_draw), (GCallback)scan.view);
    g_signal_connect(scan.view, "button-press-event", G_CALLBACK(scan_click), NULL);
    g_signal_connect(scan.view, "button-release-event", G_CALLBACK(scan_click), NULL);
    g_signal_connect(scan.view, "motion-notify-event", G_CALLBACK(scan_motion), NULL);
    g_signal_connect(scan.window, "configure-event", G_CALLBACK(scan_window_event), NULL);
    g_signal_connect(scan.window, "key-press-event", G_CALLBACK(keyboard_press), GINT_TO_POINTER(TRUE));
    g_signal_connect(scan.window, "key-release-event", G_CALLBACK(keyboard_release), NULL);
    g_signal_connect(scan.window, "destroy", G_CALLBACK(scan_destroy), NULL);

    if(scan.active)
    {
        scan_lock(TRUE);
    }

    gtk_widget_show_all(scan.window);
    gtk_window_set_transient_for(GTK_WINDOW(scan.window), NULL);
}

GtkWidget* scan_dialog_menu()
{
    GtkWidget* menu = gtk_menu_new();

    GtkWidget* title = gtk_menu_item_new_with_label("Frequency marks");
    gtk_widget_set_sensitive(title, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), title);

    GtkWidget* sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);

    GtkWidget* add_100k = gtk_image_menu_item_new_with_label("Add every 100 kHz");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(add_100k),
                                  GTK_WIDGET(gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), add_100k);
    g_signal_connect(add_100k, "activate", G_CALLBACK(scan_marks_add), GINT_TO_POINTER(100));

    GtkWidget* add_1M = gtk_image_menu_item_new_with_label("Add every 1 MHz");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(add_1M),
                                  GTK_WIDGET(gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), add_1M);
    g_signal_connect(add_1M, "activate", G_CALLBACK(scan_marks_add), GINT_TO_POINTER(1000));

    GtkWidget* add_2M = gtk_image_menu_item_new_with_label("Add every 2 MHz");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(add_2M),
                                  GTK_WIDGET(gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), add_2M);
    g_signal_connect(add_2M, "activate", G_CALLBACK(scan_marks_add), GINT_TO_POINTER(2000));

    GtkWidget* clear_visible = gtk_image_menu_item_new_with_label("Clear visible marks");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(clear_visible),
                                  GTK_WIDGET(gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), clear_visible);
    g_signal_connect(clear_visible, "activate", G_CALLBACK(scan_marks_clear), GINT_TO_POINTER(FALSE));

    GtkWidget* clear_all = gtk_image_menu_item_new_with_label("Clear all marks");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(clear_all),
                                  GTK_WIDGET(gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), clear_all);
    g_signal_connect(clear_all, "activate", G_CALLBACK(scan_marks_clear), GINT_TO_POINTER(TRUE));

    gtk_widget_show_all(menu);
    return menu;
}

void scan_destroy(GtkWidget *widget, gpointer user_data)
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

gboolean scan_window_event(GtkWidget* widget, gpointer data)
{
    gtk_window_get_position(GTK_WINDOW(scan.window), &conf.scan_x, &conf.scan_y);
    gtk_window_get_size(GTK_WINDOW(scan.window), &conf.scan_width, &conf.scan_height);
    return FALSE;
}

void scan_toggle(GtkWidget *widget, gpointer user_data)
{
    gint start, stop, step, bw;
    gboolean continuous;
    gchar buff[50];
    GtkWidget* dialog;
    gint samples;

    if(!tuner.thread)
    {
        return;
    }

    if(scan.active)
    {
        /* Try to cancel current scan */
        tuner_write("");
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
            dialog = gtk_message_dialog_new(GTK_WINDOW(scan.window),
                                            GTK_DIALOG_MODAL,
                                            GTK_MESSAGE_INFO,
                                            GTK_BUTTONS_CLOSE,
                                            (samples < SCAN_MIN_SAMPLES) ? "The selected sample count is too low." : "The selected sample count is too large.");
            gtk_window_set_title(GTK_WINDOW(dialog), "Spectral scan");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }

        g_snprintf(buff, sizeof(buff),
                   "Sa%d\nSb%d\nSc%d\nSf%d\nS%s",
                   start, stop, step, filters[bw], (continuous?"m":""));

        tuner_write(buff);
        scan_lock(TRUE);
        gtk_widget_set_sensitive(scan.b_start, FALSE);
    }

}

void scan_lock(gboolean value)
{
    gboolean sensitive = !value;
    gtk_widget_set_sensitive(scan.b_continuous, sensitive);
    gtk_widget_set_sensitive(scan.s_start, sensitive);
    gtk_widget_set_sensitive(scan.s_end, sensitive);
    gtk_widget_set_sensitive(scan.s_step, sensitive);
    gtk_widget_set_sensitive(scan.d_bw, sensitive);
    gtk_widget_set_sensitive(scan.b_ccir, sensitive);
    gtk_widget_set_sensitive(scan.b_oirt, sensitive);
    gtk_button_set_image(GTK_BUTTON(scan.b_start),
                         gtk_image_new_from_stock((value?GTK_STOCK_MEDIA_STOP:GTK_STOCK_MEDIA_PLAY), GTK_ICON_SIZE_BUTTON));
    scan.locked = value;
}

void scan_peakhold(GtkWidget *widget, gpointer user_data)
{
    if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
        if(scan.peak)
        {
            scan_data_free(scan.peak);
            scan.peak = NULL;
            gtk_widget_queue_draw(scan.view);
        }
    }
}

void scan_hold(GtkWidget *widget, gpointer user_data)
{
    gboolean redraw = FALSE;

    if(scan.hold)
    {
        scan_data_free(scan.hold);
        scan.hold = NULL;
        redraw = TRUE;
    }

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
        if(scan.data)
        {
            scan.hold = scan_data_copy(scan.data);
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
    {
        gtk_widget_queue_draw(scan.view);
    }
}

gboolean scan_redraw(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
    cairo_t *cr = gdk_cairo_create(widget->window);
    static const double grid_pattern[] = {1.0, 1.0};
    static const gint grid_pattern_len = 2;
    cairo_text_extents_t extents;
    cairo_pattern_t *gradient;
    gint width, height;
    gdouble max, min, step;
    gdouble x_focus, y_focus, point_radius;
    gchar text[50];
    GList* l;

    width = widget->allocation.width - SCAN_OFFSET_LEFT - SCAN_OFFSET_RIGHT;
    height = widget->allocation.height - SCAN_OFFSET_TOP - SCAN_OFFSET_BOTTOM;
    cairo_select_font_face(cr, SCAN_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_line_width(cr, 1.0);

    /* Clear the screen */
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    /* Give up if no data is available */
    if(!scan.data || scan.data->len < 2)
    {
        cairo_destroy(cr);
        return FALSE;
    }

    step = width/(gdouble)(scan.data->len - 1);
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan.b_relative)))
    {
        if(scan.peak && scan.hold)
        {
            min = MIN(scan.data->min, scan.hold->min);
            max = MAX(scan.peak->max, scan.hold->max);
        }
        else if(scan.peak)
        {
            min = scan.data->min;
            max = scan.peak->max;
        }
        else if(scan.hold)
        {
            min = MIN(scan.data->min, scan.hold->min);
            max = MAX(scan.data->max, scan.hold->max);
        }
        else
        {
            min = scan.data->min;
            max = scan.data->max;
        }
    }
    else
    {
        min = SCAN_DEFAULT_MIN_LEVEL;
        max = SCAN_DEFAULT_MAX_LEVEL;
    }

    /* Draw the spectrum (peak) */
    if(scan.peak)
    {
        gradient = cairo_pattern_create_linear(0.0, 0.0, 0.0, height);
        SCAN_ADD_COLOR(gradient, 1.0, 0x0030e0, SCAN_SPECTRUM_ALPHA_PEAK);
        SCAN_ADD_COLOR(gradient, 0.8, 0x00e000, SCAN_SPECTRUM_ALPHA_PEAK);
        SCAN_ADD_COLOR(gradient, 0.6, 0xe0e000, SCAN_SPECTRUM_ALPHA_PEAK);
        SCAN_ADD_COLOR(gradient, 0.4, 0xe00000, SCAN_SPECTRUM_ALPHA_PEAK);
        SCAN_ADD_COLOR(gradient, 0.0, 0x661b00, SCAN_SPECTRUM_ALPHA_PEAK);
        scan_draw_spectrum(cr, scan.peak, width, height, step, min, max);
        cairo_set_source(cr, gradient);
        cairo_close_path(cr);
        cairo_fill(cr);
        cairo_pattern_destroy(gradient);
    }

    /* Draw the spectrum (current) */
    gradient = cairo_pattern_create_linear(0.0, 0.0, 0.0, height);
    SCAN_ADD_COLOR(gradient, 1.0, 0x0030e0, SCAN_SPECTRUM_ALPHA);
    SCAN_ADD_COLOR(gradient, 0.8, 0x00e000, SCAN_SPECTRUM_ALPHA);
    SCAN_ADD_COLOR(gradient, 0.6, 0xe0e000, SCAN_SPECTRUM_ALPHA);
    SCAN_ADD_COLOR(gradient, 0.4, 0xe00000, SCAN_SPECTRUM_ALPHA);
    SCAN_ADD_COLOR(gradient, 0.0, 0x661b00, SCAN_SPECTRUM_ALPHA);
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

    /* Draw each marked frequency */
    cairo_save(cr);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_dash(cr, grid_pattern, grid_pattern_len, 0);
    for(l=conf.scan_freq_list; l; l=l->next)
    {
        gint freq = GPOINTER_TO_INT(l->data);
        if(freq >= scan.data->signals[0].freq &&
           freq <= scan.data->signals[scan.data->len-1].freq)
        {
            scan_draw_mark(cr, width, height, freq);
        }
    }
    cairo_restore(cr);

    /* Draw a bottom horizontal line  */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr, SCAN_OFFSET_LEFT-0.5, SCAN_OFFSET_TOP+height+0.5);
    cairo_line_to(cr, SCAN_OFFSET_LEFT-0.5+width+0.5, SCAN_OFFSET_TOP+height+0.5);
    cairo_stroke(cr);

    if(scan.focus >= 0 && scan.focus < scan.data->len)
    {
        /* Mark the selected position */
        x_focus = SCAN_OFFSET_LEFT+step*scan.focus;
        y_focus = SCAN_OFFSET_TOP+height - MAP(scan.data->signals[scan.focus].signal, min, max, 0.0, height);
        point_radius = (height > 200 ? 5.0 : height / 40.0);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_set_line_width(cr, SCAN_POINT_WIDTH);
        cairo_arc(cr, x_focus, y_focus, point_radius, 0.0, 2.0 * M_PI);
        cairo_stroke(cr);

        /* Show selected frequency and signal */
        g_snprintf(text, sizeof(text), "%s %d%s",
                   scan_format_frequency(scan.data->signals[scan.focus].freq),
                   (gint)ceil(signal_level(scan.data->signals[scan.focus].signal)),
                   signal_level_unit());

        cairo_select_font_face(cr, SCAN_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, round(height > 300 ? 20.0 : (2/45.0 * height + 20/3.0)));
        cairo_text_extents(cr, text, &extents);
        x_focus -= (extents.width/2.0 + extents.x_bearing);
        y_focus -= (extents.height + point_radius + SCAN_POINT_WIDTH);

        /* Fit the label within a spectrum view size */
        if(x_focus <= SCAN_OFFSET_LEFT)
        {
            x_focus = SCAN_OFFSET_LEFT;
        }
        if(x_focus+extents.width > width)
        {
            x_focus = SCAN_OFFSET_LEFT + width - extents.width;
        }
        if(y_focus < SCAN_OFFSET_TOP)
        {
            y_focus += extents.height + 2*(point_radius + SCAN_POINT_WIDTH);
        }

        /* Draw a transparent background for text */
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
        cairo_rectangle(cr, x_focus - 1.0,
                            y_focus - 1.0,
                            extents.width + extents.x_bearing + 2.0,
                            extents.height + 2.0);
        cairo_fill(cr);

        /* Draw a text */
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_move_to(cr, x_focus, y_focus-extents.y_bearing);
        cairo_show_text(cr, text);
        cairo_stroke(cr);
    }

    cairo_destroy(cr);
    return FALSE;
}

void scan_draw_spectrum(cairo_t* cr, scan_data_t* data, gint width, gint height, gdouble step, gdouble min, gdouble max)
{
    gdouble x_src, x_control, x_dest;
    gdouble y_src, y_dest;
    gint i;

    x_src = 0.0;
    x_dest = 0.0;
    y_src = height - MAP(data->signals[0].signal, min, max, 0.0, height);
    cairo_move_to(cr, SCAN_OFFSET_LEFT+x_src, SCAN_OFFSET_TOP+y_src);

    for(i=1; i<data->len; i++)
    {
        x_dest += step;
        x_control = x_src + (x_dest - x_src) / 2.0;
        y_dest = height - MAP(data->signals[i].signal, min, max, 0.0, height);
        cairo_curve_to(cr, SCAN_OFFSET_LEFT+x_control, SCAN_OFFSET_TOP+y_src,
                           SCAN_OFFSET_LEFT+x_control, SCAN_OFFSET_TOP+y_dest,
                           SCAN_OFFSET_LEFT+x_dest, SCAN_OFFSET_TOP+y_dest);
        x_src = x_dest;
        y_src = y_dest;
    }

    cairo_line_to(cr, SCAN_OFFSET_LEFT+width, SCAN_OFFSET_TOP+height);
    cairo_line_to(cr, SCAN_OFFSET_LEFT, SCAN_OFFSET_TOP+height);
}

void scan_draw_mark(cairo_t* cr, gint width, gint height, gint freq)
{
    cairo_text_extents_t extents;
    const gchar* freq_text;
    gdouble x;

    x  = freq - scan.data->signals[0].freq;
    x /= (gdouble)(scan.data->signals[scan.data->len-1].freq - scan.data->signals[0].freq);
    x *= width;

    /* Draw a vertical line */
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.75);
    cairo_move_to(cr, SCAN_OFFSET_LEFT+x, SCAN_OFFSET_TOP);
    cairo_line_to(cr, SCAN_OFFSET_LEFT+x, SCAN_OFFSET_TOP+height+4.0);
    cairo_stroke(cr);

    /* Draw a frequency label */
    cairo_set_font_size(cr, SCAN_OFFSET_BOTTOM-5.0);
    freq_text = scan_format_frequency(freq);
    cairo_text_extents(cr, freq_text, &extents);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr,
                  SCAN_OFFSET_LEFT+x-(extents.width/2.0 + extents.x_bearing),
                  SCAN_OFFSET_TOP+SCAN_OFFSET_BOTTOM+height-3.0);
    cairo_show_text(cr, freq_text);
    cairo_stroke(cr);
}

const gchar* scan_format_frequency(gint freq)
{
    static gchar buff[10];
    gint length, i;

    g_snprintf(buff, sizeof(buff), "%.3f", freq/1000.0);
    length = strlen(buff);

    for(i=length-1; i>0; i--)
    {
        if(buff[i] == '0' && buff[i-1] != '.')
        {
            buff[i] = '\0';
        }
        else
        {
            break;
        }
    }
    return buff;
}

gboolean scan_update(gpointer user_data)
{
    scan_data_t* data_new = (scan_data_t*)user_data;
    gboolean peakhold = (scan.window && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan.b_peakhold)));

    gint i;

    if(scan.data)
    {
        if(peakhold &&
           scan.data->len == data_new->len &&
           scan.data->signals[0].freq == data_new->signals[0].freq &&
           scan.data->signals[scan.data->len-1].freq == data_new->signals[data_new->len-1].freq)
        {
            if(!scan.peak)
            {
                scan.peak = scan.data;
            }
            else
            {
                scan_data_free(scan.data);
            }
        }
        else
        {
            scan_data_free(scan.data);
            if(scan.peak)
            {
                scan_data_free(scan.peak);
                scan.peak = NULL;
            }
        }
    }

    if(scan.hold &&
       !(scan.hold->len == data_new->len &&
         scan.hold->signals[0].freq == data_new->signals[0].freq &&
         scan.hold->signals[scan.hold->len-1].freq == data_new->signals[data_new->len-1].freq))
    {
        scan_data_free(scan.hold);
        scan.hold = NULL;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_hold), FALSE);
    }

    if(scan.peak)
    {
        for(i=0; i<data_new->len; i++)
        {
            if(data_new->signals[i].signal > scan.peak->signals[i].signal)
            {
                scan.peak->signals[i].signal = data_new->signals[i].signal;
            }
        }

        if(data_new->max > scan.peak->max)
        {
            scan.peak->max = data_new->max;
        }

        if(data_new->min < scan.peak->min)
        {
            scan.peak->min = data_new->min;
        }
    }

    scan.active = TRUE;
    scan.data = data_new;

    if(scan.window)
    {
        if(!scan.locked)
        {
            scan_lock(TRUE);
        }
        gtk_widget_queue_draw(scan.view);
        gtk_widget_set_sensitive(scan.b_start, TRUE);
    }
    return FALSE;
}

void scan_check_finished()
{
    if(scan.active)
    {
        scan.active = FALSE;
        if(scan.window)
        {
            gtk_widget_set_sensitive(scan.b_start, TRUE);
            scan_lock(FALSE);
        }
    }
}

gboolean scan_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    if(scan.data)
    {
        /* Left click */
        if(event->type == GDK_BUTTON_PRESS && event->button == 1)
        {
            scan.motion_tuning = TRUE;
            if(scan.focus >= 0 && scan.focus < scan.data->len)
            {
                tuner_set_frequency(scan.data->signals[scan.focus].freq);
            }
        }
        else if(event->type == GDK_BUTTON_RELEASE && event->button == 1)
        {
            scan.motion_tuning = FALSE;
        }
        /* Right click */
        else if(event->type == GDK_BUTTON_PRESS && event->button == 3 &&
                scan.focus >= 0 && scan.focus < scan.data->len)
        {
            settings_scan_mark_toggle(scan.data->signals[scan.focus].freq);
            gtk_widget_queue_draw(scan.view);
        }
    }
    return FALSE;
}

gboolean scan_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
    gdouble width = widget->allocation.width - SCAN_OFFSET_LEFT - SCAN_OFFSET_RIGHT;
    gdouble x = event->x - 0.5 - SCAN_OFFSET_LEFT;
    gint current_focus;

    if(scan.data)
    {
        current_focus = round(x/(width/(gdouble)(scan.data->len - 1)));
        if(current_focus >= 0 && current_focus < scan.data->len)
        {
            if(scan.motion_tuning && tuner.freq != scan.data->signals[current_focus].freq)
            {
                tuner_set_frequency(scan.data->signals[current_focus].freq);
            }
        }
        else
        {
            current_focus = -1;
        }

        if(scan.focus != current_focus)
        {
            scan.focus = current_focus;
            gtk_widget_queue_draw(scan.view);
        }
    }
    return TRUE;
}

void scan_ccir(GtkWidget *widget, gpointer data)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_start), 87500.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_end), 108000.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_step), 100.0);
}

void scan_oirt(GtkWidget *widget, gpointer data)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_start), 65750.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_end), 74000.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_step), 30.0);
}

gboolean scan_marks(GtkWidget *widget, GdkEventButton *event)
{
    if(event->type == GDK_BUTTON_PRESS)
    {
        gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL, event->button, event->time);
        return TRUE;
    }
    return FALSE;
}

void scan_marks_add(GtkMenuItem *menuitem, gpointer user_data)
{
    gint step = GPOINTER_TO_INT(user_data);
    gint min, max, curr;
    if(scan.data)
    {
        min = ceil(scan.data->signals[0].freq / (gdouble)step)*step;
        max = floor(scan.data->signals[scan.data->len-1].freq / (gdouble)step)*step;
        for(curr=min; curr<=max; curr+=step)
        {
            settings_scan_mark_add(curr);
        }
    }
    gtk_widget_queue_draw(scan.view);
}

void scan_marks_clear(GtkMenuItem *menuitem, gpointer user_data)
{
    gboolean all = GPOINTER_TO_INT(user_data);
    if(all)
    {
        settings_scan_mark_clear_all();
    }
    else if(scan.data)
    {
        settings_scan_mark_clear(scan.data->signals[0].freq,
                                 scan.data->signals[scan.data->len-1].freq);
    }
    gtk_widget_queue_draw(scan.view);
}

scan_data_t* scan_data_copy(scan_data_t* data)
{
    scan_data_t* copy;
    gint i;

    copy = g_new(scan_data_t, 1);
    copy->len = data->len;
    copy->max = data->max;
    copy->min = data->min;
    copy->signals = g_new(scan_node_t, data->len);
    for(i=0;i<data->len;i++)
    {
        copy->signals[i].freq = data->signals[i].freq;
        copy->signals[i].signal = data->signals[i].signal;
    }
    return copy;
}

void scan_data_free(scan_data_t* data)
{
    free(data->signals);
    free(data);
}
