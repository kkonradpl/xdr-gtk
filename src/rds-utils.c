#include <gtk/gtk.h>
#include "rdsparser.h"
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
rds_utils_text(const rdsparser_string_t *rds_string)
{
    GString *string = g_string_new(NULL);
    const wchar_t *content = rdsparser_string_get_content(rds_string);
    uint8_t len = rdsparser_string_get_length(rds_string);

    for (gint i = 0; i < len; i++)
    {
        gchar buffer[10];
        gunichar unichar = content[i];
        buffer[g_unichar_to_utf8(unichar, buffer)] = '\0';

        g_string_append(string, buffer);
    }

    return g_string_free(string, FALSE);
}

gchar*
rds_utils_text_markup(const rdsparser_string_t *rds_string,
                      gboolean                  progressive)
{
    GString *string = g_string_new(NULL);
    const wchar_t *content = rdsparser_string_get_content(rds_string);
    const uint8_t *errors = rdsparser_string_get_errors(rds_string);
    uint8_t len = rdsparser_string_get_length(rds_string);

    g_string_append_printf(string,
                           "<span alpha=\"" UI_ALPHA_INSENSITIVE "%%\">%c</span>",
                           progressive ? '(' : '[');

    for (gint i = 0; i < len; i++)
    {
        gchar buffer[10];
        gunichar unichar = content[i];
        buffer[g_unichar_to_utf8(unichar, buffer)] = '\0';

        gchar* buffer_escaped = g_markup_printf_escaped("%s", buffer);
        const gint max_alpha = 70;
        const gint alpha_range = 50;
        gint alpha = errors[i] * (alpha_range / (RDSPARSER_STRING_ERROR_LARGEST + 1));

        if (alpha)
        {
            g_string_append_printf(string, "<span alpha=\"%d%%\">%s</span>", max_alpha - alpha, buffer_escaped);
        }
        else
        {
            g_string_append(string, buffer_escaped);
        }

        g_free(buffer_escaped);
    }

    g_string_append_printf(string,
                           "<span alpha=\"" UI_ALPHA_INSENSITIVE "%%\">%c</span>",
                           progressive ? ')' : ']');

    return g_string_free(string, FALSE);
}
