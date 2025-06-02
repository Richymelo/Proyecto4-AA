/*
                Esta sección contiene las funciones de las 4 variantes
                de backtracking para obtener subconjuntos. Las variantes
                son: la básica, la delta, la mayor o igual y la mayor o
                igual acotada. Se tiene además contadores de los nodos,
                las soluciones y la suma de los ai que no sea han revisado
                todavía.
*/
//extern int nodos, soluciones;
static int *suffix_sum;
extern int *votos_criticos_global;
extern int n_votantes_global;
extern int W_global;

/*
// ========================
// VARIANTE 1: Básica
// ========================
void sumaSubconjuntosV1_collect(int* A, int n, int W, int index, int* actual_idx,
                                int tam_actual, int suma_actual, GPtrArray *sol_list) {
    // Contadores
    extern int nodos, soluciones;
    nodos++;

    // 1) Se poda si ya se pasó de W
    if (suma_actual > W)
        return;

    // 2) Se poda si ni con todo lo que queda llego a W
    if (suma_actual + suffix_sum[index] < W)
        return;

    // 3) Si llegué al final (hoja)
    if (index == n) {
        // Si la suma es W, es solución
        if (suma_actual == W) {
            soluciones++;
            gboolean *mask = g_new0(gboolean, n);
            for (int j = 0; j < tam_actual; j++)
                mask[ actual_idx[j] ] = TRUE;
            g_ptr_array_add(sol_list, mask);
        }
        return;
    }

    // 4) Rama “incluir A[index]” — solo si suma_actual + A[index] ≤ W
    if (suma_actual + A[index] <= W) {
        actual_idx[tam_actual] = index;
        sumaSubconjuntosV1_collect(
            A, n, W,
            index + 1,
            actual_idx, tam_actual + 1,
            suma_actual + A[index],
            sol_list
        );
    }

    // 5) Rama “excluir A[index]”
    sumaSubconjuntosV1_collect(
        A, n, W,
        index + 1,
        actual_idx, tam_actual,
        suma_actual,
        sol_list
    );
}
// ========================
// VARIANTE 2: Delta
// ========================
void sumaSubconjuntosV2_collect(int* A, int n, int W, int index, int* actual_idx,
                                int tam_actual, int suma_actual, GPtrArray *sol_list) {
    // Contadores
    extern int nodos, soluciones;
    nodos++;
    // 0) Se poda si ya se pasó de W+Δ
    if (suma_actual > W + delta)
        return;

    // 1) Se poda si si ni con todo lo que queda se llega a W-Δ
    if (index < n &&
        suma_actual + suffix_sum[index] < W - delta)
    {
        return;
    }

    // 2) Si la suma está ya en [W-Δ, W+Δ], es solución
    if (suma_actual >= W - delta &&
        suma_actual <= W + delta)
    {
        soluciones++;
        gboolean *mask = g_new0(gboolean, n);
        for (int j = 0; j < tam_actual; j++)
            mask[ actual_idx[j] ] = TRUE;
        g_ptr_array_add(sol_list, mask);
        return;  // <<-- poda aquí para no explorar supersets
    }

    // 3) Si se llegó al final, no hay nada más que hacer
    if (index == n)
        return;

    // 4) Rama “incluir A[index]”
    actual_idx[tam_actual] = index;
    sumaSubconjuntosV2_collect(
        A, n, W,
        index + 1,
        actual_idx, tam_actual + 1,
        suma_actual + A[index],
        sol_list
    );

    // 5) Rama “excluir A[index]”
    sumaSubconjuntosV2_collect(
        A, n, W,
        index + 1,
        actual_idx, tam_actual,
        suma_actual,
        sol_list
    );
}
*/
// ========================
// VARIANTE 3: Mayor o Igual
// ========================
void sumaSubconjuntosV3_collect(int* A, int n, int W, int index, int* actual_idx,
                                int tam_actual, int suma_actual, GPtrArray *sol_list) {
    // Contadores
    //extern int nodos, soluciones;
    //nodos++;

    // 0) Se poda si ni siquiera con todo lo que queda se llega a W
    if (suma_actual + suffix_sum[index] < W) {
        return;
    }

    // 1) Se llegó al final
    if (index == n) {
        // Si la suma es mayor o igual a W, es solución
        if (suma_actual >= W) {
            //soluciones++;

            // Aquí se calcula el voto crítico para cada votante en la coalición
            for (int j = 0; j < tam_actual; j++) {
                int votante = actual_idx[j];
                int suma_sin_votante = suma_actual - A[votante];
                if (suma_sin_votante < W) {
                    votos_criticos_global[votante]++;
                }
            }
            gboolean *mask = g_new0(gboolean, n);
            for (int j = 0; j < tam_actual; j++)
                mask[ actual_idx[j] ] = TRUE;
            g_ptr_array_add(sol_list, mask);
        }
        return;
    }

    // 2) Rama “incluir A[index]”
    actual_idx[tam_actual] = index;
    sumaSubconjuntosV3_collect(
        A, n, W,
        index + 1, actual_idx, tam_actual + 1,
        suma_actual + A[index], sol_list
    );

    // 3) Rama “excluir A[index]”
    sumaSubconjuntosV3_collect(
        A, n, W,
        index + 1, actual_idx, tam_actual,
        suma_actual, sol_list
    );
}
/*
// ====================================
// VARIANTE 4: Mayor o Igual Acotado
// ====================================
void sumaSubconjuntosV4_collect(int* A, int n, int W, int index, int* actual_idx,
                                int tam_actual, int suma_actual, GPtrArray *sol_list) {
    // Contadores
    extern int nodos, soluciones;
    nodos++;

    // 0) Se poda si ni sumando todo lo que queda se llega a W
    if (suma_actual + suffix_sum[index] < W)
        return;

    // 1) Si la suma es mayor o igual a W, es solución
    if (suma_actual >= W) {
        soluciones++;
        gboolean *mask = g_new0(gboolean, n);
        for (int j = 0; j < tam_actual; j++)
            mask[ actual_idx[j] ] = TRUE;
        g_ptr_array_add(sol_list, mask);
        return;
    }

    // 2) Si se llegó al final, no hay nada más que hacer
    if (index == n)
        return;

    // 3) Rama “incluir A[index]”
    actual_idx[tam_actual] = index;
    sumaSubconjuntosV4_collect(
        A, n, W,
        index + 1, actual_idx, tam_actual + 1,
        suma_actual + A[index],
        sol_list
    );

    // 4) Rama “excluir A[index]”
    sumaSubconjuntosV4_collect(
        A, n, W,
        index + 1, actual_idx, tam_actual,
        suma_actual,
        sol_list
    );
}
*/