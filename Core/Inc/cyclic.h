/*
 * cyclic.h
 *
 *  Created on: 2023年3月5日
 *      Author: utt.yao
 */

#ifndef SRC_CYCLIC_H_
#define SRC_CYCLIC_H_
#define num_tasks 4
#define TASK_NAME_LEN 20
#define CYCLIC_MODE_TRIGGER_PIN BUTTON_EXTI13_Pin
#define setPos(x) __asm__ __volatile__("mov r2,pc\n\tsub r2,4\n\tmov %0,r2":"=r"(pos)::"r2")
#define jumpPos(x) __asm__ __volatile__("mov pc,%0\n\tnop\n\tnop"::"r"(pos):"pc")
#define floor(x)(((float)x-(int)(x))>0.0?((int)(x) + 1):(x))
#define ceil(x) (((float)x-(int)(x))>0.0?((int)(x) - 1):(x))
#define abs(x) ((x)<0?(-(x)):(x))
#define MAX_MINOR_CYCLES 200
#define SHOW_WINDOW 10
typedef struct minor_cycle
{
	int minor_code;
	//char task_name[num_tasks][TASK_NAME_LEN];
	int task_code[num_tasks];
	void (*taskList[num_tasks])(void) ;
	int n_tasks;
}Minor_Cycle;
typedef struct task
{
	char task_name[TASK_NAME_LEN];
	void (*task)(void) ;
	int period;
	int execution;
	int periodSum;
	int periodNum;
	int executionSum;
	int executionNum;
	int taskTick;
}Task;
extern volatile int pos;
extern volatile int mode;
extern volatile Task tasks[num_tasks];
extern unsigned int major_cycle_len;
extern unsigned int minor_cycle_len;
extern unsigned int major_cycle;
extern unsigned int minor_cycle;
extern unsigned int number_minor_cycle;
extern unsigned int system_time;
extern volatile int changeModeMark;
extern volatile int tinyTime;
int gcd(int a, int b);
int lcm(int a,int b);
void buildFastMatrix(void);
void registerTask(void (*task)(void),char *name,int minor, int index, int code, int period);
void task_scheduler_tick_reset(void);
void task_scheduler(void);

void recoverDelayMark(void);
void showFastMatrix(void);
#define FAST_EN
#endif /* SRC_CYCLIC_H_ */
