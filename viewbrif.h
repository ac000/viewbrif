/* 
 * viewbrif.h
 *
 * Function definitions for viebrif.c
 *
 * Copyright (C) 2010-2011 Andrew Clayton <andrew@pccl.info>
 *
 * Released under the General Public License (GPL) version 2
 * See COPYING
 */

static void reset_stats();
static void set_window_title(GtkWidget *window, char *extra_title);
static double add_dp(long int amount);
static char *str_pad(char *newstr, char *str, int len, char *padchar, int just);
static void create_tags(GtkTextBuffer *buffer);
static gboolean find_text(GtkTextBuffer *buffer, const gchar *text,
							GtkTextIter *iter);
static gboolean cb_search(GtkWidget *search_button, gpointer data);
static void cb_about_window();
static void cb_quit(GtkMenuItem *menuitem, gpointer user_data);
static void cb_new_instance();
static void cb_file_chooser(GtkWidget *widget, gpointer data);
static void process_line(char *fline, const int line_array[][2], 
						const char *field_headers[]);
static void display_raw_line(char *fline, const int line_array[][2]);
static void gather_stats(char *fline, const int line_array[][2]);
static void display_stats();
static void do_main_record(char *fline);
static void do_purchasing_card(char *fline);
static void do_purchasing_card_item(char *fline);
static void read_file(char *fn);
static void *read_file_thread(char *arg);
