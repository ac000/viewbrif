#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>


void
cb_quit(GtkMenuItem *menuitem, gpointer user_data)
{
	exit(0);
}

void
cb_new_window(GtkMenuItem *menuitem, gpointer user_data)
{
}

static void
read_file(text_view)
{
	int hl[31][2] = { {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1},
                        {5, 8}, {13, 6}, {19, 15}, {34, 8}, {42, 1},
                        {43, 19}, {62, 4}, {66, 4}, {70, 2}, {72, 12},
                        {84, 12}, {96, 40}, {136, 25}, {161, 3}, {164, 1},
                        {165, 1}, {166, 8}, {174, 9}, {183, 3}, {186, 4},
                        {190, 11}, {201, 1}, {202, 8}, {210, 4}, {214, 84},
                        {298, 2} };

	char fline[301], fstatus[20], hline[35];
	char *field;
	FILE *fp;
	int i = 0, lpos = 0, fstart = 0, flen = 0, ln = 1;
	
	GtkTextBuffer *buffer;


	fp = fopen("data/brifn30f.req", "r");

	while (fgets(fline, 301, fp) != NULL) {
		if (strncmp(fline + 1, "A", 1) == 0) {
			sprintf(hline, "Line no: %d, Main Record\n", ln++);
			buffer = gtk_text_view_get_buffer(
                                                GTK_TEXT_VIEW(text_view));
                	gtk_text_buffer_insert_at_cursor(buffer, hline, -1);
			while (strncmp(fline + fstart, "\r\n", 2) != 0) {
				fstart = hl[i][0];
				flen = hl[i][1];

				sprintf(fstatus, "(%d, %d) \tF%d\t= ", 
						fstart + 1, flen, i + 1);
				buffer = gtk_text_view_get_buffer(
						GTK_TEXT_VIEW(text_view));
				gtk_text_buffer_insert_at_cursor(buffer, 
								fstatus, -1);
                        	field = (char *) malloc(sizeof(char) * 
								(flen  + 2));
                        	memset(field, 0, sizeof(char) * (flen  + 2));
                        	strncpy(field, fline + fstart, flen);
				strcat(field, "\n"); 
                        	buffer = gtk_text_view_get_buffer(
						GTK_TEXT_VIEW(text_view));
				gtk_text_buffer_insert_at_cursor(buffer, field,
				 					-1);
				free(field);
				lpos += flen;
				i++;
                	}
        	}
	        
		printf("Line length = %d\n", lpos);
		lpos = i = fstart = flen = 0;
        }

        fclose(fp);
}

static void 
destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}


int 
main(int argc, char *argv[])
{
	static GtkWidget *window;
	GtkWidget *scrolled_window;
	GtkWidget *text_view;
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkWidget *filemenu;
    	GtkWidget *filemenu_menu;
	GtkWidget *filemenu_new_window;
	GtkWidget *filemenu_open;
	GtkWidget *filemenu_quit;
	GtkWidget *separator_menu_item;
	GtkAccelGroup *accel_group;


	gtk_init(&argc, &argv);
    

	accel_group = gtk_accel_group_new();

	/* Main window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(window), "destroy",
                                                G_CALLBACK(destroy), NULL);
  	gtk_window_set_title(GTK_WINDOW(window), "ViewBRIF");	
	gtk_container_set_border_width(GTK_CONTAINER(window), 0);
    	gtk_widget_set_size_request(window, 500, 800);

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
  	gtk_widget_show(filemenu_new_window);
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

		
	/* Create a new scrolled window. */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_window);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 10);

	/* 
	 * The policy is one of GTK_POLICY AUTOMATIC, or GTK_POLICY_ALWAYS.
     	 * GTK_POLICY_AUTOMATIC will automatically decide whether you need
     	 * scrollbars, whereas GTK_POLICY_ALWAYS will always leave the 
	 * scrollbars there. The first one is the horizontal scrollbar, 
	 * the second, the vertical. 
	 */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	

	/* Create a text view object and set it RO with invisible cursor. */	
	text_view = gtk_text_view_new();
	gtk_widget_show(text_view);
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
	
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);		


	/* Menu item callbacks */
	g_signal_connect((gpointer) filemenu_quit, "activate", G_CALLBACK(
						cb_quit), NULL);
	g_signal_connect((gpointer) filemenu_new_window, "activate", G_CALLBACK(
                                                cb_new_window), NULL);



	gtk_widget_show(window);

	read_file(text_view);	


	gtk_main();

	return 0;
}

