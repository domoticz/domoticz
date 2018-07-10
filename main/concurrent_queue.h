/*
 * concurrent_queue.h
 *
 *  Created on: 6 oct. 2015
 *      Author: gaudryc
 *      Source: https://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
 *      Source: https://www.justsoftwaresolutions.co.uk/threading/condition-variable-spurious-wakes.html
 */
#pragma once
#ifndef MAIN_CONCURRENT_QUEUE_H_
#define MAIN_CONCURRENT_QUEUE_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

template<typename Data>
class concurrent_queue {
private:
	struct queue_not_empty {
		std::queue<Data>& queue;

		explicit queue_not_empty(std::queue<Data>& queue_): queue(queue_) {}

		bool operator()() const {
			return !queue.empty();
		}
	};

	std::queue<Data> the_queue;
	mutable std::mutex the_mutex;
	std::condition_variable the_condition_variable;

public:
	size_t size() const {
		std::unique_lock<std::mutex> lock(the_mutex);
		return the_queue.size();
	}

	void clear() {
		std::unique_lock<std::mutex> lock(the_mutex);
		while (!the_queue.empty())
			the_queue.pop();
	}

	void push(Data const& data) {
		std::unique_lock<std::mutex> lock(the_mutex);
		the_queue.push(data);
		lock.unlock();
		the_condition_variable.notify_one();
	}

	bool empty() const {
		std::unique_lock<std::mutex> lock(the_mutex);
		return the_queue.empty();
	}

	bool try_pop(Data& popped_value) {
		std::unique_lock<std::mutex> lock(the_mutex);
		if(the_queue.empty()) {
			return false;
		}

		popped_value=the_queue.front();
		the_queue.pop();
		return true;
	}

	void wait_and_pop(Data& popped_value) {
		std::unique_lock<std::mutex> lock(the_mutex);
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
		std::unique_lock<std::mutex> lock(the_mutex);
		if(!the_condition_variable.wait_for(lock, wait_duration, queue_not_empty(the_queue))) {
			return false;
		}
		popped_value=the_queue.front();
		the_queue.pop();
		return true;
	}

};

class queue_element_trigger {
private:
	struct element_popped {
		bool& popped;

		explicit element_popped(bool& popped_): popped(popped_) {}

		bool operator()() const {
			return popped;
		}
	};

	mutable std::mutex the_mutex;
	std::condition_variable the_condition_variable;
	bool elementPopped;

public:
	queue_element_trigger() {
		elementPopped = false;
	}
	void popped() {
		std::unique_lock<std::mutex> lock(the_mutex);
		elementPopped = true;
		the_condition_variable.notify_one();
	}
	void wait() {
		std::unique_lock<std::mutex> lock(the_mutex);
		the_condition_variable.wait(lock, element_popped(elementPopped));
	}
	template<typename Duration>
	bool timed_wait(Duration const& wait_duration) {
		std::unique_lock<std::mutex> lock(the_mutex);
		if(!the_condition_variable.wait_for(lock, wait_duration, element_popped(elementPopped))) {
			return false;
		}
		return true;
	}
};

#endif /* MAIN_CONCURRENT_QUEUE_H_ */
