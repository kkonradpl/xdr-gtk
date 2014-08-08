#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gui.h"
#include "settings.h"
#include "pattern.h"
#include "tuner.h"

void pattern_dialog()
{
    if(pattern.window)
    {
        gtk_window_present(GTK_WINDOW(pattern.dialog));
        return;
    }
    pattern_init(0);
    pattern.dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(pattern.dialog), "Antenna pattern");
    gtk_window_set_icon_name(GTK_WINDOW(pattern.dialog), "xdr-gtk-pattern");
    gtk_window_set_resizable(GTK_WINDOW(pattern.dialog), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(pattern.dialog), 5);

    GtkWidget *content = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(pattern.dialog), content);

    GtkWidget *menu_box = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(content), menu_box);

    pattern.b_start = gtk_button_new_with_label("Start");
    gtk_button_set_image(GTK_BUTTON(pattern.b_start), gtk_image_new_from_stock(GTK_STOCK_MEDIA_RECORD, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(pattern.b_start, "clicked", G_CALLBACK(pattern_start), (gpointer)&pattern);
    gtk_box_pack_start(GTK_BOX(menu_box), pattern.b_start, TRUE, TRUE, 0);

    pattern.b_load = gtk_button_new_with_label("Load");
    gtk_button_set_image(GTK_BUTTON(pattern.b_load), gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(pattern.b_load, "clicked", G_CALLBACK(pattern_load), NULL);
    gtk_box_pack_start(GTK_BOX(menu_box), pattern.b_load, TRUE, TRUE, 0);

    pattern.b_save = gtk_button_new_with_label("Save");
    gtk_button_set_image(GTK_BUTTON(pattern.b_save), gtk_image_new_from_stock(GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(pattern.b_save, "clicked", G_CALLBACK(pattern_save), NULL);
    gtk_box_pack_start(GTK_BOX(menu_box), pattern.b_save, TRUE, TRUE, 0);

    pattern.b_save_img = gtk_button_new_with_label("Save image");
    gtk_button_set_image(GTK_BUTTON(pattern.b_save_img), gtk_image_new_from_stock(GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(pattern.b_save_img, "clicked", G_CALLBACK(pattern_save_img), NULL);
    gtk_box_pack_start(GTK_BOX(menu_box), pattern.b_save_img, TRUE, TRUE, 0);

    pattern.b_close = gtk_button_new_with_label("Close");
    gtk_button_set_image(GTK_BUTTON(pattern.b_close), gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_BUTTON));
    g_signal_connect_swapped(pattern.b_close, "clicked", G_CALLBACK(gui_pattern_destroy), pattern.dialog);
    gtk_box_pack_start(GTK_BOX(menu_box), pattern.b_close, TRUE, TRUE, 0);

    GtkWidget *settings_box = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(content), settings_box);

    pattern.l_size = gtk_label_new("Size:");
    gtk_box_pack_start(GTK_BOX(settings_box), pattern.l_size, FALSE, FALSE, 0);
    GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(conf.pattern_size, 400, 1000.0, 10.0, 25.0, 0.0);
    pattern.s_size = gtk_spin_button_new(adj, 0, 0);
    gtk_box_pack_start(GTK_BOX(settings_box), pattern.s_size, FALSE, FALSE, 0);

    pattern.l_title = gtk_label_new("Title:");
    gtk_box_pack_start(GTK_BOX(settings_box), pattern.l_title, FALSE, FALSE, 0);
    pattern.e_title = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(pattern.e_title), 100);
    gtk_box_pack_start(GTK_BOX(settings_box), pattern.e_title, TRUE, TRUE, 0);

    pattern.image = gtk_drawing_area_new();
    gtk_widget_add_events(pattern.image, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    pattern_resize();
    gtk_box_pack_start(GTK_BOX(content), pattern.image, TRUE, TRUE, 0);

    GtkWidget *plot_settings_box = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(content), plot_settings_box);

    pattern.b_rotate_ll = gtk_button_new_with_label("");
    gtk_button_set_image(GTK_BUTTON(pattern.b_rotate_ll), gtk_image_new_from_stock(GTK_STOCK_MEDIA_REWIND, GTK_ICON_SIZE_MENU));
    g_signal_connect(pattern.b_rotate_ll, "clicked", G_CALLBACK(pattern_rotate), GINT_TO_POINTER(-2));
    gtk_box_pack_start(GTK_BOX(plot_settings_box), pattern.b_rotate_ll, TRUE, TRUE, 0);

    pattern.b_rotate_l = gtk_button_new_with_label("");
    gtk_button_set_image(GTK_BUTTON(pattern.b_rotate_l), gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU));
    g_signal_connect(pattern.b_rotate_l, "clicked", G_CALLBACK(pattern_rotate), GINT_TO_POINTER(-1));
    gtk_box_pack_start(GTK_BOX(plot_settings_box), pattern.b_rotate_l, TRUE, TRUE, 0);

    pattern.b_rotate_peak = gtk_button_new_with_label("Find peak");
    g_signal_connect(pattern.b_rotate_peak, "clicked", G_CALLBACK(pattern_rotate), GINT_TO_POINTER(0));
    gtk_box_pack_start(GTK_BOX(plot_settings_box), pattern.b_rotate_peak, TRUE, TRUE, 0);

    pattern.b_rotate_r = gtk_button_new_with_label("");
    gtk_button_set_image(GTK_BUTTON(pattern.b_rotate_r), gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU));
    g_signal_connect(pattern.b_rotate_r, "clicked", G_CALLBACK(pattern_rotate), GINT_TO_POINTER(1));
    gtk_box_pack_start(GTK_BOX(plot_settings_box), pattern.b_rotate_r, TRUE, TRUE, 0);

    pattern.b_rotate_rr = gtk_button_new_with_label("");
    gtk_button_set_image(GTK_BUTTON(pattern.b_rotate_rr), gtk_image_new_from_stock(GTK_STOCK_MEDIA_FORWARD, GTK_ICON_SIZE_MENU));
    g_signal_connect(pattern.b_rotate_rr, "clicked", G_CALLBACK(pattern_rotate), GINT_TO_POINTER(2));
    gtk_box_pack_start(GTK_BOX(plot_settings_box), pattern.b_rotate_rr, TRUE, TRUE, 0);

    pattern.fill = gtk_check_button_new_with_label("Fill");
    gtk_box_pack_start(GTK_BOX(plot_settings_box), pattern.fill, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pattern.fill), conf.pattern_fill);

    pattern.avg = gtk_check_button_new_with_label("Avg");
    gtk_box_pack_start(GTK_BOX(plot_settings_box), pattern.avg, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pattern.avg), conf.pattern_avg);

    g_signal_connect(pattern.s_size, "value-changed", G_CALLBACK(pattern_resize), NULL);
    g_signal_connect_swapped(pattern.e_title, "changed", G_CALLBACK(gtk_widget_queue_draw), pattern.image);
    g_signal_connect_swapped(pattern.fill, "toggled", G_CALLBACK(gtk_widget_queue_draw), pattern.image);
    g_signal_connect_swapped(pattern.avg, "toggled", G_CALLBACK(gtk_widget_queue_draw), pattern.image);

    g_signal_connect(pattern.image, "expose-event", G_CALLBACK(draw_pattern), NULL);
    g_signal_connect(pattern.dialog, "destroy", G_CALLBACK(gui_pattern_destroy), NULL);

    gtk_widget_show_all(pattern.dialog);
    pattern.window = TRUE;
}

gboolean draw_pattern(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    gint size, radius, i, j;
    gdouble x, y, l, a, sample;
    gchar text[20];
    const gchar *title;
    cairo_t *cr;
    cairo_text_extents_t extents;
    pattern_sig_t *node;

    static const double dash_full[] = {1.0, 2.0};
    static gint dash_len = sizeof(dash_full)/sizeof(dash_full[0]);

    size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pattern.s_size));
    radius = (size - 2*PATTERN_OFFSET) / 2;
    title = gtk_entry_get_text(GTK_ENTRY(pattern.e_title));

    cr = gdk_cairo_create(widget->window);
    cairo_set_line_width(cr, 1);
    cairo_select_font_face(cr, "Bitstream Vera Sans Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // clear image
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // set label color and size
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_font_size(cr, 16);

    // add title label
    x = 2;
    y = 14;
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, title);
    cairo_stroke(cr);

    // add frequency label
    if(pattern.freq)
    {
        g_snprintf(text, sizeof(text), "%.3fMHz", pattern.freq/1000.0);
        cairo_text_extents(cr, text, &extents);
        x = size - extents.width - 4;
        y = 14;
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, text);
        cairo_stroke(cr);
    }

    // set plot color and description font
    cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    cairo_set_font_size(cr, 10);

    // draw polar graph
    cairo_arc(cr, radius+PATTERN_OFFSET, radius+PATTERN_OFFSET, radius, 0, 2*M_PI);
    cairo_stroke(cr);

    // add angle labels
    for(i=3; i<PATTERN_ARCS; i+=3)
    {
        cairo_arc(cr, radius+PATTERN_OFFSET, radius+PATTERN_OFFSET, radius+13, (i-9)*(2*M_PI/PATTERN_ARCS), (i-9)*(2*M_PI/PATTERN_ARCS));
        cairo_get_current_point(cr, &x, &y);
        g_snprintf(text, sizeof(text), "%d°", i*(360/PATTERN_ARCS));
        cairo_text_extents(cr, text, &extents);
        x = x-(extents.width/2 + extents.x_bearing);
        y = y-(extents.height/2 + extents.y_bearing);
        cairo_move_to(cr, x, y);
        cairo_show_text (cr, text);
        cairo_stroke(cr);
    }

    // draw grid
    for(i=0; i<360; i+=10)
    {
        for(j=0; j>-30; j-=1)
        {
            if(i%30 != 0 && j%2 != 0)
            {
                continue;
            }

            if(j%10 == 0)
            {
                continue;
            }

            l = radius*pattern_log_signal(j);
            x = PATTERN_OFFSET+radius+sin(DEG2RAD(i))*l;
            y = PATTERN_OFFSET+radius+cos(DEG2RAD(i))*l;
            cairo_move_to(cr, x, y);
            cairo_line_to(cr, x+1, y+1);
        }
        cairo_stroke(cr);
    }

    // draw scale
    cairo_set_dash(cr, dash_full, dash_len, 0);
    for(i=-10; i>=-40; i-=10)
    {
        cairo_arc(cr, radius+PATTERN_OFFSET, radius+PATTERN_OFFSET, radius*pattern_log_signal(i), -0.5*M_PI, 1.5*M_PI);
        cairo_stroke_preserve(cr);

        g_snprintf(text, sizeof(text), "%d", i);
        cairo_show_text(cr, text);
        cairo_stroke(cr);
    }
    cairo_set_dash(cr, NULL, 0, 0);

    // set plot color
    cairo_set_source_rgb(cr, 0, 0, 1);

    // plot the pattern
    i = 0;
    gfloat prev, next;
    if(pattern.head)
    {
        prev = pattern.tail->sig;
    }

    for(node=pattern.head; node; node=node->next)
    {
        if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.avg)))
        {
            sample = node->sig - pattern.peak;
        }
        else
        {
            if(!node->next)
            {
                next = pattern.head->sig;
            }
            else
            {
                next = node->next->sig;
            }

            sample = (prev + node->sig + next - 3.0*pattern.peak)/ 3.0;
            prev = node->sig;
        }

        sample = pattern_log_signal(sample);

        l = radius*sample;
        a = 180 + i++/(gdouble)pattern.count*360.0 - pattern.rotate_i*360.0/(gdouble)pattern.count;

        x = PATTERN_OFFSET + radius + sin(DEG2RAD(a))*l;
        y = PATTERN_OFFSET + radius + cos(DEG2RAD(a))*l;
        cairo_line_to(cr, x, y);
    }

    // close the plot when finished
    if(!pattern.active)
    {
        cairo_close_path(cr);
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.fill)))
        {
            cairo_stroke_preserve(cr);
            cairo_set_source_rgba(cr, 0, 0, 1, 0.2);
            cairo_fill(cr);
        }
    }

    cairo_stroke(cr);
    cairo_destroy(cr);
    return FALSE;
}

void gui_pattern_destroy(gpointer dialog)
{
    pattern.window = FALSE;
    pattern_clear();
    conf.pattern_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pattern.s_size));
    conf.pattern_fill = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.fill));
    conf.pattern_avg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.avg));
    settings_write();
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

void pattern_init(gint freq)
{
    pattern.active = FALSE;
    pattern.freq = freq;
    pattern.count = 0;
    pattern.peak = 0;
    pattern.peak_i = 0;
    pattern.rotate_i = 0;
    pattern.head = NULL;
    pattern.tail = NULL;
}

void pattern_push(gfloat sig)
{
    pattern_sig_t *node = g_new(pattern_sig_t, 1);
    node->sig = sig;
    node->next = NULL;
    if(sig > pattern.peak)
    {
        pattern.peak = sig;
        pattern.peak_i = pattern.count;
    }
    if(!pattern.head)
    {
        pattern.head = node;
        pattern.tail = node;
    }
    else
    {
        pattern.tail->next = node;
        pattern.tail = node;
    }
    pattern.count++;
    pattern_update();
}

void pattern_start(GtkWidget *widget, gpointer data)
{
    if(!pattern.active)
    {
        pattern_clear();
        pattern_init(tuner.freq);
        gtk_button_set_label(GTK_BUTTON(widget), "Stop ");
        gtk_button_set_image(GTK_BUTTON(widget), gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_BUTTON));
        gtk_widget_set_sensitive(pattern.b_load, FALSE);
        gtk_widget_set_sensitive(pattern.b_save, FALSE);
        gtk_widget_set_sensitive(pattern.b_save_img, FALSE);
    }
    else
    {
        gtk_button_set_label(GTK_BUTTON(widget), "Start");
        gtk_button_set_image(GTK_BUTTON(widget), gtk_image_new_from_stock(GTK_STOCK_MEDIA_RECORD, GTK_ICON_SIZE_BUTTON));
        gtk_widget_set_sensitive(pattern.b_load, TRUE);
        gtk_widget_set_sensitive(pattern.b_save, TRUE);
        gtk_widget_set_sensitive(pattern.b_save_img, TRUE);
    }

    gtk_widget_queue_draw(pattern.image);
    pattern.active = !pattern.active;
}

void pattern_load(GtkWidget *widget, gpointer data)
{
    gchar *filename = NULL;
    FILE *f;
    gfloat sample;
    gchar buff[100];
    gint freq;

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Load file", GTK_WINDOW(gui.window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }
    gtk_widget_destroy(dialog);

    if(!filename)
    {
        return;
    }

    if((f = fopen(filename, "r")))
    {
        fgets(buff, sizeof(buff), f);
        if(!sscanf(buff, "%d", &freq))
        {
            dialog_error("Wrong file format?");
        }
        else
        {
            printf("freq: %d\n", freq);
            pattern_clear();
            pattern_init(freq);

            fgets(buff, sizeof(buff), f);
            if(strlen(buff))
            {
                // remove new line char
                buff[strlen(buff)-1] = 0;
            }
            printf("tytul: %s\n", buff);
            gtk_entry_set_text(GTK_ENTRY(pattern.e_title), buff);
            while(fscanf(f, "%f", &sample) && !feof(f))
            {
                pattern_push(sample);
            }
            gtk_widget_queue_draw(pattern.image);
        }
        fclose(f);
    }
    else
    {
        dialog_error("Unable to open the file");
    }
    g_free(filename);
}

void pattern_save(GtkWidget *widget, gpointer data)
{
    gchar *filename = NULL;
    FILE *f;
    gchar buffer[100];
    const gchar *title;
    pattern_sig_t *node;

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save signal samples", GTK_WINDOW(gui.window), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Text file");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }
    gtk_widget_destroy(dialog);

    if(!filename)
    {
        return;
    }

    if((f = fopen(filename, "w")))
    {
        g_snprintf(buffer, sizeof(buffer), "%d\n", tuner.freq);
        fwrite(buffer, 1, strlen(buffer), f);

        title = gtk_entry_get_text(GTK_ENTRY(pattern.e_title));
        g_snprintf(buffer, sizeof(buffer), "%s\n", title);
        fwrite(buffer, 1, strlen(buffer), f);

        for(node=pattern.head; node; node=node->next)
        {
            g_snprintf(buffer, sizeof(buffer), "%f\n", node->sig);
            fwrite(buffer, 1, strlen(buffer), f);
        }
        fclose(f);
    }
    else
    {
        dialog_error("Unable to save the file");
    }
    g_free(filename);
}

void pattern_save_img(GtkWidget *widget, gpointer data)
{
    gchar *filename = NULL;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save pattern plot", GTK_WINDOW(gui.window), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "PNG image");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }
    gtk_widget_destroy(dialog);

    if(!filename)
    {
        return;
    }

    char *ext = strrchr(filename, '.');
    if(!ext || g_ascii_strcasecmp(ext, ".png"))
    {
        filename = (char*)g_realloc(filename, strlen(filename) + strlen(".png") + 1);
        strcat(filename, ".png");
    }

    GdkPixmap *pixmap = gtk_widget_get_snapshot(pattern.image, NULL);
    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 0, 0, 0, 0, -1, -1);
    gdk_pixbuf_save(pixbuf, filename, "png", NULL, NULL);
    g_object_unref(G_OBJECT(pixmap));
    g_object_unref(G_OBJECT(pixbuf));

    g_free(filename);
}

void pattern_resize()
{
    gint s = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pattern.s_size));
    gtk_widget_set_size_request(pattern.image, s, s);
}

gboolean pattern_update()
{
    gtk_widget_queue_draw(pattern.image);
    return FALSE;
}

void pattern_clear()
{
    pattern.active = FALSE;
    pattern_sig_t *tmp, *node = pattern.head;
    while(node)
    {
        tmp = node;
        node = node->next;
        g_free(tmp);
    }
    pattern.head = NULL;
    pattern.tail = NULL;
    pattern.count = 0;
    pattern.peak = 0;
    pattern.peak_i = 0;
    pattern.rotate_i = 0;
}

gfloat pattern_log_signal(gfloat signal)
{
    // ARRL scale
    return pow(0.89, (-0.5*signal));
}

void pattern_rotate(GtkWidget *widget, gpointer data)
{
    gint mode = GPOINTER_TO_INT(data);
    gint diff;

    if(pattern.count)
    {

        if(!mode)
        {
            pattern.rotate_i = pattern.peak_i;
        }
        else
        {
            if(abs(mode) == 1)
            {
                diff = abs(mode);
            }
            else
            {
                diff = pattern.count/18.0;
            }

            if(mode < 0)
            {
                if(pattern.rotate_i - diff < 0)
                {
                    pattern.rotate_i = pattern.count + (pattern.rotate_i - diff);
                }
                else
                {
                    pattern.rotate_i -= diff;
                }
            }
            else
            {
                pattern.rotate_i = (pattern.rotate_i + diff) % pattern.count;
            }
        }
        gtk_widget_queue_draw(pattern.image);
    }
}
