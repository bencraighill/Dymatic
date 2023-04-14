#include "PythonTools.h"

#include "Dymatic/Core/Base.h"
#include "Dymatic/Utils/PlatformUtils.h"

#include "../EditorLayer.h"
#include "TextSymbols.h"

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

#include <fstream>

#include <glm/glm.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

namespace Dymatic {

	static Dymatic::EditorLayer* s_EditorContext = nullptr;

	static int GetVersionMajor() { return DY_VERSION_MAJOR; }
	static int GetVersionMinor() { return DY_VERSION_MINOR; }
	static int GetVersionPatch() { return DY_VERSION_PATCH; }
	static const char* GetVersionString() { return DY_VERSION_STRING; }

	class EditorPythonInterface
	{
	public:
		EditorPythonInterface() = delete;

		static void NewScene() { s_EditorContext->NewScene(); }
		static void OpenScene() { s_EditorContext->OpenScene(); }
		static void SaveScene() { s_EditorContext->SaveScene(); }
		static void SaveSceneAs() { s_EditorContext->SaveSceneAs(); }
		
		static void Compile() { s_EditorContext->Compile(); }
	};
}

PYBIND11_EMBEDDED_MODULE(Dymatic, module)
{
	module.doc() = "Dymatic Engine Python Tool Interface Module";
	
	// Dymatic Python data types
	
	// Vec2
	pybind11::class_<glm::vec2>(module, "vec2")
		.def(pybind11::init<>())
		.def(pybind11::init<float, float>())
		.def("__repr__", [](const glm::vec2& vec2) 
		{
			std::ostringstream oss;
			oss << "vec2(" << vec2.x << ", " << vec2.y << ")";
			return oss.str();
			})
		.def_readwrite("x", &glm::vec2::x)
		.def_readwrite("y", &glm::vec2::y)
		.def("__add__", [](const glm::vec2& v1, const glm::vec2& v2) { return v1 + v2; })
		.def("__sub__", [](const glm::vec2& v1, const glm::vec2& v2) { return v1 - v2; })
		.def("__mul__", [](const glm::vec2& v1, const glm::vec2& v2) { return v1 * v2; })
		.def("__truediv__", [](const glm::vec2& v1, const glm::vec2& v2) { return v1 / v2; })
		.def("__iadd__", [](glm::vec2& v1, const glm::vec2& v2) { return v1 += v2; })
		.def("__isub__", [](glm::vec2& v1, const glm::vec2& v2) { return v1 -= v2; })
		.def("__imul__", [](glm::vec2& v1, const glm::vec2& v2) { return v1 *= v2; })
		.def("__itruediv__", [](glm::vec2& v1, const glm::vec2& v2) { return v1 /= v2; })
		.def("__neg__", [](const glm::vec2& v) { return -v; })
		.def("__eq__", [](const glm::vec2& v1, const glm::vec2& v2) { return v1 == v2; })
		.def("__ne__", [](const glm::vec2& v1, const glm::vec2& v2) { return v1 != v2; })
		.def("__lt__", [](const glm::vec2& v1, const glm::vec2& v2) { return v1.x < v2.x&& v1.y < v2.y; })
		.def("__gt__", [](const glm::vec2& v1, const glm::vec2& v2) { return v1.x > v2.x && v1.y > v2.y; })
		.def("__le__", [](const glm::vec2& v1, const glm::vec2& v2) { return v1.x <= v2.x && v1.y <= v2.y; })
		.def("__ge__", [](const glm::vec2& v1, const glm::vec2& v2) { return v1.x >= v2.x && v1.y >= v2.y; });
		
	// Vec3
	pybind11::class_<glm::vec3>(module, "vec3")
		.def(pybind11::init<>())
		.def(pybind11::init<float, float, float>())
		.def("__repr__", [](const glm::vec3& vec3) 
		{
			std::ostringstream oss;
			oss << "vec3(" << vec3.x << ", " << vec3.y << ", " << vec3.z << ")";
			return oss.str();
		})
		.def_readwrite("x", &glm::vec3::x)
		.def_readwrite("y", &glm::vec3::y)
		.def_readwrite("z", &glm::vec3::z)
		.def("__add__", [](const glm::vec3& v1, const glm::vec3& v2) { return v1 + v2; })
		.def("__sub__", [](const glm::vec3& v1, const glm::vec3& v2) { return v1 - v2; })
		.def("__mul__", [](const glm::vec3& v1, const glm::vec3& v2) { return v1 * v2; })
		.def("__truediv__", [](const glm::vec3& v1, const glm::vec3& v2) { return v1 / v2; })
		.def("__iadd__", [](glm::vec3& v1, const glm::vec3& v2) { return v1 += v2; })
		.def("__isub__", [](glm::vec3& v1, const glm::vec3& v2) { return v1 -= v2; })
		.def("__imul__", [](glm::vec3& v1, const glm::vec3& v2) { return v1 *= v2; })
		.def("__itruediv__", [](glm::vec3& v1, const glm::vec3& v2) { return v1 /= v2; })
		.def("__neg__", [](const glm::vec3& v) { return -v; })
		.def("__eq__", [](const glm::vec3& v1, const glm::vec3& v2) { return v1 == v2; })
		.def("__ne__", [](const glm::vec3& v1, const glm::vec3& v2) { return v1 != v2; })
		.def("__lt__", [](const glm::vec3& v1, const glm::vec3& v2) { return v1.x < v2.x && v1.y < v2.y && v1.z < v2.z; })
		.def("__gt__", [](const glm::vec3& v1, const glm::vec3& v2) { return v1.x > v2.x && v1.y > v2.y && v1.z > v2.z; })
		.def("__le__", [](const glm::vec3& v1, const glm::vec3& v2) { return v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z; })
		.def("__ge__", [](const glm::vec3& v1, const glm::vec3& v2) { return v1.x >= v2.x && v1.y >= v2.y && v1.z >= v2.z; });
		
	// Vec4
	pybind11::class_<glm::vec4>(module, "vec4")
		.def(pybind11::init<>())
		.def(pybind11::init<float, float, float, float>())
		.def("__repr__", [](const glm::vec4& vec4) 
		{
			std::ostringstream oss;
			oss << "vec4(" << vec4.x << ", " << vec4.y << ", " << vec4.z << ", " << vec4.w << ")";
			return oss.str();
		})
		.def_readwrite("x", &glm::vec4::x)
		.def_readwrite("y", &glm::vec4::y)
		.def_readwrite("z", &glm::vec4::z)
		.def_readwrite("w", &glm::vec4::w)
		.def("__add__", [](const glm::vec4& v1, const glm::vec4& v2) { return v1 + v2; })
		.def("__sub__", [](const glm::vec4& v1, const glm::vec4& v2) { return v1 - v2; })
		.def("__mul__", [](const glm::vec4& v1, const glm::vec4& v2) { return v1 * v2; })
		.def("__truediv__", [](const glm::vec4& v1, const glm::vec4& v2) { return v1 / v2; })
		.def("__iadd__", [](glm::vec4& v1, const glm::vec4& v2) { return v1 += v2; })
		.def("__isub__", [](glm::vec4& v1, const glm::vec4& v2) { return v1 -= v2; })
		.def("__imul__", [](glm::vec4& v1, const glm::vec4& v2) { return v1 *= v2; })
		.def("__itruediv__", [](glm::vec4& v1, const glm::vec4& v2) { return v1 /= v2; })
		.def("__neg__", [](const glm::vec4& v) { return -v; })
		.def("__eq__", [](const glm::vec4& v1, const glm::vec4& v2) { return v1 == v2; })
		.def("__ne__", [](const glm::vec4& v1, const glm::vec4& v2) { return v1 != v2; })
		.def("__lt__", [](const glm::vec4& v1, const glm::vec4& v2) { return v1.x < v2.x && v1.y < v2.y && v1.z < v2.z && v1.w < v2.w; })
		.def("__gt__", [](const glm::vec4& v1, const glm::vec4& v2) { return v1.x > v2.x && v1.y > v2.y && v1.z > v2.z && v1.w > v2.w; })
		.def("__le__", [](const glm::vec4& v1, const glm::vec4& v2) { return v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z && v1.w <= v2.w; })
		.def("__ge__", [](const glm::vec4& v1, const glm::vec4& v2) { return v1.x >= v2.x && v1.y >= v2.y && v1.z >= v2.z && v1.w >= v2.w; });
		
		
	// Versioning
	module.def("GetVersionMajor", &Dymatic::GetVersionMajor);
	module.def("GetVersionMinor", &Dymatic::GetVersionMinor);
	module.def("GetVersionPatch", &Dymatic::GetVersionPatch);
	module.def("GetVersionString", &Dymatic::GetVersionString);
	
	// Platform Utils
	pybind11::class_<Dymatic::PlatformUtils>(module, "Platform")
		.def_static("OpenFileDialogue", &Dymatic::FileDialogs::OpenFile)
		.def_static("SaveFileDialogue", &Dymatic::FileDialogs::SaveFile)
		.def_static("SelectFolderDialogue", &Dymatic::FileDialogs::SelectFolder)
		;
	
	// Editor Commands
	pybind11::class_<Dymatic::EditorPythonInterface>(module, "Editor")
		.def_static("NewScene", &Dymatic::EditorPythonInterface::NewScene)
		.def_static("OpenScene", &Dymatic::EditorPythonInterface::OpenScene)
		.def_static("SaveScene", &Dymatic::EditorPythonInterface::SaveScene)
		.def_static("SaveSceneAs", &Dymatic::EditorPythonInterface::SaveSceneAs)
		.def_static("Compile", &Dymatic::EditorPythonInterface::Compile)
		;

	//========================================================================================================//
	//                                              USER INTERFACE										      //
	//========================================================================================================//

	pybind11::enum_<Dymatic::PythonUIRenderStage>(module, "UIRenderStage")
		.value("Main", Dymatic::PythonUIRenderStage::Main)
		.value("MenuBar", Dymatic::PythonUIRenderStage::MenuBar)
		.value("MenuBar_File", Dymatic::PythonUIRenderStage::MenuBar_File)
		.value("MenuBar_Edit", Dymatic::PythonUIRenderStage::MenuBar_Edit)
		.value("MenuBar_Window", Dymatic::PythonUIRenderStage::MenuBar_Window)
		.value("MenuBar_View", Dymatic::PythonUIRenderStage::MenuBar_View)
		.value("MenuBar_Script", Dymatic::PythonUIRenderStage::MenuBar_Script)
		.value("MenuBar_Help", Dymatic::PythonUIRenderStage::MenuBar_Help)
		.export_values();

	pybind11::enum_<::ImGuiTreeNodeFlags_>(module, "TreeFlags")
		.value("NoFlags", ImGuiTreeNodeFlags_None)
		.value("Selected", ImGuiTreeNodeFlags_Selected)
		.value("Framed", ImGuiTreeNodeFlags_Framed)
		.value("AllowItemOverlap", ImGuiTreeNodeFlags_AllowItemOverlap)
		.value("NoTreePushOnOpen", ImGuiTreeNodeFlags_NoTreePushOnOpen)
		.value("NoAutoOpenOnLog", ImGuiTreeNodeFlags_NoAutoOpenOnLog)
		.value("DefaultOpen", ImGuiTreeNodeFlags_DefaultOpen)
		.value("OpenOnDoubleClick", ImGuiTreeNodeFlags_OpenOnDoubleClick)
		.value("OpenOnArrow", ImGuiTreeNodeFlags_OpenOnArrow)
		.value("Leaf", ImGuiTreeNodeFlags_Leaf)
		.value("Bullet", ImGuiTreeNodeFlags_Bullet)
		.value("FramePadding", ImGuiTreeNodeFlags_FramePadding)
		.value("SpanAvailWidth", ImGuiTreeNodeFlags_SpanAvailWidth)
		.value("SpanFullWidth", ImGuiTreeNodeFlags_SpanFullWidth)
		.value("NavLeftJumpsBackHere", ImGuiTreeNodeFlags_NavLeftJumpsBackHere)
		.value("CollapsingHeader", ImGuiTreeNodeFlags_CollapsingHeader)
		.export_values();

	pybind11::enum_<::ImGuiButtonFlags_>(module, "ButtonFlags")
		.value("NoFlags", ImGuiButtonFlags_None)
		.value("MouseButtonLeft", ImGuiButtonFlags_MouseButtonLeft)
		.value("MouseButtonRight", ImGuiButtonFlags_MouseButtonRight)
		.value("MouseButtonMiddle", ImGuiButtonFlags_MouseButtonMiddle)
		.export_values();

	pybind11::enum_<::ImGuiInputTextFlags_>(module, "InputTextFlags")
		.value("NoFlags", ImGuiInputTextFlags_None)
		.value("CharsDecimal", ImGuiInputTextFlags_CharsDecimal)
		.value("CharsHexadecimal", ImGuiInputTextFlags_CharsHexadecimal)
		.value("CharsUppercase", ImGuiInputTextFlags_CharsUppercase)
		.value("CharsNoBlank", ImGuiInputTextFlags_CharsNoBlank)
		.value("AutoSelectAll", ImGuiInputTextFlags_AutoSelectAll)
		.value("EnterReturnsTrue", ImGuiInputTextFlags_EnterReturnsTrue)
		.value("CallbackCompletion", ImGuiInputTextFlags_CallbackCompletion)
		.value("CallbackHistory", ImGuiInputTextFlags_CallbackHistory)
		.value("CallbackAlways", ImGuiInputTextFlags_CallbackAlways)
		.value("CallbackCharFilter", ImGuiInputTextFlags_CallbackCharFilter)
		.value("AllowTabInput", ImGuiInputTextFlags_AllowTabInput)
		.value("CtrlEnterForNewLine", ImGuiInputTextFlags_CtrlEnterForNewLine)
		.value("NoHorizontalScroll", ImGuiInputTextFlags_NoHorizontalScroll)
		.value("AlwaysInsertMode", ImGuiInputTextFlags_AlwaysInsertMode)
		.value("ReadOnly", ImGuiInputTextFlags_ReadOnly)
		.value("Password", ImGuiInputTextFlags_Password)
		.value("NoUndoRedo", ImGuiInputTextFlags_NoUndoRedo)
		.value("CharsScientific", ImGuiInputTextFlags_CharsScientific)
		.export_values();

	pybind11::enum_<::ImGuiWindowFlags_>(module, "WindowFlags")
		.value("NoFlags", ImGuiWindowFlags_None)
		.value("NoTitleBar", ImGuiWindowFlags_NoTitleBar)
		.value("NoResize", ImGuiWindowFlags_NoResize)
		.value("NoMove", ImGuiWindowFlags_NoMove)
		.value("NoScrollbar", ImGuiWindowFlags_NoScrollbar)
		.value("NoScrollWithMouse", ImGuiWindowFlags_NoScrollWithMouse)
		.value("NoCollapse", ImGuiWindowFlags_NoCollapse)
		.value("AlwaysAutoResize", ImGuiWindowFlags_AlwaysAutoResize)
		.value("NoBackground", ImGuiWindowFlags_NoBackground)
		.value("NoSavedSettings", ImGuiWindowFlags_NoSavedSettings)
		.value("NoMouseInputs", ImGuiWindowFlags_NoMouseInputs)
		.value("MenuBar", ImGuiWindowFlags_MenuBar)
		.value("HorizontalScrollbar", ImGuiWindowFlags_HorizontalScrollbar)
		.value("NoFocusOnAppearing", ImGuiWindowFlags_NoFocusOnAppearing)
		.value("NoBringToFrontOnFocus", ImGuiWindowFlags_NoBringToFrontOnFocus)
		.value("AlwaysVerticalScrollbar", ImGuiWindowFlags_AlwaysVerticalScrollbar)
		.value("AlwaysHorizontalScrollbar", ImGuiWindowFlags_AlwaysHorizontalScrollbar)
		.value("AlwaysUseWindowPadding", ImGuiWindowFlags_AlwaysUseWindowPadding)
		.value("NoNavInputs", ImGuiWindowFlags_NoNavInputs)
		.value("NoNavFocus", ImGuiWindowFlags_NoNavFocus)
		.value("UnsavedDocument", ImGuiWindowFlags_UnsavedDocument)
		.value("NoNav", ImGuiWindowFlags_NoNav)
		.value("NoDecoration", ImGuiWindowFlags_NoDecoration)
		.value("NoInputs", ImGuiWindowFlags_NoInputs)
		.export_values();
	
	module.def_submodule("UI")
		.def("BeginWindow", [](const char* name, bool closeable, ImGuiWindowFlags flags) 
		{
			bool open = true;
			bool visible = ImGui::Begin(fmt::format(CHARACTER_ICON_PLUGIN " {}", name).c_str(), closeable ? &open : nullptr, flags);
			return std::make_tuple(visible, open);
		}, pybind11::arg("name"), pybind11::arg("closeable") = true, pybind11::arg("flags") = 0)
		
		.def("EndWindow", &ImGui::End)
		.def("BeginChildWindow", [](const char* name, glm::vec2 size, bool border, ImGuiWindowFlags flags) { return ImGui::BeginChild(name, ImVec2(size.x, size.y), border, flags); }, pybind11::arg("name"), pybind11::arg("size"), pybind11::arg("border") = false, pybind11::arg("flags") = 0)
		.def("EndChildWindow", &ImGui::EndChild)

		.def("SameLine", &ImGui::SameLine, pybind11::arg("offset_from_start_x") = 0.0f, pybind11::arg("spacing") = -1.0f)

		.def("Text", &ImGui::TextUnformatted, pybind11::arg("text"), pybind11::arg("text_end") = nullptr)
		.def("TextDisabled", &ImGui::TextDisabledUnformatted, pybind11::arg("text"), pybind11::arg("text_end") = nullptr)

		.def("Separator", &ImGui::Separator)

		.def("Button", [](const char* label, glm::vec2 size, ImGuiButtonFlags flags) { return ImGui::ButtonEx(label, ImVec2(size.x, size.y), flags); }, pybind11::arg("label"), pybind11::arg("size") = glm::vec2(0.0f), pybind11::arg("flags") = ImGuiButtonFlags_None)

		.def("InputText", [](const char* label, std::string str, ImGuiInputTextFlags flags)
		{
			bool modified = ImGui::InputText(label, &str, flags);
			return pybind11::make_tuple(modified, str);
		}, pybind11::arg("label"), pybind11::arg("str"), pybind11::arg("flags") = 0)

		.def("DragInt", [](const char* label, int v, float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
		{
			bool modified = ImGui::DragInt(label, &v, v_speed, v_min, v_max, format, flags);
			return pybind11::make_tuple(modified , v);
		}, pybind11::arg("label"), pybind11::arg("v"), pybind11::arg("v_speed") = 1.0f, pybind11::arg("v_min") = 0, pybind11::arg("v_max") = 0, pybind11::arg("format") = "%d", pybind11::arg("flags") = 0)

		.def("BeginMenu", &ImGui::BeginMenu, pybind11::arg("label"), pybind11::arg("enabled") = true)
		.def("EndMenu", &ImGui::EndMenu)
		.def("MenuItem", [](const char* label, const char* shortcut, bool selected, bool enabled)
		{
			return ImGui::MenuItem(label, shortcut, selected, enabled);
		}, pybind11::arg("label"), pybind11::arg("shortcut") = nullptr, pybind11::arg("selected") = false, pybind11::arg("enabled") = true)
		;
}

namespace Dymatic {

	class PythonPlugin;
	static std::vector<Ref<PythonPlugin>> s_Plugins;

	static pybind11::module s_InspectModule;
	static pybind11::module s_SystemModule;

	static uint32_t GetPythonArgumentCount(const pybind11::function& function)
	{
		pybind11::object result = s_InspectModule.attr("signature")(function).attr("parameters");
		return pybind11::len(result);
	}

	class PythonPlugin
	{
	public:

		inline const std::filesystem::path& GetPath() const { return m_Path; }

		pybind11::function GetFunction(const char* name)
		{
			try
			{
				if (pybind11::hasattr(m_Module, name))
					return pybind11::getattr(m_Module, name);
			}
			catch (pybind11::error_already_set& e)
			{
				DY_CORE_ERROR(e.what());
			}
			return pybind11::none();
		}

		PythonPlugin(const std::filesystem::path& path)
			: m_Path(path)
		{
			m_Path.make_preferred();

			if (!std::filesystem::exists(m_Path))
			{
				DY_CORE_ERROR("Specified python plugin path '{0}' does not exist.", m_Path.string());
				return;
			}

			if (std::filesystem::is_directory(m_Path))
			{
				DY_CORE_ERROR("Specified python plugin path '{0}' is a directory, not a file.", m_Path.string());
				return;
			}

			try
			{
				s_SystemModule.attr("path").attr("append")(m_Path.parent_path().string().c_str());
				m_Module = pybind11::module_::import(m_Path.filename().stem().string().c_str());

				// Ensure we get the latest module version (in case we are reloading)
				m_Module.reload(); 
			}
			catch (pybind11::error_already_set& e)
			{
				DY_CORE_ERROR(e.what());
			}

			if (m_OnUpdate = GetFunction("DymaticOnUpdate"))
				m_OnUpdateArgumentCount = GetPythonArgumentCount(m_OnUpdate);

			if (m_OnUIRender = GetFunction("DymaticOnUIRender"))
				m_OnUIRenderArgumentCount = GetPythonArgumentCount(m_OnUIRender);

			if (pybind11::function onLoad = GetFunction("DymaticOnLoad"))
				if (GetPythonArgumentCount(onLoad) == 0)
					onLoad();
			
		}

		~PythonPlugin()
		{
			if (pybind11::function onUnload = GetFunction("DymaticOnUnload"))
				if (GetPythonArgumentCount(onUnload) == 0)
					onUnload();
			
			
			m_Module.release();
		}

		void OnUpdate(Timestep ts)
		{
			if (!m_OnUpdate.is_none())
			{
				if (m_OnUpdateArgumentCount == 0)
					m_OnUpdate();
				else if (m_OnUpdateArgumentCount == 1)
					m_OnUpdate((float)ts);
			}
		}

		void OnUIRender(PythonUIRenderStage stage)
		{
			if (m_OnUIRender.is_none())
				return;

			if (m_OnUIRenderArgumentCount == 0)
			{
				if (stage == PythonUIRenderStage::Main)
					m_OnUIRender();
			}
			else if (m_OnUIRenderArgumentCount == 1)
				m_OnUIRender(stage);
		}

	private:
		std::filesystem::path m_Path;
		pybind11::module_ m_Module;
		
		pybind11::function m_OnUpdate;
		int m_OnUpdateArgumentCount;
		
		pybind11::function m_OnUIRender;
		int m_OnUIRenderArgumentCount;
	};

	void PythonTools::Init(EditorLayer* context)
	{
		s_EditorContext = context;
		pybind11::initialize_interpreter();

		s_SystemModule = pybind11::module::import("sys");
		s_InspectModule = pybind11::module::import("inspect");
	}

	void PythonTools::Shutdown()
	{
		s_Plugins.clear();

		s_SystemModule.release();
		s_InspectModule.release();

		pybind11::finalize_interpreter();

	}

	void PythonTools::RunScript(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
		{
			DY_CORE_ERROR("Specified python script '{0}' path does not exist.", path.string());
			return;
		}

		try
		{
			std::ifstream file(path);
			std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			pybind11::exec(contents);
		}
		catch (pybind11::error_already_set& e)
		{
			DY_CORE_ERROR(e.what());
		}
	}

	void PythonTools::RunGlobalCommand(const std::string& command)
	{
		try
		{
			pybind11::exec(command);
		}
		catch (pybind11::error_already_set & e)
		{
			DY_CORE_ERROR(e.what());
		}
	}

	void PythonTools::LoadPlugin(const std::filesystem::path& path)
	{
		s_Plugins.emplace_back(CreateRef<PythonPlugin>(path));
	}

	void PythonTools::UnloadPlugin(std::filesystem::path path)
	{
		path.make_preferred();
		for (uint32_t i = 0; i < s_Plugins.size(); i++)
		{
			if (s_Plugins[i]->GetPath() == path)
			{
				s_Plugins.erase(s_Plugins.begin() + i);
				return;
			}
		}
	}

	void PythonTools::ReloadPlugin(const std::filesystem::path& path)
	{
		UnloadPlugin(path);
		LoadPlugin(path);
	}

	void PythonTools::OnUpdate(Timestep ts)
	{
		for (auto& plugin : s_Plugins)
		{
			plugin->OnUpdate(ts);
		}
	}

	void PythonTools::OnImGuiRender(PythonUIRenderStage stage)
	{
		for (auto& plugin : s_Plugins)
		{
			plugin->OnUIRender(stage);
		}
	}

}