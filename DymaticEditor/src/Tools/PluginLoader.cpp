#include "PluginLoader.h"

#include <imgui/imgui.h>

static std::string s_CurrentID;
static bool BeginWindow(const char* name, bool* open = nullptr) { return ImGui::Begin((name + s_CurrentID).c_str(), open); }
static void EndWindow() {	ImGui::End(); }
static void Text(const char* fmt, ...) 
{
	va_list args;
	va_start(args, fmt);
	ImGui::Text(fmt, args);
	va_end(args);
}
static void SameLine() { ImGui::SameLine(); }
static bool Button(const char* label, float w = 0.0f, float h = 0.0f) { return ImGui::Button(label, { w, h }); }
static void PushColor(ImGuiCol idx, ImVec4 col) { ImGui::PushStyleColor(idx, col); }
static void PopColor(int count) { ImGui::PopStyleColor(count); }
static bool InputText(const char* label, char* buffer, size_t buf_size) { return ImGui::InputText(label, buffer, buf_size); }
static bool IsItemActive() { return ImGui::IsItemActive(); }
static bool IsWindowFocused() { return ImGui::IsWindowFocused(); }
static void SetKeyboardFocusHere() { ImGui::SetKeyboardFocusHere(); }

namespace Dymatic {

	// DLL Helpers
	static bool LoadPluginLibrary(LPCWSTR path, HINSTANCE& library)
	{
		library = LoadLibraryW(path);
		if (!library)
		{
			wprintf(L"could not load the dynamic library %s! %d", path, GetLastError());
			return false;
		}
		return true;
	}

	static void FreePluginLibrary(HINSTANCE& library)
	{
		FreeLibrary(library);
		library = nullptr;
	}

	template<typename T>
	static bool LoadPluginLibraryFunction(HINSTANCE& library, LPCSTR funcname, T* func)
	{
		*func = (T)GetProcAddress(library, funcname);
		if (!*func)
		{
			printf("could not locate the function %s! %d", funcname, GetLastError());
			return false;
		}
		return true;
	}

	typedef const char* (__stdcall* GetPluginNameFunction)();
	typedef int (__stdcall* GetPluginVersionMajorFunction)();
	typedef int (__stdcall* GetPluginVersionMinorFunction)();
	typedef int (__stdcall* GetPluginVersionPatchFunction)();
	typedef const char* (__stdcall* GetPluginDeveloperFunction)();
	typedef const char* (__stdcall* GetPluginDescriptionFunction)();

	typedef int (__stdcall* GetParamCountPluginFunction)();
	typedef void (__stdcall* GetParamPluginFunction)(const char**, const char**);
	typedef void (__stdcall* GetParamDefaultPluginFunction)(void*);
	typedef void (__stdcall* SetParamPluginFunction)(void*);

	class Plugin
	{
	public:
		Plugin(const std::filesystem::path& path, std::vector<std::pair<std::string, std::string>> params)
		{
			if (LoadPluginLibrary(path.wstring().c_str(), m_Library))
			{
				if (LoadPluginLibraryFunction(m_Library, "DYMATIC_PLUGIN_GET_NAME", &m_GetPluginName))
				{
					m_DLLName = path.filename().string();
					m_Name = m_GetPluginName();

					// Set Parameters
					GetParamCountPluginFunction getParamCountFunction = nullptr;
					if (LoadPluginLibraryFunction(m_Library, "DYMATIC_PLUGIN_GET_PARAM_COUNT", &getParamCountFunction))
					{
						int count = getParamCountFunction() / 3;
						for (size_t i = 0; i < count; i++)
						{
							GetParamPluginFunction getParamFunction = nullptr;
							if (LoadPluginLibraryFunction(m_Library, ("DYMATIC_PLUGIN_GET_PARAM_" + std::to_string(i * 3 + 1)).c_str(), &getParamFunction))
							{
								const char* name;
								const char* type;
								getParamFunction(&name, &type);

								for (auto& in_param : params)
								{
									if (in_param.first == name)
									{
										SetParamPluginFunction setParamFunction = nullptr;
										if (LoadPluginLibraryFunction(m_Library, ("DYMATIC_PLUGIN_SET_PARAM_" + std::to_string(i * 3)).c_str(), &setParamFunction))
										{
												 if (strcmp(type, "bool")	== 0) { bool		data = stoi(in_param.second);	setParamFunction(&data); }
											else if (strcmp(type, "int")	== 0) { int			data = stoi(in_param.second);	setParamFunction(&data); }
											else if (strcmp(type, "float")	== 0) { float		data = stof(in_param.second);	setParamFunction(&data); }
											else if (strcmp(type, "string")	== 0) { std::string data = in_param.second;			setParamFunction(&data); }
										}

										break;
									}
								}
							}
						}
					}

					typedef void(__stdcall* InitImGuiFunction)(void**, void**);
					InitImGuiFunction InitImGui;
					if (LoadPluginLibraryFunction(m_Library, "DYMATIC_SDK_INIT_CORE", &InitImGui))
					{
						m_Loaded = true;
						void* ui[] = {
							&BeginWindow, &EndWindow, &Text, &SameLine, &Button, &PushColor, &PopColor, &InputText, &IsItemActive, &IsWindowFocused, &SetKeyboardFocusHere
						};

						void* input[] = {
							&Input::IsKeyPressed
						};

						InitImGui(ui, input);
					}
					else
						return;

					if (LoadPluginLibraryFunction(m_Library, "DYMATIC_PLUGIN_ON_ATTACH", &m_OnAttachPlugin))
						m_OnAttachPlugin();

					LoadPluginLibraryFunction(m_Library, "DYMATIC_PLUGIN_ON_UPDATE", &m_OnUpdatePlugin);
					LoadPluginLibraryFunction(m_Library, "DYMATIC_PLUGIN_ON_UI_RENDER", &m_OnUIRenderPlugin);
				}
				else
					DY_ERROR("Plugin library {0} sent no response.", path);
			}
			else
				DY_ERROR("Could not load plugin library {0}", path);
		}

		inline bool IsLoaded() { return m_Loaded; }
		inline void OnUpdate(Timestep ts) { if (m_OnUpdatePlugin) m_OnUpdatePlugin(ts); }
		inline void OnUIRender() { s_CurrentID = "##" + m_DLLName;  if (m_OnUIRenderPlugin) m_OnUIRenderPlugin(); }

	private:
		bool m_Loaded = false;
		std::string m_Name;
		std::string m_DLLName;
		HINSTANCE m_Library;

		// GetPluginName Plugin Function
		GetPluginNameFunction m_GetPluginName = nullptr;

		// OnAttach Plugin Function
		typedef void(__stdcall* OnAttachPluginFunction)();
		OnAttachPluginFunction m_OnAttachPlugin = nullptr;

		// OnUpdate Plugin Function
		typedef void(__stdcall* OnUpdatePluginFunction)(float);
		OnUpdatePluginFunction m_OnUpdatePlugin = nullptr;

		// OnUIRender Plugin Function
		typedef void(__stdcall* OnUIRenderPluginFunction)();
		OnUIRenderPluginFunction m_OnUIRenderPlugin = nullptr;
	};
	static std::vector<Plugin> s_Plugins;

	void PluginLoader::Init()
	{
		if (std::filesystem::exists("Resources/Plugins/PluginsManifest"))
		{
			std::ifstream file("Resources/Plugins/PluginsManifest");
			std::string data = "";
			while (getline(file, data, '|'))
				if (!data.empty())
				{
					auto name = data;
					getline(file, data, '|');
					auto dir = data;
#ifdef DY_DEBUG
					std::filesystem::path path = std::filesystem::path("Resources/Plugins") / dir / "Debug" /  name;
#else
					std::filesystem::path path = std::filesystem::path("Resources/Plugins") / dir / "Release" / name;

#endif
					getline(file, data, '|');
					if (data == "1")
					{

						std::vector<std::pair<std::string, std::string>> params;
						getline(file, data, '|');
						std::string type;
						while (data != "_END_PLUGIN")
						{
							getline(file, type, '|');
							params.push_back({ data, type });

							getline(file, data, '|');
						}

						if (std::filesystem::exists(path))
						{
							s_Plugins.push_back({ path, params });
						}
					}
				}
			file.close();
		}
	}

	void PluginLoader::OnUpdate(Timestep ts)
	{
		for (auto& plugin : s_Plugins)
			if (plugin.IsLoaded())
				plugin.OnUpdate(ts);
	}

	void PluginLoader::OnUIRender()
	{
		for (auto& plugin : s_Plugins)
			if (plugin.IsLoaded())
				plugin.OnUIRender();
	}

	bool PluginLoader::GetPluginInfo(const std::filesystem::path& path, PluginInfo& info)
	{
		HINSTANCE library;
		if (LoadPluginLibrary(path.wstring().c_str(), library))
		{
			GetPluginNameFunction getNameFunction = nullptr;
			if (LoadPluginLibraryFunction(library, "DYMATIC_PLUGIN_GET_NAME", &getNameFunction))
			{
				info.path = path;
				info.name = getNameFunction();

				GetPluginVersionMajorFunction getVersionMajorFunction = nullptr;
				if (LoadPluginLibraryFunction(library, "DYMATIC_PLUGIN_GET_VERSION_MAJOR", &getVersionMajorFunction))
				{
					info.versionMajor = getVersionMajorFunction();
					info.version += std::to_string(info.versionMajor);
				}

				GetPluginVersionMinorFunction getVersionMinorFunction = nullptr;
				if (LoadPluginLibraryFunction(library, "DYMATIC_PLUGIN_GET_VERSION_MINOR", &getVersionMinorFunction))
				{
					info.versionMinor = getVersionMinorFunction();

					if (!info.version.empty())
						info.version += "." + std::to_string(info.versionMinor);
				}

				GetPluginVersionPatchFunction getVersionPatchFunction = nullptr;
				if (LoadPluginLibraryFunction(library, "DYMATIC_PLUGIN_GET_VERSION_PATCH", &getVersionPatchFunction))
				{
					info.versionPatch = getVersionPatchFunction();

					if (!info.version.empty())
						info.version += "." + std::to_string(info.versionPatch);
				}

				GetPluginDeveloperFunction getDeveloperFunction = nullptr;
				if (LoadPluginLibraryFunction(library, "DYMATIC_PLUGIN_GET_DEVELOPER", &getDeveloperFunction))
					info.developer = getDeveloperFunction();

				GetPluginDescriptionFunction getDescriptionFunction = nullptr;
				if (LoadPluginLibraryFunction(library, "DYMATIC_PLUGIN_GET_DESCRIPTION", &getDescriptionFunction))
					info.description = getDescriptionFunction();

				// Get Params

				GetParamCountPluginFunction getParamCountFunction = nullptr;
				if (LoadPluginLibraryFunction(library, "DYMATIC_PLUGIN_GET_PARAM_COUNT", &getParamCountFunction))
				{
					int size = getParamCountFunction() / 3;
					for (size_t i = 0; i < size; i++)
					{
						GetParamPluginFunction getParamsFunction = nullptr;
						GetParamDefaultPluginFunction getParamDefaultFunction = nullptr;
						if (LoadPluginLibraryFunction(library, ("DYMATIC_PLUGIN_GET_PARAM_" + std::to_string(i * 3 + 1)).c_str(), &getParamsFunction)
							&& LoadPluginLibraryFunction(library, ("DYMATIC_PLUGIN_GET_PARAM_DEFAULT_" + std::to_string(i * 3 + 2)).c_str(), &getParamDefaultFunction))
						{
							const char* _name;
							const char* _type;

							getParamsFunction(&_name, &_type);

							std::string name = _name;
							std::string type = _type;

							PluginInfo::PluginParam::PluginParamData internal_data;
							PluginInfo::PluginParam::PluginParamType internal_type;
							if (type == "bool")
							{
								internal_type = PluginInfo::PluginParam::PluginParamType::Bool;
								getParamDefaultFunction(&internal_data.Bool);
							}
							else if (type == "int")
							{
								internal_type = PluginInfo::PluginParam::PluginParamType::Int;
								getParamDefaultFunction(&internal_data.Int);
							}
							else if (type == "float")
							{
								internal_type = PluginInfo::PluginParam::PluginParamType::Float;
								getParamDefaultFunction(&internal_data.Float);
							}
							else if (type == "string")
							{
								internal_type = PluginInfo::PluginParam::PluginParamType::String;
								getParamDefaultFunction(&internal_data.String);
							}
							else
								continue;

							info.params.push_back({ name, internal_type, internal_data, internal_data });
						}
					}
				}

				FreePluginLibrary(library);
				return true;
			}
		}
		FreePluginLibrary(library);
		return false;
	}

}