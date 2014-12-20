#include <gtk/gtk.h>
#include <math.h>
#include "gui-audio.h"
#include "gui-tuner.h"

static const gchar* const icons_volume[] =
{
    "audio-volume-muted",
    "audio-volume-high",
    "audio-volume-low",
    "audio-volume-medium",
    NULL
};

static const gchar* const icon_squelch_off[] =
{
    "xdr-gtk-squelch-off",
    NULL
};

static const gchar* const icon_squelch_st[] =
{
    "xdr-gtk-squelch-st",
    NULL
};

static const gchar* const icon_squelch_on[] =
{
    "xdr-gtk-squelch-on",
    NULL
};

GtkWidget* volume_init()
{
    GtkWidget *scale, *area, *label;
    scale = gtk_scale_button_new(GTK_ICON_SIZE_MENU, 0, 100, 10, (const gchar **)icons_volume);
    gtk_widget_set_can_focus(GTK_WIDGET(scale), FALSE);
    gtk_widget_set_has_tooltip(scale, TRUE);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(scale), 100.0);
    label = gtk_label_new(NULL);
    volume_text(GTK_SCALE_BUTTON(scale), 100.0, (gpointer)label);
    area = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_scale_button_get_popup(GTK_SCALE_BUTTON(scale))))));
    gtk_box_pack_start(GTK_BOX(area), label, TRUE, TRUE,  3);
    g_signal_connect(scale, "value-changed", G_CALLBACK(tuner_set_volume), NULL);
    g_signal_connect(scale, "query-tooltip", G_CALLBACK(volume_tooltip), NULL);
    g_signal_connect(scale, "button-press-event", G_CALLBACK(volume_click), NULL);
    g_signal_connect(scale, "value-changed", G_CALLBACK(volume_text), label);
    return scale;
}

gboolean volume_tooltip(GtkWidget *button, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data)
{
    GtkScaleButton *scale_button = GTK_SCALE_BUTTON(button);
    GtkAdjustment *adjustment = gtk_scale_button_get_adjustment(scale_button);
    gchar *str;
    gint percent;

    percent = (gint)(100. * gtk_scale_button_get_value(scale_button) / (gtk_adjustment_get_upper (adjustment) - gtk_adjustment_get_lower (adjustment)) + .5);
    str = g_strdup_printf("Volume: %d%%", percent);
    gtk_tooltip_set_text(tooltip, str);
    g_free(str);
    return TRUE;
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
            gtk_scale_button_set_value(GTK_SCALE_BUTTON(widget), 100.0);
        }
        return TRUE;
    }
    return FALSE;
}

void volume_text(GtkScaleButton *widget, gdouble value, gpointer label)
{
    char *str;
    str = g_strdup_printf("<span size=\"small\">%d</span>", (gint)round(value));
    gtk_label_set_markup(GTK_LABEL(label), str);
    g_free(str);
}


GtkWidget* squelch_init()
{
    GtkWidget *scale, *area, *label;
    scale = gtk_scale_button_new(GTK_ICON_SIZE_MENU, -1, 100, 0.5, (const gchar **)icon_squelch_off);
    gtk_widget_set_can_focus(GTK_WIDGET(scale), FALSE);
    gtk_widget_set_has_tooltip(scale, TRUE);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(scale), 0.0);
    label = gtk_label_new(NULL);
    squelch_text(GTK_SCALE_BUTTON(scale), 0.0, (gpointer)label);
    area = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_scale_button_get_popup(GTK_SCALE_BUTTON(scale))))));
    gtk_box_pack_start(GTK_BOX(area), label, TRUE, TRUE,  3);
    g_signal_connect(scale, "value-changed", G_CALLBACK(squelch_icon), NULL);
    g_signal_connect(scale, "value-changed", G_CALLBACK(tuner_set_squelch), NULL);
    g_signal_connect(scale, "query-tooltip", G_CALLBACK(squelch_tooltip), NULL);
    g_signal_connect(scale, "button-press-event", G_CALLBACK(squelch_click), NULL);
    g_signal_connect(scale, "value-changed", G_CALLBACK(squelch_text), label);
    return scale;
}

gboolean squelch_tooltip(GtkWidget *button, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data)
{
    char *str;
    gint value = (int)round(gtk_scale_button_get_value(GTK_SCALE_BUTTON(button)));

    if(value >= 0)
    {
        str = g_strdup_printf("Squelch: %d dBf", value);
        gtk_tooltip_set_text(tooltip, str);
        g_free(str);
    }
    else
    {
        gtk_tooltip_set_text(tooltip, "Squelch: stereo");
    }

    return TRUE;
}

gboolean squelch_click(GtkWidget *widget, GdkEventButton *event)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) // right click
    {
        gtk_scale_button_set_value(GTK_SCALE_BUTTON(widget), 0.0);
        return TRUE;
    }
    return FALSE;
}

void squelch_text(GtkScaleButton *widget, gdouble value, gpointer label)
{
    char *str;
    gint v = (gint)round(value);

    if(v>=0)
    {
        str = g_strdup_printf("<span size=\"small\">%d</span>", v);
        gtk_label_set_markup(GTK_LABEL(label), str);
        g_free(str);
    }
    else
    {
        gtk_label_set_markup(GTK_LABEL(label), "<span size=\"small\">ST</span>");
    }
}

void squelch_icon(GtkScaleButton *button, gdouble value, gpointer user_data)
{
    gint v = (gint)round(value);
    if(v < 0)
    {
        gtk_scale_button_set_icons(button, (const gchar **)icon_squelch_st);
    }
    else if(v == 0)
    {
        gtk_scale_button_set_icons(button, (const gchar **)icon_squelch_off);
    }
    else
    {
        gtk_scale_button_set_icons(button, (const gchar **)icon_squelch_on);
    }
}
