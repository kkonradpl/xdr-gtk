#ifndef XDR_CONNECTION_READ_H_
#define XDR_CONNECTION_READ_H_

gpointer read_thread(gpointer);
void read_parse(gchar, gchar[]);
gfloat signal_level(gfloat);

#endif
