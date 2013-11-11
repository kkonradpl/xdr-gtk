#include <gtk/gtk.h>
#include "menu.h"
#include "gui.h"
#include "connection.h"
#include "alignment.h"
#include "graph.h"

void alignment_dialog()
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Antenna input alignment", GTK_WINDOW(gui.window), GTK_DIALOG_DESTROY_WITH_PARENT, NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

    GtkObject* adj = gtk_adjustment_new(daa, 0.0, 127.0, 1.0, 1.0, 0);
    GtkWidget *d_scale = gtk_hscale_new(GTK_ADJUSTMENT(adj));
    gtk_scale_set_digits(GTK_SCALE(d_scale), 0);
    gtk_box_pack_start(GTK_BOX(content), d_scale, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(adj), "value-changed", G_CALLBACK(alignment_update), NULL);
    gtk_widget_set_size_request(d_scale, 384, -1);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(content), sep, TRUE, TRUE, 0);

    GtkWidget *buttons_box = gtk_hbox_new(TRUE, 5);
    gtk_container_add(GTK_CONTAINER(content), buttons_box);

    GtkWidget *b_reset_rssi = gtk_button_new_with_label("Reset signal graph");
    gtk_box_pack_start(GTK_BOX(buttons_box), b_reset_rssi, TRUE, TRUE, 0);
    gtk_widget_set_can_focus(GTK_WIDGET(b_reset_rssi), FALSE);
    g_signal_connect(b_reset_rssi, "clicked", G_CALLBACK(graph_clear), NULL);

    GtkWidget *b_close = gtk_button_new_with_label("Close");
    gtk_button_set_image(GTK_BUTTON(b_close), gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start(GTK_BOX(buttons_box), b_close, TRUE, TRUE, 0);
    gtk_widget_set_can_focus(GTK_WIDGET(b_close), FALSE);
    g_signal_connect_swapped(b_close, "clicked", G_CALLBACK(gtk_widget_destroy), dialog);
    g_signal_connect_swapped(dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);

    gtk_widget_show_all(dialog);
#ifdef G_OS_WIN32
    // workaround for windows to keep dialog over main window
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gui.menu_items.alwaysontop)))
    {
        gtk_window_set_keep_above(GTK_WINDOW(gui.window), FALSE);
    }
    gtk_dialog_run(GTK_DIALOG(dialog));
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gui.menu_items.alwaysontop)))
    {
        gtk_window_set_keep_above(GTK_WINDOW(gui.window), TRUE);
    }
#else
    gtk_dialog_run(GTK_DIALOG(dialog));
#endif
}

void alignment_update(GtkAdjustment *get, gpointer nothing)
{
    gchar command[5];
    daa = get->value;
    g_snprintf(command, sizeof(command), "V%d", daa);
    xdr_write(command);
}
