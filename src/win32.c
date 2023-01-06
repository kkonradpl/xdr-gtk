#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <gtk/gtk.h>
#include <gdk/gdkwin32.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <dwmapi.h>
#include "ui.h"
#include "conf.h"
#include "win32.h"

static const char css_string[] =
"* {\n"
"    font-family: Sans;\n"
"    font-size: 10pt;\n"
"}\n";

#define WIN32_FONT_FILE ".\\share\\fonts\\TTF\\DejaVuSansMono.ttf"
gint win32_font;


void
win32_init(void)
{
    WSADATA wsaData;
    win32_font = AddFontResourceEx(WIN32_FONT_FILE, FR_PRIVATE, NULL);
    if(WSAStartup(MAKEWORD(2,2), &wsaData))
        ui_dialog(NULL,
                  GTK_MESSAGE_ERROR,
                  "Error", "Unable to initialize Winsock");

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css_string, -1, NULL);
    GdkScreen *screen = gdk_display_get_default_screen(gdk_display_get_default());
    gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void
win32_cleanup(void)
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
win32_realize(GtkWidget *widget,
              gpointer   user_data)
{
    gboolean dark_theme = FALSE;

    g_object_get(gtk_settings_get_default(),
                 "gtk-application-prefer-dark-theme",
                 &dark_theme, NULL);

    if (dark_theme)
        win32_dark_titlebar(widget);
}

void
win32_grab_focus(GtkWindow* window)
{
    HWND handle;
    gint fg;
    gint app;

    if(gtk_window_is_active(window))
        return;

    handle = GDK_WINDOW_HWND(gtk_widget_get_window(ui.window));
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

void
win32_dark_titlebar(GtkWidget *widget)
{
    const DWORD dark_mode = 20;
    const DWORD dark_mode_pre20h1 = 19;
    const BOOL value = TRUE;
    GdkWindow *window = gtk_widget_get_window(widget);

    if (window == NULL)
        return;

    HWND handle = GDK_WINDOW_HWND(window);
    if (!SUCCEEDED(DwmSetWindowAttribute(handle, dark_mode, &value, sizeof(value))))
        DwmSetWindowAttribute(handle, dark_mode_pre20h1, &value, sizeof(value));
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
