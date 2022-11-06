#include "dypch.h"
#include "Dymatic/Scene/ScriptEngine.h"

#include "Dymatic/Core/Input.h"

namespace Dymatic {

	static HINSTANCE s_ScriptLibrary;
	
	static std::string s_CompilationMessage;

	static bool LoadScriptDLL(LPCWSTR library)
	{
		s_ScriptLibrary = LoadLibraryW(library);
		if (!s_ScriptLibrary)
		{
			wprintf(L"could not load the dynamic library %s! %d", library, GetLastError());
			return false;
		}
		return true;
	}

	static void FreeScriptLibrary()
	{
		FreeLibrary(s_ScriptLibrary);
		s_ScriptLibrary = nullptr;
	}

	template<typename T>
	static bool LoadScriptFunction(LPCSTR funcname, T* func)
	{
		*func = (T)GetProcAddress(s_ScriptLibrary, funcname);
		if (!*func)
		{
			printf("could not locate the function %s! %d", funcname, GetLastError());
			return false;
		}
		return true;
	}

	static void LogCall(std::string message) { DY_INFO(message); }

	void ScriptEngine::Init()
	{
		LoadScriptLibrary();
	}

	bool ScriptEngine::Compile(const std::filesystem::path& devenv, const std::filesystem::path& projectPath)
	{
		CleanLibraryAssembly();

		if (devenv.empty())
		{
			s_CompilationMessage = "Development Environment Not Specified.";
			return false;
		}

		if (!std::filesystem::exists(devenv))
		{
			s_CompilationMessage = "Cannot Access Development Environment.";
			return false;
		}

		if (!std::filesystem::exists(projectPath))
		{
			s_CompilationMessage = "Project Path Does Not Exist.";
			return false;
		}

		// Generate Interface Files
		{
			// Parse All Header Files
			struct File
			{
				struct Class
				{
					std::string name;
					std::vector<ParamData> params;
					std::vector<std::string> paramValueString;
				};

				std::filesystem::path path;
				std::vector<Class> classes;
			};
			std::vector<File> files;

			for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath))
			{
				if (entry.path().extension() == ".dyheader")
				{
					std::ifstream fileStream;
					fileStream.open(entry);

					std::string word;
					std::vector<std::string> words;
					while (fileStream >> word)
					{
						std::string buffer;
						bool characterWordState = false;
						for (auto& character : word)
						{
							bool newWord = !isalnum(character) && character != '_';
							if (newWord != characterWordState || newWord)
							{
								if (!buffer.empty() && buffer != " " && buffer != "\t" && buffer != "\r" && buffer != "\n")
									words.push_back(buffer);

								// Update
								buffer = "";
								characterWordState = newWord;
							}
							buffer += character;
						}
						if (!buffer.empty() && buffer != " " && buffer != "\t" && buffer != "\r" && buffer != "\n")
							words.push_back(buffer);
					}

					bool dyparamOpen = false;
					auto wordSize = words.size();

					files.push_back({entry.path()});
					auto& file = files.back();

					for (size_t i = 0; i < wordSize; i++)
					{
						auto& word = words[i];

						if (dyparamOpen)
						{
							if (word.find(";") != std::string::npos || word.find("=") != std::string::npos)
							{
								auto& params = file.classes.back().params;
								auto& paramStrings = file.classes.back().paramValueString;
								ParamData::ParamType type = ParamData::ParamType::None;
								{
									int x = 2;
									while (words[i - x] != ")")
									{
										type = ParamData::ConvertStringToParamType(words[i - x]);
										if (type != ParamData::ParamType::None)
											break;
										x++;
									}
								}

								params.push_back({ words[i - 1], type, { 0 } });
								paramStrings.push_back({});

								if (word.find("=") != std::string::npos)
								{
									auto& string = paramStrings.back();
									int x = 1;
									while (words[i + x].find(";") == std::string::npos)
									{
										string += words[i + x];
										x++;
									}
								}
								else
									paramStrings.back() = "0";


								dyparamOpen = false;
							}								
						}
						else if (i < wordSize - 2)
						{
							if (word == "DYCLASS" && words[i + 1] == "(" && words[i + 2] == ")")
							{
								auto& name = words[i + 4];
								file.classes.push_back({name});

								i += 4;
							}
							else if ((word == "DYPARAM" && words[i + 1] == "(" && words[i + 2] == ")") && !file.classes.empty())
							{
								i += 2;
								dyparamOpen = true;
							}
						}
					}
				}
			}

			// Write To File
			std::string out;
			out += "// Script Interface\n";
			out += "#include \"Dymatic/Scene/ScriptEngine.h\"\n\n";
			out += "// Include header files\n";
			for (auto& file : files)
				out += "#include \"" + (std::filesystem::relative(file.path, projectPath)).string() + "\"\n";

			out += "\n// Compiled Script Core Functions\n";

			// (function) InitScriptCore
			out += "extern \"C\" __declspec(dllexport) void __cdecl InitScriptCore(void* log, void* isKeyPressed)\n{\n";
			out += "Script::Console::Log = static_cast<Script::Console::LogFunction>(log);\n";
			out += "Script::Input::IsKeyPressed = static_cast<Script::Input::IsKeyPressedFunction>(isKeyPressed);\n";
			out += "}\n\n";

			// (function) GetCompilerVersion
			out += "extern \"C\" __declspec(dllexport) void GetCompilerVersion(std::string & version) { version = \"1.2.5\"; }";

			// (function) QueryScriptCore
			{
				out += "extern \"C\" __declspec(dllexport) void __cdecl QueryScriptCore(std::vector<std::string>& outScripts)\n{\n";
				out += "outScripts = {\n";
				bool first = true;
				for (auto& file : files)
					for (auto& script : file.classes)
					{
						if (first)
							first = false;
						else
							out += ",\n";
						out += "\"" + script.name + "\"";
					}
				out += "};\n}\n\n";
			}

			// (function) ReflectParams
			{
				out += "extern \"C\" __declspec(dllexport) void __cdecl ReflectParams(std::string name, std::vector<ScriptEngine::ParamData>& outParams)\n{\noutParams.clear();\n";
				bool first = true;
				for (auto& file : files)
					for (auto& script : file.classes)
					{
						if (first)
						{
							first = false;
							out += "if (";
						}
						else
							out += "else if (";
						out += "name == \"" + script.name + "\") { outParams = {";

						bool first = true;
						for (size_t i = 0; i < script.params.size(); i++)
						{
							auto& param = script.params[i];
							auto& string = script.paramValueString[i];

							if (first)
								first = false;
							else
								out += ", ";

							out += "{ \"" + param.name + "\", ScriptEngine::ParamData::" + ParamData::ConvertParamTypeToString(param) + ", " + ((param.type == ParamData::ParamType::None) ? "" : ("(" + ParamData::ConvertParamTypeToDataString(param) + ")(" + string + ")")) + " }";
						}
						out += "}; return; }\n";
					}
				out += "\n}\n\n";
			}

			// (function) CreateScriptComponent
			{
				out += "extern \"C\" __declspec(dllexport) bool __cdecl CreateScriptComponent(NativeScriptComponent & nsc)\n{\n";
				bool first = true;
				for (auto& file : files)
					for (auto& script : file.classes)
					{
						if (first)
						{
							first = false;
							out += "if (";
						}
						else
							out += "else if (";
						out += "nsc.ScriptName == \"" + script.name + "\") { nsc.Bind<" + script.name + ">(); return true; }\n";
					}
				out += "return false;\n";
				out += "}\n\n";
			}

			// (function) DoesScriptExist
			{
				out += "extern \"C\" __declspec(dllexport) bool __cdecl DoesScriptExist(const std::string& name)\n{\n";
				out += "if (\n";
				bool first = true;
				for (auto& file : files)
					for (auto& script : file.classes)
					{
						if (first) first = false;
						else out += " ||\n";
						out += "name == \"" + script.name + "\"";
					}
				out += ") return true;\nreturn false;\n";
				out += "}\n\n";
			}

			// (function) InstantiateScriptComponent
			out += "extern \"C\" __declspec(dllexport) void __cdecl InstantiateScriptComponent(NativeScriptComponent& nsc)\n{\n";
			for (auto& file : files)
				for (auto& script : file.classes)
					if (!script.params.empty())
					{
						out += "if (nsc.ScriptName == \"" + script.name + "\")\n{\n";
						for (size_t i = 0; i < script.params.size(); i++)
							if (script.params[i].type != ParamData::ParamType::None)
								out += "static_cast<" + script.name + "*>(nsc.Instance)->" + script.params[i].name + " = nsc.Params[" + std::to_string(i) + "]." + ParamData::ConvertParamTypeToString(script.params[i]) + ";\n";
						out += "return;\n}\n";
					}
			out += "}\n\n";

			// (function) GetInstanceParamPointers
			out += "extern \"C\" __declspec(dllexport) void __cdecl GetInstanceParamPointers(NativeScriptComponent& nsc, std::vector<void*>& data)\n{\ndata.clear();\n";
			for (auto& file : files)
				for (auto& script : file.classes)
					if (!script.params.empty())
					{
						out += "if (nsc.ScriptName == \"" + script.name + "\")\n{\n";
						out += "auto script = static_cast<" + script.name + "*>(nsc.Instance);\n";
						out += "data = { ";
						bool first = true;
						for (auto& param : script.params)
						{
							if (first)
								first = false;
							else
								out += ", ";
							out += "&script->" + param.name;
						}
						out += " };\n";
						out += "return;\n}\n";
					}
			out += "}\n\n";

			std::ofstream fout(projectPath / "ScriptLibraryInterface.cpp");
			fout << out;
		}

		// Access project file and add the generated file
		for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath))
		{
			if (entry.path().extension() == ".vcxproj")
			{
				std::string result;
				std::ifstream in(entry.path(), std::ios::in | std::ios::binary);
				if (in)
				{
					in.seekg(0, std::ios::end);
					size_t size = in.tellg();
					if (size != -1)
					{
						result.resize(size);
						in.seekg(0, std::ios::beg);
						in.read(&result[0], size);
					}
				}
				
				if (result.find("ScriptLibraryInterface.cpp") == std::string::npos)
				{
					result.insert(result.find("</Project>"), "<ItemGroup><ClInclude Include = \"ScriptLibraryInterface.cpp\"/></ItemGroup><ItemGroup><ClInclude Include = \"NodeGenerated\"/></ItemGroup>");

					std::ofstream fout(entry.path());
					fout << result;
				}
		
				break;
			}
		}

		// Access .sln File and Rebuild if found
		for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath))
		{
			if (entry.path().extension() == ".sln")
			{
				system(("cmd /S /C \"\"" + devenv.string() + "\" \"" + std::filesystem::absolute(entry.path()).string() + "\" /rebuild Debug > DymaticScriptCompilation.log\"").c_str());
				break;
			}
		}


		// Access Log file from compilation output and success status
		std::ifstream stream("DymaticScriptCompilation.log");
		std::stringstream buffer;
		buffer << stream.rdbuf();
		s_CompilationMessage = buffer.str();

		int outputLocation = s_CompilationMessage.find(" failed, ");
		if (outputLocation != std::string::npos)
			if (s_CompilationMessage[outputLocation - 1] != '0')
				return false;

		// Find Library To Copy
		for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath))
		{
			if (entry.path().extension() == ".dll")
			{
				FreeScriptLibrary();

				std::filesystem::remove("ScriptLibrary.dll");

				std::ifstream src(entry.path(), std::ios::binary);
				std::ofstream dst("ScriptLibrary.dll", std::ios::binary);

				dst << src.rdbuf();
				src.close();
				dst.close();

				// Load Script Library DLL
				return LoadScriptLibrary();
			}
		}
	}

	static std::vector<std::string> s_ScriptList;

	// Query Interface Function
	typedef void(__stdcall* QueryScriptCoreFunction)(std::vector<std::string>&);
	static QueryScriptCoreFunction s_QueryCoreFunction = nullptr;

	// Reflect Interface Function
	typedef void(__stdcall* ReflectScriptCoreFunction)(std::string, std::vector<ScriptEngine::ParamData>&);
	static ReflectScriptCoreFunction s_ReflectCoreFunction = nullptr;

	// CreateScriptComponent Interface Function
	typedef bool(__stdcall* CreateScriptComponentFunction)(NativeScriptComponent&);
	static CreateScriptComponentFunction s_CreateScriptComponentFunction = nullptr;

	// DoesScriptExist Interface Function
	typedef bool(__stdcall* DoesScriptExistFunction)(const std::string&);
	static DoesScriptExistFunction s_DoesScriptExistFunction = nullptr;

	// InstantiateScript Interface Function
	typedef bool(__stdcall* InstantiateScriptFunction)(NativeScriptComponent&);
	static InstantiateScriptFunction s_InstantiateScriptFunction = nullptr;

	// GetInstanceParamPointers Interface Function
	typedef bool(__stdcall* GetInstanceParamPointers)(NativeScriptComponent&, std::vector<void*>&);
	static GetInstanceParamPointers s_GetInstanceParamPointers = nullptr;

	bool ScriptEngine::LoadScriptLibrary()
	{
		CleanLibraryAssembly();

		if (LoadScriptDLL(L"ScriptLibrary.dll"))
		{
			typedef void(__stdcall* getCompilerVersionFunction)(std::string&);
			getCompilerVersionFunction getCompilerVersionFunc = nullptr;
			if (LoadScriptFunction("GetCompilerVersion", &getCompilerVersionFunc))
			{
				std::string version;
				getCompilerVersionFunc(version);
				if (version != "1.2.5")
				{
					s_CompilationMessage = "Engine Version Not Compatible.";
					return false;
				}
			}
			else
			{
				s_CompilationMessage = "Could not access Engine Version.";
				return false;
			}

			// Log Function
			typedef void(*LogFunction)(std::string);
			LogFunction logFunc = &LogCall;

			// Input Function
			typedef bool(*IsKeyPressedFunction)(KeyCode);
			IsKeyPressedFunction IsKeyPressedFunc = &Input::IsKeyPressed;

			// Init Core Function
			typedef void(__stdcall* InitScriptCoreFunction)(LogFunction, IsKeyPressedFunction);
			InitScriptCoreFunction function = nullptr;

			if (LoadScriptFunction("InitScriptCore", &function))
				function(logFunc, IsKeyPressedFunc);
			else
			{
				s_CompilationMessage = "Could not initialize ScriptCore.";
				return false;
			}

			if (LoadScriptFunction("QueryScriptCore", &s_QueryCoreFunction))
				s_QueryCoreFunction(s_ScriptList);
			else
			{
				s_CompilationMessage = "Could not query ScriptCore interface.";
				return false;
			}
			
			if (!LoadScriptFunction("ReflectParams", &s_ReflectCoreFunction))
			{
				s_CompilationMessage = "Could not reflect ScriptCore.";
				return false;
			}

			if (!LoadScriptFunction("CreateScriptComponent", &s_CreateScriptComponentFunction))
			{
				s_CompilationMessage = "Could not load ScriptCore Function 'CreateScriptComponent'";
				return false;
			}

			if (!LoadScriptFunction("DoesScriptExist", &s_DoesScriptExistFunction))
			{
				s_CompilationMessage = "Could not load ScriptCore Function 'DoesScriptExist'";
				return false;
			}

			if (!LoadScriptFunction("InstantiateScriptComponent", &s_InstantiateScriptFunction))
			{
				s_CompilationMessage = "Could not load ScriptCore Function 'InstantiateScriptComponent'";
				return false;
			}

			if (!LoadScriptFunction("GetInstanceParamPointers", &s_GetInstanceParamPointers))
			{
				s_CompilationMessage = "Could not load ScriptCore Function 'GetInstanceParamPointers'";
				return false;
			}
			
			return true;
		}
		s_CompilationMessage = "Could not load 'ScriptLibrary.dll'";
		return false;
	}

	void ScriptEngine::CleanLibraryAssembly()
	{
		FreeScriptLibrary();

		s_ScriptList.clear();

		s_QueryCoreFunction = nullptr;
		s_ReflectCoreFunction = nullptr;
		s_CreateScriptComponentFunction = nullptr;
		s_DoesScriptExistFunction = nullptr;
		s_InstantiateScriptFunction = nullptr;
		s_GetInstanceParamPointers = nullptr;
	}

	bool ScriptEngine::DoesScriptExist(const std::string& name)
	{
		if (s_DoesScriptExistFunction)
			return s_DoesScriptExistFunction(name);
		return false;
	}

	bool ScriptEngine::SetupScript(NativeScriptComponent& nsc)
	{
		nsc.Params.clear();
		if (DoesScriptExist(nsc.ScriptName))
		{
			std::vector<ParamData> params;
			s_ReflectCoreFunction(nsc.ScriptName, params);


			for (auto& param : params)
				nsc.Params.push_back(*reinterpret_cast<NativeScriptComponent::ParamStore*>(&param.defaultValue));
			return true;
		}
		return false;
	}

	bool ScriptEngine::UpdateScriptName(NativeScriptComponent& nsc, const std::string& name)
	{
		nsc.ScriptName = name;
		return SetupScript(nsc);
	}

	void ScriptEngine::GetScriptParamData(const std::string& name, std::vector<ParamData>& data)
	{
		data.clear();
		if (DoesScriptExist(name))
			s_ReflectCoreFunction(name, data);
	}

	bool ScriptEngine::BindScript(NativeScriptComponent& nsc)
	{
		if (!s_CreateScriptComponentFunction)
			return false;
		
		if (s_CreateScriptComponentFunction(nsc))
			return true;
		nsc.Unbind();
		return false;
	}

	std::vector<std::string>& ScriptEngine::GetScriptList()
	{
		return s_ScriptList;
	}

	void ScriptEngine::GetInstanceParamPointers(NativeScriptComponent& nsc, std::vector<void*>& data)
	{
		if (s_GetInstanceParamPointers)
			s_GetInstanceParamPointers(nsc, data);
	}

	void ScriptEngine::InstantiateScript(NativeScriptComponent& nsc)
	{
		if (s_InstantiateScriptFunction)
			s_InstantiateScriptFunction(nsc);
	}

	std::string& ScriptEngine::GetCompilationMessage()
	{
		return s_CompilationMessage;
	}

	std::string ScriptEngine::ParamData::ConvertParamTypeToString(const ParamData& param)
	{
		switch (param.type)
		{
		case Boolean: return "Boolean";
		case Integer: return "Boolean";
		case Float: return "Float";
		default: return "None";
		}
	}

	std::string ScriptEngine::ParamData::ConvertParamDefaultToString(const ParamData& param)
	{
		switch (param.type)
		{
		case Boolean: return param.defaultValue.Boolean ? "true" : "false";
		case Integer: return std::to_string(param.defaultValue.Integer);
		case Float: return std::to_string(param.defaultValue.Float) + "f";
		default: return "";
		}
	}

	ScriptEngine::ParamData::ParamType ScriptEngine::ParamData::ConvertStringToParamType(const std::string& string)
	{
		if (string == "bool") return Boolean;
		if (string == "int") return Integer;
		if (string == "float") return Float;
		return None;
	}

	std::string ScriptEngine::ParamData::ConvertParamTypeToDataString(const ParamData& param)
	{
		switch (param.type)
		{
		case None: return "";
		case Boolean: return "bool";
		case Integer: return "int";
		case Float: return "float";
		}
	}

}