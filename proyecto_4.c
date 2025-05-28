/*
                Proyecto 3: Suma de Subconjuntos
                Hecho por: Carmen Hidalgo Paz, Jorge Guevara Chavarría y Ricardo Castro Jiménez
                Fecha: Jueves 15 de mayo del 2025

                Esta sección contiene el main, donde se indica lo que tiene que hacer
                cada objeto mostrado en la interfaz. Además se tienen funciones para
                verificar el valor delta, modificar la cantidad de números del conjunto
                que desea el usuario y lo que ocurre cuando se presiona el botón ejecutar.
*/
#include <gtk/gtk.h>
#include <cairo.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include <glib.h>
#include "Widgets.h"
#include "Variantes.h"

#define _USE_MATH_DEFINES

// Contadores
int nodos = 0;
int soluciones = 0;
int delta = 0;

static const char *default_colors[12] = {
    "#1f77b4", // blue
    "#ff7f0e", // orange
    "#2ca02c", // green
    "#d62728", // red
    "#9467bd", // purple
    "#8c564b", // brown
    "#e377c2", // pink
    "#f5c211", // yellow
    "#195903", // dark green
    "#17becf", // cyan
    "#aec7e8", // light‐blue
    "#ffbb78"  // light‐orange
};

// Para que no se mueva la línea del panel
void fijar_panel(GtkPaned *panel, GParamSpec *pspec, gpointer user_data) {
    // Posición donde se fija la división
    const int pos_fijada = 920;
    int current_pos = gtk_paned_get_position(panel);
    if (current_pos != pos_fijada) {
        gtk_paned_set_position(panel, pos_fijada);
    }
}
static void on_data_changed(GtkWidget *widget, gpointer user_data) {
    AppWidgets *w = user_data;
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_bar));
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_parliament));
}
static void on_color_changed(GtkColorButton *btn, gpointer user_data) {
  AppWidgets *w = user_data;
  int i = GPOINTER_TO_INT(
            g_object_get_data(G_OBJECT(btn), "voter-index"));
  gtk_color_chooser_get_rgba(
      GTK_COLOR_CHOOSER(btn),
      &w->last_color[i]);
  gtk_widget_queue_draw(GTK_WIDGET(w->drawing_bar));
}
static void
on_solution_selected(GtkListBox   *box,
                     GtkListBoxRow*row,
                     gpointer      data)
{
    AppWidgets *w = data;

    if (row) {
        // get the mask we stored on this row
        w->selected_mask =
          g_object_get_data(G_OBJECT(row), "mask");
    }
    else {
        w->selected_mask = NULL;
    }

    // force a redraw of the drawing area
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_parliament));
}
static gboolean on_draw_bar(GtkDrawingArea *area, cairo_t *cr, gpointer user_data) {
    AppWidgets *w = user_data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
    int W = alloc.width;
    int H = alloc.height;

    /* how many voters are active? */
    /* if you track `n` globally or in w, grab that; otherwise
       you can scan spin_ai[] until NULL */
    int n = 0;
    while (n < 12 && w->spin_ai[n]) n++;

    /* read all vote‐counts and total them */
    int total = 0;
    int votes[12];
    for (int i = 0; i < n; i++) {
        votes[i] = gtk_spin_button_get_value_as_int(w->spin_ai[i]);
        total += votes[i];
    }
    if (total == 0) return FALSE;  /* nothing to draw */

    /* draw each slice */
    int x = 0;
    for (int i = 0; i < n; i++) {
        double frac = (double)votes[i] / total;
        int slice_w = frac * W;

        /* pick up the voter’s color */
        GdkRGBA col;
        gtk_color_chooser_get_rgba(
            GTK_COLOR_CHOOSER(w->colorbtn[i]), &col);
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

    // grab the user’s current row‐selection mask
    gboolean *sel = w->selected_mask;

    // define a uniform “disabled” gray
    GdkRGBA gray = { .red   = 1.0,
                     .green = 1.0,
                     .blue  = 1.0,
                     .alpha = 1.0 };

    // 1) Read votes & total them
    int votes[12], n = 0, total = 0;
    while (n < 12 && w->spin_ai[n]) {
        votes[n] = gtk_spin_button_get_value_as_int(w->spin_ai[n]);
        total   += votes[n];
        n++;
    }
    if (n == 0 || total == 0) return FALSE;

    // 2) Base geometry
    const double margin  = 20.0;
    const double cx      = W/2.0;
    const double cy      = H - margin;

    // logical radius used for all spacing/fit calculations:
    double dr_logic      = MIN(W, H) * 0.02;
    double row_gap       = dr_logic * 2.4;
    double r_base        = MIN(cx, cy - dr_logic) - dr_logic;

    // actual draw radius (e.g. 60% of logical):
    double dr_draw       = dr_logic * 0.6;

    // 3) For each voter, carve out its angular slice and fill ring by ring
    double angle_cursor = M_PI;  // start at leftmost
    for (int i = 0; i < n; i++) {
        int seats = votes[i];
        if (seats <= 0) continue;

        // 3a) compute wedge angles
        double ang_span  = M_PI * seats / total;
        double ang_start = angle_cursor;
        double ang_end   = angle_cursor - ang_span;
        angle_cursor     = ang_end; // for the next wedge

        // 3b) fill this wedge in concentric rings
        int  remaining = seats;
        double r       = r_base;
        while (remaining > 0 && r >= dr_logic) {
            // ==== DYNAMIC per‐ring capacity ====
            // how many can fit on this ring?
            int fit = (int)floor((r * ang_span) / (2.0 * dr_logic)) + 1;
            if (fit < 1) fit = 1;
            int draw_count = remaining < fit ? remaining : fit;

            // draw exactly draw_count dots
            for (int k = 0; k < draw_count; k++) {
                // space them in the *interior* of the slice:
                double step = ang_span / (double)draw_count;
                double t    = ang_start - (k + 0.5) * step;

                double x = cx + r * cos(t);
                double y = cy - r * sin(t);

                // pick this voter’s color
                GdkRGBA col;
                if (sel && !sel[i]) {
                    col = gray;
                } else {
                    gtk_color_chooser_get_rgba(
                    GTK_COLOR_CHOOSER(w->colorbtn[i]),
                    &col);
                }
                cairo_set_source_rgba(cr,
                                    col.red,
                                    col.green,
                                    col.blue,
                                    col.alpha);

                cairo_arc(cr, x, y, dr_draw, 0, 2*M_PI);
                cairo_fill(cr);
            }

            remaining -= draw_count;
            r         -= row_gap;
        }
    }

    return FALSE;
}
// Recorre box_ai para encontrar el mínimo de todos los ai
static int get_min_ai(AppWidgets *w) {
    GList *kids = gtk_container_get_children(GTK_CONTAINER(w->box_ai));
    int min_ai = INT_MAX;
    // Loop que recorre la lista de los ai para encontrar el más pequeño
    for (GList *iter = kids; iter; iter = iter->next) {
        GtkWidget *hbox = GTK_WIDGET(iter->data);
        GList *hc = gtk_container_get_children(GTK_CONTAINER(hbox));
        for (GList *hiter = hc; hiter; hiter = hiter->next) {
            if (GTK_IS_SPIN_BUTTON(hiter->data)) {
                int val = gtk_spin_button_get_value_as_int(
                              GTK_SPIN_BUTTON(hiter->data));
                if (val < min_ai) min_ai = val;
            }
        }
        g_list_free(hc);
    }
    g_list_free(kids);
    return (min_ai == INT_MAX) ? 1 : min_ai;
}
// Al cambiar el valor de un ai, fuerza que el siguiente valor de
// ai sea igual o mayor para que el orden sea de menor a mayor
static void on_prev_ai_changed(GtkSpinButton *spin_prev, gpointer user_data) {
    GtkSpinButton *spin_next = GTK_SPIN_BUTTON(user_data);
    int val_prev = gtk_spin_button_get_value_as_int(spin_prev);

    // Ajuste del límite inferior de spin_next
    GtkAdjustment *adj_next = gtk_spin_button_get_adjustment(spin_next);
    gtk_adjustment_set_lower(adj_next, val_prev);

    // Si el valor actual de spin_next es menor, se cambia
    int val_next = gtk_spin_button_get_value_as_int(spin_next);
    if (val_next < val_prev)
        gtk_spin_button_set_value(spin_next, val_prev);
}
// Cuando el usuario cambia la cantidad de ais que quiere
static void on_size_value_changed(GtkSpinButton *spin, gpointer user_data) {
    AppWidgets *w = user_data;
    int n = gtk_spin_button_get_value_as_int(spin);
    if (n < 3)  n = 3;
    if (n > 12) n = 12;

    // 0) Harvest any existing color‐button values:
    for (int i = 0; i < 12; i++) {
        if (w->colorbtn[i]) {
            gtk_color_chooser_get_rgba(
                GTK_COLOR_CHOOSER(w->colorbtn[i]),
                &w->last_color[i]);
        }
    }

    /*— 1) Destroy old rows —*/
    GList *kids = gtk_container_get_children(GTK_CONTAINER(w->box_ai));
    for (GList *it = kids; it; it = it->next)
        gtk_widget_destroy(GTK_WIDGET(it->data));
    g_list_free(kids);

    /*— 2) Clear out any stale pointers —*/
    for (int i = 0; i < 12; i++) {
        w->spin_ai[i]   = NULL;
        w->colorbtn[i]  = NULL;
        w->lbl_ibp[i]   = NULL;
    }

    /*— 3) Build new rows —*/
    GPtrArray *spins = g_ptr_array_new();
    for (int i = 0; i < n; i++) {
        /* a) the vote‐count spin as before */
        GtkAdjustment *adj = gtk_adjustment_new(1, 1, G_MAXINT, 1, 10, 0);
        GtkWidget    *spin_ai = gtk_spin_button_new(adj, 1, 0);

        /* ← NEW: remember it in your struct */
        w->spin_ai[i] = GTK_SPIN_BUTTON(spin_ai);

        /* pack label + spin */
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        char buf[16];
        g_snprintf(buf, sizeof(buf), "a%d:", i+1);
        gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(buf), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), spin_ai, FALSE, FALSE, 0);

        /* recálculo de ais */
        g_signal_connect(spin_ai, "value-changed", G_CALLBACK(on_data_changed), w);
        g_ptr_array_add(spins, spin_ai);

        /* — NEW: create a color‐picker — */
        GtkWidget *cb = gtk_color_button_new();
        w->colorbtn[i] = GTK_COLOR_BUTTON(cb);

        // tag and set the *harvested* color
        g_object_set_data(G_OBJECT(cb), "voter-index", GINT_TO_POINTER(i));
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(cb), &w->last_color[i]);

        // now hook the change to update last_color[] + redraw
        g_signal_connect(cb, "color-set", G_CALLBACK(on_color_changed), w);

        gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);

        g_signal_connect(cb, "color-set", G_CALLBACK(on_data_changed), w);

        /* — NEW: create an “IBP: —” label placeholder — */
        char ibp_txt[32];
        g_snprintf(ibp_txt, sizeof(ibp_txt), "IBP: —");
        GtkWidget *lbl = gtk_label_new(ibp_txt);
        w->lbl_ibp[i] = GTK_LABEL(lbl);
        gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, FALSE, 0);

        /* finally add the completed row */
        gtk_box_pack_start(GTK_BOX(w->box_ai), hbox, FALSE, FALSE, 2);
        gtk_widget_show_all(hbox);
    }

    /* reconnect the chaining on spins, as you had it */
    for (guint i = 0; i + 1 < spins->len; i++) {
        GtkSpinButton *prev = g_ptr_array_index(spins, i);
        GtkSpinButton *next = g_ptr_array_index(spins, i+1);
        g_signal_connect(prev, "value-changed", G_CALLBACK(on_prev_ai_changed), next);
    }
    g_ptr_array_free(spins, TRUE);

    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_bar));
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_parliament));
}
// Cuando se presiona el botón de ejecutar
static void on_execute_clicked(GtkButton *btn, gpointer data) {
    AppWidgets *w = data;
    extern int nodos, soluciones;

    // Leer todos los ai
    GList *kids = gtk_container_get_children(
                      GTK_CONTAINER(w->box_ai));
    int n = g_list_length(kids);
    int *A = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        GtkBox *hrow = GTK_BOX(g_list_nth_data(kids, i));
        GList *rowkids = gtk_container_get_children(GTK_CONTAINER(hrow));
        if (g_list_length(rowkids) >= 2) {
            GtkSpinButton *sp = GTK_SPIN_BUTTON(g_list_nth_data(rowkids, 1));
            A[i] = gtk_spin_button_get_value_as_int(sp);
        } else {
            A[i] = 0;
        }
        g_list_free(rowkids);
    }
    g_list_free(kids);

    // Leer W
    int W = gtk_spin_button_get_value_as_int(w->spin_w);

    // 3) Crear suffix_sum, que tiene la suma de todos los ai
    if (suffix_sum) {
        free(suffix_sum);
        suffix_sum = NULL;
    }
    suffix_sum = malloc(sizeof(int) * (n + 1));
    suffix_sum[n] = 0;
    for (int i = n - 1; i >= 0; --i) {
        suffix_sum[i] = A[i] + suffix_sum[i + 1];
    }

    // 4) Se prepara sol_list, que es la lista de soluciones
    nodos = soluciones = 0;
    GPtrArray *sol_list =
        g_ptr_array_new_with_free_func(g_free);
    int actual_idx[12];

    // 5) Se ejecuta el backtracking para obtener las soluciones
    //de igual o mayor valor
    sumaSubconjuntosV3_collect(A, n, W, 0, actual_idx, 0, 0, sol_list);

    // 6) Actualizar labels de cantidad de soluciones y nodos recorridos
    gtk_label_set_text(w->lbl_count, g_strdup_printf("Soluciones: %u", sol_list->len));
    gtk_label_set_text(w->lbl_nodes, g_strdup_printf("Nodos visitados: %d", nodos));

    // 7) Limpiar viejas filas
    GList *old_rows = gtk_container_get_children(GTK_CONTAINER(w->box_results));
    for (GList *r = old_rows; r; r = r->next)
        gtk_widget_destroy(GTK_WIDGET(r->data));
    g_list_free(old_rows);

    // 8) Agregar soluciones a la pantalla
    for (guint s = 0; s < sol_list->len; s++) {
        gboolean *mask = g_ptr_array_index(sol_list, s);

        // 1) Make a copy of the mask so it outlives our temporary 'sol_list'
        gboolean *mask_copy = g_new(gboolean, n);
        memcpy(mask_copy, mask, sizeof(gboolean) * n);

        // 2) Build the row & its checkbuttons exactly as before
        GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        GtkWidget *hbox   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

        int suma = 0;
        for (int i = 0; i < n; i++) {
            if (mask_copy[i]) suma += A[i];

            GtkWidget *chk = gtk_check_button_new();
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk),
                                        mask_copy[i]);
            gtk_widget_set_sensitive(chk, FALSE);
            gtk_box_pack_start(GTK_BOX(hbox), chk, FALSE, FALSE, 0);
        }

        GtkWidget *lbl_sum =
            gtk_label_new(g_strdup_printf(" Suma = %d", suma));
        gtk_box_pack_end(GTK_BOX(hbox), lbl_sum, FALSE, FALSE, 4);

        gtk_container_add(GTK_CONTAINER(row), hbox);

        // 3) Attach the copy to the row; free it automatically when row is destroyed
        g_object_set_data_full(G_OBJECT(row),
                            "mask",
                            mask_copy,
                            g_free);

        // 4) Insert the row into the list
        gtk_list_box_insert(GTK_LIST_BOX(w->box_results),
                            GTK_WIDGET(row),
                            -1);
        gtk_widget_show_all(GTK_WIDGET(row));
    }

    // 9) Liberar memoria
    free(A);
    free(suffix_sum);
    suffix_sum = NULL;
    g_ptr_array_free(sol_list, TRUE);
}
// El main
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    // Para aplicar fondo blanco a lista de soluciones
    const gchar *scroll_css =
    /* Aplica a cualquier widget con la clase “white-bg” */
    ".white-bg {\n"
    "  background-image: none;\n"
    "  background-color: #FFFFFF;\n"
    "}\n"
    /* Hace los labels dentro de .white-bg en negro */
    ".white-bg label {\n"
    "  color: #000000;\n"
    "}\n";
    GtkCssProvider *scroll_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(
        scroll_provider, scroll_css, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(scroll_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(scroll_provider);

    // Se inicializa el builder
    GtkBuilder *builder = gtk_builder_new_from_file("interfaz.glade");
    AppWidgets *w = g_new0(AppWidgets, 1);

    // Panel derecho: a_i y Δ
    w->box_ai = GTK_BOX(gtk_builder_get_object(builder, "box_ai"));
    
    for (int i = 0; i < 12; i++) {
        /* seed with your defaults */
        gdk_rgba_parse(&w->last_color[i], default_colors[i]);
        }

    // Spin de tamaño para regenerar los ai
    GtkSpinButton *spin_size = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "size"));

    // Spin de W
    w->spin_w = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spin_w"));

    // Resultados
    w->lbl_count = GTK_LABEL(gtk_builder_get_object(builder, "lbl_count"));
    w->lbl_nodes = GTK_LABEL(gtk_builder_get_object(builder, "lbl_nodes"));
    w->box_results = GTK_LIST_BOX(gtk_builder_get_object(builder, "list_solutions"));
    g_return_val_if_fail(GTK_IS_LIST_BOX(w->box_results), 1);
    g_signal_connect(w->box_results, "row-selected", G_CALLBACK(on_solution_selected), w);

    // Botón Ejecutar y Botón Salir
    w->btn_execute = GTK_BUTTON(gtk_builder_get_object(builder, "btn_execute"));
    GtkButton *btn_exit = GTK_BUTTON(gtk_builder_get_object(builder, "buttonFinish"));
    
    // Lista de soluciones
    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(w->box_results), TRUE);
        
    // --- Conectar señales ---
    // Redibujar los ai al cambiar el spin de tamaño
    g_signal_connect(spin_size, "value-changed", G_CALLBACK(on_size_value_changed), w);
    // Presionar el botón ejecutar
    g_signal_connect(w->btn_execute, "clicked", G_CALLBACK(on_execute_clicked), w);
    // Presionar el botón salir
    g_signal_connect(btn_exit, "clicked",G_CALLBACK(gtk_main_quit), NULL);
    // Panel divisor
    GtkWidget *panel = GTK_WIDGET(gtk_builder_get_object(builder, "division"));
    g_signal_connect(panel, "notify::position", G_CALLBACK(fijar_panel), NULL);

    // Áreas de dibujo
    w->drawing_bar = GTK_DRAWING_AREA(gtk_builder_get_object(builder, "drawing_bar"));
    g_signal_connect(w->drawing_bar, "draw", G_CALLBACK(on_draw_bar), w);
    
    w->drawing_parliament = GTK_DRAWING_AREA(gtk_builder_get_object(builder, "drawing_parliament"));
    g_signal_connect(w->drawing_parliament, "draw", G_CALLBACK(on_draw_parliament), w);
    
    // Para seleccionar una fila de las soluciones
    w->selected_mask = NULL;

    // --- Estado inicial de la interfaz ---
    // 1) Genera los ai con el valor inicial de "size"
    on_size_value_changed(spin_size, w);

    // --- Muestra todo y fullscreen ---
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "ventana"));
    gtk_widget_show_all(window);
    gtk_window_fullscreen(GTK_WINDOW(window));

    gtk_main();
    
    // Liberar memoria
    g_free(w);
    g_object_unref(builder);
    return 0;
}