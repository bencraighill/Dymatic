#pragma once
#include "Dymatic.h"
#include "Dymatic/Core/Base.h"

namespace Dymatic {

	struct PluginInfo
	{
		struct PluginParam
		{
			enum PluginParamType
			{
				Bool = 0,
				Int,
				Float,
				String
			};

			struct PluginParamData
			{
				union
				{
					bool Bool;
					int Int;
					float Float;
				};
				std::string String;
			};

			std::string name;
			PluginParamType type;
			PluginParamData data;
			PluginParamData defaultValue;
		};

		std::string name;
		int versionMajor = -1;
		int versionMinor = -1;
		int versionPatch = -1;
		std::string version;
		std::string developer;
		std::string description;
		std::filesystem::path path;
		bool enabled = false;

		std::vector<PluginParam> params;
	};

	class PluginLoader
	{
	public:
		static void Init();
		static void OnUpdate(Timestep ts);
		static void OnUIRender();

		static bool GetPluginInfo(const std::filesystem::path& path, PluginInfo& info);
	};
}