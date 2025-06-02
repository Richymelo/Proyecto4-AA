# Proyecto 4: Índice de Poder de Banzhaf

## Descripción General

Este proyecto consiste en desarrollar una aplicación en C sobre Linux, utilizando GTK y Glade, para calcular el Índice de Poder de Banzhaf (IPB) en un sistema de votación asimétrico. En un sistema de votos asimétricos, cada votante tiene una cantidad diferente de votos. El IPB es una métrica del poder real de cada votante, considerando la proporción de votos críticos que posee.

## Entrada

El programa debe presentar una interfaz gráfica que permita al usuario:

* Ingresar la cantidad `n` de votantes (3 ≤ n ≤ 12).
* Introducir la cantidad de votos `v_i` para cada votante. Se debe validar que estos valores sean enteros positivos y, por defecto, cada votante tiene un voto.
* Especificar la cantidad `K` de votos necesarios para ganar una votación (debe ser un entero mayor o igual a 0).
* Cada votante tendrá un color asignado, mostrado en la interfaz.
* Opcionalmente (Trabajo extra 1), permitir al usuario editar el color asignado a cada votante en una subventana.
* Opcionalmente (Trabajo extra 2), solicitar un nombre o etiqueta para cada votante, a ser usado en los despliegues.
* Haber un botón de "ejecución" que comienza el proceso de solución.

## Proceso

El programa debe:

* Reutilizar funciones del proyecto previo que resolvían el problema de la Suma de Subconjuntos usando backtracking para encontrar todas las coaliciones ganadoras.
* Identificar los votos críticos dentro de las coaliciones ganadoras para calcular el IPB de cada votante.

## Salida

La aplicación debe mostrar:

* El modelo de votación en el formato (K; v1, v2, ..., vn). [cite: 16]
* Un rectángulo que represente los votos de cada votante con un área proporcional a su cantidad de votos y el color asignado.
* Una zona con scroll vertical que muestre todas las coaliciones ganadoras.
* Cada coalición ganadora representada por un rectángulo similar al de la representación de todos los votos.
* Los votos críticos marcados dentro de los rectángulos de las coaliciones ganadoras.
* Mostrar cada coalición ganadora y sus votos críticos sobre un gráfico de "parlamento".
* La cantidad total de votos críticos en el modelo.
* El IPB de cada votante, mostrado como un número real (e.g., 0.2353) y como la fracción de votos críticos que poseen (e.g., 4/17).
