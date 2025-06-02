/*
                Proyecto 4: Índice de Poder de Banzhaf
                Hecho por: Carmen Hidalgo Paz, Jorge Guevara Chavarría y Ricardo Castro Jiménez
                Fecha: Martes 3 de junio del 2025

                Esta sección contiene el main, donde se indica lo que tiene que hacer
                cada objeto mostrado en la interfaz. Además se tienen funciones para
                modificar la cantidad de números del conjunto que desea el usuario,
                lo que ocurre cuando se presiona el botón ejecutar y el dibujo de la
                barra y el gráfico estilo parlamento.
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

// Contadores
//int nodos = 0;
//int soluciones = 0;
int *votos_criticos_global = NULL;
int n_votantes_global = 0;
int W_global = 0;

// Colores iniciales para los votantes
static const char *default_colors[12] = {
    "#1f77b4", // azul
    "#ff7f0e", // naranja
    "#2ca02c", // verde claro
    "#d62728", // rojo
    "#9467bd", // morado
    "#8c564b", // café
    "#e377c2", // rosado
    "#f5c211", // amarillo
    "#195903", // verde oscuro
    "#17becf", // cian
    "#aec7e8", // celeste
    "#ffbb78"  // piel
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
// Cambiar el label con la información del modelo
static void update_model_label(AppWidgets *w)
{
    // Leer el valor de W
    int K = gtk_spin_button_get_value_as_int(w->spin_w);

    // Obtener cantidad de votantes
    int n = 0;
    for (; n < 12; n++) {
        if (!w->spin_ai[n]) break;
    }

    // Crear la lista con los nombres de los votantes
    GString *names = g_string_new(NULL);
    for (int i = 0; i < n; i++) {
        if (i) g_string_append(names, ", ");
        g_string_append(names, w->voter_names[i]);
    }

    // Crear la lista con los valores de los votos
    GString *values = g_string_new(NULL);
    for (int i = 0; i < n; i++) {
        int v = gtk_spin_button_get_value_as_int(w->spin_ai[i]);
        if (i) g_string_append(values, ", ");
        g_string_append_printf(values, "%d", v);
    }

    // Crear todo el label
    char *text = g_strdup_printf(
        "Modelo (%d; %s): (%d; %s)",
        K,
        names->str,
        K,
        values->str
    );

    // Guardarlo en el label
    gtk_label_set_text(w->lbl_model, text);

    // Borrar memoria utilizada
    g_free(text);
    g_string_free(names, TRUE);
    g_string_free(values, TRUE);
}
// Volver a pintar los dibujos cuando algún dato se modifica
static void on_data_changed(GtkWidget *widget, gpointer user_data) {
    AppWidgets *w = user_data;
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_bar));
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_parliament));
    update_model_label(w);
}
// Volver a pintar la barra cuando algún color se modifica
static void on_color_changed(GtkColorButton *btn, gpointer user_data) {
  AppWidgets *w = user_data;
  int i = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "voter-index"));
  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(btn), &w->last_color[i]);
  gtk_widget_queue_draw(GTK_WIDGET(w->drawing_bar));
}
// Mostrar cambios en el dibujo de parlamento cuando se selecciona una solución
static void on_solution_selected(GtkListBox *box, GtkListBoxRow *row, gpointer data) {
    AppWidgets *w = data;

    if (row) {
        // Obtener la fila seleccionada
        w->selected_mask = g_object_get_data(G_OBJECT(row), "mask");
    }
    else {
        w->selected_mask = NULL;
    }

    // Volver a dibujar el gráfico del parlamento
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_parliament));
}
// Cuando se cambia el nombre de un votante
static void on_voter_name_changed(GtkEntry *entry, gpointer user_data) {
    AppWidgets *w = user_data;
    
    // Obtener el votante al que se le está haciendo el cambio
    int idx = GPOINTER_TO_INT(
      g_object_get_data(G_OBJECT(entry), "voter-index"));

    // GUardar el nuevo nombre
    g_free(w->voter_names[idx]);
    w->voter_names[idx] = g_strdup(gtk_entry_get_text(entry));

    // Volver a mostrar el label con el modelo con los cambios
    update_model_label(w);
}
// Para dibujar la barra
static gboolean on_draw_bar(GtkDrawingArea *area, cairo_t *cr, gpointer user_data) {
    AppWidgets *w = user_data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
    int W = alloc.width;
    int H = alloc.height;

    // Contar cantidad de votantes
    int n = 0;
    while (n < 12 && w->spin_ai[n]) n++;

    // Contar todos los votos
    int total = 0;
    int votes[12];
    for (int i = 0; i < n; i++) {
        votes[i] = gtk_spin_button_get_value_as_int(w->spin_ai[i]);
        total += votes[i];
    }
    if (total == 0) return FALSE;

    // Dibujar cada fracción en la barra
    int x = 0;
    for (int i = 0; i < n; i++) {
        double frac = (double)votes[i] / total;
        int slice_w = frac * W;

        // Escoger el color del votante
        GdkRGBA col;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w->colorbtn[i]), &col);
        cairo_set_source_rgba(cr, col.red, col.green, col.blue, col.alpha);

        cairo_rectangle(cr, x, 0, slice_w, H);
        cairo_fill(cr);

        x += slice_w;
    }

    return FALSE;
}
// Dibujar una estrella en lugar de un punto
void draw_star(cairo_t *cr, double cx, double cy, double radius) {
    const int spikes = 5;
    const double outer_radius = radius * 1.4;
    const double inner_radius = radius * 0.6;

    cairo_move_to(cr,
        cx + outer_radius * cos(0),
        cy - outer_radius * sin(0));

    for (int i = 1; i < spikes * 2; i++) {
        double angle = i * M_PI / spikes;
        double r = (i % 2 == 0) ? outer_radius : inner_radius;
        cairo_line_to(cr,
            cx + r * cos(angle),
            cy - r * sin(angle));
    }

    cairo_close_path(cr);
    cairo_fill(cr);
}
// Dibujar el gráfico estilo parlamento
static gboolean on_draw_parliament(GtkDrawingArea *area, cairo_t *cr, gpointer user_data) {
    AppWidgets *w = user_data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
    double W = alloc.width, H = alloc.height;

    // Obtener la selección de la fila que el usuario escogió
    gboolean *sel = w->selected_mask;

    // Definir el color blanco, que son los que no son parte de la coalición ganadora
    GdkRGBA white = { .red   = 1.0,
                     .green = 1.0,
                     .blue  = 1.0,
                     .alpha = 1.0 };

    // 1) Leer votos y obtener el total
    int votes[12], n = 0, total = 0;
    int votos_criticos[12] = {0};  // up to 12 voters

    while (n < 12 && w->spin_ai[n]) {
        votes[n] = gtk_spin_button_get_value_as_int(w->spin_ai[n]);
        total   += votes[n];
        n++;
    }
    if (n == 0 || total == 0) return FALSE;

    if (sel) {
        // Revisar cuáles son los votantes críticos
        int suma_total = 0;
        for (int i = 0; i < n; i++) {
            if (sel[i]) suma_total += gtk_spin_button_get_value_as_int(w->spin_ai[i]);
        }

        for (int i = 0; i < n; i++) {
            if (!sel[i]) continue;

            int suma_sin_i = suma_total - gtk_spin_button_get_value_as_int(w->spin_ai[i]);
            if (suma_total >= gtk_spin_button_get_value_as_int(w->spin_w) &&
                suma_sin_i < gtk_spin_button_get_value_as_int(w->spin_w)) {
                votos_criticos[i] = 1;
            }
        }
    }

    // 2) Geometría para los márgenes y el centro del círculo
    const double margin  = 20.0;
    const double cx = W/2.0;
    const double cy = H - margin;

    // Lógica para el espaciado
    // Para saber cuántos puntos caben en un anillo
    double dr_logic = MIN(W, H) * 0.02;
    // Para saber cuánto espacio por fila
    double row_gap = dr_logic * 2.4;
    // El radio del primer anillo
    double r_base = MIN(cx, cy - dr_logic) - dr_logic;
    // Tamaño de cada punto
    double dr_draw = dr_logic * 0.8;

    // 3) Cada vontante tiene su pedazo triangular
    // Saber donde se está en la mitad del círculo
    double angle_cursor = M_PI;
    // Un pedazo por votante
    for (int i = 0; i < n; i++) {
        int seats = votes[i];
        if (seats <= 0) continue;

        // 3a) Obtener los ángulos de cada pedazo
        // Cuantos radianes debe tomar el pedazo
        double ang_span = M_PI * seats / total;
        // Donde es que empieza
        double ang_start = angle_cursor;
        // Donde es que termina 
        double ang_end = angle_cursor - ang_span;
        // Para saber donde comienza el siguiente pedazo
        angle_cursor = ang_end;

        // 3b) Llenar el pedazo con puntos
        // Cantidad de puntos que hay que dibujar
        int  remaining = seats;
        // Anillo donde se inicia
        double r = r_base;
        // Mientras haya que seguir dibujando y no se acabe el espacio del anillo
        while (remaining > 0 && r >= dr_logic) {
            // Ir dibujando en los anillos
            // Se aproxima la longitud del arco y se divide por el diámetro
            // para obtener la cantidad de puntos máximo por anillo
            int fit = (int)floor((r * ang_span) / (2.0 * dr_logic)) + 1;
            if (fit < 1) fit = 1;
            // Cantidad de puntos que faltan después de llenar el anillo
            int draw_count = remaining < fit ? remaining : fit;

            // Dibujar los puntos
            for (int k = 0; k < draw_count; k++) {
                // Espaciado de cada punto
                double step = ang_span / (double)draw_count;
                double t = ang_start - (k + 0.5) * step;

                // Centro del punto
                double x = cx + r * cos(t);
                double y = cy - r * sin(t);

                // Escoger el color del votante
                GdkRGBA col;
                if (sel && !sel[i]) {
                    col = white;
                } else {
                    gtk_color_chooser_get_rgba(
                    GTK_COLOR_CHOOSER(w->colorbtn[i]),
                    &col);
                }
                cairo_set_source_rgba(cr, col.red, col.green, col.blue, col.alpha);

                // Pintar el punto
                
                if (votos_criticos[i] > 0) {
                    draw_star(cr, x, y, dr_draw);  // Reemplazar punto con estrella
                } else {
                    cairo_arc(cr, x, y, dr_draw, 0, 2 * M_PI);
                    cairo_fill(cr);
                }
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
                int val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(hiter->data));
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

    // 0) Obtener colores de cada votante
    for (int i = 0; i < 12; i++) {
        if (w->colorbtn[i]) {
            gtk_color_chooser_get_rgba(
                GTK_COLOR_CHOOSER(w->colorbtn[i]),
                &w->last_color[i]);
        }
    }
    // 1) Destruir filas viejas
    GList *kids = gtk_container_get_children(GTK_CONTAINER(w->box_ai));
    for (GList *it = kids; it; it = it->next)
        gtk_widget_destroy(GTK_WIDGET(it->data));
    g_list_free(kids);

    // 2) Limpiar punteros
    for (int i = 0; i < 12; i++) {
        w->spin_ai[i]   = NULL;
        w->colorbtn[i]  = NULL;
        w->lbl_ibp[i]   = NULL;
    }

    // 3) Crear nuevas filas
    GPtrArray *spins = g_ptr_array_new();
    for (int i = 0; i < n; i++) {
        // 3a: Botones para los votos por votante
        GtkAdjustment *adj    = gtk_adjustment_new(1, 1, G_MAXINT, 1, 10, 0);
        GtkWidget    *spin_ai = gtk_spin_button_new(adj, 1, 0);
        w->spin_ai[i] = GTK_SPIN_BUTTON(spin_ai);

        // Modificar los valores cuando hay un cambio
        g_signal_connect(spin_ai, "value-changed", G_CALLBACK(on_data_changed), w);
        g_ptr_array_add(spins, spin_ai);

        // 3b: Ingresar un nombre por votante y tener los dos puntos (:)
        char buf[16];
        g_snprintf(buf, sizeof(buf), "a%d", i+1);

        GtkWidget *entry = gtk_entry_new();
        w->entry_name[i] = GTK_ENTRY(entry);

        // Inicializar o guardar el nombre cambiado
        if (w->voter_names[i]) {
            gtk_entry_set_text(GTK_ENTRY(entry), w->voter_names[i]);
        } else {
            w->voter_names[i] = g_strdup(buf);
            gtk_entry_set_text(GTK_ENTRY(entry), buf);
        }

        // Tamaño de espacio para el nombre
        gtk_entry_set_width_chars(GTK_ENTRY(entry), 6);
        gtk_widget_set_hexpand(entry, FALSE);

        // Mostrar los cambios cuando se modifica un nombre o número
        g_object_set_data(G_OBJECT(entry), "voter-index", GINT_TO_POINTER(i));
        g_signal_connect(entry, "changed", G_CALLBACK(on_voter_name_changed), w);
        g_signal_connect(spin_ai, "value-changed", G_CALLBACK(on_data_changed), w);
        g_signal_connect(w->spin_w, "value-changed", G_CALLBACK(on_data_changed), w);

        GtkWidget *colon = gtk_label_new(":");

        // 3c: Escoger un color
        GtkWidget *cb = gtk_color_button_new();
        w->colorbtn[i] = GTK_COLOR_BUTTON(cb);
        g_object_set_data(G_OBJECT(cb), "voter-index", GINT_TO_POINTER(i));
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(cb), &w->last_color[i]);
        g_signal_connect(cb, "color-set", G_CALLBACK(on_color_changed), w);
        g_signal_connect(cb, "color-set", G_CALLBACK(on_data_changed), w);

        // 3.d: Para poder tener el label de IPB debajo de cada votante
        GtkWidget *cell_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        // EN la fila de arriba se tiene el nombre, la cantidad de votos y el color
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        gtk_box_pack_start(GTK_BOX(hbox), entry,   FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), colon,   FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), spin_ai, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), cb,      FALSE, FALSE, 0);

        // En la fila de abajo se tiene el label IBP
        char ibp_txt[32];
        g_snprintf(ibp_txt, sizeof(ibp_txt), "IBP: —");
        GtkWidget *lbl = gtk_label_new(ibp_txt);
        w->lbl_ibp[i]  = GTK_LABEL(lbl);

        // Mostrar cada votante
        gtk_box_pack_start(GTK_BOX(cell_vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(cell_vbox), lbl,  FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(w->box_ai),  cell_vbox, FALSE, FALSE, 2);
        gtk_widget_show_all(cell_vbox);
    }

    // Mostrar cambios cuando se cambia un valor
    for (guint i = 0; i + 1 < spins->len; i++) {
        GtkSpinButton *prev = g_ptr_array_index(spins, i);
        GtkSpinButton *next = g_ptr_array_index(spins, i+1);
        g_signal_connect(prev, "value-changed", G_CALLBACK(on_prev_ai_changed), next);
    }
    g_ptr_array_free(spins, TRUE);

    // Mostrar la barra y el gráfico de parlamento
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_bar));
    gtk_widget_queue_draw(GTK_WIDGET(w->drawing_parliament));
    update_model_label(w);
}
// Cuando se presiona el botón de ejecutar
static void on_execute_clicked(GtkButton *btn, gpointer data) {
    AppWidgets *w = data;
    //extern int nodos, soluciones;

    // Leer todos los ai
    GList *rows = gtk_container_get_children(GTK_CONTAINER(w->box_ai));
    int  n = g_list_length(rows);
    int *A = malloc(sizeof(int) * n);

    int i = 0;
    for (GList *r = rows; r; r = r->next, i++) {
        GtkWidget *cell_vbox = GTK_WIDGET(r->data);

        // Obtener el ai
        GList *vbox_kids = gtk_container_get_children(GTK_CONTAINER(cell_vbox));
        GtkWidget *hbox = GTK_WIDGET(vbox_kids->data);
        g_list_free(vbox_kids);

        // Obtener el valor de los votos
        GList *hbox_kids = gtk_container_get_children(GTK_CONTAINER(hbox));
        GtkSpinButton *sp = GTK_SPIN_BUTTON(g_list_nth_data(hbox_kids, 2));
        g_list_free(hbox_kids);

        //  Leer el valor
        A[i] = gtk_spin_button_get_value_as_int(sp);
    }

    g_list_free(rows);

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
    //nodos = soluciones = 0;
    GPtrArray *sol_list = g_ptr_array_new_with_free_func(g_free);
    int actual_idx[12];

    // 5) Se ejecuta el backtracking para obtener las soluciones
    //de igual o mayor valor
    if (votos_criticos_global) {
        free(votos_criticos_global);
    }
    votos_criticos_global = calloc(n, sizeof(int));  // allocate and zero-initialize
    n_votantes_global = n;
    W_global = W;

    sumaSubconjuntosV3_collect(A, n, W, 0, actual_idx, 0, 0, sol_list);

    // 6) Actualizar labels de cantidad de soluciones y nodos recorridos
    //gtk_label_set_text(w->lbl_count, g_strdup_printf("Soluciones: %u", sol_list->len));
    //gtk_label_set_text(w->lbl_nodes, g_strdup_printf("Nodos visitados: %d", nodos));
    int total_criticos = 0;
    for (int i = 0; i < n; i++)
        total_criticos += votos_criticos_global[i];

    for (int i = 0; i < n; i++) {
        if (w->lbl_ibp[i]) {
            if (total_criticos > 0) {
                double ibp = (double)votos_criticos_global[i] / total_criticos;
                char ibp_txt[64];
                snprintf(ibp_txt, sizeof(ibp_txt), "IBP: %.4f (%d/%d)", ibp,
                        votos_criticos_global[i], total_criticos);
                gtk_label_set_text(w->lbl_ibp[i], ibp_txt);
            } else {
                gtk_label_set_text(w->lbl_ibp[i], "IBP: —");
            }
        }
    }

    char crit_txt[64];
    snprintf(crit_txt, sizeof(crit_txt), "Cantidad de valores críticos: %d", total_criticos);
    gtk_label_set_text(GTK_LABEL(w->lbl_critical), crit_txt);

    // 7) Limpiar viejas filas
    GList *old_rows = gtk_container_get_children(GTK_CONTAINER(w->box_results));
    for (GList *r = old_rows; r; r = r->next)
        gtk_widget_destroy(GTK_WIDGET(r->data));
    g_list_free(old_rows);

    // 8) Agregar soluciones a la pantalla
    for (guint s = 0; s < sol_list->len; s++) {
        gboolean *mask = g_ptr_array_index(sol_list, s);

        // 8a) Para poder guardar la fila escogida
        gboolean *mask_copy = g_new(gboolean, n);
        memcpy(mask_copy, mask, sizeof(gboolean) * n);

        // 8b) Crear la fila con sus checkboxes
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

        // 8c) Cuando se selecciona una fila se obtienen los datos de esta
        g_object_set_data_full(G_OBJECT(row), "mask", mask_copy, g_free);

        // 8d) Agregar la fila a la lista
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
    if (votos_criticos_global) {
        free(votos_criticos_global);
        votos_criticos_global = NULL;
    }
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

    // Panel derecho: a_i
    w->box_ai = GTK_BOX(gtk_builder_get_object(builder, "box_ai"));
    
    for (int i = 0; i < 12; i++) {
        /* seed with your defaults */
        gdk_rgba_parse(&w->last_color[i], default_colors[i]);
        }

    // Spin de tamaño para regenerar los ai
    GtkSpinButton *spin_size = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "size"));

    // Spin de W
    w->spin_w = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spin_w"));

    // Nombres de cada votante
    for (int i = 0; i < 12; i++) {
        w->entry_name[i]   = NULL;
        w->voter_names[i]  = NULL;
        }
        w->selected_mask = NULL;

    // Resultados
    //w->lbl_count = GTK_LABEL(gtk_builder_get_object(builder, "lbl_count"));
    //w->lbl_nodes = GTK_LABEL(gtk_builder_get_object(builder, "lbl_nodes"));
    w->lbl_model = GTK_LABEL(gtk_builder_get_object(builder, "lbl_model"));
    w->lbl_critical = GTK_LABEL(gtk_builder_get_object(builder, "lbl_critical"));
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