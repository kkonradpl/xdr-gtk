#include <gtk/gtk.h>
#ifdef G_OS_WIN32
#define _WIN32_WINNT 0x0500
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "gui.h"
#include "win32.h"

gint win32_font;

void win32_init()
{
    win32_font = AddFontResourceEx(WIN32_FONT_FILE, FR_PRIVATE, NULL);
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData))
    {
        dialog_error("Unable to initialize Winsock");
    }
}

void win32_cleanup()
{
    if(win32_font)
    {
        RemoveFontResourceEx(WIN32_FONT_FILE, FR_PRIVATE, NULL);
    }
    WSACleanup();
}

char *strsep(char **sp, char *sep)
{
    char *p, *s;
    if (sp == NULL || *sp == NULL || **sp == '\0') return(NULL);
    s = *sp;
    p = s + strcspn(s, sep);
    if (*p != '\0') *p++ = '\0';
    *sp = p;
    return(s);
}

gboolean win32_uri(GtkAboutDialog *label, gchar *uri, gpointer user_data)
{
    ShellExecute(0, "open", uri, NULL, NULL, 1);
    return TRUE;
}

gint win32_dialog_workaround(GtkDialog *dialog)
{
    // keep dialog over main window
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.b_ontop)))
    {
        gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    }
    return gtk_dialog_run(dialog);
}
#endif
