#ifndef _Q3E_BT_H
#define _Q3E_BT_H

#define SAMPLE_SOLUTION_CFI 1
#define SAMPLE_SOLUTION_FP  2
#define SAMPLE_SOLUTION_EH  4

void Q3E_BT_SetupSolution(int mask);
void Q3E_BT_Init(void);
void Q3E_BT_Shutdown(void);
void Q3E_BT_SignalCaughted(void (*func) (int num, int pid, int tid, int mask, const char *cfi, const char *fp, const char *eh));
void Q3E_BT_AfterCaught(int (*func) (int));

#endif //_XUNWIND
