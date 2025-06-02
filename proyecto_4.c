/*
    Proyecto 4: Índice de Poder de Banzhaf
    Hecho por: Carmen Hidalgo Paz, Jorge Guevara Chavarría y Ricardo Castro Jiménez
    Fecha: Martes 3 de junio del 2025
*/

#include <gtk/gtk.h>
#include <cairo.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include <glib.h>
#include "Widgets.h"
#include "Backtracking.h"

#define _USE_MATH_DEFINES

// Colores iniciales para los votantes
static const char *default_colors[12] = {
    "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd",
    "#8c564b", "#e377c2", "#f5c211", "#195903", "#17becf",
    "#aec7e8", "#ffbb78"
};

// Para que no se mueva la línea del panel
void fijar_panel(GtkPaned *panel, GParamSpec *pspec, gpointer user_data) {
    const int pos_fijada = 920;
    int current_pos = gtk_paned_get_position(panel);
    if (current_pos != pos_fijada) {
        gtk_paned_set_position(panel, pos_fijada);
    }
}

// Cambiar el label con la información del modelo
static void update_model_label(AppWidgets *w)
{
    int K = gtk_spin_button_get_value_as_int(w->spin_w);

    int n = 0;
    for (; n < 12; n++) {
        if (!w->spin_ai[n]) break;
    }

    GString *names = g_string_new(NULL);
    for (int i = 0; i < n; i++) {
        if (i) g_string_append(names, ", ");
        g_string_append(names, w->voter_names[i]);
    }

    GString *values = g_string_new(NULL);
    for (int i = 0; i < n; i++) {
        int v = gtk_spin_button_get_value_as_int(w->spin_ai[i]);
        if (i) g_string_append(values, ", ");
        g_string_append_printf(values, "%d", v);
    }

    char *text = g_strdup_printf(
        "Modelo (%d; %s): (%d; %s)",
        K,
        names->str,
        K,
        values->str
    );

    gtk_label_set_text(w->lbl_model, text);

    g_free(text);
    g_string_free(names, TRUE);
    g_string_free(values, TRUE);
}

static void on_data_changed(GtkWidget *widget, gpointer user_data) {
    AppWidgets *w = user_data;
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_bar));
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_parliament));
    update_model_label(w);
}

static void on_color_changed(GtkColorButton *btn, gpointer user_data) {
  AppWidgets *w = user_data;
  int i = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "voter-index"));
  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(btn), &w->last_color[i]);
  gtk_widget_queue_draw(GTK_WIDGET(w->drawing_bar));
}

static void on_solution_selected(GtkListBox *box, GtkListBoxRow *row, gpointer data) {
    AppWidgets *w = data;
    if (row) {
        w->selected_mask = g_object_get_data(G_OBJECT(row), "mask");
    } else {
        w->selected_mask = NULL;
    }
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_parliament));
}

static void on_voter_name_changed(GtkEntry *entry, gpointer user_data) {
    AppWidgets *w = user_data;
    int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "voter-index"));
    g_free(w->voter_names[idx]);
    w->voter_names[idx] = g_strdup(gtk_entry_get_text(entry));
    update_model_label(w);
}

static gboolean on_draw_bar(GtkDrawingArea *area, cairo_t *cr, gpointer user_data) {
    AppWidgets *w = user_data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
    int W = alloc.width;
    int H = alloc.height;

    int n = 0;
    while (n < 12 && w->spin_ai[n]) n++;

    int total = 0;
    int votes[12];
    for (int i = 0; i < n; i++) {
        votes[i] = gtk_spin_button_get_value_as_int(w->spin_ai[i]);
        total += votes[i];
    }
    if (total == 0) return FALSE;

    int x = 0;
    for (int i = 0; i < n; i++) {
        double frac = (double)votes[i] / total;
        int slice_w = frac * W;

        GdkRGBA col;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w->colorbtn[i]), &col);
        cairo_set_source_rgba(cr, col.red, col.green, col.blue, col.alpha);

        cairo_rectangle(cr, x, 0, slice_w, H);
        cairo_fill(cr);

        x += slice_w;
    }

    return FALSE;
}

static gboolean on_draw_parliament(GtkDrawingArea *area, cairo_t *cr, gpointer user_data) {
    AppWidgets *w = user_data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
    double W = alloc.width, H = alloc.height;

    gboolean *sel = w->selected_mask;

    GdkRGBA white = {1.0, 1.0, 1.0, 1.0};

    int votes[12], n = 0, total = 0;
    while (n < 12 && w->spin_ai[n]) {
        votes[n] = gtk_spin_button_get_value_as_int(w->spin_ai[n]);
        total += votes[n];
        n++;
    }
    if (n == 0 || total == 0) return FALSE;

    const double margin = 20.0;
    const double cx = W / 2.0;
    const double cy = H - margin;

    double dr_logic = MIN(W, H) * 0.02;
    double row_gap = dr_logic * 2.4;
    double r_base = MIN(cx, cy - dr_logic) - dr_logic;
    double dr_draw = dr_logic * 0.6;

    double angle_cursor = M_PI;
    for (int i = 0; i < n; i++) {
        int seats = votes[i];
        if (seats <= 0) continue;

        double ang_span = M_PI * seats / total;
        double ang_start = angle_cursor;
        double ang_end = angle_cursor - ang_span;
        angle_cursor = ang_end;

        int remaining = seats;
        double r = r_base;
        while (remaining > 0 && r >= dr_logic) {
            int fit = (int)floor((r * ang_span) / (2.0 * dr_logic)) + 1;
            if (fit < 1) fit = 1;
            int draw_count = remaining < fit ? remaining : fit;

            for (int k = 0; k < draw_count; k++) {
                double step = ang_span / (double)draw_count;
                double t = ang_start - (k + 0.5) * step;

                double x = cx + r * cos(t);
                double y = cy - r * sin(t);

                GdkRGBA col;
                if (sel && !sel[i]) {
                    col = white;
                } else {
                    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w->colorbtn[i]), &col);
                }
                cairo_set_source_rgba(cr, col.red, col.green, col.blue, col.alpha);

                cairo_arc(cr, x, y, dr_draw, 0, 2 * M_PI);
                cairo_fill(cr);
            }

            remaining -= draw_count;
            r -= row_gap;
        }
    }

    return FALSE;
}

static int get_min_ai(AppWidgets *w) {
    GList *kids = gtk_container_get_children(GTK_CONTAINER(w->box_ai));
    int min_ai = INT_MAX;
    for (GList *iter = kids; iter; iter = iter->next) {
        GtkWidget *hbox = GTK_WIDGET(iter->data);
        GList *hc = gtk_container_get_children(GTK_CONTAINER(hbox));
        for (GList *hiter = hc; hiter; hiter = hiter->next) {
            if (GTK_IS_SPIN_BUTTON(hiter->data)) {
                int val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(hiter->data));
                if (val < min_ai) min_ai = val;
            }
        }
        g_list_free(hc);
    }
    g_list_free(kids);
    return (min_ai == INT_MAX) ? 1 : min_ai;
}

static void on_prev_ai_changed(GtkSpinButton *spin_prev, gpointer user_data) {
    GtkSpinButton *spin_next = GTK_SPIN_BUTTON(user_data);
    int val_prev = gtk_spin_button_get_value_as_int(spin_prev);

    GtkAdjustment *adj_next = gtk_spin_button_get_adjustment(spin_next);
    gtk_adjustment_set_lower(adj_next, val_prev);

    int val_next = gtk_spin_button_get_value_as_int(spin_next);
    if (val_next < val_prev)
        gtk_spin_button_set_value(spin_next, val_prev);
}

static void on_size_value_changed(GtkSpinButton *spin, gpointer user_data) {
    AppWidgets *w = user_data;
    int n = gtk_spin_button_get_value_as_int(spin);
    if (n < 3) n = 3;
    if (n > 12) n = 12;

    for (int i = 0; i < 12; i++) {
        if (w->colorbtn[i]) {
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w->colorbtn[i]), &w->last_color[i]);
        }
    }

    GList *kids = gtk_container_get_children(GTK_CONTAINER(w->box_ai));
    for (GList *it = kids; it; it = it->next)
        gtk_widget_destroy(GTK_WIDGET(it->data));
    g_list_free(kids);

    for (int i = 0; i < 12; i++) {
        w->spin_ai[i] = NULL;
        w->colorbtn[i] = NULL;
        w->lbl_ibp[i] = NULL;
    }

    GPtrArray *spins = g_ptr_array_new();
    for (int i = 0; i < n; i++) {
        GtkAdjustment *adj = gtk_adjustment_new(1, 1, G_MAXINT, 1, 10, 0);
        GtkWidget *spin_ai = gtk_spin_button_new(adj, 1, 0);
        w->spin_ai[i] = GTK_SPIN_BUTTON(spin_ai);

        g_signal_connect(spin_ai, "value-changed", G_CALLBACK(on_data_changed), w);
        g_ptr_array_add(spins, spin_ai);

        char buf[16];
        g_snprintf(buf, sizeof(buf), "a%d", i + 1);

        GtkWidget *entry = gtk_entry_new();
        w->entry_name[i] = GTK_ENTRY(entry);

        if (w->voter_names[i]) {
            gtk_entry_set_text(GTK_ENTRY(entry), w->voter_names[i]);
        } else {
            w->voter_names[i] = g_strdup(buf);
            gtk_entry_set_text(GTK_ENTRY(entry), buf);
        }

        gtk_entry_set_width_chars(GTK_ENTRY(entry), 6);
        gtk_widget_set_hexpand(entry, FALSE);

        g_object_set_data(G_OBJECT(entry), "voter-index", GINT_TO_POINTER(i));
        g_signal_connect(entry, "changed", G_CALLBACK(on_voter_name_changed), w);
        g_signal_connect(spin_ai, "value-changed", G_CALLBACK(on_data_changed), w);
        g_signal_connect(w->spin_w, "value-changed", G_CALLBACK(on_data_changed), w);

        GtkWidget *colon = gtk_label_new(":");

        GtkWidget *cb = gtk_color_button_new();
        w->colorbtn[i] = GTK_COLOR_BUTTON(cb);
        g_object_set_data(G_OBJECT(cb), "voter-index", GINT_TO_POINTER(i));
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(cb), &w->last_color[i]);
        g_signal_connect(cb, "color-set", G_CALLBACK(on_color_changed), w);
        g_signal_connect(cb, "color-set", G_CALLBACK(on_data_changed), w);

        GtkWidget *cell_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), colon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), spin_ai, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);

        char ibp_txt[32];
        g_snprintf(ibp_txt, sizeof(ibp_txt), "IBP: —");
        GtkWidget *lbl = gtk_label_new(ibp_txt);
        w->lbl_ibp[i] = GTK_LABEL(lbl);

        gtk_box_pack_start(GTK_BOX(cell_vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(cell_vbox), lbl, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(w->box_ai), cell_vbox, FALSE, FALSE, 2);
        gtk_widget_show_all(cell_vbox);
    }

    for (guint i = 0; i + 1 < spins->len; i++) {
        GtkSpinButton *prev = g_ptr_array_index(spins, i);
        GtkSpinButton *next = g_ptr_array_index(spins, i + 1);
        g_signal_connect(prev, "value-changed", G_CALLBACK(on_prev_ai_changed), next);
    }
    g_ptr_array_free(spins, TRUE);

    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_bar));
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_parliament));
    update_model_label(w);
}

static void on_execute_clicked(GtkButton *btn, gpointer data) {
    AppWidgets *w = data;

    GList *rows = gtk_container_get_children(GTK_CONTAINER(w->box_ai));
    int n = g_list_length(rows);
    int *A = malloc(sizeof(int) * n);

    int i = 0;
    for (GList *r = rows; r; r = r->next, i++) {
        GtkWidget *cell_vbox = GTK_WIDGET(r->data);

        GList *vbox_kids = gtk_container_get_children(GTK_CONTAINER(cell_vbox));
        GtkWidget *hbox = GTK_WIDGET(vbox_kids->data);
        g_list_free(vbox_kids);

        GList *hbox_kids = gtk_container_get_children(GTK_CONTAINER(hbox));
        GtkSpinButton *sp = GTK_SPIN_BUTTON(g_list_nth_data(hbox_kids, 2));
        g_list_free(hbox_kids);

        A[i] = gtk_spin_button_get_value_as_int(sp);
    }

    g_list_free(rows);

    int W = gtk_spin_button_get_value_as_int(w->spin_w);

    if (suffix_sum) {
        free(suffix_sum);
        suffix_sum = NULL;
    }
    suffix_sum = malloc(sizeof(int) * (n + 1));
    suffix_sum[n] = 0;
    for (int i = n - 1; i >= 0; --i) {
        suffix_sum[i] = A[i] + suffix_sum[i + 1];
    }

    GPtrArray *sol_list = g_ptr_array_new_with_free_func(g_free);
    int actual_idx[12];

    sumaSubconjuntosV3_collect(A, n, W, 0, actual_idx, 0, 0, sol_list);

    // Aquí contamos los votos críticos
    int votos_criticos[12];
    for (int i = 0; i < 12; i++)
        votos_criticos[i] = 0;

    int total_votos_criticos = 0;

    // Limpiar viejas filas de resultados
    GList *old_rows = gtk_container_get_children(GTK_CONTAINER(w->box_results));
    for (GList *r = old_rows; r; r = r->next)
        gtk_widget_destroy(GTK_WIDGET(r->data));
    g_list_free(old_rows);

    for (guint s = 0; s < sol_list->len; s++) {
        gboolean *mask = g_ptr_array_index(sol_list, s);

        // Calcular suma de la coalición
        int suma_coal = 0;
        for (int i = 0; i < n; i++) {
            if (mask[i]) suma_coal += A[i];
        }

        // Revisar votos críticos
        for (int i = 0; i < n; i++) {
            if (mask[i]) {
                int suma_sin_i = suma_coal - A[i];
                if (suma_sin_i < W) {
                    votos_criticos[i]++;
                    total_votos_criticos++;
                }
            }
        }

        gboolean *mask_copy = g_new(gboolean, n);
        memcpy(mask_copy, mask, sizeof(gboolean) * n);

        GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

        int suma = 0;
        for (int i = 0; i < n; i++) {
            if (mask_copy[i]) suma += A[i];

            GtkWidget *chk = gtk_check_button_new();
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk), mask_copy[i]);
            gtk_widget_set_sensitive(chk, FALSE);
            gtk_box_pack_start(GTK_BOX(hbox), chk, FALSE, FALSE, 0);
        }

        GtkWidget *lbl_sum = gtk_label_new(g_strdup_printf(" Suma = %d", suma));
        gtk_box_pack_end(GTK_BOX(hbox), lbl_sum, FALSE, FALSE, 4);

        g_object_set_data_full(G_OBJECT(row), "mask", mask_copy, g_free);

        gtk_list_box_insert(GTK_LIST_BOX(w->box_results), GTK_WIDGET(row), -1);
        gtk_widget_show_all(GTK_WIDGET(row));
    }

    // Actualizar labels de IBP con resultados
    if (total_votos_criticos > 0) {
        for (int i = 0; i < n; i++) {
            double ipb_real = (double)votos_criticos[i] / total_votos_criticos;
            char ipb_text[64];
            snprintf(ipb_text, sizeof(ipb_text), "IBP: %.4f (%d/%d)", ipb_real, votos_criticos[i], total_votos_criticos);
            gtk_label_set_text(w->lbl_ibp[i], ipb_text);
        }
    } else {
        // Si no hay votos críticos (caso raro), mostrar —
        for (int i = 0; i < n; i++) {
            gtk_label_set_text(w->lbl_ibp[i], "IBP: —");
        }
    }

    free(A);
    free(suffix_sum);
    suffix_sum = NULL;
    g_ptr_array_free(sol_list, TRUE);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    const gchar *scroll_css =
    ".white-bg {\n"
    "  background-image: none;\n"
    "  background-color: #FFFFFF;\n"
    "}\n"
    ".white-bg label {\n"
    "  color: #000000;\n"
    "}\n";

    GtkCssProvider *scroll_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(scroll_provider, scroll_css, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(scroll_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(scroll_provider);

    GtkBuilder *builder = gtk_builder_new_from_file("interfaz.glade");
    AppWidgets *w = g_new0(AppWidgets, 1);

    w->box_ai = GTK_BOX(gtk_builder_get_object(builder, "box_ai"));
    for (int i = 0; i < 12; i++) {
        gdk_rgba_parse(&w->last_color[i], default_colors[i]);
    }

    GtkSpinButton *spin_size = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "size"));
    w->spin_w = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spin_w"));

    for (int i = 0; i < 12; i++) {
        w->entry_name[i] = NULL;
        w->voter_names[i] = NULL;
    }
    w->selected_mask = NULL;

    w->lbl_model = GTK_LABEL(gtk_builder_get_object(builder, "lbl_model"));
    w->box_results = GTK_LIST_BOX(gtk_builder_get_object(builder, "list_solutions"));
    g_return_val_if_fail(GTK_IS_LIST_BOX(w->box_results), 1);
    g_signal_connect(w->box_results, "row-selected", G_CALLBACK(on_solution_selected), w);

    w->btn_execute = GTK_BUTTON(gtk_builder_get_object(builder, "btn_execute"));
    GtkButton *btn_exit = GTK_BUTTON(gtk_builder_get_object(builder, "buttonFinish"));

    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(w->box_results), TRUE);

    g_signal_connect(spin_size, "value-changed", G_CALLBACK(on_size_value_changed), w);
    g_signal_connect(w->btn_execute, "clicked", G_CALLBACK(on_execute_clicked), w);
    g_signal_connect(btn_exit, "clicked", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *panel = GTK_WIDGET(gtk_builder_get_object(builder, "division"));
    g_signal_connect(panel, "notify::position", G_CALLBACK(fijar_panel), NULL);

    w->drawing_bar = GTK_DRAWING_AREA(gtk_builder_get_object(builder, "drawing_bar"));
    g_signal_connect(w->drawing_bar, "draw", G_CALLBACK(on_draw_bar), w);

    w->drawing_parliament = GTK_DRAWING_AREA(gtk_builder_get_object(builder, "drawing_parliament"));
    g_signal_connect(w->drawing_parliament, "draw", G_CALLBACK(on_draw_parliament), w);

    w->selected_mask = NULL;

    on_size_value_changed(spin_size, w);

    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "ventana"));
    gtk_widget_show_all(window);
    gtk_window_fullscreen(GTK_WINDOW(window));

    gtk_main();

    g_free(w);
    g_object_unref(builder);
    return 0;
}
