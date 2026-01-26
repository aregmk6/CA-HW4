/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"

typedef struct {
    double CPI_blocked;

    tcontext *blocked_thread_context;

} Blocked_stats;

typedef struct {
    double CPI_fineGrained;

    tcontext *grained_thread_context;

} Grained_stats;

typedef struct {
    bool is_alive;
    int blocked;
    int line;
} Thread_data;

static Blocked_stats blocked_stats = {0};
static Grained_stats grained_stats = {0};

void init_grained_stats(int thread_nr) {
    grained_stats.grained_thread_context =
        (tcontext *)malloc(sizeof(tcontext) * thread_nr);
    memset(
        grained_stats.grained_thread_context, 0, sizeof(tcontext) * thread_nr);
}

void init_blocked_stats(int thread_nr) {
    blocked_stats.blocked_thread_context =
        (tcontext *)malloc(sizeof(tcontext) * thread_nr);
    memset(
        blocked_stats.blocked_thread_context, 0, sizeof(tcontext) * thread_nr);
}

// void SIM_MemDataRead(uint32_t addr, int32_t *dst);
//
// void SIM_MemDataWrite(uint32_t addr, int32_t val);
//
// void SIM_MemInstRead(uint32_t line, Instruction *dst, int tid);
//
// typedef enum {
// 	CMD_NOP = 0,
//     CMD_ADD,     // dst <- src1 + src2
//     CMD_SUB,     // dst <- src1 - src2
//     CMD_ADDI,    // dst <- src1 + imm
//     CMD_SUBI,    // dst <- src1 - imm
//     CMD_LOAD,    // dst <- Mem[src1 + src2]  (src2 may be an immediate)
//     CMD_STORE,   // Mem[dst + src2] <- src1  (src2 may be an immediate)
// 	CMD_HALT,
// } cmd_opcode;
//
// typedef struct _inst {
// 	cmd_opcode opcode;
// 	int dst_index;
// 	int src1_index;
// 	int src2_index_imm;
// 	bool isSrc2Imm; // if the second argument is immediate
// } Instruction;

void CORE_BlockedMT() {
    int thread_nr = SIM_GetThreadsNum();
    int total_cycles = 0;
    int total_instructions = 0;
    int dead_threads_nr = 0;
    int alive_threads_nr = thread_nr;
    int blocked_threads_nr = 0;
    int load_lat = SIM_GetLoadLat();
    int store_lat = SIM_GetStoreLat();
    int switch_lat = SIM_GetSwitchCycles();
    int cur_thread = 0;
    int cur_line = 0;

    Instruction cur_inst = {0};

    Thread_data *live_threads =
        (Thread_data *)malloc(sizeof(Thread_data) * thread_nr);
    /* create alias for safety */
    Thread_data *alias_live_threads = live_threads;
    for (int t = 0; t < thread_nr; t++) {
        alias_live_threads[t].is_alive = true;
        alias_live_threads[t].blocked = 0;
        alias_live_threads[t].line = 0;
    }

    init_blocked_stats(thread_nr);

    int *cur_dest = NULL;
    int cur_src1 = 0;
    int cur_src2 = 0;

    while (dead_threads_nr < thread_nr) {
        /* make blocked threads do their work */
        if (alias_live_threads[cur_thread].is_alive &&
            !alias_live_threads[cur_thread].blocked) {
            /* only do work if thread is still alive */

            /* init instruction */
            cur_line = alias_live_threads[cur_thread].line;
            SIM_MemInstRead(cur_line, &cur_inst, cur_thread);
            cur_dest = &blocked_stats.blocked_thread_context[cur_thread]
                            .reg[cur_inst.dst_index];
            cur_src1 = blocked_stats.blocked_thread_context[cur_thread]
                           .reg[cur_inst.src1_index];
            if (!cur_inst.isSrc2Imm) {
                cur_src2 = blocked_stats.blocked_thread_context[cur_thread]
                               .reg[cur_inst.src2_index_imm];
            } else {
                cur_src2 = cur_inst.src2_index_imm;
            }

            /* do instruction */
            switch (cur_inst.opcode) {
            case CMD_NOP:
                break;
            case CMD_ADD:
            case CMD_ADDI:
                *cur_dest = cur_src1 + cur_src2;
                break;
            case CMD_SUB:
            case CMD_SUBI:
                *cur_dest = cur_src1 - cur_src2;
                break;
            case CMD_LOAD:
                // LOAD $dst, $src1, $src2 dst <- Mem[src1 + src2]
                SIM_MemDataRead(cur_src1 + cur_src2, cur_dest);
                alias_live_threads[cur_thread].blocked = load_lat;
                blocked_threads_nr++;
                break;
            case CMD_STORE:
                // STORE $dst, $src1, $src2 Mem[dst + src2] < -src1
                SIM_MemDataWrite(*cur_dest + cur_src2, cur_src1);
                alias_live_threads[cur_thread].blocked = store_lat;
                blocked_threads_nr++;
                break;
            case CMD_HALT:
                alias_live_threads[cur_thread].is_alive = false;
                dead_threads_nr++;
                alive_threads_nr--;
                break;
            }
            alias_live_threads[cur_thread].line++;
            total_cycles++;
            total_instructions++;
            /* decrement blocked counters */
            for (int t = 0; t < thread_nr; t++) {
                if (t == cur_thread) continue;
                if (alias_live_threads[t].is_alive &&
                    alias_live_threads[t].blocked > 0) {
                    alias_live_threads[t].blocked--;
                    if (alias_live_threads[t].blocked == 0) {
                        blocked_threads_nr--;
                    }
                }
            }
        } else { /* in idle */
            /* decrement blocked for all blocked threads */
            for (int t = 0; t < thread_nr; t++) {
                if (alias_live_threads[t].is_alive &&
                    alias_live_threads[t].blocked > 0) {
                    alias_live_threads[t].blocked--;
                    if (alias_live_threads[t].blocked == 0) {
                        blocked_threads_nr--;
                    }
                }
            }
            total_cycles++;
        }

        for (int i = 0; i < thread_nr; i++) {
            int t = (cur_thread + i) % thread_nr;
            if (alias_live_threads[t].is_alive &&
                !alias_live_threads[t].blocked) {
                cur_thread = t;
                if (i > 0) {
                    total_cycles += switch_lat;
                    for (int t = 0; t < thread_nr; t++) {
                        if (alias_live_threads[t].is_alive &&
                            alias_live_threads[t].blocked > 0) {
                            for (int i = 0;                         //
                                 i < switch_lat &&                  //
                                 alias_live_threads[t].blocked > 0; //
                                 i++) {
                                alias_live_threads[t].blocked--;
                                if (alias_live_threads[t].blocked == 0) {
                                    blocked_threads_nr--;
                                }
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    blocked_stats.CPI_blocked = (double)(total_cycles) / total_instructions;

    free(live_threads);
}

void CORE_FinegrainedMT() {
    int thread_nr = SIM_GetThreadsNum();
    int total_cycles = 0;
    int total_instructions = 0;
    int dead_threads_nr = 0;
    int alive_threads_nr = thread_nr;
    int blocked_threads_nr = 0;
    int load_lat = SIM_GetLoadLat();
    int store_lat = SIM_GetStoreLat();
    int cur_thread = 0;
    int cur_line = 0;

    Instruction cur_inst = {0};

    Thread_data *live_threads =
        (Thread_data *)malloc(sizeof(Thread_data) * thread_nr);
    /* create alias for safety */
    Thread_data *alias_live_threads = live_threads;
    for (int t = 0; t < thread_nr; t++) {
        alias_live_threads[t].is_alive = true;
        alias_live_threads[t].blocked = 0;
        alias_live_threads[t].line = 0;
    }

    init_grained_stats(thread_nr);

    int *cur_dest = NULL;
    int cur_src1 = 0;
    int cur_src2 = 0;

    while (dead_threads_nr < thread_nr) {
        /* make blocked threads do their work */
        if (alias_live_threads[cur_thread].is_alive &&
            !alias_live_threads[cur_thread].blocked) {
            /* only do work if thread is still alive */

            /* init instruction */
            cur_line = alias_live_threads[cur_thread].line;
            SIM_MemInstRead(cur_line, &cur_inst, cur_thread);
            cur_dest = &grained_stats.grained_thread_context[cur_thread]
                            .reg[cur_inst.dst_index];
            cur_src1 = grained_stats.grained_thread_context[cur_thread]
                           .reg[cur_inst.src1_index];
            if (!cur_inst.isSrc2Imm) {
                cur_src2 = grained_stats.grained_thread_context[cur_thread]
                               .reg[cur_inst.src2_index_imm];
            } else {
                cur_src2 = cur_inst.src2_index_imm;
            }

            /* do instruction */
            switch (cur_inst.opcode) {
            case CMD_NOP:
                break;
            case CMD_ADD:
            case CMD_ADDI:
                *cur_dest = cur_src1 + cur_src2;
                break;
            case CMD_SUB:
            case CMD_SUBI:
                *cur_dest = cur_src1 - cur_src2;
                break;
            case CMD_LOAD:
                // LOAD $dst, $src1, $src2 dst <- Mem[src1 + src2]
                SIM_MemDataRead(cur_src1 + cur_src2, cur_dest);
                alias_live_threads[cur_thread].blocked = load_lat;
                blocked_threads_nr++;
                break;
            case CMD_STORE:
                // STORE $dst, $src1, $src2 Mem[dst + src2] < -src1
                SIM_MemDataWrite(*cur_dest + cur_src2, cur_src1);
                alias_live_threads[cur_thread].blocked = store_lat;
                blocked_threads_nr++;
                break;
            case CMD_HALT:
                alias_live_threads[cur_thread].is_alive = false;
                dead_threads_nr++;
                alive_threads_nr--;
                break;
            }
            alias_live_threads[cur_thread].line++;
            total_cycles++;
            total_instructions++;
            /* decrement blocked counters */
            for (int t = 0; t < thread_nr; t++) {
                if (t == cur_thread) continue;
                if (alias_live_threads[t].is_alive &&
                    alias_live_threads[t].blocked > 0) {
                    alias_live_threads[t].blocked--;
                    if (alias_live_threads[t].blocked == 0) {
                        blocked_threads_nr--;
                    }
                }
            }
        } else { /* in idle */
            /* decrement blocked for all blocked threads */
            for (int t = 0; t < thread_nr; t++) {
                if (alias_live_threads[t].is_alive &&
                    alias_live_threads[t].blocked > 0) {
                    alias_live_threads[t].blocked--;
                    if (alias_live_threads[t].blocked == 0) {
                        blocked_threads_nr--;
                    }
                }
            }
            total_cycles++;
        }

        for (int i = 1; i < thread_nr; i++) {
            int t = (cur_thread + i) % thread_nr;
            if (alias_live_threads[t].is_alive &&
                !alias_live_threads[t].blocked) {
                cur_thread = t;
                break;
            }
        }
    }

    grained_stats.CPI_fineGrained = (double)(total_cycles) / total_instructions;

    free(live_threads);
}

double CORE_BlockedMT_CPI() {
    double ret = blocked_stats.CPI_blocked;
    free(blocked_stats.blocked_thread_context);
    return ret;
}

double CORE_FinegrainedMT_CPI() {
    double ret = grained_stats.CPI_fineGrained;
    free(grained_stats.grained_thread_context);
    return ret;
}

void CORE_BlockedMT_CTX(tcontext *context, int threadid) {
    context[threadid] = blocked_stats.blocked_thread_context[threadid];
}

void CORE_FinegrainedMT_CTX(tcontext *context, int threadid) {
    context[threadid] = grained_stats.grained_thread_context[threadid];
}
