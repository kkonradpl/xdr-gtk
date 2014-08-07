#include <math.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "gui.h"
#include "gui-tuner.h"
#include "settings.h"
#include "scan.h"
#include "tuner.h"

void scan_dialog()
{
    if(scan.window)
    {
        gtk_window_present(GTK_WINDOW(scan.dialog));
        return;
    }
    scan_init(&scan);
    scan.dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(scan.dialog), "Spectral scan");
    gtk_window_set_icon_name(GTK_WINDOW(scan.dialog), "xdr-gtk-scan");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(scan.dialog), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(scan.dialog), 2);
    gtk_window_set_default_size(GTK_WINDOW(scan.dialog), conf.scan_width, conf.scan_height);

    GtkWidget *content = gtk_vbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(scan.dialog), content);

    GtkWidget *menu_box = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), menu_box, FALSE, FALSE, 0);

    scan.b_start = gtk_button_new_with_label("Scan");
    gtk_button_set_image(GTK_BUTTON(scan.b_start), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_name(scan.b_start, "small-button");
    g_signal_connect(scan.b_start, "clicked", G_CALLBACK(scan_toggle), (gpointer)&scan);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.b_start, TRUE, TRUE, 0);

    scan.b_continuous = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(scan.b_continuous, "Continous scan");
    gtk_button_set_image(GTK_BUTTON(scan.b_continuous), gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_name(scan.b_continuous, "small-button");
    g_signal_connect(scan.b_continuous, "clicked", G_CALLBACK(scan_continuous), (gpointer)&scan);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.b_continuous, TRUE, TRUE, 0);

    scan.b_tune = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(scan.b_tune, "Tune");
    gtk_button_set_image(GTK_BUTTON(scan.b_tune), gtk_image_new_from_stock(GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_name(scan.b_tune, "small-button");
    g_signal_connect(scan.b_tune, "clicked", G_CALLBACK(scan_tune), (gpointer)&scan);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.b_tune, TRUE, TRUE, 0);

    scan.b_relative = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(scan.b_relative, "Relative scale");
    gtk_button_set_image(GTK_BUTTON(scan.b_relative), gtk_image_new_from_stock(GTK_STOCK_ZOOM_FIT, GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_name(scan.b_relative, "small-button");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan.b_relative), conf.scan_relative);
    g_signal_connect(scan.b_relative, "clicked", G_CALLBACK(scan_relative), NULL);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.b_relative, TRUE, TRUE, 0);

    scan.l_frange = gtk_label_new("Range:");
    scan.e_fstart = gtk_entry_new_with_max_length(6);
    gtk_entry_set_width_chars(GTK_ENTRY(scan.e_fstart), 6);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.l_frange, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.e_fstart, TRUE, TRUE, 0);
    scan.l_frange_ = gtk_label_new(" - ");
    scan.e_fstop = gtk_entry_new_with_max_length(6);
    gtk_entry_set_width_chars(GTK_ENTRY(scan.e_fstop), 6);
    scan.l_fstop_kHz = gtk_label_new("kHz");
    gtk_box_pack_start(GTK_BOX(menu_box), scan.l_frange_, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.e_fstop, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.l_fstop_kHz, TRUE, TRUE, 2);

    scan.l_fstep = gtk_label_new("Step:");
    GtkAdjustment *adj_s = (GtkAdjustment*)gtk_adjustment_new(conf.scan_step, 5.0, 1000.0, 5.0, 5.0, 0.0);
    scan.s_fstep = gtk_spin_button_new(adj_s, 0, 0);
    scan.l_fstep_kHz = gtk_label_new("kHz");
    gtk_box_pack_start(GTK_BOX(menu_box), scan.l_fstep, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.s_fstep, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.l_fstep_kHz, TRUE, TRUE, 2);

    scan.b_ccir = gtk_button_new_with_label("CCIR");
    gtk_box_pack_start(GTK_BOX(menu_box), scan.b_ccir, TRUE, TRUE, 0);
    g_signal_connect(scan.b_ccir, "clicked", G_CALLBACK(scan_ccir), NULL);

    scan.b_oirt = gtk_button_new_with_label("OIRT");
    gtk_box_pack_start(GTK_BOX(menu_box), scan.b_oirt, TRUE, TRUE, 0);
    g_signal_connect(scan.b_oirt, "clicked", G_CALLBACK(scan_oirt), NULL);

    scan.l_bw = gtk_label_new("BW:");
    scan.d_bw = gtk_combo_box_new_text();
    gui_fill_bandwidths(scan.d_bw, FALSE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(scan.d_bw), 13);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.l_bw, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(menu_box), scan.d_bw, TRUE, TRUE, 0);

    gchar buff[20];
    if(conf.scan_start > 0)
    {
        g_snprintf(buff, 20, "%d", conf.scan_start);
        gtk_entry_set_text(GTK_ENTRY(scan.e_fstart), buff);
    }
    if(conf.scan_end > 0)
    {
        g_snprintf(buff, 20, "%d", conf.scan_end);
        gtk_entry_set_text(GTK_ENTRY(scan.e_fstop), buff);
    }
    if(conf.scan_bw >= 0)
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(scan.d_bw), conf.scan_bw);
    }

    scan.image = gtk_drawing_area_new();
    gtk_widget_add_events(scan.image, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    gtk_widget_set_size_request(scan.image, -1, 100);
    gtk_box_pack_start(GTK_BOX(content), scan.image, TRUE, TRUE, 0);

    scan.label = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(content), scan.label, FALSE, FALSE, 0);

    g_signal_connect(scan.image, "expose-event", G_CALLBACK(scan_redraw), (gpointer)&scan);
    g_signal_connect_swapped(scan.b_relative, "clicked", G_CALLBACK(gtk_widget_queue_draw), (gpointer)(scan.image));
    g_signal_connect(scan.image, "button-press-event", G_CALLBACK(scan_click), (gpointer)&scan);
    g_signal_connect(scan.image, "motion-notify-event", G_CALLBACK(scan_motion), (gpointer)&scan);
    g_signal_connect(scan.image, "configure-event", G_CALLBACK(scan_resize), NULL);
    g_signal_connect(scan.dialog, "destroy", G_CALLBACK(scan_destroy), NULL);

    gtk_widget_show_all(scan.dialog);
}

void scan_toggle(GtkWidget *widget, scan_t* scan)
{
    gchar buff[50];
    if(!tuner.thread)
    {
        return;
    }
    if(!scan->active)
    {
        gint bw = gtk_combo_box_get_active(GTK_COMBO_BOX(scan->d_bw));
        gint start = atoi(gtk_entry_get_text(GTK_ENTRY(scan->e_fstart)));
        gint stop = atoi(gtk_entry_get_text(GTK_ENTRY(scan->e_fstop)));
        gint step = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(scan->s_fstep));
        gboolean continuous = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan->b_continuous));
        g_snprintf(buff, sizeof(buff), "Sa%d\nSb%d\nSc%d\nSf%d\nS%s", start, stop, step, filters[bw], continuous?"m":"");
        tuner_write(buff);

        gtk_button_set_image(GTK_BUTTON(widget), gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_BUTTON));
        if(!continuous)
        {
            gtk_widget_set_sensitive(scan->b_start, FALSE);
        }
        gtk_widget_set_sensitive(scan->b_continuous, FALSE);
        gtk_widget_set_sensitive(scan->b_tune, FALSE);
        gtk_widget_set_sensitive(scan->e_fstart, FALSE);
        gtk_widget_set_sensitive(scan->e_fstop, FALSE);
        gtk_widget_set_sensitive(scan->s_fstep, FALSE);
        gtk_widget_set_sensitive(scan->d_bw, FALSE);
        gtk_widget_set_sensitive(scan->b_ccir, FALSE);
        gtk_widget_set_sensitive(scan->b_oirt, FALSE);
    }
    else
    {
        tuner_write("");
        gtk_button_set_image(GTK_BUTTON(widget), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON));
        gtk_widget_set_sensitive(scan->b_start, TRUE);
        gtk_widget_set_sensitive(scan->b_continuous, TRUE);
        gtk_widget_set_sensitive(scan->b_tune, TRUE);
        gtk_widget_set_sensitive(scan->e_fstart, TRUE);
        gtk_widget_set_sensitive(scan->e_fstop, TRUE);
        gtk_widget_set_sensitive(scan->s_fstep, TRUE);
        gtk_widget_set_sensitive(scan->d_bw, TRUE);
        gtk_widget_set_sensitive(scan->b_ccir, TRUE);
        gtk_widget_set_sensitive(scan->b_oirt, TRUE);
    }

    gtk_widget_queue_draw(scan->image);
    scan->active = !scan->active;
}

void scan_continuous(GtkWidget *widget, scan_t* scan)
{
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan->b_tune), FALSE);
    }
}

void scan_tune(GtkWidget *widget, scan_t* scan)
{
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan->b_continuous), FALSE);
    }
}

void scan_relative(GtkWidget *widget)
{
    conf.scan_relative = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

void scan_destroy(GtkWidget *widget, gpointer data)
{
    scan.window = FALSE;
    scan.active = FALSE;
    conf.scan_start = atoi(gtk_entry_get_text(GTK_ENTRY(scan.e_fstart)));
    conf.scan_end = atoi(gtk_entry_get_text(GTK_ENTRY(scan.e_fstop)));
    conf.scan_step = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(scan.s_fstep));
    conf.scan_bw = gtk_combo_box_get_active(GTK_COMBO_BOX(scan.d_bw));
    settings_write();

    if(scan.data)
    {
        g_free(scan.data->signals);
        g_free(scan.data);
    }
}

gboolean scan_redraw(GtkWidget *widget, GdkEventExpose *event, scan_t* scan)
{
    gint w = widget->allocation.width;
    gint h = widget->allocation.height;
    gint i;

    cairo_t *cr = gdk_cairo_create(widget->window);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_set_line_width(cr, 1);
    if(!scan->data || !scan->data->len)
    {
        cairo_destroy(cr);
        return FALSE;
    }
    gfloat step = w/(gfloat)(scan->data->len-1);

    cairo_set_source_rgb(cr, 0, 0, 0);
    gdouble j = 0.0, sig;

    for(i = 0; i <= scan->data->len; i++)
    {
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan->b_relative)))
        {
            sig = 100.0/(scan->data->max - scan->data->min)*(scan->data->signals[i].signal-scan->data->min);
        }
        else
        {
            sig = scan->data->signals[i].signal;
        }

        cairo_line_to(cr, j, h-(sig * (h/100.0)));
        j+=step;
    }
    cairo_line_to(cr, w, h);
    cairo_line_to(cr, 0, h);
    cairo_pattern_t *gradient;
    gradient = cairo_pattern_create_linear(0.0, 0.0, 0, h);
    cairo_pattern_add_color_stop_rgb(gradient, 1.0, 0, 0.25, 1);
    cairo_pattern_add_color_stop_rgb(gradient, 0.75, 0, 1, 0.5);
    cairo_pattern_add_color_stop_rgb(gradient, 0.5, 0.5, 1, 0);
    cairo_pattern_add_color_stop_rgb(gradient, 0.25, 1, 1, 0);
    cairo_pattern_add_color_stop_rgb(gradient, 0.0, 1, 0, 0);
    cairo_set_source(cr, gradient);
    cairo_close_path(cr);
    cairo_fill(cr);
    if(scan->focus >= 0)
    {
        cairo_set_source_rgb(cr, 0, 0, 0);
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan->b_relative)))
        {
            sig = 100.0/(scan->data->max - scan->data->min)*(scan->data->signals[scan->focus].signal-scan->data->min);
        }
        else
        {
            sig = scan->data->signals[scan->focus].signal;
        }
        cairo_arc(cr, step*scan->focus, h-(sig * (h/100.0)), 4 , 0, 2 * M_PI);
        cairo_stroke(cr);
    }
    cairo_pattern_destroy(gradient);
    cairo_destroy(cr);
    return FALSE;
}

void scan_init(scan_t *scan)
{
    scan->window = TRUE;
    scan->active = FALSE;
    scan->data = NULL;
    scan->focus = -1;
}

gboolean scan_update(gpointer tmp)
{
    scan_data_t* data_new = tmp;
    if(!scan.active)
    {
        g_free(data_new->signals);
        g_free(data_new);
        return FALSE;
    }

    if(scan.data)
    {
        g_free(scan.data->signals);
        g_free(scan.data);
    }
    scan.data = data_new;
    gtk_widget_queue_draw(scan.image);
    if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan.b_continuous)))
    {
        scan_toggle(scan.b_start, &scan);
    }
    return FALSE;
}

gboolean scan_click(GtkWidget *widget, GdkEventButton *event, scan_t* scan)
{
    if(scan->data)
    {
        if(event->button == 1)
        {
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan->b_tune)))
            {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scan->b_tune), FALSE);
            }
            else
            {
                tuner_set_frequency(scan->data->signals[scan->focus].freq);
            }
        }
    }
    return TRUE;
}

gboolean scan_motion(GtkWidget *widget, GdkEventMotion *event, scan_t* scan)
{
    gchar* text;
    if(scan->data)
    {
        scan->focus = round(event->x/(widget->allocation.width/(gfloat)(scan->data->len-1)));
        if(scan->focus < scan->data->len)
        {
            gint f = scan->data->signals[scan->focus].freq;
            text = g_markup_printf_escaped("<span size=\"20000\">%d kHz (%.0f)</span>", f, scan->data->signals[scan->focus].signal);
            gtk_label_set_markup(GTK_LABEL(scan->label), text);
            g_free(text);
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(scan->b_tune)))
            {
                if(tuner.freq!=f)
                {
                    tuner_set_frequency(f);
                }
            }
        gtk_widget_queue_draw(scan->image);
        }
    }
    return TRUE;
}

void scan_resize(GtkWidget* widget, gpointer data)
{
    gtk_window_get_size(GTK_WINDOW(scan.dialog), &conf.scan_width, &conf.scan_height);
}

void scan_ccir(GtkWidget *widget, gpointer data)
{
    gtk_entry_set_text(GTK_ENTRY(scan.e_fstart), "87500");
    gtk_entry_set_text(GTK_ENTRY(scan.e_fstop), "108000");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_fstep), 100.0);
}

void scan_oirt(GtkWidget *widget, gpointer data)
{
    gtk_entry_set_text(GTK_ENTRY(scan.e_fstart), "65750");
    gtk_entry_set_text(GTK_ENTRY(scan.e_fstop), "74000");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scan.s_fstep), 30.0);
}
