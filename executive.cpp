#include <cassert>
#include <iostream>
#include "rt/priority.h"
#include "rt/affinity.h"
#include "executive.h"

// Consegna 9/6, SORT4

Executive::Executive(size_t num_tasks, unsigned int frame_length, unsigned int unit_duration)
	: p_tasks(num_tasks), frame_length(frame_length), unit_time(unit_duration)
{
}

void Executive::set_periodic_task(size_t task_id, std::function<void()> periodic_task, unsigned int wcet)
{
	assert(task_id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)
	
	p_tasks[task_id].function = periodic_task;
	p_tasks[task_id].wcet = wcet;
}

void Executive::set_aperiodic_task(std::function<void()> aperiodic_task, unsigned int wcet)
{
 	ap_task.function = aperiodic_task;
 	ap_task.wcet = wcet;
}
		
void Executive::add_frame(std::vector<size_t> frame)
{
	for (auto & id: frame)
		assert(id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)
	
	frames.push_back(frame);

	/* ... */
}

void Executive::run()
{
	for (size_t id = 0; id < p_tasks.size(); ++id)
	{
		assert(p_tasks[id].function); // Fallisce se set_periodic_task() non e' stato invocato per questo id
		
		p_tasks[id].thread = std::thread(&Executive::task_function, std::ref(p_tasks[id]));
		
		// Set affinity for p_tasks
		rt::set_affinity(p_tasks[id].thread, rt::affinity(0));

		p_tasks[id].status = task_data::IDLE;
	}
	
	assert(ap_task.function); // Fallisce se set_aperiodic_task() non e' stato invocato
	
	ap_task.thread = std::thread(&Executive::task_function, std::ref(ap_task));

	// Set ap_task priority to lowest among all p_tasks
	rt::set_priority(ap_task.thread, rt::priority::rt_max - 1 - p_tasks.size());

	// Set affinity for ap_task
	rt::set_affinity(ap_task.thread, rt::affinity(0));

	// Set status for ap_task
	ap_task.status = task_data::IDLE;

	std::thread exec_thread(&Executive::exec_function, this);
	
	// Set max priority for exec_thread
	rt::set_priority(exec_thread, rt::priority::rt_max);

	// Set affinity for exec_task
	rt::set_affinity(exec_thread, rt::affinity(0));


	// Wait for other threads
	exec_thread.join();	
	ap_task.thread.join();
	for (auto & pt: p_tasks)
		pt.thread.join();
}

void Executive::ap_task_request()
{
	// set ap_task to start at the begginig of the next frame
	{
		std::unique_lock<std::mutex> l(ap_task.mutex);

		ap_scheduled = true;
	}
}

void Executive::task_function(Executive::task_data &task)
{

	while(true)
	{
		{
			std::unique_lock<std::mutex> l(task.mutex);
			
			while(task.status != task_data::PENDING)
				task.cond.wait(l);

		}

		task.status = task_data::RUNNING;

		task.function();

		task.status = task_data::IDLE;
	}
}

void Executive::exec_function()
{
	size_t frame_id = 0;

	auto start_time = std::chrono::steady_clock::now();

	auto frame_deadline_time = start_time;

	for(int t = 0;/* t < 10*/; t++)
	// while (true)
	{

		frame_deadline_time += std::chrono::milliseconds(frame_length * unit_time);

		auto current_frame = frames[frame_id];

		/* Rilascio dei task periodici del frame corrente e aperiodico (se necessario)... */
		
		std::cout << "-- Frame " << t << " -- " << std::endl;

		for(size_t i = 0; i < current_frame.size(); i++)
		{

			auto task_id = current_frame[i];
			auto & task = p_tasks[task_id];

			if(task.status == task_data::IDLE)
			{
				// Set priority
				rt::set_priority(task.thread, rt::priority::rt_max - 1 - i);
				
				{
					std::unique_lock<std::mutex> l(task.mutex);

					// Set status
					task.status = task_data::PENDING;
				
					// Notify
					task.cond.notify_one();
				}
			}
			else
			{
				// A deadline was missed for this task
				// policy: skip next executions until task completes
				std::cout << "!! Skipping exec for task_" << task_id << std::endl;
			}
		}
		
		{
			std::unique_lock<std::mutex> l(ap_task.mutex);

			if(ap_scheduled)
			{
				ap_scheduled = false;
				if(ap_task.status == task_data::IDLE)
				{
					ap_task.status = task_data::PENDING;
					ap_task.cond.notify_one();
				}
				else
				{
					// A deadline was missed for ap_task
					std::cout << "!! Deadline miss for ap_task !!" << std::endl;
					std::cout << "!! Skipping exec for ap_task !!" << std::endl;
					// policy: let ap_task keep running and skip
					// next executions until ap_task completes
				}
			}
		}

		/* Attesa fino al prossimo inizio frame ... */
		
		std::this_thread::sleep_until(frame_deadline_time);

		/* Controllo delle deadline ... */
		for(size_t i = 0; i < current_frame.size(); i++)
		{
			auto task_id = current_frame[i];
			auto & task = p_tasks[task_id];

			// Check status
			if(task.status != task_data::IDLE)
			{
				// Deadline miss
				std::cout << " !! Deadline miss for task " << task_id << " !!" << std::endl;
				// policy: let this taks running and skip
				// next executions until task completes

				// raise priority so next tasks could
				// we could change priority for this task
			}
		}

		if (++frame_id == frames.size())
			frame_id = 0;
	}
}


