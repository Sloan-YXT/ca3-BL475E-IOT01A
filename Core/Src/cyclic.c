#include "cyclic.h"
#include "hal_config.h"
#include "string.h"
#include "stdio.h"
#include "stm32l4xx.h"
#define MAJOR_CYCLE_LEN 1500
#define MINOR_CYCLE_LEN 300
#define NUMBER_MINOR_CYCLE 5
unsigned int major_cycle_len = MAJOR_CYCLE_LEN;
unsigned int minor_cycle_len = MINOR_CYCLE_LEN;
unsigned int major_cycle;
unsigned int minor_cycle;
unsigned int number_minor_cycle = NUMBER_MINOR_CYCLE;
//sys_time is for delay recovery
unsigned int system_time;
volatile Task tasks[num_tasks];
volatile int jpos = 0;
volatile int mode = 0;
volatile int pos;
volatile int changeModeMark = 0;
volatile int tinyTime = 0;
Minor_Cycle basic_matrix[num_tasks];
//Minor_Cycle *fast_matrix;
Minor_Cycle fast_matrix[MAX_MINOR_CYCLES+1];
int gcd(int a, int b)
{
    int _max, _min;
    while (1)
    {
        _max = a >= b ? a : b;
        _min = a < b ? a : b;
        int r = _max % _min;
        if (r == 0)
            break;
        a = r;
        b = _min;
    }
    return _min;
}
//void strcpy(char *a,char *b)
//{
//	while(*b)
//	{
//		*a = *b;
//		a++;
//		b++;
//	}
//}
void registerTask(void (*task)(void),char *name,int minor, int index, int code,int period)
{
	basic_matrix[minor].minor_code = minor;
	//strcpy(basic_matrix[minor].task_name[index],name);
	basic_matrix[minor].task_code[index] = code;
	basic_matrix[minor].taskList[index] = task;
	basic_matrix[minor].n_tasks++;
	tasks[code].task = task;
	strcpy(tasks[code].task_name,name);
	tasks[code].period = period;
}
void task_scheduler_tick_reset(void)
{
	major_cycle = 0;
	minor_cycle = 0;
	HAL_TIM_Base_Stop_IT(&TIM1_Handler);
	TIM1_Handler.Init.Period=SystemCoreClock/(TIM1_Handler.Init.Prescaler+1)/1000*minor_cycle_len;//100ms
	HAL_TIM_Base_Init(&TIM1_Handler);
	HAL_TIM_Base_Start_IT(&TIM1_Handler);
}
static void changeMode(int showWindowMs)
{
	//change mode
#ifdef  FAST_EN
	char *message = (mode == 0?"----------------------------now in mode basic------------------------------\r\n":"---------------------now in mode fast--------------------\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
	if(mode == 0)
	{
		for(int i=0;i<num_tasks;i++)
		{
			tasks[i].periodSum = 0;
			tasks[i].periodNum = 0;
			tasks[i].taskTick = 0;
			tasks[i].executionSum = 0;
			tasks[i].executionNum = 0;
		}
#ifdef PERIOD_MEASUREMENT
		lsm6dsl_dready_en();
		lis3mdl_dready_en();
		lps22hb_dready_en();
		hts221_dready_en();
#endif
		major_cycle_len = MAJOR_CYCLE_LEN;
		minor_cycle_len = MINOR_CYCLE_LEN;
		number_minor_cycle = NUMBER_MINOR_CYCLE;
		major_cycle = 0;
		minor_cycle = 0;
	}

	else
	{
#ifdef PERIOD_MEASUREMENT
		lsm6dsl_dready_dis();
		lis3mdl_dready_dis();
		lps22hb_dready_dis();
		hts221_dready_dis();
#endif
		buildFastMatrix();
		showFastMatrix();
		//check the matrix
		char message[200];
		sprintf(message,"major_cycle=%d,minor_cycle=%d,number_of_minor=%d\r\n",major_cycle_len,minor_cycle_len,number_minor_cycle);
		HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
		for(int i=0;i<num_tasks;i++)
		{
			sprintf(message,"task %s:(period,execution):(%d,%d)\r\n",tasks[i].task_name,tasks[i].period,tasks[i].execution);
			HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
		}
		HAL_Delay(showWindowMs*1000);

	}
	task_scheduler_tick_reset();
#else
	for(int i=0;i<num_tasks;i++)
	{
#ifdef PERIOD_MEASUREMENT
		tasks[i].period = floor(tasks[i].periodSum/tasks[i].periodNum);
#endif
		tasks[i].execution = floor(tasks[i].executionSum/tasks[i].executionNum);
		char message[200];
		sprintf(message,"task %s:(period,execution):(%d,%d)\r\n",tasks[i].task_name,tasks[i].period,tasks[i].execution);
		HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
	}
	HAL_Delay(showWindowMs*1000);
#endif
}
void task_scheduler(void)
{
	task_scheduler_tick_reset();
	while(1)
	{
		//setPos();
		system_time = HAL_GetTick();
		if(changeModeMark)
		{
			changeModeMark = 0;
			changeMode(SHOW_WINDOW);
		}
		pos=0;
		if(mode==0)
		{
			for(int i=0;i<basic_matrix[minor_cycle].n_tasks;i++)
			{
				int t1 = HAL_GetTick();
				basic_matrix[minor_cycle].taskList[i]();
				int t2 = HAL_GetTick();
				int code = basic_matrix[minor_cycle].task_code[i];
				tasks[code].executionSum+=(t2-t1);
				tasks[code].executionNum++;
			}
		}
		else
		{
			for(int i=0;i<fast_matrix[minor_cycle].n_tasks;i++)
			{
				char message[200];
//				sprintf(message,"(%d tasks)(%d,%d)execution addr:%x\r\n",fast_matrix[minor_cycle].n_tasks,minor_cycle,i,fast_matrix[minor_cycle].taskList[i]);
//				HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
				fast_matrix[minor_cycle].taskList[i]();
			}
		}
		minor_cycle++;
		if(minor_cycle==number_minor_cycle)
		{
			minor_cycle = 0;
			major_cycle++;
		}
		while(pos==0);
	}
}
int lcm(int a,int b)
{
	int m = gcd(a,b);
	int a1 = a/m;
	int b1 = b/m;
	return a1*b1*m;
}
//surprisingly, high robustness~!
void buildFastMatrix(void)
{
	for(int i=0;i<num_tasks;i++)
	{
#ifdef PERIOD_MEASUREMENT
		tasks[i].period = floor(tasks[i].periodSum/tasks[i].periodNum);
#endif
		if( tasks[i].executionNum==0)
		{
			tasks[i].execution = tasks[i].period;
			continue;
		}
		tasks[i].execution = floor(tasks[i].executionSum/ tasks[i].executionNum);
		//align period to 5
		int r = tasks[i].period%5;
		if(r>0)
		tasks[i].period = tasks[i].period+5-r;
 	}
	if(num_tasks>1)
	{
		int gcd_matrix[num_tasks-1],lcm_matrix[num_tasks-1];
		gcd_matrix[0] = gcd(tasks[0].period,tasks[1].period);
		lcm_matrix[0] = lcm(tasks[0].period,tasks[1].period);
		for(int i=0;i<=num_tasks-3;i++)
		{
			gcd_matrix[i+1] = gcd(gcd_matrix[i],tasks[i+2].period);
			lcm_matrix[i+1] = lcm(lcm_matrix[i],tasks[i+2].period);
		}

		minor_cycle_len = gcd_matrix[num_tasks-2];
		major_cycle_len = lcm_matrix[num_tasks-2];
	}
	else
	{
		minor_cycle_len = tasks[0].execution;
		major_cycle_len = tasks[0].period;
	}
	//n-lcm is hard to compute, so here we don't use lcm, we don't have much chance to
	//use lcm where gcd > max(E) anyway
	int ready_queen[num_tasks+1];
	int rest_time_tasks[num_tasks];
	struct task_bond
	{
		int exe;
		int code;
	};
	//we need this code Map for minor_cycle modification
	struct task_bond taskOrder[num_tasks];
	for(int i=0;i<num_tasks;i++)
	{
		taskOrder[i].code = i;
		taskOrder[i].exe = tasks[i].execution;
	}
	for(int i=num_tasks;i>1;i--)
	{
		int pos = 0;
		for(int j=0;j<i;j++)
		{
			if(taskOrder[j].exe>taskOrder[pos].exe)
			{
				pos = j;
			}
		}
		struct task_bond tmp = taskOrder[i-1];
		taskOrder[i-1] = taskOrder[pos];
		taskOrder[pos] = tmp;
	}
	if(taskOrder[num_tasks-1].exe>minor_cycle_len)
	{
		minor_cycle_len = taskOrder[num_tasks-1].exe;
		int r = minor_cycle_len % 5;
		if(r>0)
		{
			minor_cycle_len+=(5-r);
		}
	}
	//Get largestT as major cycle
	//We also need this cause I want a period oriented sjf like scheduler in each minor cycle
	struct task_bond taskT[num_tasks];
	for(int i=0;i<num_tasks;i++)
	{
		taskT[i].code = i;
		taskT[i].exe = tasks[i].period;
	}
	for(int i=num_tasks;i>1;i--)
	{
		int pos = 0;
		for(int j=0;j<i;j++)
		{
			if(taskT[j].exe>taskT[pos].exe)
			{
				pos = j;
			}
		}
		struct task_bond tmp = taskT[i-1];
		taskT[i-1] = taskT[pos];
		taskT[pos] = tmp;
	}
	number_minor_cycle = floor(major_cycle_len/minor_cycle_len);
	int tail_idle = 0;
	if(number_minor_cycle> MAX_MINOR_CYCLES)
	{
		//for table scheduler, we may miss scheduling between each task's periodic points, but generally
		//in each major cycle, the states of each tasks should be ok, in another word, each major_cycle should
		//be identical, and the tasks shouldn't execute before its' period point come.
		//From what I can see, this one is not obvious, we need to check if last execution of taskX will exceed major cycle(lcm),
		//if it will, the taskX shouldn't execute, there needs a blank before next major cycle.
		//Missing scheduling between scheduling points is tolerable in table scheduler(if ddl is absent), but execute task before its scheduling point is
		//not tolerable in some cases.
		//What's more, for our accelerometer, execution time may even be longer than period,
		//a good proof of missing scheduling is ok...(rare, but happened actually)
		number_minor_cycle =  MAX_MINOR_CYCLES;
		//this one is to guarantee period behavior doesn't go wrong
		//when new cycle begins, all the tasks should be ready(at pos_t 0)
		//though not necessary for our tasks, but for general purpose
		//We should change length of minor cycles instead of length of major cycles, for period task principle
		minor_cycle_len = major_cycle_len/number_minor_cycle;
		if(minor_cycle_len*number_minor_cycle!=major_cycle_len)
		{
			number_minor_cycle+=1;
			//shouldn't do any tasks in idle minor cycle,
			//or there's possibility we don't get all tasks ready in new major cycle
			tail_idle = 1;
		}
		char message[200];
		sprintf(message,"warning:number of minor cycles(%d) overlapping boundary(%d)",number_minor_cycle,MAX_MINOR_CYCLES);
		HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
	}
	//rdy queue doesn't allow 2 same task
	int queen_mark[num_tasks];
	//first enqueue use execution order, and the rest time use
	for(int i=0;i<num_tasks;i++)
	{
		ready_queen[i] = taskOrder[i].code;
		rest_time_tasks[i] = 0;
		queen_mark[i] = 1;
	}
//	extern int _eheap;
//	fast_matrix = (Minor_Cycle*)&_eheap;
	//fast_matrix = (Minor_Cycle*)(HEAP_BASE - sizeof(Minor_Cycle)*number_minor_cycle);
	int head = 0,tail = num_tasks;
	//core of the scheduler
	for(int i=0;i<number_minor_cycle;i++)
	{
		int rest_time = minor_cycle_len;
		int numer = 0;
		fast_matrix[i].minor_code = i;
		while(1)
		{
			//queen is the same size as num_tasks, thus never gets full
			if(head==tail)
			{
				//there maybe hole minor cycle, however handles it would be impossible in our algorithm which
				//complies relatively generic way of scheduling
				break;
			}
			else
			{
				int taskCode = ready_queen[(head)%(num_tasks+1)];

				rest_time-=tasks[taskCode].execution;
				if(rest_time<0)
				{
					break;
				}
				//dequeue
				head++;
				rest_time_tasks[taskCode] = tasks[taskCode].period;
				queen_mark[taskCode] = 0;
				fast_matrix[i].task_code[numer] = taskCode;
				//strcpy(fast_matrix[i].task_name[numer],tasks[taskCode].task_name);
				fast_matrix[i].taskList[numer] = tasks[taskCode].task;
				numer++;
			}
		}
		for(int j=0;j<num_tasks;j++)
		{
			//already in queue: skip
			if(queen_mark[taskT[j].code]==1)
			{
				continue;
			}
			rest_time_tasks[taskT[j].code]-=minor_cycle_len;
			if(rest_time_tasks[taskT[j].code]<=0)
			{
				ready_queen[tail%(num_tasks+1)] = taskT[j].code;
				tail++;
				queen_mark[taskT[j].code] = 1;
			}
		}
		fast_matrix[i].n_tasks = numer;
	}
	if(tail_idle==1)
	{
		fast_matrix[number_minor_cycle-1].n_tasks = 0;
	}
}
//these to eliminate higher priority interrupt's bad consequence(higher than timer1 tick which is scheduling tick)
void recoverDelayMark(void)
{
	unsigned int major_pass,minor_pass;
	unsigned int delay = HAL_GetTick();
	major_pass = ceil(abs(delay-system_time)/major_cycle_len);
	unsigned int tmp = abs(delay-system_time)%major_cycle_len;
    minor_pass = ceil(tmp/minor_cycle);
    major_cycle += major_pass;
    minor_cycle += minor_pass;
    unsigned int rd = minor_cycle / (number_minor_cycle);
    if(rd>0)
    {
    	major_cycle+=1;
    	minor_cycle = minor_cycle % number_minor_cycle;
    }
//    char message[200];
//    sprintf(message,"(major:%d,minor:%d)\r\n",major_cycle,minor_cycle);
//    HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message),0xFFFF);
    //if time chunk between minor cycle, we must wait until the end of that cycle to guarantee predictability
    //though may lose some time, but delay is unpredictable, and if we want to make it predictable we need to pay a price
    if(tmp%minor_cycle>0)
    {
    	//error range: 1 minor cycle
    	//solve this needs execesive system consumption and much complex software implementation
    	//E(error) = 0.5 minor cycle
    	tinyTime = tmp%minor_cycle;
    }

}
//save stack code is needed! uart many times is time trade stack, it's after careful consideration!!!!!!!!!!
void showFastMatrix(void)
{
	char tableInfo[60+(TASK_NAME_LEN+8)*num_tasks];
	sprintf(tableInfo,"[[-------------------showing scheduling table for %d seconds-------------------]]\r\n",SHOW_WINDOW);
	HAL_UART_Transmit(&huart1, (uint8_t*)tableInfo, strlen(tableInfo),0xFFFF);
	char tmp2[200];
	for(int i=0;i<number_minor_cycle;i++)
	{
		sprintf(tableInfo,"| Minor cycle %d |  %dtasks  ",i,fast_matrix[i].n_tasks);
		if(fast_matrix[i].n_tasks==0)
		{
			strcat(tableInfo,"[no tasks]\r\n");
		}
		for(int j=0;j<fast_matrix[i].n_tasks;j++)
		{
			char *ins = ( (j==fast_matrix[i].n_tasks-1) ?"%s]\r\n":"%s,\t");
			if(j==0)
			{
				ins = "[%s,";
				if(j==fast_matrix[i].n_tasks-1)
				{
					ins = "[%s]\r\n";
				}
			}
			sprintf(tmp2,ins,tasks[fast_matrix[i].task_code[j]].task_name);
			strcat(tableInfo,tmp2);
		}
		HAL_UART_Transmit(&huart1, (uint8_t*)tableInfo, strlen(tableInfo),0xFFFF);
	}
	sprintf(tableInfo,"\r\n\r\n");
	HAL_UART_Transmit(&huart1, (uint8_t*)tableInfo, strlen(tableInfo),0xFFFF);
}
