#pragma once
#include "Dymatic/Scene/ScriptableEntity.h"

namespace Dymatic {

	class ScriptEngine
	{
	public:

		struct ParamData
		{
			enum ParamType
			{
				None = 0,
				Boolean,
				Integer,
				Float
			};

			union ParamStore
			{
				ParamStore() = default;
				ParamStore(bool value) { Boolean = value; }
				ParamStore(int value) { Integer = value; }
				ParamStore(float value) { Float = value; }

				bool Boolean;
				int Integer;
				float Float;
			};

			std::string name;
			ParamType type;
			ParamStore defaultValue;

			static std::string ConvertParamTypeToString(const ParamData& param);
			static std::string ConvertParamDefaultToString(const ParamData& param);
			static ParamType ConvertStringToParamType(const std::string& string);
			static std::string ConvertParamTypeToDataString(const ParamData& param);
		};

		static void Init();

		static bool Compile(const std::filesystem::path& devenv, const std::filesystem::path& projectPath);
		static bool LoadScriptLibrary();
		static void CleanLibraryAssembly();

		static bool DoesScriptExist(const std::string& name);
		static bool SetupScript(NativeScriptComponent& nsc);
		static bool UpdateScriptName(NativeScriptComponent& nsc, const std::string& name);
		static void GetScriptParamData(const std::string& name, std::vector<ParamData>& data);
		static bool BindScript(NativeScriptComponent& nsc);

		static std::vector<std::string>& GetScriptList();

		static void GetInstanceParamPointers(NativeScriptComponent& nsc, std::vector<void*>& data);

		static void InstantiateScript(NativeScriptComponent& nsc);

		static std::string& GetCompilationMessage();
	};

}