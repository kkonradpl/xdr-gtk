#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include "ui.h"
#include "conf.h"
#include "pattern.h"
#include "tuner.h"

#define PATTERN_OFFSET  30
#define PATTERN_FONT_SIZE_TITLE 16
#define PATTERN_FONT_SIZE_FREQ  18
#define PATTERN_FONT_SIZE_SCALE 11
#define PATTERN_ARCS 36

#define PATTERN_COLOR_NONE  0
#define PATTERN_COLOR_RED   1
#define PATTERN_COLOR_GREEN 2
#define PATTERN_COLOR_BLUE  3

#define PATTERN_ALPHA_PLOT_FG  0.95
#define PATTERN_ALPHA_PLOT_BG  0.175

#define DEG2RAD(DEG) ((DEG)*((M_PI)/(180.0)))

static const gint signal_scale_points[] = {-10, -20, -30, -40, 0};

typedef struct pattern_sig
{
    gfloat sig;
    struct pattern_sig *next;
} pattern_sig_t;

typedef struct pattern
{
    GtkWidget *window;
    volatile gboolean active;

    gint freq;
    gint count;
    gfloat peak;
    gint peak_i;
    gint rotate_i;

    GtkWidget *b_start;
    GtkWidget *b_load;
    GtkWidget *b_save;
    GtkWidget *b_save_img;
    GtkWidget *b_close;

    GtkWidget *l_color;
    GtkWidget *c_color;
    GtkWidget *l_size;
    GtkWidget *s_size;
    GtkWidget *l_title;
    GtkWidget *e_title;

    GtkWidget *plot;

    GtkWidget *x_reverse;
    GtkWidget *b_rotate_ll;
    GtkWidget *b_rotate_l;
    GtkWidget *b_rotate_peak;
    GtkWidget *b_rotate_r;
    GtkWidget *b_rotate_rr;
    GtkWidget *x_inv;
    GtkWidget *x_fill;
    GtkWidget *x_avg;

    pattern_sig_t *head;
    pattern_sig_t *tail;
} pattern_t;

pattern_t pattern = {0};

static void pattern_init(gint);
static void pattern_destroy(GtkWidget*, gpointer);
static gboolean pattern_draw(GtkWidget*, GdkEventExpose*, gpointer);
static void pattern_push_sample(gfloat);
static void pattern_start(GtkWidget*, gpointer);
static void pattern_load(GtkWidget*, gpointer);
static void pattern_save(GtkWidget*, gpointer);
static void pattern_save_img(GtkWidget*, gpointer);
static void pattern_resize();
static void pattern_clear();
static void pattern_rotate(GtkWidget*, gpointer);
static gfloat pattern_log_signal(gfloat);

void
pattern_dialog()
{
    if(pattern.window)
    {
        gtk_window_present(GTK_WINDOW(pattern.window));
        return;
    }
    pattern_init(0);
    pattern.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(pattern.window), "Antenna pattern");
    gtk_window_set_icon_name(GTK_WINDOW(pattern.window), "xdr-gtk-pattern");
    gtk_window_set_resizable(GTK_WINDOW(pattern.window), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(pattern.window), 5);
    gtk_window_set_transient_for(GTK_WINDOW(pattern.window), GTK_WINDOW(ui.window));

    GtkWidget *content = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(pattern.window), content);

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
    g_signal_connect_swapped(pattern.b_close, "clicked", G_CALLBACK(pattern_destroy), pattern.window);
    gtk_box_pack_start(GTK_BOX(menu_box), pattern.b_close, TRUE, TRUE, 0);

    GtkWidget *settings_box = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(content), settings_box);

    pattern.l_color = gtk_label_new("Color:");
    gtk_box_pack_start(GTK_BOX(settings_box), pattern.l_color, FALSE, FALSE, 0);
    pattern.c_color = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(pattern.c_color), "b&w");
    gtk_combo_box_append_text(GTK_COMBO_BOX(pattern.c_color), "red");
    gtk_combo_box_append_text(GTK_COMBO_BOX(pattern.c_color), "green");
    gtk_combo_box_append_text(GTK_COMBO_BOX(pattern.c_color), "blue");
    gtk_combo_box_set_active(GTK_COMBO_BOX(pattern.c_color), conf.pattern_color);
    gtk_box_pack_start(GTK_BOX(settings_box), pattern.c_color, FALSE, FALSE, 0);

    pattern.l_size = gtk_label_new("Size:");
    gtk_box_pack_start(GTK_BOX(settings_box), pattern.l_size, FALSE, FALSE, 0);
    pattern.s_size = gtk_spin_button_new((GtkAdjustment*)gtk_adjustment_new(conf.pattern_size, 450.0, 1000.0, 10.0, 25.0, 0.0), 0, 0);
    gtk_box_pack_start(GTK_BOX(settings_box), pattern.s_size, FALSE, FALSE, 0);

    pattern.l_title = gtk_label_new("Title:");
    gtk_box_pack_start(GTK_BOX(settings_box), pattern.l_title, FALSE, FALSE, 0);
    pattern.e_title = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(pattern.e_title), 100);
    gtk_box_pack_start(GTK_BOX(settings_box), pattern.e_title, TRUE, TRUE, 0);

    pattern.plot = gtk_drawing_area_new();
    gtk_widget_add_events(pattern.plot, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    pattern_resize();
    gtk_box_pack_start(GTK_BOX(content), pattern.plot, TRUE, TRUE, 0);

    GtkWidget *plot_settings_box = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(content), plot_settings_box);

    pattern.x_reverse = gtk_check_button_new_with_label("Reverse");
    gtk_box_pack_start(GTK_BOX(plot_settings_box), pattern.x_reverse, FALSE, FALSE, 0);

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

    pattern.x_inv = gtk_check_button_new_with_label("Inv");
    gtk_box_pack_start(GTK_BOX(plot_settings_box), pattern.x_inv, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pattern.x_inv), conf.pattern_inv);

    pattern.x_fill = gtk_check_button_new_with_label("Fill");
    gtk_box_pack_start(GTK_BOX(plot_settings_box), pattern.x_fill, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pattern.x_fill), conf.pattern_fill);

    pattern.x_avg = gtk_check_button_new_with_label("Avg");
    gtk_box_pack_start(GTK_BOX(plot_settings_box), pattern.x_avg, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pattern.x_avg), conf.pattern_avg);

    g_signal_connect(pattern.s_size, "value-changed", G_CALLBACK(pattern_resize), NULL);
    g_signal_connect_swapped(pattern.c_color, "changed", G_CALLBACK(gtk_widget_queue_draw), pattern.plot);
    g_signal_connect_swapped(pattern.e_title, "changed", G_CALLBACK(gtk_widget_queue_draw), pattern.plot);
    g_signal_connect_swapped(pattern.x_reverse, "toggled", G_CALLBACK(gtk_widget_queue_draw), pattern.plot);
    g_signal_connect_swapped(pattern.x_inv, "toggled", G_CALLBACK(gtk_widget_queue_draw), pattern.plot);
    g_signal_connect_swapped(pattern.x_fill, "toggled", G_CALLBACK(gtk_widget_queue_draw), pattern.plot);
    g_signal_connect_swapped(pattern.x_avg, "toggled", G_CALLBACK(gtk_widget_queue_draw), pattern.plot);
    g_signal_connect(pattern.plot, "expose-event", G_CALLBACK(pattern_draw), NULL);
    g_signal_connect(pattern.window, "destroy", G_CALLBACK(pattern_destroy), NULL);

    gtk_widget_show_all(pattern.window);
    gtk_window_set_transient_for(GTK_WINDOW(pattern.window), NULL);
}

static void
pattern_init(gint freq)
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

static void
pattern_destroy(GtkWidget *widget,
                gpointer   user_data)
{
    pattern_clear();
    conf.pattern_color = gtk_combo_box_get_active(GTK_COMBO_BOX(pattern.c_color));
    conf.pattern_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pattern.s_size));
    conf.pattern_inv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.x_inv));
    conf.pattern_fill = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.x_fill));
    conf.pattern_avg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.x_avg));
    gtk_widget_destroy(GTK_WIDGET(widget));
    pattern.window = NULL;
}

static gboolean
pattern_draw(GtkWidget      *widget,
             GdkEventExpose *event,
             gpointer        data)
{
    static const double dash_full[] = {1.0, 2.0};
    static gint dash_len = sizeof(dash_full)/sizeof(dash_full[0]);
    gint size, radius, i, j;
    gdouble x, y, l, a, sample;
    gchar text[20];
    const gchar *title;
    cairo_t *cr;
    cairo_text_extents_t extents;
    pattern_sig_t *node;
    gfloat prev, next;
    gboolean rev = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.x_reverse));
    gboolean inv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.x_inv));
    gboolean avg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.x_avg));
    gint color = gtk_combo_box_get_active(GTK_COMBO_BOX(pattern.c_color));

    size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pattern.s_size));
    radius = size/2 - PATTERN_OFFSET;
    title = gtk_entry_get_text(GTK_ENTRY(pattern.e_title));

    cr = gdk_cairo_create(widget->window);
    cairo_set_line_width(cr, 1.0);
    cairo_select_font_face(cr, "DejaVu Sans Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    /* clear the canvas */
    cairo_set_source_rgb(cr, (inv ? 0.0 : 1.0), (inv ? 0.0 : 1.0), (inv ? 0.0 : 1.0));
    cairo_paint(cr);
    cairo_set_source_rgb(cr, (inv ? 1.0 : 0.0), (inv ? 1.0 : 0.0), (inv ? 1.0 : 0.0));

    /* add title label */
    cairo_set_font_size(cr, PATTERN_FONT_SIZE_TITLE);
    cairo_text_extents(cr, title, &extents);
    cairo_move_to(cr, (size-extents.width)/2.0, PATTERN_FONT_SIZE_TITLE+2.0);
    cairo_show_text(cr, title);

    /* add frequency label */
    if(pattern.freq)
    {
        cairo_set_font_size(cr, PATTERN_FONT_SIZE_FREQ);
        g_snprintf(text, sizeof(text), "%.3f MHz", pattern.freq/1000.0);
        cairo_text_extents(cr, text, &extents);
        cairo_move_to(cr, size-extents.width-6.0, size-4.0);
        cairo_show_text(cr, text);
    }
    cairo_stroke(cr);

    /* add angle labels */
    cairo_set_font_size(cr, PATTERN_FONT_SIZE_SCALE);
    cairo_set_source_rgb(cr, (inv ? 0.75 : 0.25), (inv ? 0.75 : 0.25), (inv ? 0.75 : 0.25));
    for(i=3; i<PATTERN_ARCS; i+=3)
    {
        cairo_arc(cr, radius+PATTERN_OFFSET, radius+PATTERN_OFFSET, radius+14.5, (i-9)*(2*M_PI/PATTERN_ARCS), (i-9)*(2*M_PI/PATTERN_ARCS));
        cairo_get_current_point(cr, &x, &y);
        g_snprintf(text, sizeof(text), "%d°", i*(360/PATTERN_ARCS));
        cairo_text_extents(cr, text, &extents);
        x -= extents.width/2.0 + extents.x_bearing;
        y -= extents.height/2.0 + extents.y_bearing;
        cairo_move_to(cr, x, y);
        cairo_show_text (cr, text);
        cairo_stroke(cr);
    }

    /* set color and draw polar coordinates */
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_arc(cr, radius+PATTERN_OFFSET, radius+PATTERN_OFFSET, radius, 0, 2*M_PI);
    cairo_stroke(cr);

    /* draw grid */
    for(i=0; i<360; i+=10)
    {
        for(j=0; j>-30; j-=1)
        {
            if(i%30 != 0 && j%2 != 0)
                continue;
            if(j%10 == 0)
                continue;

            l = radius*pattern_log_signal(j);
            x = PATTERN_OFFSET+radius+sin(DEG2RAD(i))*l;
            y = PATTERN_OFFSET+radius+cos(DEG2RAD(i))*l;
            cairo_move_to(cr, x, y);
            cairo_line_to(cr, x+1.0, y+1.0);
            cairo_stroke(cr);
        }
    }

    /* draw scale */
    cairo_save(cr);
    cairo_set_dash(cr, dash_full, dash_len, 0);
    for(i=0; signal_scale_points[i]; i++)
    {
        cairo_arc(cr, radius+PATTERN_OFFSET, radius+PATTERN_OFFSET, radius*pattern_log_signal(signal_scale_points[i]), -0.5*M_PI, 1.5*M_PI);
        cairo_stroke_preserve(cr);
        g_snprintf(text, sizeof(text), "%d", signal_scale_points[i]);
        cairo_show_text(cr, text);
        cairo_stroke(cr);
    }
    cairo_restore(cr);

    /* plot the pattern */
    switch(color)
    {
    case PATTERN_COLOR_RED:
        if(inv)
            cairo_set_source_rgba(cr, 0.9, 0.6, 0.6, PATTERN_ALPHA_PLOT_FG);
        else
            cairo_set_source_rgba(cr, 0.9, 0.0, 0.0, PATTERN_ALPHA_PLOT_FG);
        break;
    case PATTERN_COLOR_GREEN:
        if(inv)
            cairo_set_source_rgba(cr, 0.6, 0.9, 0.6, PATTERN_ALPHA_PLOT_FG);
        else
            cairo_set_source_rgba(cr, 0.0, 0.9, 0.0, PATTERN_ALPHA_PLOT_FG);
        break;
    case PATTERN_COLOR_BLUE:
        if(inv)
            cairo_set_source_rgba(cr, 0.6, 0.6, 0.9, PATTERN_ALPHA_PLOT_FG);
        else
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.9, PATTERN_ALPHA_PLOT_FG);
        break;
    default:
        if(inv)
            cairo_set_source_rgba(cr, 0.9, 0.9, 0.9, PATTERN_ALPHA_PLOT_FG);
        else
            cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, PATTERN_ALPHA_PLOT_FG);
        break;
    }

    cairo_set_line_width(cr, 1.5);
    if(pattern.head)
        prev = pattern.tail->sig;
    i = 0;
    for(node=pattern.head; node; node=node->next)
    {
        if(avg)
        {
            next = (node->next ? node->next->sig : pattern.head->sig);
            sample = (prev + node->sig + next) / 3.0;
            prev = node->sig;
        }
        else
        {
            sample = node->sig;
        }

        l = radius * pattern_log_signal(sample - pattern.peak);
        a = i/(gdouble)pattern.count * 2*M_PI - pattern.rotate_i*2*M_PI/(gdouble)pattern.count;
        a = (rev ? M_PI - a : M_PI + a);
        x = PATTERN_OFFSET + radius + sin(a)*l;
        y = PATTERN_OFFSET + radius + cos(a)*l;
        cairo_line_to(cr, x, y);
        i++;
    }

    /* close the plot when finished */
    if(!pattern.active)
    {
        cairo_close_path(cr);
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.x_fill)))
        {
            cairo_stroke_preserve(cr);
            switch(color)
            {
            case PATTERN_COLOR_RED:
                if(inv)
                    cairo_set_source_rgba(cr, 0.9, 0.6, 0.6, PATTERN_ALPHA_PLOT_BG);
                else
                    cairo_set_source_rgba(cr, 0.9, 0.0, 0.0, PATTERN_ALPHA_PLOT_BG);
                break;
            case PATTERN_COLOR_GREEN:
                if(inv)
                    cairo_set_source_rgba(cr, 0.6, 0.9, 0.6, PATTERN_ALPHA_PLOT_BG);
                else
                    cairo_set_source_rgba(cr, 0.0, 0.9, 0.0, PATTERN_ALPHA_PLOT_BG);
                break;
            case PATTERN_COLOR_BLUE:
                if(inv)
                    cairo_set_source_rgba(cr, 0.6, 0.6, 0.9, PATTERN_ALPHA_PLOT_BG);
                else
                    cairo_set_source_rgba(cr, 0.0, 0.0, 0.9, PATTERN_ALPHA_PLOT_BG);
                break;
            default:
                if(inv)
                    cairo_set_source_rgba(cr, 0.9, 0.9, 0.9, PATTERN_ALPHA_PLOT_BG);
                else
                    cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, PATTERN_ALPHA_PLOT_BG);
                break;
            }
            cairo_fill(cr);
        }
    }
    cairo_stroke(cr);
    cairo_destroy(cr);
    return FALSE;
}

void
pattern_push(gfloat sig)
{
    if(!pattern.active)
        return;

    pattern_push_sample(sig);
    gtk_widget_queue_draw(pattern.plot);
}

static void
pattern_push_sample(gfloat sig)
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
}

static void
pattern_start(GtkWidget *widget,
              gpointer   data)
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

    gtk_widget_queue_draw(pattern.plot);
    pattern.active = !pattern.active;
}

static void
pattern_load(GtkWidget *widget,
             gpointer   data)
{
    gchar *filename = NULL;
    FILE *f;
    gfloat sample;
    gchar buff[100];
    gint freq;
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new("Load file",
                                         GTK_WINDOW(pattern.window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_widget_destroy(dialog);

    if(!filename)
        return;

    if((f = g_fopen(filename, "r")))
    {
        fgets(buff, sizeof(buff), f);
        if(!sscanf(buff, "%d", &freq))
        {
            ui_dialog(pattern.window,
                      GTK_MESSAGE_ERROR,
                      "Antenna pattern",
                      "Invalid file format");
        }
        else
        {
            pattern_clear();
            pattern_init(freq);

            fgets(buff, sizeof(buff), f);
            if(strlen(buff))
            {
                // remove new line char
                buff[strlen(buff)-1] = 0;
            }
            gtk_entry_set_text(GTK_ENTRY(pattern.e_title), buff);


            while(fscanf(f, "%f", &sample) && !feof(f))
                pattern_push_sample(sample);
            gtk_widget_queue_draw(pattern.plot);
        }
        fclose(f);
    }
    else
    {
        ui_dialog(pattern.window,
                  GTK_MESSAGE_ERROR,
                  "Antenna pattern",
                  "Unable to open the file");
    }
    g_free(filename);
}

static void
pattern_save(GtkWidget *widget,
             gpointer   data)
{
    gchar *filename = NULL;
    FILE *f;
    gchar buffer[100];
    const gchar *title;
    pattern_sig_t *node;
    GtkWidget *dialog;
    GtkFileFilter *filter;

    dialog  = gtk_file_chooser_dialog_new("Save signal samples",
                                          GTK_WINDOW(pattern.window),
                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Text file");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_widget_destroy(dialog);

    if(!filename)
        return;

    if((f = g_fopen(filename, "w")))
    {
        g_snprintf(buffer, sizeof(buffer), "%d\n", tuner.freq);
        fwrite(buffer, 1, strlen(buffer), f);

        title = gtk_entry_get_text(GTK_ENTRY(pattern.e_title));
        g_snprintf(buffer, sizeof(buffer), "%s\n", title);
        fwrite(buffer, 1, strlen(buffer), f);

        for(node=pattern.head; node; node=node->next)
        {
            g_snprintf(buffer, sizeof(buffer), "%.2f\n", node->sig);
            fwrite(buffer, 1, strlen(buffer), f);
        }
        fclose(f);
    }
    else
    {
        ui_dialog(pattern.window,
                  GTK_MESSAGE_ERROR,
                  "Antenna pattern",
                  "Unable to save the file");
    }
    g_free(filename);
}

static void
pattern_save_img(GtkWidget *widget,
                 gpointer   data)
{
    gchar *filename = NULL;
    GtkWidget *dialog;
    GtkFileFilter *filter;
    gchar *ext;
    GdkPixmap *pixmap;
    GdkPixbuf *pixbuf;

    dialog = gtk_file_chooser_dialog_new("Save pattern plot",
                                         GTK_WINDOW(pattern.window),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                         NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "PNG image");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    gtk_widget_destroy(dialog);

    if(!filename)
        return;

    ext = strrchr(filename, '.');
    if(!ext || g_ascii_strcasecmp(ext, ".png"))
    {
        filename = (char*)g_realloc(filename, strlen(filename) + strlen(".png") + 1);
        strcat(filename, ".png");
    }

    pixmap = gtk_widget_get_snapshot(pattern.plot, NULL);
    pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 0, 0, 0, 0, -1, -1);
    if(!gdk_pixbuf_save(pixbuf, filename, "png", NULL, NULL))
    {
        ui_dialog(pattern.window,
                  GTK_MESSAGE_ERROR,
                  "Antenna pattern",
                  "Unable to save the image");
    }
    g_object_unref(G_OBJECT(pixmap));
    g_object_unref(G_OBJECT(pixbuf));
    g_free(filename);
}

static void
pattern_resize()
{
    gint s = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pattern.s_size));
    gtk_widget_set_size_request(pattern.plot, s, s);
}

static void
pattern_clear()
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

static void
pattern_rotate(GtkWidget *widget,
               gpointer   data)
{
    gboolean rev = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pattern.x_reverse));
    gint mode = GPOINTER_TO_INT(data);
    gint diff;

    if(pattern.count)
    {
        if(!mode)
            pattern.rotate_i = pattern.peak_i;
        else
        {
            if(abs(mode) == 1)
                diff = abs(mode);
            else
                diff = pattern.count/18.0;

            if((mode < 0 && !rev) ||
               (mode > 0 && rev))
            {
                if(pattern.rotate_i - diff < 0)
                    pattern.rotate_i = pattern.count + (pattern.rotate_i - diff);
                else
                    pattern.rotate_i -= diff;
            }
            else
            {
                pattern.rotate_i = (pattern.rotate_i + diff) % pattern.count;
            }
        }
        gtk_widget_queue_draw(pattern.plot);
    }
}

static inline gfloat
pattern_log_signal(gfloat signal)
{
    /* ARRL scale */
    return pow(0.89, (-0.5*signal));
}
