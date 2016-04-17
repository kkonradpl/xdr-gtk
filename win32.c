#include <gtk/gtk.h>
#define _WIN32_WINNT 0x0500
#include <gdk/gdkwin32.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "ui.h"
#include "win32.h"

#define WIN32_FONT_FILE "DejaVuSansMono.ttf"
gint win32_font;

void
win32_init()
{
    WSADATA wsaData;
    win32_font = AddFontResourceEx(WIN32_FONT_FILE, FR_PRIVATE, NULL);
    if(WSAStartup(MAKEWORD(2,2), &wsaData))
        ui_dialog(NULL,
                  GTK_MESSAGE_ERROR,
                  "Error", "Unable to initialize Winsock");
}

void
win32_cleanup()
{
    if(win32_font)
        RemoveFontResourceEx(WIN32_FONT_FILE, FR_PRIVATE, NULL);
    WSACleanup();
}

gboolean
win32_uri(GtkWidget *label,
          gchar     *uri,
          gpointer   user_data)
{
    ShellExecute(0, "open", uri, NULL, NULL, 1);
    return TRUE;
}

gint
win32_dialog_workaround(GtkDialog *dialog)
{
    /* Always keep a dialog over the main window */
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.b_ontop)))
        gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    return gtk_dialog_run(dialog);
}

void
win32_grab_focus(GtkWindow* window)
{
    HWND handle;
    gint fg;
    gint app;

    if(gtk_window_is_active(window))
        return;

    handle = gdk_win32_drawable_get_handle(gtk_widget_get_window(ui.window));
    fg = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    app = GetCurrentThreadId();

    if(IsIconic(handle))
    {
        ShowWindow(handle, SW_RESTORE);
    }
    else if(fg != app)
    {
        AttachThreadInput(fg, app, TRUE);
        BringWindowToTop(handle);
        ShowWindow(handle, SW_SHOW);
        AttachThreadInput(fg, app, FALSE);
    }
    else
    {
        gtk_window_present(window);
    }
}

gchar*
strsep(gchar **string,
       gchar  *del)
{
    gchar *start = *string;
    gchar *p = (start ? strpbrk(start, del) : NULL);

    if(!p)
    {
        *string = NULL;
    }
    else
    {
        *p = '\0';
        *string = p + 1;
    }
    return start;
}
