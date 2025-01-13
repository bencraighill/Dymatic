#include "dypch.h"
#include "Dymatic/Asset/AssetThread.h"

#include <thread>
#include <queue>
#include <vector>
#include <mutex>

namespace Dymatic {

	static std::thread s_AssetThread;

	static std::mutex s_AssetThreadQueueMutex;
	static std::queue<AssetThread::AssetThreadWorkData> s_AssetThreadQueue;

	static std::mutex s_MainThreadQueueMutex;
	static std::vector<std::function<void()>> s_MainThreadQueue;

	static std::condition_variable s_AssetThreadQueueCondition;
	static bool s_AssetThreadRunning = true;
	static uint32_t s_AssetThreadWorkCount = 0;

	void AssetThread::Init()
	{
		s_AssetThread = std::thread(AssetThread::AssetThreadLoop);
	}

	void AssetThread::Shutdown()
	{
		{
			std::unique_lock<std::mutex> lock(s_AssetThreadQueueMutex);
			s_AssetThreadRunning = false;
		}

		s_AssetThreadQueueCondition.notify_all();
		s_AssetThread.join();
	}

	void AssetThread::QueueWork(AssetThreadWorkData work)
	{
		{
			std::scoped_lock<std::mutex> lock(s_AssetThreadQueueMutex);
			s_AssetThreadQueue.push(work);
		}

		s_AssetThreadWorkCount++;
		s_AssetThreadQueueCondition.notify_one();
	}

	void AssetThread::ExecuteMainThreadQueue()
	{
		if (s_MainThreadQueue.empty())
			return;

		std::scoped_lock<std::mutex> lock(s_MainThreadQueueMutex);

		for (const auto& func : s_MainThreadQueue)
		{
			func();
			s_AssetThreadWorkCount--;
		}

		s_MainThreadQueue.clear();
	}

	uint32_t AssetThread::QueuedWorkCount()
	{
		return s_AssetThreadWorkCount;
	}

	void AssetThread::AssetThreadLoop()
	{
		while (true)
		{
			std::unique_lock<std::mutex> lock(s_AssetThreadQueueMutex);
			s_AssetThreadQueueCondition.wait(lock, []
			{
				return !s_AssetThreadQueue.empty() || !s_AssetThreadRunning;
			});

			if (!s_AssetThreadRunning)
				break;

			if (s_AssetThreadQueue.empty())
				continue;

			AssetThreadWorkData work = s_AssetThreadQueue.front();
			s_AssetThreadQueue.pop();

			// Don't hold the lock longer than necessary to avoid bottlenecking the main thread
			lock.unlock();

			// Run Asset Thread Work
			DY_CORE_INFO("[Asset Thread]: Picking up work...");
			work.AssetThreadWork();

			{
				// Enqueue main thread work to commence after asset thread work is completed
				std::scoped_lock<std::mutex> lock(s_MainThreadQueueMutex);
				s_MainThreadQueue.push_back(work.MainThreadWork);
			}
		}
	}

}