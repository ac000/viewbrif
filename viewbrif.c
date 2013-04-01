/*
 * ViewBRIF
 *
 * A GUI BRIF file viewer
 *
 * Copyright (C) 2006-2013	Andrew Clayton <andrew@pccl.info>
 *
 * Released under the General Public License (GPL) version 2.
 * See COPYING
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <locale.h>
#include <string.h>
#include <monetary.h>
#include <ctype.h>
#include <stdbool.h>
#include <pthread.h>

#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "brif_spec.h"

/* Update for application version. */
#define VERSION		"032.90"

/*
 * DEBUG levels
 * 0 NO DEBUG
 * 1 Print stat debugging
 * 2 also print line lengths and which type of line is being processed
 * 3 Print line contents
 * 4 also lots of other stuff.
 */
#define DEBUG		0

#define PAD_LEFT	0
#define PAD_RIGHT	1
#define PAD_CENT	2

#define SPLIT_VIEW	0
#define RAW_VIEW	1
#define STATS_VIEW	2

#define BRIF_LINE_LEN	300

int line_no = 1;
int line_pos = 0;

/* Array of source id's from g_idle_add_full() */
static GArray *sid_array;

GtkWidget *notebook;
GtkWidget *text_view;
GtkWidget *text_view_raw;
GtkWidget *text_view_stats;
GtkTextIter iter;
GtkTextIter iter_raw;
GtkTextIter iter_stats;

/* Structure to hold brif file stats */
struct stats {
	int trans;
	int credits;
	int sales;
	long amount;
	long sales_amt;
	long credits_amt;
	long sales_vat_amt;
	long credits_vat_amt;
	off_t file_size;
} brif_stats;

static gboolean free_sid_array(void)
{
	if (sid_array) {
		g_array_free(sid_array, TRUE);
		sid_array = NULL;
	}
	return FALSE;
}

/*
 * Set the window title to 'ViewBRIF (/path/to/brifile)'
 */
static void set_window_title(GtkWidget *window, const char *extra_title)
{
	char *window_title;

	window_title = malloc(strlen(extra_title) + 12);
	sprintf(window_title, "ViewBRIF (%s)", extra_title);
	gtk_window_set_title(GTK_WINDOW(window), window_title);
	free(window_title);
}

/*
 * Takes a brif format amount, e.g 17520 and returns this value with
 * the decimal point added, i.e 175.20
 */
static double add_dp(long amount)
{
	return (double)amount / 100.0;
}

static void create_tags(GtkTextBuffer *buffer)
{
	gtk_text_buffer_create_tag(buffer, "bold", "weight",
			PANGO_WEIGHT_BOLD, NULL);

	gtk_text_buffer_create_tag(buffer, "blue_foreground", "foreground",
			"blue", NULL);

	gtk_text_buffer_create_tag(buffer, "green_foreground", "foreground",
			"darkgreen", NULL);

	gtk_text_buffer_create_tag(buffer, "red_foreground", "foreground",
			"darkred", NULL);

	gtk_text_buffer_create_tag(buffer, "grey_background", "background",
			"lightgrey", NULL);

	gtk_text_buffer_create_tag(buffer, "orange_background",
			"background", "#ffd24B", NULL);

	gtk_text_buffer_create_tag(buffer, "lightblue_background",
			"background", "#cce4ff", NULL);
}

static gboolean find_text(GtkTextBuffer *buffer, const gchar *text,
			  GtkTextIter *iter)
{
	GtkTextIter mstart;
	GtkTextIter mend;
	GtkTextMark *last_pos;
	gboolean found;

	found = gtk_text_iter_forward_search(iter, text, 0, &mstart, &mend,
			NULL);

	if (found) {
		gtk_text_buffer_select_range(buffer, &mstart, &mend);
		last_pos = gtk_text_buffer_create_mark(buffer, "last_pos",
				&mend, FALSE);

		if (gtk_notebook_get_current_page(
					GTK_NOTEBOOK(notebook)) == SPLIT_VIEW)
			gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(
						text_view), last_pos);
		else if (gtk_notebook_get_current_page(
					GTK_NOTEBOOK(notebook)) == RAW_VIEW)
			gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(
						text_view_raw), last_pos);

		return TRUE;
	}

	return FALSE;
}

static gboolean cb_switch_tab(GtkNotebook *notebook, GtkWidget *page,
			      guint page_num, GtkWidget *sbox)
{
	if (page_num == STATS_VIEW)
		gtk_widget_hide(sbox);
	else
		gtk_widget_show(sbox);

	return TRUE;
}

static gboolean cb_search(GtkWidget *search_button, gpointer data)
{
	const gchar *text;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GtkTextMark *last_pos;
	static bool searched;
	static bool found;

	text = gtk_entry_get_text(GTK_ENTRY(data));
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)) == SPLIT_VIEW)
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	else if (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)) ==
			RAW_VIEW)
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_raw));
	else
		return TRUE;

	last_pos = gtk_text_buffer_get_mark(buffer, "last_pos");
	if (searched && last_pos != NULL) {
		gtk_text_buffer_get_iter_at_mark(buffer, &iter, last_pos);
	} else if (!found || last_pos == NULL) {
		searched = true;
		gtk_text_buffer_get_start_iter(buffer, &iter);
	}

	found = find_text(buffer, text, &iter);
	if (!found)
		searched = false;

	return TRUE;
}

static void cb_about_window(void)
{
	GtkWidget *about;
	const gchar *authors[2] = { "Andrew Clayton <andrew@pccl.info>\n"
		"Graham Thomson <g.thomson@pccl.co.uk>", (const char *)NULL };

	about = gtk_about_dialog_new();

	gtk_window_set_title(GTK_WINDOW(about), "About ViewBRIF");
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), "ViewBRIF");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about),
			"Copyright (C) 2006-2013 Andrew Clayton");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about),
			(const gchar **)&authors);

	/* Connect the close_button to destroy the widget */
	g_signal_connect(G_OBJECT(about), "response",
			G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_widget_show(about);
}

static void cb_quit(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_main_quit();
}

static void cb_new_instance(void)
{
	pid_t pid;
	struct sigaction sa;

	sa.sa_handler = SIG_DFL;
	sa.sa_flags = SA_NOCLDWAIT;
	sigaction(SIGCHLD, &sa, NULL);

	pid = fork();
	if (pid == 0) {
		if (access("./viewbrif", X_OK) == -1)
			execlp("viewbrif", "viewbrif", (char *)NULL);
		else
			execlp("./viewbrif", "viewbrif", (char *)NULL);
	}
}

static void display_raw_line(const char *fline, const int line_array[][2])
{
	char data[BRIF_LINE_LEN + 1];
	char ln[7];
	int i = 0;
	int fstart = 0;
	int flen = 0;
	bool color_flag = false;
	GtkTextBuffer *buffer_raw;

	buffer_raw = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_raw));
	gtk_text_buffer_get_end_iter(buffer_raw, &iter_raw);

	sprintf(ln, "%-6d", line_no - 1);
	gtk_text_buffer_insert_with_tags_by_name(buffer_raw, &iter_raw, ln, -1,
			"lightblue_background", NULL);

	while (strncmp(fline + fstart, "\r\n", 2) != 0) {
		fstart = line_array[i][0];
		flen = line_array[i][1];
		memset(data, '\0', sizeof(data));
		strncpy(data, fline + fstart, flen);

		if (!color_flag) {
			gtk_text_buffer_insert(buffer_raw, &iter_raw, data, -1);
			color_flag = true;
		} else if (color_flag) {
			gtk_text_buffer_insert_with_tags_by_name(buffer_raw,
					&iter_raw, data, -1, "grey_background",
					NULL);
			color_flag = false;
		}
		i++;
	}
}

static void process_line(const char *fline, const int line_array[][2],
			 const char *field_headers[])
{
	char pos[12];
	char fnum[5];
	char fname[31];
	char data[BRIF_LINE_LEN + 1];
	int i = 0;
	int fstart = 0;
	int flen = 0;
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_end_iter(buffer, &iter);

	while (strncmp(fline + fstart, "\r\n", 2) != 0) {
		fstart = line_array[i][0];
		flen = line_array[i][1];

		sprintf(pos, "(%3d, %3d)", fstart + 1, flen);
		sprintf(fnum, "F%d", i + 1);

		if (DEBUG > 3)
			printf("Field name = %s\n", field_headers[i]);

		sprintf(fname, "[ %s", field_headers[i]);
		sprintf(pos, "%-11s", pos);
		sprintf(fnum, "%-4s", fnum);
		sprintf(fname, "%-30s", fname);

		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, pos,
				-1, "red_foreground", NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, fnum,
				-1, "red_foreground", NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, fname,
				-1, "blue_foreground", NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "] ",
				-1, "blue_foreground", NULL);

		memset(data, '\0', sizeof(data));
		strncpy(data, fline + fstart, flen);
		strcat(data, "\n");
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, data,
				-1, "orange_background", NULL);
		line_pos += flen;
		i++;

		if (DEBUG > 2)
			printf("Line: %s\n", data);
	}

	display_raw_line(fline, line_array);
	/* Allow the interface to be responsive */
	gtk_main_iteration_do(FALSE);
}

static gboolean gather_stats(const char *line)
{
	int fstart = 0;
	int flen = 0;
	char data[BRIF_LINE_LEN + 1];
	static char trans_type[2] = "\0";

	memset(data, '\0', sizeof(data));

	if (strncmp(line + 1, "A", 1) == 0) {
		brif_stats.trans++;

		fstart = mrl[14][0];
		flen = mrl[14][1];
		strncpy(data, line + fstart, flen);
		brif_stats.amount += atoi(data);

		if (strncmp(line + 2, "S", 1) == 0) {
			brif_stats.sales++;
			brif_stats.sales_amt += atoi(data);
			strcpy(trans_type, "S");
		}

		if (strncmp(line + 2, "R", 1) == 0) {
			brif_stats.credits++;
			brif_stats.credits_amt += atoi(data);
			strcpy(trans_type, "R");
		}
	} else if (strncmp(line + 2, "P", 1) == 0) {
		fstart = pcl[5][0];
		flen = pcl[5][1];
		strncpy(data, line + fstart, flen);
		if (strncmp(trans_type, "S", 1) == 0)
			brif_stats.sales_vat_amt += atoi(data);
		else if (strncmp(trans_type, "R", 1) == 0)
			brif_stats.credits_vat_amt += atoi(data);
	}

	return FALSE;
}

static gboolean display_stats(void)
{
	char *val;
	char fn[31];
	char famount[31];
	char samount[31];
	char camount[31];
	const char *fmt = "%-30s";
	double amnt = 0.0;
	double samnt = 0.0;
	double camnt = 0.0;
	GtkTextBuffer *buffer_stats;

	/* Set the locale to the users */
	setlocale(LC_ALL, "");

	buffer_stats = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_stats));
	gtk_text_buffer_get_end_iter(buffer_stats, &iter_stats);

	/* File Size */
	val = malloc(sizeof(brif_stats.file_size) + 1);
	memset(val, '\0', sizeof(brif_stats.file_size) + 1);
	sprintf(val, "%ld", brif_stats.file_size);
	sprintf(fn, fmt, "File size");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, val, -1);
	free(val);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, " bytes\n\n", -1);

	/* Number of transactions */
	val = malloc(sizeof(brif_stats.trans) + 2);
	memset(val, '\0', sizeof(brif_stats.trans) + 2);
	sprintf(val, "%d\n\n", brif_stats.trans);
	sprintf(fn, fmt, "Number of transactions");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, val, -1);
	free(val);

	/* Number of sales */
	val = malloc(sizeof(brif_stats.sales) + 2);
	memset(val, '\0', sizeof(brif_stats.sales) + 2);
	sprintf(val, "%d\n", brif_stats.sales);
	sprintf(fn, fmt, "                 Sales");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, val, -1);
	free(val);
	/* Total of sales (net) */
	samnt = add_dp(brif_stats.sales_amt - brif_stats.sales_vat_amt);
	strfmon(samount, sizeof(samount), " %!12n\n", samnt);
	sprintf(fn, fmt, "     Sales total (net)");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, samount, -1);
	/* Total of sales VAT */
	samnt = add_dp(brif_stats.sales_vat_amt);
	strfmon(samount, sizeof(samount), " %!12n\n\n", samnt);
	sprintf(fn, fmt, "       Sales VAT total");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, samount, -1);

	/* Number of credits */
	val = malloc(sizeof(brif_stats.credits) + 3);
	memset(val, '\0', sizeof(brif_stats.credits) + 3);
	sprintf(val, "%d\n", brif_stats.credits);
	sprintf(fn, fmt, "               Credits");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, val, -1);
	free(val);
	/* Total of credits (net) */
	camnt = add_dp(brif_stats.credits_amt - brif_stats.credits_vat_amt);
	strfmon(camount, sizeof(camount), " %!12n\n", camnt);
	sprintf(fn, fmt, "   Credits total (net)");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, camount, -1);
	/* Total of credits VAT */
	camnt = add_dp(brif_stats.credits_vat_amt);
	strfmon(camount, sizeof(camount), " %!12n\n\n", camnt);
	sprintf(fn, fmt, "     Credits VAT total");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, camount, -1);

	/* Total NET amount, sales - credits */
	amnt = add_dp((brif_stats.sales_amt - brif_stats.sales_vat_amt) -
						(brif_stats.credits_amt -
						brif_stats.credits_vat_amt));
	strfmon(famount, sizeof(famount), " %!12n\n", amnt);
	sprintf(fn, fmt, "File total (net) S-R");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, famount, -1);

	/* VAT transaction amount, sales - credits */
	amnt = add_dp(brif_stats.sales_vat_amt - brif_stats.credits_vat_amt);
	sprintf(fn, fmt, "VAT total S-R");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	strfmon(famount, sizeof(famount), " %!12n\n\n", amnt);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, famount, -1);

	/* Total NET amount */
	amnt = add_dp(brif_stats.amount - brif_stats.sales_vat_amt -
						brif_stats.credits_vat_amt);
	strfmon(famount, sizeof(famount), " %!12n\n", amnt);
	sprintf(fn, fmt, "File total (net)");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, famount, -1);

	/* VAT transaction amount */
	amnt = add_dp(brif_stats.sales_vat_amt + brif_stats.credits_vat_amt);
	sprintf(fn, fmt, "VAT total");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	strfmon(famount, sizeof(famount), " %!12n\n\n", amnt);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, famount, -1);

	/* Total GROSS amount, sales - credits*/
	amnt = add_dp(brif_stats.sales_amt - brif_stats.credits_amt);
	strfmon(famount, sizeof(famount), " %!12n\n", amnt);
	sprintf(fn, fmt, "File total (gross) S-R");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, famount, -1);

	/* Total GROSS amount */
	amnt = add_dp(brif_stats.amount);
	strfmon(famount, sizeof(famount), " %!12n\n", amnt);
	sprintf(fn, fmt, "File total (gross)");
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, famount, -1);

	return FALSE;
}

static gboolean do_main_record(const char *line)
{
	char hline[35];
	GtkTextBuffer *buffer;

	sprintf(hline, "Line no: %d, Main Record\n", line_no++);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, hline, -1,
			"green_foreground", NULL);

	process_line(line, mrl, mrn);

	return FALSE;
}

static gboolean do_purchasing_card(const char *line)
{
	char hline[35];
	GtkTextBuffer *buffer;

	sprintf(hline, "Line no: %d, Purchasing Card\n", line_no++);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, hline, -1,
			"green_foreground", NULL);

	process_line(line, pcl, pcln);

	return FALSE;
}

static gboolean do_purchasing_card_item(const char *line)
{
	char hline[35];
	GtkTextBuffer *buffer;

	sprintf(hline, "Line no: %d, Purchasing Card Item\n", line_no++);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, hline, -1,
			"green_foreground", NULL);

	process_line(line, pcil, pciln);

	return FALSE;
}

static void *read_file_thread(void *fn)
{
	int fd;
	off_t offset = 0;
	off_t size = brif_stats.file_size;
	char *bf_map;
	char *line;
	unsigned int sid;

	/* Open file RO and apply some fadvise hints */
	fd = open((char *)fn, O_RDONLY | O_CLOEXEC);
	posix_fadvise(fd, 0, 0, POSIX_FADV_WILLNEED);

	bf_map = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (bf_map == MAP_FAILED) {
		printf("mmap() failed.\n");
		close(fd);
		_exit(EXIT_FAILURE);
	}
	close(fd);

	sid_array = g_array_new(TRUE, FALSE, sizeof(guint));

	/* Gather the stats first */
	while (offset < size) {
		line = malloc(BRIF_LINE_LEN + 1);
		memcpy(line, bf_map + offset, BRIF_LINE_LEN);
		line[BRIF_LINE_LEN] = '\0';
		offset += BRIF_LINE_LEN;

		/* See comment below */
		if (!isprint((int)line[0])) {
			free(line);
			continue;
		}

		sid = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
				(GSourceFunc)gather_stats, (char *)line, free);
		g_array_append_val(sid_array, sid);
	}
	sid = g_idle_add((GSourceFunc)display_stats, NULL);
	g_array_append_val(sid_array, sid);

	offset = 0;
	while (offset < size) {
		line = malloc(BRIF_LINE_LEN + 1);
		memcpy(line, bf_map + offset, BRIF_LINE_LEN);
		line[BRIF_LINE_LEN] = '\0';
		offset += BRIF_LINE_LEN;
		if (DEBUG > 2)
			printf("%c\n", (int)line[0]);

		/*
		 * Catch non-printable characters, probably ctrl-z that
		 * has been added to the end of the file by pceft
		 */
		if (!isprint((int)line[0])) {
			free(line);
			continue;
		}

		if (strncmp(line + 1, "A", 1) == 0) {
			if (DEBUG > 1)
				printf("Doing main record line.\n");

			sid = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
					(GSourceFunc)do_main_record,
					(char *)line, free);
			g_array_append_val(sid_array, sid);
		} else if (strncmp(line + 2, "P", 1) == 0) {
			if (DEBUG > 1)
				printf("Doing purchasing card line.\n");

			sid = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
					(GSourceFunc)do_purchasing_card,
					(char *)line, free);
			g_array_append_val(sid_array, sid);
		} else if (strncmp(line + 2, "I", 1) == 0) {
			if (DEBUG > 1)
				printf("Doing purchasing card item line.\n");

			sid = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
					(GSourceFunc)do_purchasing_card_item,
					(char *)line, free);
			g_array_append_val(sid_array, sid);
		}

		if (DEBUG > 1)
			printf("Line length = %d\n", line_pos);
		line_pos = 0;
	}
	munmap(bf_map, size);
	g_idle_add((GSourceFunc)free_sid_array, NULL);

	pthread_exit(NULL);
}

static void read_file(const char *fn)
{
	char emesg[255];
	struct stat st;
	GtkTextBuffer *buffer;
	GtkTextBuffer *buffer_raw;
	GtkTextBuffer *buffer_stats;
	pthread_t thr;
	pthread_attr_t attr;

	/* If there is a file loading in progress, we want to stop it */
	if (sid_array) {
		int i = 0;
		unsigned int sid;

		for (;;) {
			sid = g_array_index(sid_array, guint, i);
			if (sid == 0)
				break;
			g_source_remove(sid);
			i++;
		}
		g_array_free(sid_array, TRUE);
		sid_array = NULL;
	}

	/* Reset global counters and clear the text view */
	line_no = 1;
	line_pos = 0;
	memset(&brif_stats, 0, sizeof(brif_stats));

	/* Get file size */
	stat(fn, &st);
	brif_stats.file_size = st.st_size;

	/* Reset the text views */
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), NULL);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view_raw), NULL);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view_stats), NULL);

	/* Pretty print filename */
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_start_iter(buffer, &iter);

	/* Get the raw buffer */
	buffer_raw = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_raw));
	gtk_text_buffer_get_start_iter(buffer_raw, &iter_raw);

	/* Get the stats buffer */
	buffer_stats = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_stats));
	gtk_text_buffer_get_start_iter(buffer_stats, &iter_stats);

	/* Create the text tags for the text buffers */
	create_tags(buffer);
	create_tags(buffer_raw);
	create_tags(buffer_stats);

	/*
	 * If the file size is NOT a multiple of 300 or is 0 size,
	 * don't read it in. Instead display an error message to
	 * the user.
	 */
	if ((st.st_size % BRIF_LINE_LEN) != 0) {
		sprintf(emesg, "ERROR: Size (%ld) of file (%s) is not a "
				"multiple of %d.", st.st_size, fn,
				BRIF_LINE_LEN);
		printf("%s\n", emesg);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, emesg,
				-1, "bold", NULL);
		return;
	} else if (st.st_size == 0) {
		sprintf(emesg, "ERROR: File (%s) seems to be empty.", fn);
		printf("%s\n", emesg);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, emesg,
				-1, "bold", NULL);
		return;
	}

	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
			"Displaying file: ", -1, "bold", NULL);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, fn, -1,
			"blue_foreground", "bold", NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thr, &attr, &read_file_thread, (void *)fn);
	pthread_setname_np(thr, "read-brif-file");
	pthread_attr_destroy(&attr);
}

static void cb_file_chooser(GtkWidget *widget, gpointer data)
{
	GtkWidget *file_chooser;
	GtkFileFilter *filter_all;
	GtkFileFilter *filter_brif;

	/* Create a new file selection widget */
	file_chooser = gtk_file_chooser_dialog_new("File selection",
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN,
			GTK_RESPONSE_ACCEPT,
			NULL);

	filter_all = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter_all, "*.*");
	gtk_file_filter_set_name(filter_all, "All files (*.*)");

	filter_brif = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter_brif, "*.req");
	gtk_file_filter_add_pattern(filter_brif, "*.ans");
	gtk_file_filter_set_name(filter_brif, "BRIF files (*.ans, *.req)");

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser),
			filter_all);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser),
			filter_brif);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(file_chooser),
			filter_brif);

	if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
		char *filename;

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
				(file_chooser));
		if (filename) {
			char fn[PATH_MAX];

			snprintf(fn, PATH_MAX, "%s", filename);
			g_free(filename);
			set_window_title(data, fn);
			printf("Selected file = %s\n", fn);
			read_file(fn);
		}
	}
	gtk_widget_destroy(file_chooser);
}

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *scrolled_window;
	GtkWidget *scrolled_window_raw;
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkWidget *filemenu;
	GtkWidget *filemenu_menu;
	GtkWidget *filemenu_new_instance;
	GtkWidget *filemenu_open;
	GtkWidget *filemenu_quit;
	GtkWidget *helpmenu;
	GtkWidget *helpmenu_menu;
	GtkWidget *helpmenu_about;
	GtkWidget *separator_menu_item;
	GtkWidget *split_label;
	GtkWidget *raw_label;
	GtkWidget *stats_label;
	GtkAccelGroup *accel_group;
	PangoFontDescription *font_desc;
	GtkWidget *sbox;
	GtkWidget *search_entry;
	GtkWidget *search_button;

	gtk_init(&argc, &argv);

	accel_group = gtk_accel_group_new();

	/* Main window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(cb_quit),
			NULL);
	gtk_window_set_title(GTK_WINDOW(window), "ViewBRIF");
	gtk_container_set_border_width(GTK_CONTAINER(window), 0);
	gtk_widget_set_size_request(window, 700, 800);

	/* vbox to hold stuff */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* Create the menubar */
	menubar = gtk_menu_bar_new();
	gtk_widget_show(menubar);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

	/* Create the file menu */
	filemenu = gtk_menu_item_new_with_mnemonic(("_File"));
	gtk_widget_show(filemenu);
	gtk_container_add(GTK_CONTAINER(menubar), filemenu);

	filemenu_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(filemenu), filemenu_menu);

	/* Create the new menu item */
	filemenu_new_instance = gtk_image_menu_item_new_from_stock("gtk-new",
			accel_group);
	gtk_widget_show(filemenu_new_instance);
	gtk_container_add(GTK_CONTAINER(filemenu_menu), filemenu_new_instance);

	/* Create the open menu item */
	filemenu_open = gtk_image_menu_item_new_from_stock("gtk-open",
			accel_group);
	gtk_widget_show(filemenu_open);
	gtk_container_add(GTK_CONTAINER(filemenu_menu), filemenu_open);

	separator_menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(separator_menu_item);
	gtk_container_add(GTK_CONTAINER(filemenu_menu), separator_menu_item);
	gtk_widget_set_sensitive(separator_menu_item, FALSE);

	/* Create the quit menu item */
	filemenu_quit = gtk_image_menu_item_new_from_stock("gtk-quit",
			accel_group);
	gtk_widget_show(filemenu_quit);
	gtk_container_add(GTK_CONTAINER(filemenu_menu), filemenu_quit);

	/* Create the help menu */
	helpmenu = gtk_menu_item_new_with_mnemonic(("_Help"));
	gtk_widget_show(helpmenu);
	gtk_container_add(GTK_CONTAINER(menubar), helpmenu);

	helpmenu_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpmenu), helpmenu_menu);

	/* Create the about menu item */
	helpmenu_about = gtk_image_menu_item_new_from_stock("gtk-about",
			accel_group);
	gtk_widget_show(helpmenu_about);
	gtk_container_add(GTK_CONTAINER(helpmenu_menu), helpmenu_about);

	/* Create a notebook */
	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(notebook), 10);

	/* Create a new scrolled window for the split view. */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_window);
	gtk_container_add(GTK_CONTAINER(notebook), scrolled_window),
	gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 5);

	/*
	 * The policy is one of GTK_POLICY AUTOMATIC, or GTK_POLICY_ALWAYS.
	 * GTK_POLICY_AUTOMATIC will automatically decide whether you need
	 * scrollbars, whereas GTK_POLICY_ALWAYS will always leave the
	 * scrollbars there. The first one is the horizontal scrollbar,
	 * the second, the vertical.
	 */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	/*
	 * Create a text view for the split view and set it RO with
	 * invisible cursor.
	 */
	text_view = gtk_text_view_new();
	gtk_widget_show(text_view);
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);

	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	/* Create a new scrolled window for the raw view. */
	scrolled_window_raw = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_window_raw);
	gtk_container_add(GTK_CONTAINER(notebook), scrolled_window_raw);
	gtk_container_set_border_width(GTK_CONTAINER(scrolled_window_raw), 5);

	/*
	 * The policy is one of GTK_POLICY AUTOMATIC, or GTK_POLICY_ALWAYS.
	 * GTK_POLICY_AUTOMATIC will automatically decide whether you need
	 * scrollbars, whereas GTK_POLICY_ALWAYS will always leave the
	 * scrollbars there. The first one is the horizontal scrollbar,
	 * the second, the vertical.
	 */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_raw),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	/*
	 * Create a text view for the raw view and set it RO with
	 * invisible cursor.
	 */
	text_view_raw = gtk_text_view_new();
	gtk_widget_show(text_view_raw);
	gtk_container_add(GTK_CONTAINER(scrolled_window_raw), text_view_raw);

	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view_raw), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view_raw), FALSE);

	/*
	 * Create a text view for the stats view and set it RO with
	 * invisible cursor.
	 */
	text_view_stats = gtk_text_view_new();
	gtk_widget_show(text_view_stats);
	gtk_container_add(GTK_CONTAINER(notebook), text_view_stats);

	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view_stats), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view_stats), FALSE);

	/* Set tab labels */
	split_label = gtk_label_new(" Split View ");
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), scrolled_window,
			split_label);
	raw_label = gtk_label_new(" Raw View ");
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), scrolled_window_raw,
			raw_label);

	stats_label = gtk_label_new(" Stats ");
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), text_view_stats,
			stats_label);

	/* Menu item callbacks */
	g_signal_connect((gpointer)filemenu_quit, "activate",
			G_CALLBACK(cb_quit), NULL);
	g_signal_connect((gpointer)filemenu_new_instance, "activate",
			G_CALLBACK(cb_new_instance), NULL);
	g_signal_connect((gpointer)filemenu_open, "activate",
			G_CALLBACK(cb_file_chooser), window);
	g_signal_connect((gpointer)helpmenu_about, "activate",
			G_CALLBACK(cb_about_window), NULL);

	/* Change default font throughout the text views */
	font_desc = pango_font_description_from_string("Monospace");
	gtk_widget_modify_font(text_view, font_desc);
	gtk_widget_modify_font(text_view_raw, font_desc);
	gtk_widget_modify_font(text_view_stats, font_desc);
	pango_font_description_free(font_desc);

	/* Horizontal box to hold the search entry and button */
	sbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), sbox, FALSE, FALSE, 0);
	/* Signal to notify when to show/hide the search box */
	g_signal_connect(G_OBJECT(notebook), "switch-page", G_CALLBACK(
				cb_switch_tab), sbox);
	gtk_widget_show(sbox);

	search_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(sbox), search_entry, TRUE, TRUE, 0);
	gtk_widget_show(search_entry);
	g_signal_connect(G_OBJECT(search_entry), "activate",
			G_CALLBACK(cb_search), search_entry);

	search_button = gtk_button_new_with_label("Search");
	gtk_box_pack_start(GTK_BOX(sbox), search_button, FALSE, FALSE, 0);
	gtk_widget_show(search_button);
	g_signal_connect(G_OBJECT(search_button), "clicked",
			G_CALLBACK(cb_search), search_entry);

	gtk_widget_show(window);

	/* If we got a filename as an argument, open that up in the viewer. */
	if (argc > 1) {
		read_file(argv[1]);
		set_window_title(window, argv[1]);
	}

	gtk_main();

	exit(EXIT_SUCCESS);
}
