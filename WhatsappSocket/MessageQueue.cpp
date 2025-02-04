#include "MessageQueue.h"

void MessageQueue::push(const std::string& message)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.push(message);
	_cond.notify_one();
}

std::string MessageQueue::pop()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.wait(lock, [this] { return !_queue.empty(); });

	std::string message = _queue.front();
	_queue.pop();

	return message;
}
