/**************************************************************
 *  Library Management System -  UI Example
 **************************************************************/

 #include <gtk/gtk.h>
 #include <mysql/mysql.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 
 /* ------------------ MySQL Connection Info ------------------ */
 #define DB_HOST     "localhost"
 #define DB_USER     "root"
 #define DB_PASS     "abdoudz1"
 #define DB_NAME     "librarydb"
 #define DB_PORT     3306
 
 /* ------------------ Global Variables ------------------ */
 MYSQL *conn = NULL;
 GtkWidget *books_flowbox;
 
 /* ------------------ Data Structures ------------------ */
 typedef struct {
     int id;
     char title[101];
     char author[101];
     int year;
     int isIssued;
 } Book;
 
 /* ------------------ Prototypes ------------------ */
 MYSQL* connectDB();
 void   closeDB(MYSQL *conn);
 void   createTables(MYSQL *conn);
 
 int    addBookDB(const char *title, const char *author, int year);
 int    updateBookDB(int id, const char *title, const char *author, int year);
 int    deleteBookDB(int id);
 int    issueBookDB(int bookID, const char *borrower);
 int    returnBookByBookID(int bookID);
 GList* getAllBooks();
 void   free_book_list(GList *list);
 
 /* GTK UI */
 static void load_css(void);
 static void activate(GtkApplication *app, gpointer user_data);
 
 void refresh_books_flowbox();
 void show_add_book_dialog(GtkWidget *widget, gpointer data);
 void show_update_book_dialog(GtkWidget *widget, gpointer data);
 void show_issue_book_dialog(GtkWidget *widget, gpointer data);
 void on_delete_book_clicked(GtkWidget *widget, gpointer data);
 static void on_return_book_clicked(GtkWidget *widget, gpointer data);
 
 /* ------------------ Database Functions ------------------ */
 MYSQL* connectDB() {
     MYSQL *c = mysql_init(NULL);
     if (!c) {
         fprintf(stderr, "mysql_init() failed.\n");
         exit(1);
     }
     if (!mysql_real_connect(c, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, NULL, 0)) {
         fprintf(stderr, "Connection error: %s\n", mysql_error(c));
         mysql_close(c);
         exit(1);
     }
     return c;
 }
 
 void closeDB(MYSQL *c) {
     if (c) {
         mysql_close(c);
     }
 }
 
 void createTables(MYSQL *c) {
     const char *createBooksQuery =
         "CREATE TABLE IF NOT EXISTS books ("
         "  id INT PRIMARY KEY AUTO_INCREMENT,"
         "  title VARCHAR(100) NOT NULL,"
         "  author VARCHAR(100) NOT NULL,"
         "  year INT NOT NULL,"
         "  isIssued TINYINT NOT NULL DEFAULT 0"
         ") ENGINE=InnoDB;";
 
     if (mysql_query(c, createBooksQuery)) {
         fprintf(stderr, "Error creating books table: %s\n", mysql_error(c));
     }
 
     const char *createBorrowsQuery =
         "CREATE TABLE IF NOT EXISTS borrows ("
         "  borrowID INT PRIMARY KEY AUTO_INCREMENT,"
         "  bookID INT NOT NULL,"
         "  borrower VARCHAR(100) NOT NULL,"
         "  dateBorrowed DATE NOT NULL,"
         "  dateReturned DATE,"
         "  isReturned TINYINT NOT NULL DEFAULT 0,"
         "  FOREIGN KEY (bookID) REFERENCES books(id) ON DELETE CASCADE"
         ") ENGINE=InnoDB;";
 
     if (mysql_query(c, createBorrowsQuery)) {
         fprintf(stderr, "Error creating borrows table: %s\n", mysql_error(c));
     }
 }
 
 /* CRUD Operations */
 int addBookDB(const char *title, const char *author, int year) {
     char query[512];
     snprintf(query, sizeof(query),
              "INSERT INTO books (title, author, year) VALUES ('%s','%s',%d);",
              title, author, year);
     if (mysql_query(conn, query)) {
         fprintf(stderr, "Error adding book: %s\n", mysql_error(conn));
         return -1;
     }
     return (int)mysql_insert_id(conn);
 }
 
 int updateBookDB(int id, const char *title, const char *author, int year) {
     char query[512];
     snprintf(query, sizeof(query),
              "UPDATE books SET title='%s', author='%s', year=%d WHERE id=%d;",
              title, author, year, id);
     if (mysql_query(conn, query)) {
         fprintf(stderr, "Error updating book: %s\n", mysql_error(conn));
         return 0;
     }
     return (mysql_affected_rows(conn) > 0);
 }
 
 int deleteBookDB(int id) {
     char query[256];
     snprintf(query, sizeof(query), "DELETE FROM books WHERE id=%d;", id);
     if (mysql_query(conn, query)) {
         fprintf(stderr, "Error deleting book: %s\n", mysql_error(conn));
         return 0;
     }
     return (mysql_affected_rows(conn) > 0);
 }
 
 int issueBookDB(int bookID, const char *borrower) {
     char checkQuery[256];
     snprintf(checkQuery, sizeof(checkQuery),
              "SELECT isIssued FROM books WHERE id=%d;", bookID);
     if (mysql_query(conn, checkQuery)) {
         fprintf(stderr, "Error checking book: %s\n", mysql_error(conn));
         return 0;
     }
     MYSQL_RES *res = mysql_store_result(conn);
     if (!res || mysql_num_rows(res) == 0) {
         if (res) mysql_free_result(res);
         return 0;
     }
     MYSQL_ROW row = mysql_fetch_row(res);
     int isIssued = atoi(row[0]);
     mysql_free_result(res);
 
     if (isIssued) {
         return 0; // Already issued
     }
 
     time_t now = time(NULL);
     struct tm *tm_info = localtime(&now);
     char dateBorrowed[11];
     strftime(dateBorrowed, 11, "%Y-%m-%d", tm_info);
 
     char insertQuery[512];
     snprintf(insertQuery, sizeof(insertQuery),
              "INSERT INTO borrows(bookID, borrower, dateBorrowed) "
              "VALUES(%d, '%s', '%s');",
              bookID, borrower, dateBorrowed);
     if (mysql_query(conn, insertQuery)) {
         fprintf(stderr, "Error issuing book: %s\n", mysql_error(conn));
         return 0;
     }
 
     char updateQuery[256];
     snprintf(updateQuery, sizeof(updateQuery),
              "UPDATE books SET isIssued=1 WHERE id=%d;", bookID);
     if (mysql_query(conn, updateQuery)) {
         fprintf(stderr, "Error updating book status: %s\n", mysql_error(conn));
         return 0;
     }
 
     return 1;
 }
 
 int returnBookByBookID(int bookID) {
     char checkQuery[256];
     snprintf(checkQuery, sizeof(checkQuery),
              "SELECT borrowID FROM borrows WHERE bookID=%d AND isReturned=0;", bookID);
     if (mysql_query(conn, checkQuery)) {
         fprintf(stderr, "Error checking borrow: %s\n", mysql_error(conn));
         return 0;
     }
     MYSQL_RES *res = mysql_store_result(conn);
     if (!res || mysql_num_rows(res) == 0) {
         if (res) mysql_free_result(res);
         return 0;
     }
     MYSQL_ROW row = mysql_fetch_row(res);
     int borrowID = atoi(row[0]);
     mysql_free_result(res);
 
     // Mark as returned
     time_t now = time(NULL);
     struct tm *tm_info = localtime(&now);
     char dateReturned[11];
     strftime(dateReturned, 11, "%Y-%m-%d", tm_info);
 
     char updateBorrowQuery[512];
     snprintf(updateBorrowQuery, sizeof(updateBorrowQuery),
              "UPDATE borrows SET isReturned=1, dateReturned='%s' WHERE borrowID=%d;",
              dateReturned, borrowID);
     if (mysql_query(conn, updateBorrowQuery)) {
         fprintf(stderr, "Error updating borrow record: %s\n", mysql_error(conn));
         return 0;
     }
 
     char updateBookQuery[256];
     snprintf(updateBookQuery, sizeof(updateBookQuery),
              "UPDATE books SET isIssued=0 WHERE id=%d;", bookID);
     if (mysql_query(conn, updateBookQuery)) {
         fprintf(stderr, "Error updating book status: %s\n", mysql_error(conn));
         return 0;
     }
     return 1;
 }
 
 GList* getAllBooks() {
     GList *list = NULL;
     const char *query = "SELECT id, title, author, year, isIssued FROM books;";
     if (mysql_query(conn, query)) {
         fprintf(stderr, "Error fetching books: %s\n", mysql_error(conn));
         return list;
     }
     MYSQL_RES *res = mysql_store_result(conn);
     if (!res) return list;
 
     MYSQL_ROW row;
     while ((row = mysql_fetch_row(res))) {
         Book *b = malloc(sizeof(Book));
         b->id = atoi(row[0]);
         strncpy(b->title, row[1], 100);
         b->title[100] = '\0';
         strncpy(b->author, row[2], 100);
         b->author[100] = '\0';
         b->year = atoi(row[3]);
         b->isIssued = atoi(row[4]);
         list = g_list_append(list, b);
     }
     mysql_free_result(res);
     return list;
 }
 
 void free_book_list(GList *list) {
     GList *iter = list;
     while (iter) {
         free(iter->data);
         iter = iter->next;
     }
     g_list_free(list);
 }
 
 /* ------------------ CSS Loading ------------------ */
 static void load_css(void) {
     GtkCssProvider *provider = gtk_css_provider_new();
 
     /* CSS for styling the window, header, buttons, and cards */
     const gchar *css_data =
         "window {\n"
         "  background: #f8f9fa;\n"
         "  font-family: 'Segoe UI', Tahoma, sans-serif;\n"
         "  font-size: 14px;\n"
         "}\n"
 
         /* Header bar styling */
         ".header-bar {\n"
         "  background: #ffffff;\n"
         "  padding: 10px 20px;\n"
         "  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);\n"
         "}\n"
 
         /* Purple button styling (pill shape) */
         ".purple-button {\n"
         "  background-color: #6f42c1;\n"
         "  color: #ffffff;\n"
         "  border: none;\n"
         "  border-radius: 20px;\n"
         "  padding: 8px 16px;\n"
         "  font-weight: 500;\n"
         "}\n"
         ".purple-button:hover {\n"
         "  background-color: #5a359c;\n"
         "}\n"
 
         /* Card styling */
         ".book-card {\n"
         "  background: #ffffff;\n"
         "  border-radius: 8px;\n"
         "  border: 1px solid #dee2e6;\n"
         "  box-shadow: 0 2px 4px rgba(0,0,0,0.05);\n"
         "  padding: 15px;\n"
         "  margin: 10px;\n"
         "  max-width: 280px;\n"
         "}\n"
 
         /* Dialog styling */
         "dialog {\n"
         "  background-color: #ffffff;\n"
         "  border-radius: 8px;\n"
         "  padding: 20px;\n"
         "}\n"
 
         "label {\n"
         "  color: #212529;\n"
         "}\n";
 
     gtk_css_provider_load_from_data(provider, css_data, -1, NULL);
     gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER);
     g_object_unref(provider);
 }
 
 /* ------------------ FlowBox Refresh ------------------ */
 void refresh_books_flowbox() {
     // Remove all children first
     GList *children = gtk_container_get_children(GTK_CONTAINER(books_flowbox));
     for (GList *iter = children; iter; iter = iter->next) {
         gtk_container_remove(GTK_CONTAINER(books_flowbox), GTK_WIDGET(iter->data));
     }
     g_list_free(children);
 
     // Fetch books from DB
     GList *books = getAllBooks();
     for (GList *iter = books; iter; iter = iter->next) {
         Book *b = (Book*)iter->data;
 
         // Card container
         GtkWidget *card = gtk_frame_new(NULL);
         gtk_frame_set_shadow_type(GTK_FRAME(card), GTK_SHADOW_NONE);
         gtk_style_context_add_class(gtk_widget_get_style_context(card), "book-card");
 
         // VBox inside card
         GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
         gtk_container_add(GTK_CONTAINER(card), vbox);
 
         // Title (using allowed Pango markup attributes)
         GtkWidget *lbl_title = gtk_label_new(NULL);
         char title_markup[256];
         snprintf(title_markup, sizeof(title_markup), "<span weight='bold' size='16000'>%s</span>", b->title);
         gtk_label_set_markup(GTK_LABEL(lbl_title), title_markup);
         gtk_box_pack_start(GTK_BOX(vbox), lbl_title, FALSE, FALSE, 0);
 
         // Author
         char author_str[128];
         snprintf(author_str, sizeof(author_str), "by %s", b->author);
         GtkWidget *lbl_author = gtk_label_new(author_str);
         gtk_box_pack_start(GTK_BOX(vbox), lbl_author, FALSE, FALSE, 0);
 
         // Year
         char year_str[64];
         snprintf(year_str, sizeof(year_str), "Year: %d", b->year);
         GtkWidget *lbl_year = gtk_label_new(year_str);
         gtk_box_pack_start(GTK_BOX(vbox), lbl_year, FALSE, FALSE, 0);
 
         // Status
         char status_str[64];
         snprintf(status_str, sizeof(status_str), "Status: %s",
                  (b->isIssued ? "Issued" : "Available"));
         GtkWidget *lbl_status = gtk_label_new(status_str);
         gtk_box_pack_start(GTK_BOX(vbox), lbl_status, FALSE, FALSE, 0);
 
         // Button row
         GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
         gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
 
         // Update button
         GtkWidget *btn_update = gtk_button_new_with_label("Update");
         gtk_style_context_add_class(gtk_widget_get_style_context(btn_update), "purple-button");
         g_object_set_data(G_OBJECT(btn_update), "book_id", GINT_TO_POINTER(b->id));
         g_object_set_data(G_OBJECT(btn_update), "book_title", g_strdup(b->title));
         g_object_set_data(G_OBJECT(btn_update), "book_author", g_strdup(b->author));
         g_object_set_data(G_OBJECT(btn_update), "book_year", GINT_TO_POINTER(b->year));
         g_signal_connect(btn_update, "clicked", G_CALLBACK(show_update_book_dialog), NULL);
         gtk_box_pack_start(GTK_BOX(hbox), btn_update, TRUE, TRUE, 0);
 
         // Delete button
         GtkWidget *btn_delete = gtk_button_new_with_label("Delete");
         gtk_style_context_add_class(gtk_widget_get_style_context(btn_delete), "purple-button");
         g_object_set_data(G_OBJECT(btn_delete), "book_id", GINT_TO_POINTER(b->id));
         g_signal_connect(btn_delete, "clicked", G_CALLBACK(on_delete_book_clicked), NULL);
         gtk_box_pack_start(GTK_BOX(hbox), btn_delete, TRUE, TRUE, 0);
 
         // Issue / Return button
         if (b->isIssued) {
             GtkWidget *btn_return = gtk_button_new_with_label("Return");
             gtk_style_context_add_class(gtk_widget_get_style_context(btn_return), "purple-button");
             g_object_set_data(G_OBJECT(btn_return), "book_id", GINT_TO_POINTER(b->id));
             g_signal_connect(btn_return, "clicked", G_CALLBACK(on_return_book_clicked), NULL);
             gtk_box_pack_start(GTK_BOX(hbox), btn_return, TRUE, TRUE, 0);
         } else {
             GtkWidget *btn_issue = gtk_button_new_with_label("Issue");
             gtk_style_context_add_class(gtk_widget_get_style_context(btn_issue), "purple-button");
             g_object_set_data(G_OBJECT(btn_issue), "book_id", GINT_TO_POINTER(b->id));
             g_signal_connect(btn_issue, "clicked", G_CALLBACK(show_issue_book_dialog), NULL);
             gtk_box_pack_start(GTK_BOX(hbox), btn_issue, TRUE, TRUE, 0);
         }
 
         // Insert card into flowbox
         gtk_flow_box_insert(GTK_FLOW_BOX(books_flowbox), card, -1);
     }
     free_book_list(books);
     gtk_widget_show_all(books_flowbox);
 }
 
 /* ------------------ Dialogs ------------------ */
 void show_add_book_dialog(GtkWidget *widget, gpointer data) {
     GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Book",
             GTK_WINDOW(gtk_widget_get_toplevel(widget)),
             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
             "_Add", GTK_RESPONSE_ACCEPT,
             "_Cancel", GTK_RESPONSE_REJECT,
             NULL);
 
     GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
     GtkWidget *grid = gtk_grid_new();
     gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
     gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
     gtk_container_set_border_width(GTK_CONTAINER(grid), 20);
     gtk_container_add(GTK_CONTAINER(content_area), grid);
 
     GtkWidget *lblTitle  = gtk_label_new("Title:");
     GtkWidget *lblAuthor = gtk_label_new("Author:");
     GtkWidget *lblYear   = gtk_label_new("Year:");
 
     GtkWidget *entryTitle  = gtk_entry_new();
     GtkWidget *entryAuthor = gtk_entry_new();
     GtkWidget *entryYear   = gtk_entry_new();
 
     gtk_grid_attach(GTK_GRID(grid), lblTitle,   0, 0, 1, 1);
     gtk_grid_attach(GTK_GRID(grid), entryTitle, 1, 0, 1, 1);
     gtk_grid_attach(GTK_GRID(grid), lblAuthor,  0, 1, 1, 1);
     gtk_grid_attach(GTK_GRID(grid), entryAuthor,1, 1, 1, 1);
     gtk_grid_attach(GTK_GRID(grid), lblYear,    0, 2, 1, 1);
     gtk_grid_attach(GTK_GRID(grid), entryYear,  1, 2, 1, 1);
 
     gtk_widget_show_all(dialog);
 
     if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
         const char *title  = gtk_entry_get_text(GTK_ENTRY(entryTitle));
         const char *author = gtk_entry_get_text(GTK_ENTRY(entryAuthor));
         int year = atoi(gtk_entry_get_text(GTK_ENTRY(entryYear)));
         if (addBookDB(title, author, year) >= 0) {
             refresh_books_flowbox();
         }
     }
     gtk_widget_destroy(dialog);
 }
 
 void show_update_book_dialog(GtkWidget *widget, gpointer data) {
     int book_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "book_id"));
     const char *old_title  = g_object_get_data(G_OBJECT(widget), "book_title");
     const char *old_author = g_object_get_data(G_OBJECT(widget), "book_author");
     int old_year           = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "book_year"));
 
     GtkWidget *dialog = gtk_dialog_new_with_buttons("Update Book",
             GTK_WINDOW(gtk_widget_get_toplevel(widget)),
             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
             "_Update", GTK_RESPONSE_ACCEPT,
             "_Cancel", GTK_RESPONSE_REJECT,
             NULL);
 
     GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
     GtkWidget *grid = gtk_grid_new();
     gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
     gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
     gtk_container_set_border_width(GTK_CONTAINER(grid), 20);
     gtk_container_add(GTK_CONTAINER(content_area), grid);
 
     GtkWidget *lblTitle  = gtk_label_new("Title:");
     GtkWidget *lblAuthor = gtk_label_new("Author:");
     GtkWidget *lblYear   = gtk_label_new("Year:");
 
     GtkWidget *entryTitle  = gtk_entry_new();
     GtkWidget *entryAuthor = gtk_entry_new();
     GtkWidget *entryYear   = gtk_entry_new();
 
     gtk_entry_set_text(GTK_ENTRY(entryTitle), old_title);
     gtk_entry_set_text(GTK_ENTRY(entryAuthor), old_author);
     char buf[16];
     snprintf(buf, sizeof(buf), "%d", old_year);
     gtk_entry_set_text(GTK_ENTRY(entryYear), buf);
 
     gtk_grid_attach(GTK_GRID(grid), lblTitle,   0, 0, 1, 1);
     gtk_grid_attach(GTK_GRID(grid), entryTitle, 1, 0, 1, 1);
     gtk_grid_attach(GTK_GRID(grid), lblAuthor,  0, 1, 1, 1);
     gtk_grid_attach(GTK_GRID(grid), entryAuthor,1, 1, 1, 1);
     gtk_grid_attach(GTK_GRID(grid), lblYear,    0, 2, 1, 1);
     gtk_grid_attach(GTK_GRID(grid), entryYear,  1, 2, 1, 1);
 
     gtk_widget_show_all(dialog);
 
     if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
         const char *new_title  = gtk_entry_get_text(GTK_ENTRY(entryTitle));
         const char *new_author = gtk_entry_get_text(GTK_ENTRY(entryAuthor));
         int new_year = atoi(gtk_entry_get_text(GTK_ENTRY(entryYear)));
         if (updateBookDB(book_id, new_title, new_author, new_year)) {
             refresh_books_flowbox();
         }
     }
     gtk_widget_destroy(dialog);
 }
 
 void show_issue_book_dialog(GtkWidget *widget, gpointer data) {
     int book_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "book_id"));
 
     GtkWidget *dialog = gtk_dialog_new_with_buttons("Issue Book",
             GTK_WINDOW(gtk_widget_get_toplevel(widget)),
             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
             "_Issue", GTK_RESPONSE_ACCEPT,
             "_Cancel", GTK_RESPONSE_REJECT,
             NULL);
 
     GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
     GtkWidget *grid = gtk_grid_new();
     gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
     gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
     gtk_container_set_border_width(GTK_CONTAINER(grid), 20);
     gtk_container_add(GTK_CONTAINER(content_area), grid);
 
     GtkWidget *lblBorrower = gtk_label_new("Borrower:");
     GtkWidget *entryBorrower = gtk_entry_new();
 
     gtk_grid_attach(GTK_GRID(grid), lblBorrower,   0, 0, 1, 1);
     gtk_grid_attach(GTK_GRID(grid), entryBorrower, 1, 0, 1, 1);
 
     gtk_widget_show_all(dialog);
 
     if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
         const char *borrower = gtk_entry_get_text(GTK_ENTRY(entryBorrower));
         if (issueBookDB(book_id, borrower)) {
             refresh_books_flowbox();
         }
     }
     gtk_widget_destroy(dialog);
 }
 
 /* ------------------ Button Callbacks ------------------ */
 void on_delete_book_clicked(GtkWidget *widget, gpointer data) {
     int book_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "book_id"));
     if (deleteBookDB(book_id)) {
         refresh_books_flowbox();
     }
 }
 
 static void on_return_book_clicked(GtkWidget *widget, gpointer data) {
     int book_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "book_id"));
     if (returnBookByBookID(book_id)) {
         refresh_books_flowbox();
     }
 }
 
 /* ------------------ GTK Application ------------------ */
 static void activate(GtkApplication *app, gpointer user_data) {
     load_css();
 
     GtkWidget *window = gtk_application_window_new(app);
     gtk_window_set_title(GTK_WINDOW(window), "Book Management");
     gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
 
     // Main vertical container
     GtkWidget *vbox_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
     gtk_container_add(GTK_CONTAINER(window), vbox_main);
 
     /* Header bar: a horizontal box with an "Add Book" button on the left and a centered label */
     GtkWidget *header_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
     gtk_style_context_add_class(gtk_widget_get_style_context(header_bar), "header-bar");
     gtk_box_pack_start(GTK_BOX(vbox_main), header_bar, FALSE, FALSE, 0);
 
     // "Add Book" button on the left
     GtkWidget *btn_add = gtk_button_new_with_label("Add Book");
     gtk_style_context_add_class(gtk_widget_get_style_context(btn_add), "purple-button");
     g_signal_connect(btn_add, "clicked", G_CALLBACK(show_add_book_dialog), NULL);
     gtk_box_pack_start(GTK_BOX(header_bar), btn_add, FALSE, FALSE, 0);
 
     // Spacer to center the label
     GtkWidget *header_spacer_left = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
     gtk_box_pack_start(GTK_BOX(header_bar), header_spacer_left, TRUE, TRUE, 0);
 
     // "Book Management" label (using Pango markup without a class attribute)
     GtkWidget *lbl_header = gtk_label_new(NULL);
     gtk_label_set_markup(GTK_LABEL(lbl_header),
                          "<span weight='bold' size='18000'>Book Management</span>");
     gtk_box_pack_start(GTK_BOX(header_bar), lbl_header, FALSE, FALSE, 0);
 
     // Spacer on the right
     GtkWidget *header_spacer_right = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
     gtk_box_pack_start(GTK_BOX(header_bar), header_spacer_right, TRUE, TRUE, 0);
 
     // Scrolled window for the FlowBox
     GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
     gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
     gtk_box_pack_start(GTK_BOX(vbox_main), scrolled, TRUE, TRUE, 0);
 
     // Create FlowBox for book cards
     books_flowbox = gtk_flow_box_new();
     gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(books_flowbox), 10);
     gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(books_flowbox), 10);
     gtk_container_set_border_width(GTK_CONTAINER(books_flowbox), 10);
     gtk_container_add(GTK_CONTAINER(scrolled), books_flowbox);
 
     refresh_books_flowbox();
     gtk_widget_show_all(window);
 }
 
 int main(int argc, char **argv) {
     conn = connectDB();
     createTables(conn);
 
     GtkApplication *app = gtk_application_new("org.example.LibraryApp", G_APPLICATION_DEFAULT_FLAGS);
     g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
 
     int status = g_application_run(G_APPLICATION(app), argc, argv);
     g_object_unref(app);
 
     closeDB(conn);
     return status;
 }