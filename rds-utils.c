#include <gtk/gtk.h>

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
