#pragma once

#include <functional>

namespace Dymatic {

	class AssetThread
	{
	public:
		struct AssetThreadWorkData
		{
			std::function<void()> AssetThreadWork;
			std::function<void()> MainThreadWork;
		};

	public:
		static void Init();
		static void Shutdown();

		static void QueueWork(AssetThreadWorkData work);

		static void ExecuteMainThreadQueue();
		static uint32_t QueuedWorkCount();

	private:
		static void AssetThreadLoop();
	};

}