/*
                Esta sección contiene una estructura con la mayoría de
                Widgets con los que el usuario puede interactuar en el
                programa. Están los widgets de la sección de números ai
                del conjunto, los widgets que controlan el número delta,
                el widget de w (solución que se quiere), los widgets de
                las 4 variantes de algoritmos y los widgets para los
                resultados.
*/
typedef struct {
    /* sección A_i*/
    GtkBox       *box_ai;

    /* W */
    GtkSpinButton *spin_w;

    /* Resultados */
    GtkLabel    *lbl_count;
    GtkLabel    *lbl_nodes;
    GtkListBox  *box_results;

    /* Botón “Ejecutar” */
    GtkButton   *btn_execute;

    /* colores y labels para cada votante */
    GtkColorButton *colorbtn[12];
    GtkLabel       *lbl_ibp[12];
    GtkSpinButton  *spin_ai[12];
    GdkRGBA        last_color[12];
    
    /* Áreas de dibujo */
    GtkDrawingArea *drawing_bar;
    GtkDrawingArea *drawing_parliament;

    // Para saber cuál fila se seleccionó
    gboolean    *selected_mask;
} AppWidgets;