/*
 * ViewBRIF
 *
 * A GUI BRIF file viewer
 *
 * Copyright (C) 2006-2010, Andrew Clayton <andrew@pccl.info>
 *
 * Released under the General Public License (GPL) version 2.
 * See COPYING
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <locale.h>
#include <string.h>
#include <monetary.h>
#include <ctype.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

/* Update for application version. */
#define VERSION		"013"

/*
 * DEBUG levels
 * 0 NO DEBUG
 * 1 Print stat debugging
 * 2 also print line lengths and which type of line is being processed
 * 3 Print line contents
 * 4 also lots of other stuff.  
 */ 
#define DEBUG		0

#define PAD_LEFT        0
#define PAD_RIGHT       1
#define PAD_CENT        2

int line_no = 1;
int line_pos = 0;

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
	long int amount;
	long int sales_amt;
	long int credits_amt;
	long int vat_ta;
	long int file_size;
};

struct stats brif_stats = {
	0, 
	0, 
	0, 
	0,
	0,
	0,
	0,
	0
};

static void reset_stats();
static double add_dp(long int amount);
static char *str_pad(char *newstr, char *str, int len, char *padchar, int just);
static void create_tags(GtkTextBuffer *buffer);
static void cb_about_window();
static void cb_quit(GtkMenuItem *menuitem, gpointer user_data);
static void cb_new_window(GtkMenuItem *menuitem, gpointer user_data);
static void cb_file_selector();
static void cb_file_selected(GtkWidget *w, GtkFileSelection *fs);
static void process_line(char *fline, int line_array[][2], 
							char *field_headers[]);
static void display_raw_line(char *fline, int line_array[][2]);
static void gather_stats(char *fline, int line_array[][2]);
static void display_stats();
static void do_main_record(char *fline);
static void do_purchasing_card(char *fline);
static void do_purchasing_card_item(char *fline);
static void read_file(char *fn);

static void reset_stats()
{
	brif_stats.trans       = 0;
	brif_stats.credits     = 0;
	brif_stats.sales       = 0;
	brif_stats.amount      = 0;
	brif_stats.sales_amt   = 0;
	brif_stats.credits_amt = 0;
	brif_stats.vat_ta      = 0;
	brif_stats.file_size   = 0;
}

/*
 * Takes a brif format amount, e.g 17520 and returns this value with
 * the decimal point added, i.e 175.20
 */
static double add_dp(long int amount)
{
	char *na, *na2;
	double da;

	/* brif amount format */
	na = (char *)malloc(sizeof(amount) + 1);
	memset(na, '\0', sizeof(amount) + 1);
	sprintf(na, "%ld", amount);

	/* brif amount format with the dp added */
	na2 = (char *)malloc(sizeof(na) + 2);
	memset(na2, '\0', sizeof(na) + 2);

	/* If we got less than 100p prepend a 0 to the value for strfmon() */
	if (amount < 100)
		strcat(na2, "0");

	strncat(na2, na, strlen(na) - 2);
	strcat(na2, ".");
	strncat(na2, na + strlen(na) - 2, 2);

	da = atof(na2);
        
	if (DEBUG > 0)
		printf("%f\t%s\t%s\n", da, na, na2);

	free(na);
	free(na2);

	return da;
}

static char *str_pad(char *newstr, char *str, int len, char *padchar, int just)
{
        char *padstr;
        int i, ppos;
        
	if (just == PAD_LEFT || just == PAD_RIGHT) {
                padstr = (char *)malloc(sizeof(char) * (len - strlen(str))
                                                                        + 1);
                memset(padstr, '\0', sizeof(char) * (len - strlen(str)) + 1);

                for (i = 0; i < (len - strlen(str)); i++)
                        strcat(padstr, padchar);

                if (just == PAD_LEFT)
                        sprintf(newstr, "%s%s", padstr, str);
                else if (just == PAD_RIGHT)
                        sprintf(newstr, "%s%s", str, padstr);
        } else if (just == PAD_CENT) {
                padstr = (char *)malloc(sizeof(char) * len + 1);
                memset(padstr, '\0', sizeof(char) * len + 1);

                for (i = 0; i < len; i++)
                        strcat(padstr, padchar);

                ppos = (len - strlen(str)) / 2;
                strncpy(padstr + ppos, str, strlen(str));
                strcpy(newstr, padstr);
        }

	if (DEBUG > 3) {
        	printf("Original string = %s, Pad string = %s, "
				"Padded String = %s\n", str, padstr, newstr);
	}

        free(padstr);

	return newstr;
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

static void cb_about_window()
{
	GtkWidget *about;
	const gchar *authors[2] = { "Andrew Clayton <andrew@pccl.info>\n"
				"Graham Thomson <g.thomson@pccl.co.uk>",
							(const char*)NULL };

	about = gtk_about_dialog_new();

	gtk_window_set_title(GTK_WINDOW(about), "About ViewBRIF");
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about), "ViewBRIF");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), 
				"Copyright (C) 2006-2010 Andrew Clayton");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), 
						(const gchar **)&authors);

	/* Connect the close_button to destroy the widget */
	g_signal_connect(G_OBJECT(about), "response", 
					G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_widget_show(about);
}

static void cb_file_selected(GtkWidget *w, GtkFileSelection *fs)
{
	char *fpath;
	int len = 0;
	
	len = strlen(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs)));
	fpath = (char *)malloc(sizeof(char) * (len + 1));
	memset(fpath, '\0', sizeof(char) * (len + 1));
	strcpy(fpath, gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs)));

	printf("Selected file path = %s\n", fpath);
	
	read_file(fpath);
	
	free(fpath);	
}

static void cb_file_selector()
{
	GtkWidget *filew;

	/* Create a new file selection widget */
	filew = gtk_file_selection_new("File selection");

	/* Connect the ok_button to file_ok_sel function */
	g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(filew)->ok_button),
			"clicked", G_CALLBACK(cb_file_selected), 
							(gpointer) filew);
    
	/* Connect the cancel_button to destroy the widget */
	g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(
				filew)->cancel_button), "clicked", G_CALLBACK(
					gtk_widget_destroy), G_OBJECT(filew));
    	
	/* 
	 * Ensure that the dialog box is destroyed when the user clicks a 
	 * button. 
	 */
   
	g_signal_connect_swapped(GTK_FILE_SELECTION(filew)->ok_button,
				"clicked", G_CALLBACK(gtk_widget_destroy), 
							filew);

	g_signal_connect_swapped(GTK_FILE_SELECTION(
				filew)->cancel_button, "clicked",
				G_CALLBACK(gtk_widget_destroy), filew);
	
	gtk_widget_show(filew);
}

static void cb_quit(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_main_quit();
}

static void cb_new_window(GtkMenuItem *menuitem, gpointer user_data)
{
}

static void process_line(char *fline, int line_array[][2], 
							char *field_headers[])
{
	char pos[12], fnum[5], fname[31];
        char data[301];
	int i = 0, fstart = 0, flen = 0;
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_end_iter(buffer, &iter);

	while (strncmp(fline + fstart, "\r\n", 2) != 0) {
                fstart = line_array[i][0];
                flen = line_array[i][1];
		
		sprintf(pos, "(%d, %d)", fstart + 1, flen);
		sprintf(fnum, "F%d", i + 1);

		if (DEBUG > 3)
			printf("Field name = %s\n", field_headers[i]);

                sprintf(fname, "[ %s", field_headers[i]);
		str_pad(pos, pos, 11, " ", PAD_RIGHT);
		str_pad(fnum, fnum, 4, " ", PAD_RIGHT);
		str_pad(fname, fname, 30, " ", PAD_RIGHT);		

		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, pos,
                                                -1, "red_foreground", NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, fnum,
						-1, "red_foreground", NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, fname,
						-1, "blue_foreground", NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "] ",
                                                -1, "blue_foreground", NULL);
		
		memset(data, '\0', sizeof(char) * 301);
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
	gather_stats(fline, line_array);
}

static void display_raw_line(char *fline, int line_array[][2])
{
	char data[301], ln[7];
	int i = 0, fstart = 0, flen = 0, color_flag = 0;
	GtkTextBuffer *buffer_raw;

	buffer_raw = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_raw));
	gtk_text_buffer_get_end_iter(buffer_raw, &iter_raw);

	sprintf(ln, "%-6d", line_no - 1);
 	gtk_text_buffer_insert_with_tags_by_name(buffer_raw, &iter_raw, ln, -1,
						"lightblue_background", NULL);
	
	while (strncmp(fline + fstart, "\r\n", 2) != 0) {
		fstart = line_array[i][0];
		flen = line_array[i][1];
		memset(data, '\0', sizeof(char) * 301);
		strncpy(data, fline + fstart, flen);		
		
		if (color_flag == 0) {
			gtk_text_buffer_insert(buffer_raw, &iter_raw, data, 
									-1);
			color_flag = 1;
		} else if (color_flag == 1) {
			gtk_text_buffer_insert_with_tags_by_name(buffer_raw, 
							&iter_raw, data, -1, 
						"grey_background", NULL);
			color_flag = 0;
		}
	
		i++;
	}
}

static void gather_stats(char *fline, int line_array[][2])
{
	int fstart = 0, flen = 0;
	char data[301];

	memset(data, '\0', sizeof(char) * 301);

	if (strncmp(fline + 1, "A", 1) == 0) {
		brif_stats.trans++;

		fstart = line_array[14][0];
		flen = line_array[14][1];
		strncpy(data, fline + fstart, flen);

		if (strncmp(fline + 2, "S", 1) == 0) {
			brif_stats.sales++;
			brif_stats.amount += atoi(data);
			brif_stats.sales_amt += atoi(data);
		}

		if (strncmp(fline + 2, "R", 1) == 0) {
			brif_stats.credits++;
			brif_stats.amount -= atoi(data);
			brif_stats.credits_amt += atoi(data);
		}
	} else if (strncmp(fline + 2, "P", 1) == 0) {
		fstart = line_array[5][0];
		flen = line_array[5][1];
		strncpy(data, fline + fstart, flen);

		brif_stats.vat_ta += atoi(data);
	}
}

static void display_stats()
{
	char *val, fn[31], famount[31], samount[31], camount[31];
	int flen = 30;
	double amnt = 0.0, samnt = 0.0, camnt = 0.0;
	GtkTextBuffer *buffer_stats;

	/* Set the locale to the users */
	setlocale(LC_ALL, "");

	buffer_stats = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_stats));
	gtk_text_buffer_get_end_iter(buffer_stats, &iter_stats);	

	/* File Size */
	val = (char *)malloc(sizeof(brif_stats.file_size) + 1);
	memset(val, '\0', sizeof(brif_stats.file_size) + 1);
	sprintf(val, "%ld", brif_stats.file_size);
	str_pad(fn, "File size", flen, " ", PAD_RIGHT); 	
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, val, -1);
	free(val);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, " bytes\n\n", -1);
	
	/* Number of transactions */
	val = (char *)malloc(sizeof(brif_stats.trans) + 2);
	memset(val, '\0', sizeof(brif_stats.trans) + 2);
	sprintf(val, "%d\n\n", brif_stats.trans);
	str_pad(fn, "Number of transactions", flen, " ", PAD_RIGHT);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, val, -1);
	free(val);

	/* Number of sales */
	val = (char *)malloc(sizeof(brif_stats.sales) + 2);
	memset(val, '\0', sizeof(brif_stats.sales) + 2);
	sprintf(val, "%d\n", brif_stats.sales);
	str_pad(fn, "                 Sales", flen, " ", PAD_RIGHT);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, val, -1);
	free(val);
	samnt = add_dp(brif_stats.sales_amt);
	strfmon(samount, sizeof(samount), "%=15n\n\n", samnt);
	str_pad(fn, "           Sales total", flen, " ", PAD_RIGHT);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, samount, -1);

	/* Number of credits */
	val = (char *)malloc(sizeof(brif_stats.credits) + 3);
	memset(val, '\0', sizeof(brif_stats.credits) + 3);
	sprintf(val, "%d\n", brif_stats.credits);
	str_pad(fn, "               Credits", flen, " ", PAD_RIGHT);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, val, -1);
	free(val);
	camnt = add_dp(brif_stats.credits_amt);
	strfmon(camount, sizeof(camount), "-%=15n\n\n", camnt);
	str_pad(fn, "         Credits total", flen, " ", PAD_RIGHT);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, camount, -1);

	/* Amount from main record */
	amnt = add_dp(brif_stats.amount);
	strfmon(famount, sizeof(famount), "%=15n\n", amnt);
	str_pad(fn, "File total", flen, " ", PAD_RIGHT);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, famount, -1);
	
	/* VAT transaction amount */
	amnt = add_dp(brif_stats.vat_ta);
	str_pad(fn, "VAT total", flen, " ", PAD_RIGHT);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, fn, -1);
	strfmon(famount, sizeof(famount), "%=15n", amnt);
	gtk_text_buffer_insert(buffer_stats, &iter_stats, famount, -1);
}

static void do_main_record(char *fline)
{
	char *mrn[] = { "Record Processed", "Record Type", "Transaction Type",
                        "Cont/Aux Following", "Live/Test", "Transaction Date",
                        "Transaction Time", "Merchant ID", "Terminal ID",
                        "Capture Method", "PAN", "Expiry Date", "Start Date",
			"Issue No.", "Amount", "Cash Back Amount", 
			"Card Track 2", "Originators Trans Ref", 
			"Currency Code", "Sort Code", "Auth Method", "User ID",
			"RESERVED", "Card Scheme Code", "Network Terminal No.",
			"Transaction Number", "Status", "Auth Code", 
			"Error No.", "Error/Auth Message", "CRLF" };
	int mrl[31][2] = { {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1},
                        {5, 8}, {13, 6}, {19, 15}, {34, 8}, {42, 1},
                        {43, 19}, {62, 4}, {66, 4}, {70, 2}, {72, 12},
                        {84, 12}, {96, 40}, {136, 25}, {161, 3}, {164, 1},
                        {165, 1}, {166, 8}, {174, 9}, {183, 3}, {186, 4},
                        {190, 11}, {201, 1}, {202, 8}, {210, 4}, {214, 84},
                        {298, 2} };
	char hline[35];
	GtkTextBuffer *buffer;

	sprintf(hline, "Line no: %d, Main Record\n", line_no++);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, hline, -1,
						"green_foreground", NULL);

	process_line(fline, mrl, mrn);
}

static void do_purchasing_card(char *fline)
{
	char *pcln[] = { "Record Processed", "Record Type", "Aux Type",
			"Continuation", "Page No.", "VAT Transaction Amount",
			"Customer VAT Reg No.", "Supplier Order Ref", 
			"Order Date", "Discount Amount", "Freight Amount",
			"Dest Post Code", "Ship From Post Code", 
			"Dest Country Code", "Freight VAT Rate", 
			"Transaction VAT Status", "Customer Ref", 
			"Customer Account No.", "Invoice No.", 
			"Original Invoice No.", "Cost Centre", 
			"Total Items Amount", "Filler", "CRLF" };
	int pcl[24][2] = { {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1},
                        {5, 12}, {17, 13}, {30, 12}, {42, 8}, {50, 12},
                        {62, 12}, {74, 10}, {84, 10}, {94, 3}, {97, 4},
                        {101, 1}, {102, 20}, {122, 12}, {134, 15}, {149, 15},
                        {164, 20}, {184, 12}, {196, 102}, {298, 2} };
	
	char hline[35];
	GtkTextBuffer *buffer;

	sprintf(hline, "Line no: %d, Purchasing Card\n", line_no++);
        buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_end_iter(buffer, &iter);
        gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, hline, -1,
                                                "green_foreground", NULL);

	process_line(fline, pcl, pcln);
}

static void do_purchasing_card_item(char *fline)
{
	char *pciln[] = { "Record Processed", "Record Type", "Aux Type",
			"Continuation", "Page No.", "Item No.", "Unit Cost",
			"Unit of Measure", "Commodity Code", "Item Description",
			"Quantity", "VAT Rate", "Line Discount Amount", 
			"Product Code", "Line VAT Amount", "Line Total Amount",
			"VAT Rate Type", "Original Line Total Amount", 
			"Debit/Credit", "Filler", "CRLF" };
	int pcil[21][2] = { {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1},
                        {5, 3}, {8, 12}, {20, 12}, {32, 15}, {47, 40},
                        {87, 12}, {99, 4}, {103, 12}, {115, 12}, {127, 12},
                        {139, 12}, {151, 1}, {152, 12}, {164, 1}, {165, 133},
                        {298, 2} };

	char hline[35];
        GtkTextBuffer *buffer;

	sprintf(hline, "Line no: %d, Purchasing Card Item\n", line_no++);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_end_iter(buffer, &iter);
        gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, hline, -1,
                                                "green_foreground", NULL);

	process_line(fline, pcil, pciln);
}

static void read_file(char *fn)
{
	char fline[300];
	char *bf_map;
	char emesg[255];
	int fd;
	int offset = 0;	
	struct stat st;
	GtkTextBuffer *buffer;
	GtkTextBuffer *buffer_raw;
	GtkTextBuffer *buffer_stats;

	/* Reset global counters and clear the text view */
	line_no = 1;
	line_pos = 0;
	reset_stats();

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
	if ((st.st_size % 300) != 0) {
		sprintf(emesg, 
		"ERROR: Size (%lu) of file (%s) is not a multiple of 300.", 
								st.st_size, fn);
		printf("%s\n", emesg);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
						emesg, -1, "bold", NULL);
		return;
	} else if (st.st_size == 0) {
		sprintf(emesg, "ERROR: File (%s) seems to be empty.", fn);
		printf("%s\n", emesg);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
						emesg, -1, "bold", NULL);
		return;
	}

	/* Open file RO and apply some fadvise hints */
	fd = open(fn, O_RDONLY);
	posix_fadvise(fd, 0, 0, POSIX_FADV_WILLNEED);

	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, 
					"Displaying file: ", -1, "bold", NULL);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, fn, -1, 
					"blue_foreground", "bold", NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

	bf_map = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (bf_map == MAP_FAILED) {
		printf("mmap() failed.\n");
		close(fd);
		exit(1);
	} 

	close(fd);

	while (offset < st.st_size) {
		memcpy(fline, bf_map + offset, 300);
		offset += 300;
		if (DEBUG > 2)
			printf("%c\n", (int)fline[0]);

		/*      
		 * Catch non-printable characters, probably ctrl-z that
		 * has been added to the end of the file by pceft
		 */
		if (!isprint((int)fline[0]))
			continue;

		if (strncmp(fline + 1, "A", 1) == 0) {
			if (DEBUG > 1)
				printf("Doing main record line.\n");

			do_main_record(fline);
		} else if (strncmp(fline + 2, "P", 1) == 0) {
			if (DEBUG > 1)
				printf("Doing purchasing card line.\n");

			do_purchasing_card(fline);
		} else if (strncmp(fline + 2, "I", 1) == 0) {
			if (DEBUG > 1)
				printf("Doing purchasing card item line.\n");

			do_purchasing_card_item(fline);
		}

		if (DEBUG > 1)
			printf("Line length = %d\n", line_pos);

		line_pos = 0;
        }
	
	display_stats();
	
	munmap(bf_map, st.st_size);
}

int main(int argc, char *argv[])
{
	static GtkWidget *window;
	GtkWidget *scrolled_window;
	GtkWidget *scrolled_window_raw;
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkWidget *filemenu;
    	GtkWidget *filemenu_menu;
	GtkWidget *filemenu_new_window;
	GtkWidget *filemenu_open;
	GtkWidget *filemenu_quit;
	GtkWidget *helpmenu;
	GtkWidget *helpmenu_menu;
	GtkWidget *helpmenu_about;
	GtkWidget *separator_menu_item;
	GtkWidget *notebook;
	GtkWidget *split_label;
	GtkWidget *raw_label;
	GtkWidget *stats_label;
	GtkAccelGroup *accel_group;
	PangoFontDescription *font_desc;

	gtk_init(&argc, &argv);
    
	accel_group = gtk_accel_group_new();

	/* Main window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(window), "destroy",
                                                G_CALLBACK(cb_quit), NULL);
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
  	filemenu_new_window = gtk_image_menu_item_new_from_stock("gtk-new", 
								accel_group);
  	/*gtk_widget_show(filemenu_new_window);*/
  	gtk_container_add(GTK_CONTAINER(filemenu_menu), filemenu_new_window);

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
	gtk_container_add(GTK_CONTAINER(notebook), scrolled_window);	
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
	g_signal_connect((gpointer) filemenu_quit, "activate", G_CALLBACK(
						cb_quit), NULL);
	g_signal_connect((gpointer) filemenu_new_window, "activate", G_CALLBACK(
                                                cb_new_window), NULL);
	g_signal_connect((gpointer) filemenu_open, "activate", G_CALLBACK(      				cb_file_selector), (gpointer) text_view);
	g_signal_connect((gpointer) helpmenu_about, "activate", G_CALLBACK(                                 				cb_about_window), NULL);

	/* Change default font throughout the text views */
	font_desc = pango_font_description_from_string("Monospace");
	gtk_widget_modify_font(text_view, font_desc);
	gtk_widget_modify_font(text_view_raw, font_desc);
	gtk_widget_modify_font(text_view_stats, font_desc);
	pango_font_description_free(font_desc);

	gtk_widget_show(window);

	/* If we got a filename as an argument, open that up in the viewer. */
	if (argc > 1)
		read_file(argv[1]);

	gtk_main();
	
	exit(0);
}

