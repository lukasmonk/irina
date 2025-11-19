#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "protos.h"
#include "globals.h"
#include "tt.h"

TtEntry * ttTable;

Bitmap register_max;


void init_ttTable(int max_mb)
{
    register_max = 1024L * 1024L * max_mb / sizeof (TtEntry);
    if(register_max % 2 == 1) register_max += 1;
    ttTable = (TtEntry *)calloc(register_max, sizeof(TtEntry));
    memset(ttTable, 0, sizeof(&ttTable));
}


void set_hash_ttTable(char * value)
{
    int mb;
    sscanf(value, "%d", &mb);
    if( mb >= 2 && mb <= 1024 ) {
        free(ttTable);
        init_ttTable(mb);
    }
}


// Convierte puntuaciones de mate/submate a un valor absoluto para almacenar
int scoreToTt(int score, int ply)
{
    // Si es mate, ajusta el score para que sea relativo a la raíz (ply=0)
    if (score > MATESCORE) return score + ply;
    if (score < -MATESCORE) return score - ply;
    return score;
}

// Convierte puntuaciones almacenadas a un valor relativo a la profundidad actual (ply)
int scoreFromTt(int score, int ply)
{
    // Si es mate, ajusta el score para que sea relativo a la profundidad actual
    if (score > MATESCORE) return score - ply;
    if (score < -MATESCORE) return score + ply;
    return score;
}

/**
 * Busca en la Tabla de Transposiciones.
 * @return La puntuación de corte si se encuentra, o NO_TT_HIT.
 */
int probeEntry(Bitmap hashKey, int depth, int alpha, int beta, Move* ttMove)
{
    // Usamos el hashkey para encontrar el índice (MAX_TT_SIZE debe ser una potencia de 2)
    unsigned int index = hashKey & (register_max - 1);
    TtEntry* entry = &ttTable[index];

    // 1. Verificar si hay un 'hit' (el hashkey coincide y no está vacía)
    if (entry->hashKey == hashKey)
    {
        // Pasa el mejor movimiento (Hash Move) para la ordenación
        *ttMove = entry->bestMove;

        // 2. Verificar la profundidad
        if (entry->depth >= depth)
        {
            // Puntuación almacenada, ajustada a la profundidad de la búsqueda actual
            int score = scoreFromTt(entry->score, board.ply);

            // 3. Verificar el flag (determinar si la puntuación es un corte)
            if (entry->flag == FLAG_EXACT)
            {
                return score;
            }
            if (entry->flag == FLAG_LOWERBOUND) // Límite inferior (alfa)
            {
                if (score >= beta)
                {
                    return beta; // Poda Beta instantánea
                }
                // Actualiza el límite alfa local
                // alpha = MAX(alpha, score); // Esto se maneja fuera
            }
            if (entry->flag == FLAG_UPPERBOUND) // Límite superior (beta)
            {
                if (score <= alpha)
                {
                    return alpha; // Poda Alfa instantánea
                }
                // Actualiza el límite beta local
                // beta = MIN(beta, score); // Esto se maneja fuera
            }
        }
    }
    // Si no hubo un hit útil, no devuelve un corte
    return NO_TT_HIT;
}

/**
 * Almacena el resultado de la búsqueda en la Tabla de Transposiciones.
 */
void storeEntry(Bitmap hashKey, int score, int depth, int flag, Move bestMove)
{
    unsigned int index = hashKey & (register_max - 1);
    TtEntry* entry = &ttTable[index];

    // Estrategia de reemplazo: siempre reemplazar si la nueva búsqueda es más profunda.
    // También reemplazamos si es un hit exacto (más valioso) o si el slot está vacío (hashKey == 0).
    if (entry->hashKey == hashKey || entry->depth < depth || entry->hashKey == 0)
    {
        entry->hashKey = hashKey;
        entry->bestMove = bestMove;
        entry->score = scoreToTt(score, board.ply); // Almacena el score ajustado
        entry->depth = depth;
        entry->flag = flag;
    }
}
