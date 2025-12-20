#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif
#include <time.h>

#include "defs.h"
#include "tt.h"
#include "protos.h"
#include "globals.h"

bool ok_time_kb;
Bitmap time_ini;
Bitmap time_end;
Bitmap time_last;

uint64_t xxx;
#define TEST_KEY_TIME    32543*10
#define MSG_INTERVAL     1800

int working_depth;

int triangularLength[MAX_PLY];
Move triangularArray[MAX_PLY][MAX_PLY];

Bitmap historia[MAX_PLY];
int len_historia;

typedef struct MoveOrder
{
    Move move;
    int score;
} MoveOrder;
MoveOrder moveOrder[MAX_PLY];
int maxMoveOrder;

FunctionEval functionEval = &eval;

void orderMoves( int ply, Move ttMove );

void quick_sort(int low, int high);

// void show_move(Move move) {
    // printf("[%c %s%s ", NAMEPZ[move.piece], POS_AH[move.from], POS_AH[move.to]);
    // if (move.capture) {
        // printf("x%c ", NAMEPZ[move.capture]);
    // }
    // if (move.promotion) {
        // printf("prom%c ", NAMEPZ[move.promotion]);
    // }
    // if (move.is_ep) {
        // printf("ep ");
    // }
    // if (move.is_2p) {
        // printf("2p ");
    // }
    // if (move.is_castle) {
        // printf("castle %d ", move.is_castle);
    // }
    // printf("]");
// }

// void test_npslimit()
// {
//     long unsigned int nps, ms;

//     if(npslimit == 0) return;

//     ms = get_ms() - time_ini;
//     if(ms==0) ms = 1;
//     nps = (long unsigned int) (inodes * 1000 / ms);

//     while (nps > npslimit)
//     {
//         #ifdef WIN32
//             Sleep(10);
//         #else
//             usleep(10000);
//         #endif
//         ms = get_ms() - time_ini;
//         if(ms==0) ms = 1;
//         nps = (long unsigned int) (inodes * 1000 / ms);
//         if ((time_end && (time_end < get_ms())) || bioskey())
//         {
//             ok_time_kb = false;
//             break;
//         }
//     }


// }

void play(int depth, int time)
{
    char fen[100];
    char bestmove[6];

    if( using_book() )
    {
        board_fen(fen);
        if( check_book(fen, bestmove) )
        {
            printf("bestmove %s\n", bestmove);
            #ifdef LOG
                fprintf(flog, "bestmove %s\n", bestmove);
            #endif
            return;
        }
        else
        {
            close_book();
        }
    }

    if( is_personality ) return play_person( depth, time );
    else play_irina( depth, time );
}

void play_irina(int depth, int time)
{
    int score;
    int i;
    char str_score[20];
    char str_move[20];
    char bestmove[6], ponder[6];
    Bitmap ms;

    ok_time_kb = true;
    time_ini = get_ms();
    if( time ) time_end = time_ini + time;
    else time_end = 0;
    time_last = time_ini;
    xxx = TEST_KEY_TIME;
    inodes = 0;
    bestmove[0] = '\0';
    ponder[0] = '\0';
    if (depth<=0) depth = 120;

    len_historia = board.ply;
    for(i=0; i < board.ply; i++)
        historia[i] = board.history[i].hashkey;
    board_reset();

    for (working_depth = 1; working_depth <= depth && ok_time_kb; working_depth++)
    {
        memset(triangularLength, 0, sizeof (triangularLength));
        memset(triangularArray, 0, sizeof (triangularArray));

        score = alphaBeta(-BIGNUMBER, +BIGNUMBER, working_depth, 0, working_depth+2);

        if (ok_time_kb)
        {
            if ((score > 9000) || (score < -9000))
            {
                sprintf(str_score, "mate %d", (score > 9000) ? 10000 - score : -(10000 + score));
            }
            else
            {
                sprintf(str_score, "cp %d", score);
            }
            //test_npslimit();
            ms = get_ms() - time_ini;
            if (ms == 0)
            {
                ms = 1;
            }
            printf("info depth %d score %s time %lu nodes %lu nps %lu pv",
                   working_depth, str_score, (long unsigned int) ms, (long unsigned int) inodes,
                   (long unsigned int) (inodes * 1000 / ms));

            #ifdef LOG
                fprintf(flog, "info depth %d score %s time %lu nodes %lu nps %lu pv",
                       working_depth, str_score, (long unsigned int) ms, (long unsigned int) inodes,
                       (long unsigned int) (inodes * 1000 / ms));
            #endif
            for (i = 0; i < triangularLength[0]; i++)
            {
                printf(" %s", move2str(triangularArray[0][i], str_move));
                #ifdef LOG
                    fprintf(flog, " %s", move2str(triangularArray[0][i], str_move));
                #endif
            }
            printf("\n");
            #ifdef LOG
                fprintf(flog, "\n");
            #endif
            if (triangularLength[0])
            {
                move2str(triangularArray[0][0], bestmove);
                if (triangularLength[0] > 1)
                {
                    move2str(triangularArray[0][1], ponder);
                }
            }
            if ((score > 9000) || (score < -9000))
            {
                break;
            }
        }
    }
    if (!bestmove[0])
    {
        if (triangularLength[0])
        {
            move2str(triangularArray[0][0], bestmove);
            if (triangularLength[0] > 1)
            {
                move2str(triangularArray[0][1], ponder);
            }
        }
    }
    if (bestmove[0])
    {
        if(is_personality && time==0) // From play_material
        {
            person_sleep(0);
        }
        printf("bestmove %s", bestmove);
        #ifdef LOG
            fprintf(flog, "bestmove %s", bestmove);
        #endif
        if (ponder[0])
        {
            printf(" ponder %s", ponder);
            #ifdef LOG
                fprintf(flog, " ponder %s", ponder);
            #endif
        }
        printf("\n");
        #ifdef LOG
            fprintf(flog, "\n");
        #endif
    }
    if( max_time )
    {
        time_test(get_ms()-time_ini);
    }
}

int noMovesScore(int ply)
{
    if (inCheck())
    {
        return -MATESCORE + ply / 2 + 1;
    }
    return DRAWSCORE;
}


inline void test_time()
{
    Bitmap ms;

    if (--xxx == 0)
    {
        ms = get_ms();
        if ((time_end && (time_end < ms)) || bioskey())
        {
            ok_time_kb = false;
        }
        xxx = TEST_KEY_TIME;

        if (ms - time_last > MSG_INTERVAL)
        {
            time_last = ms;
            ms -= time_ini;
            if (ms)
            {
                printf("info depth %d time %lu nodes %lu nps %lu\n",
                       working_depth,
                       (long unsigned int) ms, (long unsigned int) inodes,
                       (long unsigned int) (inodes * 1000 / ms));
                #ifdef LOG
                    fprintf(flog, "info depth %d time %lu nodes %lu nps %lu\n",
                        working_depth,
                        (long unsigned int) ms, (long unsigned int) inodes,
                        (long unsigned int) (inodes * 1000 / ms));
                #endif
            }
        }
    }
}

void make_null_move()
{
    // 1. Guardar el estado actual del tablero en el historial
    // Esto es crucial para poder deshacer el movimiento nulo.
    // Asume que board.history[board.ply] es el punto de guardado.

    // Copiamos la informacion esencial antes de modificarla
    board.history[board.ply].hashkey = board.hashkey;
    board.history[board.ply].ep = board.ep;
    board.history[board.ply].castle = board.castle;
    board.history[board.ply].fifty = board.fifty;

    // 2. Actualizar el Hash Zobrist (CRUCIAL)

    // Quitar la clave del lado a mover actual (ej. BLANCO)
    board.hashkey ^= HASH_side;;

    // Quitar el hash de la casilla En Passant actual (si existe)
    if (board.ep != EMPTY) {
        board.hashkey ^= HASH_ep[board.ep];
    }

    // 3. Cambiar el lado a mover y actualizar el estado

    // Cambiar el turno: Blanco -> Negro, o Negro -> Blanco
    board.side ^= 1;

    // Aumentar la profundidad (ply) para el pr?ximo movimiento
    board.ply++;

    // El movimiento nulo elimina la posibilidad de En Passant
    board.ep = EMPTY;

    // La regla de 50 movimientos aumenta (como si fuera un movimiento sin captura)
    board.fifty++;

    // 4. Actualizar el Hash Zobrist despues del cambio

    // Poner la clave del nuevo lado a mover (el mismo ZOBRIST_SIDE)
    board.hashkey ^= HASH_side;
}

void unmake_null_move()
{
    // 1. Reducir la profundidad (ply)
    board.ply--;

    // 2. Restaurar el estado anterior
    // Usamos la informacion guardada en la posicion (board.ply)

    board.hashkey = board.history[board.ply].hashkey;
    board.ep = board.history[board.ply].ep;
    board.castle = board.history[board.ply].castle;
    board.fifty = board.history[board.ply].fifty;

    // 3. Cambiar el lado a mover de vuelta
    // Esto revierte la linea 'board.side ^= 1' hecha en make_null_move
    board.side ^= 1;

    // NOTA: No es necesario re-actualizar el Hash Zobrist, ya que se restaura
    // el hashkey completo guardado en el paso 2.
}

int quiescence(int alpha, int beta, int ply, int max_ply)
{
    int k, j;
    int score;

    test_time();

    if (inCheck()) {
          return alphaBeta(alpha, beta, 1, ply, max_ply);
    }

    triangularLength[ply] = ply;

    score = functionEval();

    if( ply > max_ply) return score;

    if (score >= beta)
    {
        return beta;
    }
    if (score > alpha)
    {
        alpha = score;
    }

    movegenCaptures();
    for (k = board.ply_moves[ply]; k < board.ply_moves[ply + 1] && ok_time_kb; k++)
    {
        make_move(board.moves[k]);
        inodes++;
        //if(npslimit) test_npslimit();
        score = -quiescence(-beta, -alpha, ply + 1, max_ply);
        unmake_move();
        if (score >= beta)
        {
            return beta;
        }
        if (score > alpha)
        {
            alpha = score;
            triangularArray[ply][ply] = board.moves[k];
            for (j = ply + 1; j < triangularLength[ply + 1]; j++)
            {
                triangularArray[ply][j] = triangularArray[ply + 1][j];
            }
            triangularLength[ply] = triangularLength[ply + 1];
        }
    }
    return alpha;
}


int alphaBeta(int alpha, int beta, int depth, int ply, int max_ply)
{
    int score, best_score;
    int desde, hasta;
    unsigned k, j;
    Move move;
    Move ttMove = {0};
    int flag;

    test_time();

    if (depth == 0)
    {
        return quiescence(alpha, beta, ply, max_ply);
    }

    if (!movegen())
    {
        return noMovesScore(ply);
    }

    if( repetitions() >= 3) {
        return DRAWSCORE;
    }

    // --- TT PROBE ---
    int tt_score = probeEntry(board.hashkey, depth, alpha, beta, &ttMove);
    if (tt_score != NO_TT_HIT) {
        return tt_score;
    }

    // --- NULL MOVE ---
    if (depth >= 4 && !inCheck() && !isEndgame())
    {
        make_null_move();
        int R = (depth > 6) ? 3 : 2; // Reduccion variable

        int score = -alphaBeta(-beta, -beta + 1, depth - 1 - R, ply + 1, max_ply);

        unmake_null_move();

        if (score >= beta)
        {
            // Poda NMP exitosa
            return beta;
        }
    }

    // Pasar el ttMove a la funcion de ordenacion
    orderMoves(ply, ttMove);



    desde = board.ply_moves[ply];
    hasta = board.ply_moves[ply + 1];
    best_score = -BIGNUMBER;

    for (k = desde; k < hasta && ok_time_kb; k++)
    {
        move = board.moves[k];
        make_move(move);
        inodes++;
        //if(npslimit) test_npslimit();
        score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1, max_ply);
        unmake_move();

        if (score > best_score) {
            best_score = score;
            if (score >= beta)
            {
                // Se encontro poda Beta (Fail-hard beta cut-off)
                flag = FLAG_LOWERBOUND; // Determinar el flag aqu?

                // --- TT STORE ---
                storeEntry(board.hashkey, best_score, depth, flag, move);
                return beta;
            }
            if (score > alpha)
            {
                alpha = score;
                triangularArray[ply][ply] = move;
                for (j = ply + 1; j < triangularLength[ply + 1]; j++)
                {
                    triangularArray[ply][j] = triangularArray[ply + 1][j];
                }
                if (triangularLength[ply + 1]) triangularLength[ply] = triangularLength[ply + 1];
                else triangularLength[ply] = 1;
            }
        }
    }

    // --- TT STORE (Final) ---
    // Si salimos sin un corte beta, el resultado es o EXACTO o UPPERBOUND
    if (best_score <= alpha) {
        flag = FLAG_UPPERBOUND;
    } else {
        flag = FLAG_EXACT; // best_score fue entre el alpha original y beta
    }

    // Es CRUCIAL que se almacene el movimiento que genero el best_score como el mejor movimiento
    storeEntry(board.hashkey, best_score, depth, flag, triangularArray[ply][ply]);

    return best_score;
}


void orderMoves( int ply, Move ttMove )
{
    unsigned k, i, n;

    if (ttMove.from != 0 || ttMove.to != 0) {
        for (k = board.ply_moves[ply]; k < board.ply_moves[ply + 1]; k++) {
            if (board.moves[k].from == ttMove.from && board.moves[k].to == ttMove.to) {
                // Mover ttMove al inicio de la lista de movimientos
                Move temp = board.moves[k];
                board.moves[k] = board.moves[board.ply_moves[ply]];
                board.moves[board.ply_moves[ply]] = temp;
                break;
            }
        }
    }

    for (i = 0, n = 0, k = board.ply_moves[ply]; k < board.ply_moves[ply + 1]; k++, i++)
    {
        moveOrder[i].move = board.moves[k];
        n++;
        make_move(board.moves[k]);
        moveOrder[i].score = functionEval();
        unmake_move();
    }
    quick_sort( 0, n-1);
    for (i = 0, k = board.ply_moves[ply]; i < n; k++, i++)
    {
        board.moves[k] = moveOrder[i].move;
    }
}

int repetitions()
{
    int i, ilast, rep;
    Bitmap hashkey = board.hashkey;
    rep = 1;
    ilast = board.ply - board.fifty;
    if(ilast < 0)
    {
        for (i = len_historia - 1; i >= (len_historia+ilast)-1 && i; i--)
        {
            if( historia[i] == hashkey ) rep++;
        }
    }
    for (i = board.ply - 2; i >= ilast && i; i -= 2)
    {
        if (board.history[i].hashkey == hashkey) rep++;
    }
    return rep;
}



void quick_sort(int low, int high)
{
    int pivot,j,i;
    MoveOrder temp;

    if( low < high )
    {
        pivot = low;
        i = low;
        j = high;

        while( i < j )
        {
            while( (moveOrder[i].score <= moveOrder[pivot].score) && (i<high) ) i++;

            while(moveOrder[j].score>moveOrder[pivot].score) j--;

            if(i<j)
            {
                temp=moveOrder[i];
                moveOrder[i]=moveOrder[j];
                moveOrder[j]=temp;
            }
        }

        temp=moveOrder[pivot];
        moveOrder[pivot]=moveOrder[j];
        moveOrder[j]=temp;
        quick_sort(low,j-1);
        quick_sort(j+1,high);
    }
}

