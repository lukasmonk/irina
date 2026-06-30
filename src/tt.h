#ifndef TT_H
#define TT_H

#define NO_TT_HIT 20000 // Valor de puntuación que indica que no hubo un 'hit' de corte

// Flags para el tipo de entrada
#define FLAG_EXACT 0
#define FLAG_LOWERBOUND 1
#define FLAG_UPPERBOUND 2

typedef struct TtEntry
{
    Bitmap hashKey; // Hash Zobrist completo (para verificar colisiones)
    Move bestMove;  // El mejor movimiento de esta posición (Hash Move)
    int score;      // Puntuación de la posición
    int depth;      // Profundidad restante a la que se buscó
    int flag;       // Tipo de puntuación (Exact, Lowerbound, Upperbound)
} TtEntry;

// Variable global para la tabla
extern TtEntry *ttTable;

void init_ttTable(int max_mb);
void clear_ttTable(void);
void set_hash_ttTable(char * value);
int probeEntry(Bitmap hashKey, int depth, int alpha, int beta, Move* bestMove);
void storeEntry(Bitmap hashKey, int score, int depth, int flag, Move bestMove);

#endif // TT_H
