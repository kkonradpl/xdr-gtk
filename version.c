#include <gtk/gtk.h>
#include "gui.h"
#include "version.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

void version_dialog()
{
    GtkWidget *dialog;
    dialog = gtk_about_dialog_new();
    gtk_window_set_icon_name(GTK_WINDOW(dialog), "xdr-gtk-about");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gui.window));
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), APP_NAME);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), APP_VERSION);
    gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog), APP_ICON);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "Copyright (C) 2012-2014  Konrad Kosmatka");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), "XDR-F1HD controlling software\nfor Linux, Windows and OS X");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "http://fmdx.pl/xdr-gtk/");

#ifdef G_OS_WIN32
    g_signal_connect(dialog, "activate-link", G_CALLBACK(win32_uri), NULL);
    win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    gtk_dialog_run(GTK_DIALOG(dialog));
#endif

    gtk_widget_destroy(dialog);
}
