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

Move killer_moves[MAX_PLY][2];
int history[16][64];

static int piece_mvv[8] = {0, 100, 0, 320, 0, 330, 500, 900};

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

void stop_search(void)
{
    ok_time_kb = false;
    time_end = 1;
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

    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history, 0, sizeof(history));

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
        else
        {
            movegen();
            if (board.idx_moves > board.ply_moves[0])
            {
                move2str(board.moves[board.ply_moves[0]], bestmove);
            }
        }
    }
    if (bestmove[0])
    {
        // Validate ponder move before sending to GUI
        if (ponder[0]) {
            unsigned int save_ply = board.ply;
            unsigned int save_idx = board.idx_moves;
            make_move(triangularArray[0][0]);
            unsigned int gen_start = board.idx_moves;
            movegen();
            char tmp[6];
            int i;
            bool found = false;
            for (i = gen_start; i < (int)board.idx_moves; i++) {
                if (!strcmp(move2str(board.moves[i], tmp), ponder)) {
                    found = true;
                    break;
                }
            }
            unmake_move();
            board.ply = save_ply;
            board.idx_moves = save_idx;
            if (!found) ponder[0] = '\0';
        }
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


void test_time()
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

    // 2. Actualizar el Hash Zobrist

    // Quitar el hash de la casilla En Passant actual (si existe)
    if (board.ep != EMPTY) {
        board.hashkey ^= HASH_ep[board.ep];
    }

    // 3. Cambiar el lado a mover y actualizar el estado

    // Cambiar el turno: Blanco -> Negro, o Negro -> Blanco
    board.side ^= 1;
    // Ajustar el hash para el nuevo lado
    board.hashkey ^= HASH_side;

    // Aumentar la profundidad (ply)
    board.ply++;

    // El movimiento nulo elimina la posibilidad de En Passant
    board.ep = EMPTY;

    // La regla de 50 movimientos aumenta
    board.fifty++;
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

static int get_attacker(int square, int side, Bitmap occ)
{
    Bitmap atk;
    int from;

    if (side == WHITE) {
        atk = board.white_pawns & BLACK_PAWN_ATTACKS[square] & occ;
        if (atk) return first_one(atk);
        atk = board.white_knights & KNIGHT_ATTACKS[square] & occ;
        if (atk) return first_one(atk);
        atk = board.white_bishops & DIAG_ATTACKS[square] & occ;
        while (atk) { from = first_one(atk); if (!(FREEWAY[from][square] & occ)) return from; atk ^= BITSET[from]; }
        atk = board.white_rooks & LINE_ATTACKS[square] & occ;
        while (atk) { from = first_one(atk); if (!(FREEWAY[from][square] & occ)) return from; atk ^= BITSET[from]; }
        atk = board.white_queens & (LINE_ATTACKS[square] | DIAG_ATTACKS[square]) & occ;
        while (atk) { from = first_one(atk); if (!(FREEWAY[from][square] & occ)) return from; atk ^= BITSET[from]; }
        atk = board.white_king & KING_ATTACKS[square] & occ;
        if (atk) return first_one(atk);
    } else {
        atk = board.black_pawns & WHITE_PAWN_ATTACKS[square] & occ;
        if (atk) return first_one(atk);
        atk = board.black_knights & KNIGHT_ATTACKS[square] & occ;
        if (atk) return first_one(atk);
        atk = board.black_bishops & DIAG_ATTACKS[square] & occ;
        while (atk) { from = first_one(atk); if (!(FREEWAY[from][square] & occ)) return from; atk ^= BITSET[from]; }
        atk = board.black_rooks & LINE_ATTACKS[square] & occ;
        while (atk) { from = first_one(atk); if (!(FREEWAY[from][square] & occ)) return from; atk ^= BITSET[from]; }
        atk = board.black_queens & (LINE_ATTACKS[square] | DIAG_ATTACKS[square]) & occ;
        while (atk) { from = first_one(atk); if (!(FREEWAY[from][square] & occ)) return from; atk ^= BITSET[from]; }
        atk = board.black_king & KING_ATTACKS[square] & occ;
        if (atk) return first_one(atk);
    }
    return -1;
}

static int see(int square, int side, int captured_value, Bitmap occ)
{
    int from = get_attacker(square, side, occ);
    if (from == -1) return 0;

    occ ^= BITSET[from];
    int attacker_value = piece_mvv[board.pz[from] & 7];
    return captured_value - see(square, !side, attacker_value, occ);
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
        // Skip losing captures (SEE < 0)
        if (!board.moves[k].is_ep) {
            int victim = board.moves[k].capture & 7;
            int see_val = see(board.moves[k].to, board.side, piece_mvv[victim], board.all_pieces);
            if (see_val < 0) continue;
            // Delta pruning: skip captures that can't raise alpha
            if (score + piece_mvv[victim] + 200 < alpha) continue;
        }
        make_move(board.moves[k]);
        inodes++;
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

    // Check extension: only when deep enough to avoid search explosion
    if (inCheck() && depth >= 4) {
        depth++;
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
        // LMR: reduce late quiet moves
        if (depth >= 3 && k - desde >= 3 && !move.capture && !move.promotion && !inCheck()) {
            int R = 1;
            if (depth >= 6) R++;
            if (k - desde >= 6) R++;
            int rd = depth - 1 - R;
            if (rd < 1) rd = 1;
            score = -alphaBeta(-beta, -alpha, rd, ply + 1, max_ply);
            if (score > alpha) {
                score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1, max_ply);
            }
        } else {
            score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1, max_ply);
        }
        unmake_move();

        if (score > best_score) {
            best_score = score;
            if (score >= beta)
            {
                if (!move.capture && !move.promotion)
                {
                    killer_moves[ply][1] = killer_moves[ply][0];
                    killer_moves[ply][0] = move;
                    history[move.piece][move.to] += depth * depth;
                }
                flag = FLAG_LOWERBOUND;

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

        if (board.moves[k].capture) {
            int attacker = board.moves[k].piece & 7;
            int victim = board.moves[k].capture & 7;
            int mvv_lva = piece_mvv[victim] * 10 - piece_mvv[attacker];
            if (board.moves[k].is_ep) {
                moveOrder[i].score = 1000000 + mvv_lva;
            } else {
                int see_val = see(board.moves[k].to, board.side, piece_mvv[victim], board.all_pieces);
                if (see_val > 0)
                    moveOrder[i].score = 2000000 + mvv_lva;
                else if (see_val == 0)
                    moveOrder[i].score = 1000000 + mvv_lva;
                else
                    moveOrder[i].score = -1000000 + mvv_lva;
            }
        }
        else {
            make_move(board.moves[k]);
            moveOrder[i].score = functionEval();
            unmake_move();
            if (board.moves[k].from == killer_moves[ply][0].from && board.moves[k].to == killer_moves[ply][0].to)
                moveOrder[i].score += 500;
            else if (board.moves[k].from == killer_moves[ply][1].from && board.moves[k].to == killer_moves[ply][1].to)
                moveOrder[i].score += 400;
            moveOrder[i].score += history[board.moves[k].piece][board.moves[k].to];
        }
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
    for (i = board.ply - 2; i >= ilast && i > 0; i -= 2)
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
            while( (moveOrder[i].score >= moveOrder[pivot].score) && (i<high) ) i++;

            while(moveOrder[j].score<moveOrder[pivot].score) j--;

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

