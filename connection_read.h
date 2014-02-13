#ifndef XDR_CONNECTION_READ_H_
#define XDR_CONNECTION_READ_H_

// RDS
#define BLOCK_B 0
#define BLOCK_C 1
#define BLOCK_D 2

gpointer read_thread(gpointer);
void read_parse(gchar, gchar[]);
gfloat signal_level(gfloat);

#endif
