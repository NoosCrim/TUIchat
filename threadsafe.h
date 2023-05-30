#pragma once
#include <mutex>
#include <queue>
template<typename T>
class threadsafe_queue
{
private:
	std::mutex mutex;
	std::queue<T> q;
public:
	T& front()
	{
		mutex.lock();
		T& a = q.front();
		mutex.unlock();
		return a;
	}
	T& back()
	{
		mutex.lock();
		T& a = q.back();
		mutex.unlock();
		return a;
	}
	T pop()
	{
		mutex.lock();
		T f = q.front();
		q.pop();
		mutex.unlock();
		return f;
	}
	void push(const T& t)
	{
		mutex.lock();
		q.push(t);
		mutex.unlock();
		return;
	}
	bool empty()
	{
		mutex.lock();
		bool a = q.empty();
		mutex.unlock();
		return a;
	}
};