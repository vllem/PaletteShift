#include <gtk/gtk.h>
#include <string.h>

static GtkWidget *colors_box;

static gchar* uri_to_filename(const gchar *uri) {
    gchar *filename = NULL;
    if (g_str_has_prefix(uri, "file:///")) {
        filename = g_filename_from_uri(uri, NULL, NULL);
    }
    return filename;
}

static void on_text_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
                         GtkSelectionData *selection_data, guint info, guint time, gpointer user_data) {
    gchar **uris = gtk_selection_data_get_uris(selection_data);
    if (uris != NULL) {
        gchar *filename = uri_to_filename(uris[0]);
        if (filename != NULL) {
            gchar *contents;
            if (g_file_get_contents(filename, &contents, NULL, NULL)) {
                gchar **lines = g_strsplit(contents, "\n", -1);
                
                int num_colors = 0;
                for (int i = 0; lines[i] != NULL; i++) {
                    gchar *line = g_strstrip(lines[i]);
                    if (line[0] != '\0') {
                        num_colors++;
                    }
                }

                if (num_colors > 256) {
                    GList *children = gtk_container_get_children(GTK_CONTAINER(colors_box));
                    for (GList *iter = children; iter != NULL; iter = iter->next) {
                        gtk_widget_destroy(GTK_WIDGET(iter->data));
                    }
                    g_list_free(children);

                    GtkWidget *warning_label = gtk_label_new("Your palette has too many colors");
                    gtk_container_add(GTK_CONTAINER(colors_box), warning_label);
                    g_print("Your palette has too many colors\n");
                } else {
                    GList *children = gtk_container_get_children(GTK_CONTAINER(colors_box));
                    for (GList *iter = children; iter != NULL; iter = iter->next) {
                        gtk_widget_destroy(GTK_WIDGET(iter->data));
                    }
                    g_list_free(children);

                    for (int i = 0; lines[i] != NULL; i++) {
                        gchar *line = g_strstrip(lines[i]);
                        if (line[0] != '\0') {
                            GtkWidget *color_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
                            GtkWidget *color_button = gtk_button_new();
                            gtk_widget_set_size_request(color_button, 20, 20);

                            gchar *color_code;
                            if (line[0] != '#') {
                                color_code = g_strdup_printf("#%s", line);
                            } else {
                                color_code = g_strdup(line);
                            }

                            gchar *css = g_strdup_printf("button { background-color: %s; border: none; }", color_code);
                            GtkCssProvider *css_provider = gtk_css_provider_new();
                            gtk_css_provider_load_from_data(css_provider, css, -1, NULL);
                            GtkStyleContext *style_context = gtk_widget_get_style_context(color_button);
                            gtk_style_context_add_provider(style_context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
                            g_object_unref(css_provider);
                            g_free(css);

                            gtk_box_pack_start(GTK_BOX(color_box), color_button, TRUE, TRUE, 0);
                            gtk_container_add(GTK_CONTAINER(colors_box), color_box);

                            g_free(color_code);
                        }
                    }
                    gtk_widget_show_all(colors_box);
                }
                
                gtk_widget_show_all(GTK_WIDGET(gtk_widget_get_parent(colors_box)));
                g_strfreev(lines);
                g_free(contents);
            }
            g_free(filename);
        }
        g_strfreev(uris);
    }
    gtk_drag_finish(context, TRUE, FALSE, time);
}

static void load_and_display_image(GtkWidget *image_widget, const gchar *filename) {
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    if (pixbuf) {
        gint width = gdk_pixbuf_get_width(pixbuf);
        gint height = gdk_pixbuf_get_height(pixbuf);

        gint max_width = 400; 
        gint max_height = 300;
        gdouble aspect_ratio = (gdouble)width / (gdouble)height;

        if (width > max_width) {
            width = max_width;
            height = (gint)(width / aspect_ratio);
        }
        if (height > max_height) {
            height = max_height;
            width = (gint)(height * aspect_ratio);
        }

        GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, width, height, GDK_INTERP_BILINEAR);

        gtk_image_set_from_pixbuf(GTK_IMAGE(image_widget), scaled_pixbuf);

        g_object_unref(pixbuf);
        g_object_unref(scaled_pixbuf);
    }
}

static void on_image_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
                          GtkSelectionData *selection_data, guint info, guint time, gpointer user_data) {
    gchar **uris = gtk_selection_data_get_uris(selection_data);
    if (uris != NULL) {
        gchar *filename = uri_to_filename(uris[0]);
        if (filename != NULL) {
            load_and_display_image(widget, filename);
            g_free(filename);
        }
        g_strfreev(uris);
    }
    gtk_drag_finish(context, TRUE, FALSE, time);
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    const gchar *css = 
        "box, label {"
        "  border: 1px solid #333333;"
        "  margin: 2px;"
        "  padding: 2px;"
        "}";

    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(css_provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);


    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, 200, -1);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(main_box), scrolled_window, FALSE, TRUE, 0);

    colors_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(scrolled_window), colors_box);

    GtkWidget *image_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *image = gtk_image_new();
    GtkTargetEntry targets[] = {{"text/uri-list", 0, 0}};
    gtk_drag_dest_set(image, GTK_DEST_DEFAULT_ALL, targets, G_N_ELEMENTS(targets), GDK_ACTION_COPY);
    g_signal_connect(image, "drag-data-received", G_CALLBACK(on_image_drop), NULL);

    gtk_box_pack_start(GTK_BOX(image_vbox), image, TRUE, TRUE, 0);

    GtkWidget *placeholder_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *placeholder_label = gtk_label_new("Placeholder");
    gtk_widget_set_halign(placeholder_label, GTK_ALIGN_START); // Align horizontally to start
    gtk_widget_set_valign(placeholder_label, GTK_ALIGN_START); // Align vertically to start
    gtk_widget_set_margin_top(placeholder_label, 0); // Top margin
    gtk_widget_set_margin_start(placeholder_label, 0); // Left margin (start for RTL compatibility)
    gtk_box_pack_start(GTK_BOX(placeholder_box), placeholder_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(image_vbox), placeholder_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), image_vbox, TRUE, TRUE, 0);

    gtk_drag_dest_set(colors_box, GTK_DEST_DEFAULT_ALL, targets, G_N_ELEMENTS(targets), GDK_ACTION_COPY);
    g_signal_connect(colors_box, "drag-data-received", G_CALLBACK(on_text_drop), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

