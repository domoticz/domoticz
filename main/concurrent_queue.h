/*
 * concurrent_queue.h
 *
 *  Created on: 6 oct. 2015
 *      Author: gaudryc
 *      Source: https://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
 *      Source: https://www.justsoftwaresolutions.co.uk/threading/condition-variable-spurious-wakes.html
 */

#ifndef MAIN_CONCURRENT_QUEUE_H_
#define MAIN_CONCURRENT_QUEUE_H_

#include <queue>

template<typename Data>
class concurrent_queue
{
private:
	struct queue_not_empty {
		std::queue<Data>& queue;

		queue_not_empty(std::queue<Data>& queue_): queue(queue_) {}

		bool operator()() const {
			return !queue.empty();
		}
	};

	std::queue<Data> the_queue;
	mutable boost::mutex the_mutex;
	boost::condition_variable the_condition_variable;

public:
	void push(Data const& data) {
		boost::mutex::scoped_lock lock(the_mutex);
		the_queue.push(data);
		lock.unlock();
		the_condition_variable.notify_one();
	}

	bool empty() const {
		boost::mutex::scoped_lock lock(the_mutex);
		return the_queue.empty();
	}

	bool try_pop(Data& popped_value) {
		boost::mutex::scoped_lock lock(the_mutex);
		if(the_queue.empty()) {
			return false;
		}

		popped_value=the_queue.front();
		the_queue.pop();
		return true;
	}

	void wait_and_pop(Data& popped_value) {
		boost::mutex::scoped_lock lock(the_mutex);
		/*
		while(the_queue.empty()) {
			the_condition_variable.wait(lock);
		}
		*/
		the_condition_variable.wait(lock, queue_not_empty(the_queue));

		popped_value=the_queue.front();
		the_queue.pop();
	}

	template<typename Duration>
	bool timed_wait_and_pop(Data& popped_value, Duration const& wait_duration) {
		boost::mutex::scoped_lock lock(the_mutex);
		if(!the_condition_variable.timed_wait(lock, wait_duration, queue_not_empty(the_queue))) {
			return false;
		}
		popped_value=the_queue.front();
		the_queue.pop();
		return true;
	}

};

#endif /* MAIN_CONCURRENT_QUEUE_H_ */
