/*
 *  XDR-GTK - user interface for XDR-F1HD tuner with XDR-I2C modification
 *  Copyright (c) 2012-2023  Konrad Kosmatka
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <gtk/gtk.h>
#include <getopt.h>
#include "ui.h"
#include "conf.h"
#include "rdsspy.h"
#include "stationlist.h"
#include "log.h"
#include "resources.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

static const gchar*
get_config_path(gint   argc,
                gchar *argv[])
{
    gchar *path = NULL;
    gint c;
    while((c = getopt(argc, argv, "c:")) != -1)
    {
        switch(c)
        {
        case 'c':
            path = optarg;
            break;
        case '?':
            if(optopt == 'c')
                fprintf(stderr, "No configuration path argument found, using default.\n");
            break;
        }
    }
    return path;
}

gint
main(gint   argc,
     gchar *argv[])
{
    gtk_disable_setlocale();
    gtk_init(&argc, &argv);

    g_resources_register(icons_get_resource());
    gtk_icon_theme_add_resource_path(gtk_icon_theme_get_default(), "/org/xdr-gtk/icons");

#ifdef G_OS_WIN32
    win32_init();
#endif
    conf_init(get_config_path(argc, argv));

    g_object_set(gtk_settings_get_default(),
                 "gtk-application-prefer-dark-theme",
                 conf.dark_theme, NULL);

    ui_init();

    if (conf.rdsspy_auto)
        rdsspy_toggle();

    if (conf.srcp)
        stationlist_init();

    gtk_main();
    log_cleanup();
#ifdef G_OS_WIN32
    win32_cleanup();
#endif
    return 0;
}
