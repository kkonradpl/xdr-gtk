#include <gtk/gtk.h>
#include "librds.h"
#include "ui.h"

const gchar*
rds_utils_pty_to_string(gboolean rbds, gint number)
{
    static const gchar* const pty_list[][32] =
    {
        { "None", "News", "Affairs", "Info", "Sport", "Educate", "Drama", "Culture", "Science", "Varied", "Pop M", "Rock M", "Easy M", "Light M", "Classics", "Other M", "Weather", "Finance", "Children", "Social", "Religion", "Phone In", "Travel", "Leisure", "Jazz", "Country", "Nation M", "Oldies", "Folk M", "Document", "TEST", "Alarm !" },
        { "None", "News", "Inform", "Sports", "Talk", "Rock", "Cls Rock", "Adlt Hit", "Soft Rck", "Top 40", "Country", "Oldies", "Soft", "Nostalga", "Jazz", "Classicl", "R & B", "Soft R&B", "Language", "Rel Musc", "Rel Talk", "Persnlty", "Public", "College", "N/A", "N/A", "N/A", "N/A", "N/A", "Weather", "Test", "ALERT!" }
    };

    return (number >= 0 && number < 32) ? pty_list[(gint)rbds][number] : "Invalid";
}


gchar*
rds_utils_text_markup(const gchar                 *text,
                      const librds_string_error_t *errors,
                      gboolean                     progressive)
{
    GString *string = g_string_new(NULL);

    g_string_append_printf(string,
                           "<span alpha=\"" UI_ALPHA_INSENSITIVE "%%\">%c</span>",
                           progressive ? '(' : '[');

    for (gint i = 0; i < strlen(text); i++)
    {
        if (errors[i] == LIBRDS_STRING_ERROR_UNCORRECTABLE)
        {
            g_string_append(string, " ");
            continue;
        }

        g_autofree gchar* letter = g_markup_printf_escaped("%c", text[i]);
        const gint max_alpha = 70;
        const gint alpha_range = 50;
        gint alpha = errors[i] * (alpha_range / (LIBRDS_STRING_ERROR_LARGEST + 1));

        if (alpha)
        {
            g_string_append_printf(string, "<span alpha=\"%d%%\">%s</span>", max_alpha - alpha, letter);
        }
        else
        {
            g_string_append(string, letter);
        }
    }

    g_string_append_printf(string,
                           "<span alpha=\"" UI_ALPHA_INSENSITIVE "%%\">%c</span>",
                           progressive ? ')' : ']');

    gchar* markup = g_string_free(string, FALSE);
    return markup;
}
