#include <iostream>
#include "executive.h"
#include "busy_wait.h"

//used to change unit duration of executive and tasks
#define UNIT_DURATION 500

//used to reduce the duration of the busy_wait to avoid deadline miss
#define TASK_DURATION 0.9

#define TASK_0_DURATION 1	// tau_1
#define TASK_1_DURATION 2	// tau_2
#define TASK_2_DURATION 1	// tau_3,1	
#define TASK_3_DURATION 3	// tau_3,2
#define TASK_4_DURATION 1	// tau_3,3

#define TASK_AP_DURATION 1	// tau_ap


// ap_task will always miss the deadline

Executive exec(5, 4, UNIT_DURATION);


void task0()
{
	std::cout << "task_0 start" << std::endl;
	busy_wait(TASK_0_DURATION * UNIT_DURATION * TASK_DURATION);
	std::cout << "task_0 done" << std::endl;
}

void task1()
{
	std::cout << "task_1 start" << std::endl;	
	busy_wait(TASK_1_DURATION * UNIT_DURATION * TASK_DURATION);
	std::cout << "task_1 done" << std::endl;
}

void task2()
{
	std::cout << "task_2 start" << std::endl;
	busy_wait(TASK_2_DURATION * UNIT_DURATION * TASK_DURATION);
	std::cout << "task_2 done" << std::endl;
}

void task3()
{
	std::cout << "task_3 start" << std::endl;
	busy_wait(TASK_3_DURATION * UNIT_DURATION * TASK_DURATION / 2);

	exec.ap_task_request();

	busy_wait(TASK_3_DURATION * UNIT_DURATION * TASK_DURATION / 2);
	std::cout << "task_3 done" << std::endl;
}

void task4()
{
	std::cout << "task_4 start" << std::endl;
	busy_wait(TASK_4_DURATION * UNIT_DURATION * TASK_DURATION);
	std::cout << "task_4 done" << std::endl;
}

void ap_task()
{
	std::cout << "task_ap start" << std::endl;
	// wcet is actually ten times bigger then the estimated one
	busy_wait(TASK_AP_DURATION * UNIT_DURATION * 10);
	std::cout << "task_ap done" << std::endl;
}

int main()
{
	busy_wait_init();

	exec.set_periodic_task(0, task0, TASK_0_DURATION); // tau_1
	exec.set_periodic_task(1, task1, TASK_1_DURATION); // tau_2
	exec.set_periodic_task(2, task2, TASK_2_DURATION); // tau_3,1
	exec.set_periodic_task(3, task3, TASK_3_DURATION); // tau_3,2
	exec.set_periodic_task(4, task4, TASK_4_DURATION); // tau_3,3
	/* ... */
	
	exec.set_aperiodic_task(ap_task, TASK_AP_DURATION);
	
	exec.add_frame({0,1,2});
	exec.add_frame({0,3});
	exec.add_frame({0,1});
	exec.add_frame({0,1});
	exec.add_frame({0,1,4});
	/* ... */
	
	exec.run();
	
	return 0;
}
