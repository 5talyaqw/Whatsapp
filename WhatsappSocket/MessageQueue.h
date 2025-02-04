#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>

class MessageQueue
{
public:
	void push(const std::string& message);
	std::string pop();

private:
	std::queue<std::string> _queue;
	std::mutex _mutex;
	std::condition_variable _cond;
};