#include <gtk/gtk.h>
#include <string.h>
#include "gui.h"
#include "menu.h"
#include "scan.h"
#include "settings.h"
#include "connection.h"
#include "graph.h"
#include "alignment.h"
#include "pattern.h"
#define VERSION "v0.2a"

GtkWidget* menu_create()
{
    GtkWidget* menu = gtk_menu_new();

    gui.menu_items.connect = gtk_image_menu_item_new_with_label("Connect");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(gui.menu_items.connect), GTK_WIDGET(gtk_image_new_from_stock(GTK_STOCK_CONNECT, GTK_ICON_SIZE_MENU)));    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gui.menu_items.connect);
    g_signal_connect(gui.menu_items.connect, "activate", G_CALLBACK(connection_dialog), NULL);

    gui.menu_items.alignment = gtk_image_menu_item_new_with_label("Alignment");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(gui.menu_items.alignment), GTK_WIDGET(gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_MENU)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gui.menu_items.alignment);
    g_signal_connect(gui.menu_items.alignment, "activate", G_CALLBACK(alignment_dialog), NULL);

    gui.menu_items.scan = gtk_image_menu_item_new_with_label("Spectral scan");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(gui.menu_items.scan), GTK_WIDGET(gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gui.menu_items.scan);
    g_signal_connect(gui.menu_items.scan, "activate", G_CALLBACK(scan_dialog), NULL);

    gui.menu_items.pattern = gtk_image_menu_item_new_with_label("Antenna pattern");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(gui.menu_items.pattern), GTK_WIDGET(gtk_image_new_from_stock(GTK_STOCK_YES, GTK_ICON_SIZE_MENU)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gui.menu_items.pattern);
    g_signal_connect(gui.menu_items.pattern, "activate", G_CALLBACK(pattern_dialog), NULL);

    gui.menu_items.alwaysontop = gtk_check_menu_item_new_with_label("Always on top");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gui.menu_items.alwaysontop);
    g_signal_connect(gui.menu_items.alwaysontop, "toggled", G_CALLBACK(window_on_top), NULL);

    gui.menu_items.settings = gtk_image_menu_item_new_with_label("Settings");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(gui.menu_items.settings), GTK_WIDGET(gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gui.menu_items.settings);
    g_signal_connect(gui.menu_items.settings, "activate", G_CALLBACK(settings_dialog), NULL);

    gui.menu_items.about = gtk_image_menu_item_new_with_label("About...");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(gui.menu_items.about), GTK_WIDGET(gtk_image_new_from_stock(GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gui.menu_items.about);
    g_signal_connect(gui.menu_items.about, "activate", G_CALLBACK(about_dialog), NULL);

    gtk_widget_show_all(menu);
    return menu;
}

gboolean menu_popup(GtkWidget *widget, GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_PRESS)
    {
        gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL, event->button, event->time);
        return TRUE;
    }
    return FALSE;
}

void window_on_top(GtkCheckMenuItem *item)
{
    gtk_window_set_keep_above(GTK_WINDOW(gui.window), gtk_check_menu_item_get_active(item));
}

void about_dialog()
{
    GtkWidget *dialog = gtk_about_dialog_new();
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gui.window));
    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), "XDR-GTK");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "Copyright (C) 2012-2013  Konrad Kosmatka");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), "XDR-F1HD controlling software\nfor Linux, Windows and OS X\n\nhttp://redakcja.radiopolska.pl/konrad/");

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
