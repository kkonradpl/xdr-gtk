#include <gtk/gtk.h>
#include "menu.h"
#include "gui.h"
#include "connection.h"
#include "alignment.h"

void alignment_dialog()
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Antenna input alignment", GTK_WINDOW(gui.window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    GtkWidget *d_box = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(content), d_box);

    GtkObject* adj = gtk_adjustment_new(daa, 0.0, 127.0, 1.0, 1.0, 0);
    GtkWidget *d_scale = gtk_hscale_new(GTK_ADJUSTMENT(adj));
    gtk_scale_set_digits(GTK_SCALE(d_scale), 0);
    gtk_box_pack_start(GTK_BOX(d_box), d_scale, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(adj), "value-changed", G_CALLBACK(alignment_update), NULL);
    gtk_widget_set_size_request(d_scale, 384, -1);
    gtk_scale_set_value_pos(GTK_SCALE(d_scale), GTK_POS_RIGHT);
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
    gtk_widget_destroy(dialog);
}

void alignment_update(GtkAdjustment *get, gpointer nothing)
{
    gchar command[5];
    daa = get->value;
    g_snprintf(command, sizeof(command), "V%d", daa);
    xdr_write(command);
}
