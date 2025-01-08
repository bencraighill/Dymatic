#include "PythonTools.h"

#include "EditorResources.h"

#include "Dymatic/Core/Base.h"
#include "Dymatic/Utils/PlatformUtils.h"
#include "Dymatic/Core/Input.h"

#include "Dymatic/Core/TransactionManager.h"

#include "../EditorLayer.h"
#include "../Panels/PopupsAndNotifications.h"
#include "TextSymbols.h"

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include <fstream>

#include <glm/glm.hpp>

#include "Panels/UI.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

typedef uint64_t PythonContext;

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

		// Internal Usage
		static Ref<Scene> GetActiveScene() { return s_EditorContext->m_ActiveScene; }
		static Entity GetEntityInternal(UUID uuid) { return GetActiveScene()->GetEntityByUUID(uuid); }

		// Python Bindings
		static void NewScene() { s_EditorContext->NewScene(); }
		static void OpenScene() { s_EditorContext->OpenScene(); }
		static void SaveScene() { s_EditorContext->SaveScene(); }
		static void SaveSceneAs() { s_EditorContext->SaveSceneAs(); }

		static void Compile() { s_EditorContext->Compile(); }

		static void Undo() { s_EditorContext->Undo(); }
		static void Redo() { s_EditorContext->Redo(); }
	};

	class PythonTransaction : public Transaction
	{
	public:
		PythonTransaction(const std::string& name, const std::function<void()> execute, const std::function<void()> undo)
			: m_Name(name), m_Execute(execute), m_Undo(undo) {}

		virtual void Execute() override
		{
			if (m_Execute)
				m_Execute();
		}

		virtual void Undo() override
		{
			if (m_Undo)
				m_Undo();
		}

		virtual const std::string GetName() const override
		{
			return m_Name;
		}

	private:
		std::string m_Name;
		std::function<void()> m_Execute;
		std::function<void()> m_Undo;
	};

	class PythonAsset
	{
	public:
		PythonAsset()
			: Handle(0)
		{}

		PythonAsset(uint64_t handle)
			: Handle(handle)
		{}

		std::string GetPath() const
		{
			return AssetManager::GetMetadata(Handle).FilePath.string();
		}

		AssetType GetType() const
		{
			return AssetManager::GetMetadata(Handle).Type;
		}

		bool IsAlive()
		{
			return AssetManager::IsAssetAlive(Handle);
		}

		void Serialize()
		{
			AssetManager::SerializeAsset(Handle);
		}

		void Retarget(const std::string& filepath)
		{
			//AssetManager::RenameAsset(Handle, filepath);
		}

	public:
		uint64_t Handle;
	};

	class AssetManagerPythonInterface
	{
	public:
		static PythonAsset GetAsset(const std::string& filepath)
		{
			return PythonAsset(AssetManager::GetAssetHandleFromFilePath(filepath));
		}

		static bool DoesAssetExist(const std::string& filepath)
		{
			return AssetManager::DoesAssetExist(filepath);
		}
	};

	// Component Interfaces
	class PythonComponent
	{
	public:
		PythonComponent(uint64_t handle)
			: Handle(handle)
		{}

	protected:
		uint64_t Handle;
	};

	class PythonSpriteRendererComponent : public PythonComponent
	{
	public:
		const glm::vec4 GetColor() const { return EditorPythonInterface::GetEntityInternal(Handle).GetComponent<SpriteRendererComponent>().Color; }
		void SetColor(const glm::vec4& color) const { EditorPythonInterface::GetEntityInternal(Handle).GetComponent<SpriteRendererComponent>().Color = color; }
	};

	class PythonStaticMeshComponent : public PythonComponent
	{
	public:
		const PythonAsset GetMesh() const { return PythonAsset(EditorPythonInterface::GetEntityInternal(Handle).GetComponent<StaticMeshComponent>().GetModel()->Handle); }

		void SetMesh(const PythonAsset& mesh) const
		{
			Ref<Model> model = AssetManager::GetAsset<Model>(mesh.Handle);
			EditorPythonInterface::GetEntityInternal(Handle).GetComponent<StaticMeshComponent>().SetModel(model);
		}
	};

	class PythonEntity
	{
	public:
		PythonEntity(UUID handle)
			: Handle(handle)
		{
		}
		
		pybind11::object GetComponent(const pybind11::object pythonComponent)
		{
			if (!CheckPythonComponent(pythonComponent))
				return {};
		
			return pythonComponent(Handle);
		}

		pybind11::object AddComponent(pybind11::object pythonComponent)
		{
			if (!CheckPythonComponent(pythonComponent))
				return {};

			Entity e = EditorPythonInterface::GetEntityInternal(Handle);

			const auto instance = pythonComponent(Handle);

			if (pybind11::isinstance<PythonSpriteRendererComponent>(instance))
				e.AddComponent<SpriteRendererComponent>();
			else if (pybind11::isinstance<PythonStaticMeshComponent>(instance))
				e.AddComponent<StaticMeshComponent>();

			return instance;
		}

		void RemoveComponent(pybind11::object pythonComponent)
		{
			if (!CheckPythonComponent(pythonComponent))
				return;

			Entity e = EditorPythonInterface::GetEntityInternal(Handle);

			const auto instance = pythonComponent(Handle);

			if (pybind11::isinstance<PythonSpriteRendererComponent>(instance))
				e.RemoveComponent<SpriteRendererComponent>();
			else if (pybind11::isinstance<PythonStaticMeshComponent>(instance))
				e.RemoveComponent<StaticMeshComponent>();
		}

		bool HasComponent(pybind11::object pythonComponent)
		{
			if (!CheckPythonComponent(pythonComponent))
				return {};

			Entity e = EditorPythonInterface::GetEntityInternal(Handle);

			const auto instance = pythonComponent(Handle);

			if (pybind11::isinstance<PythonSpriteRendererComponent>(instance))
				return e.HasComponent<SpriteRendererComponent>();
			else if (pybind11::isinstance<PythonStaticMeshComponent>(instance))
				return e.HasComponent<PythonStaticMeshComponent>();

			return false;
		}

		bool HasParent()
		{
			return EditorPythonInterface::GetEntityInternal(Handle).HasParent();
		}

		bool HasChildren()
		{
			return EditorPythonInterface::GetEntityInternal(Handle).HasChildren();
		}

		PythonEntity GetParent()
		{
			return PythonEntity(EditorPythonInterface::GetEntityInternal(Handle).GetParent().GetUUID());
		}

		std::vector<PythonEntity> GetChildren()
		{
			std::vector<Entity> children = EditorPythonInterface::GetEntityInternal(Handle).GetChildren();
			
			std::vector<PythonEntity> pythonChildren;
			for (auto& child : children)
				pythonChildren.push_back(PythonEntity(child.GetUUID()));
			
			return pythonChildren;
		}

		void Parent(const PythonEntity parent)
		{
			Entity entity = EditorPythonInterface::GetEntityInternal(Handle);
			Entity parentEntity = EditorPythonInterface::GetEntityInternal(parent.Handle);

			EditorPythonInterface::GetActiveScene()->ParentEntity(entity, parentEntity);
		}

		void Unparent()
		{
			Entity entity = EditorPythonInterface::GetEntityInternal(Handle);
			EditorPythonInterface::GetActiveScene()->UnparentEntity(entity);
		}

		bool IsAlive()
		{
			return !Handle || EditorPythonInterface::GetEntityInternal(Handle);
		}

		std::string GetTag()
		{
			return EditorPythonInterface::GetEntityInternal(Handle).GetComponent<TagComponent>().Tag;
		}

		void SetTag(std::string tag)
		{
			EditorPythonInterface::GetEntityInternal(Handle).GetComponent<TagComponent>().Tag = tag;
		}

		glm::vec3 GetTranslation()
		{
			return EditorPythonInterface::GetEntityInternal(Handle).GetComponent<TransformComponent>().Transform.Translation;
		}

		void SetTranslation(glm::vec3 translation)
		{
			EditorPythonInterface::GetEntityInternal(Handle).GetComponent<TransformComponent>().Transform.Translation = translation;
		}

		glm::vec3 GetRotation()
		{
			return EditorPythonInterface::GetEntityInternal(Handle).GetComponent<TransformComponent>().Transform.GetRotationDegrees();
		}

		void SetRotation(glm::vec3 rotation)
		{
			EditorPythonInterface::GetEntityInternal(Handle).GetComponent<TransformComponent>().Transform.SetRotationDegrees(rotation);
		}

		glm::vec3 GetScale()
		{
			return EditorPythonInterface::GetEntityInternal(Handle).GetComponent<TransformComponent>().Transform.Scale;
		}

		void SetScale(glm::vec3 scale)
		{
			EditorPythonInterface::GetEntityInternal(Handle).GetComponent<TransformComponent>().Transform.Scale = scale;
		}

	private:
		bool CheckPythonComponent(pybind11::object pyComponent)
		{
			// Ensure a class was provided
			if (!pybind11::isinstance<pybind11::type>(pyComponent))
			{
				DY_CORE_ERROR("Python component is not a valid class type");
				return false;
			}

			// Ensure the entity is null and valid
			if (!IsAlive())
			{
				DY_CORE_ERROR("Python entity is null");
				return false;
			}

			return true;
		}

	public:
		uint64_t Handle;
	};

	class ScenePythonInterface
	{
	public:

		// Binding Functions

		static PythonEntity CreateEntity(const std::string& name)
		{
			return EditorPythonInterface::GetActiveScene()->CreateEntity(name).GetUUID();
		}
		
		static PythonEntity DuplicateEntity(const PythonEntity entity)
		{
			Entity e = EditorPythonInterface::GetEntityInternal(entity.Handle);
			return EditorPythonInterface::GetActiveScene()->DuplicateEntity(e).GetUUID();
		}

		static void DestroyEntity(const PythonEntity entity)
		{
			Entity e = EditorPythonInterface::GetEntityInternal(entity.Handle);
			EditorPythonInterface::GetActiveScene()->DestroyEntity(e);
		}

		static PythonEntity FindEntityByName(const std::string& name)
		{
			Entity e = EditorPythonInterface::GetActiveScene()->FindEntityByName(name);

			if (e)
				return e.GetUUID();

			return 0;
		}
	};
}

namespace Dymatic {

	class PythonPlugin;
	static std::map<uint64_t, Ref<PythonPlugin>> s_Plugins;

	// If we add additional assets that are specifically for usage by a given python plugin/interpreter then this
	// could be changed to store generic assets. Can't see much use for this as this is NOT part of the Asset System/Pipeline.
	// Textures are stored here so their lifetime is tied to the plugin.
	std::unordered_map<AssetHandle, Ref<Texture2D>> s_PluginTextures;

	static pybind11::module s_InspectModule;
	static pybind11::module s_SystemModule;

	static uint32_t GetPythonArgumentCount(const pybind11::function& function)
	{
		pybind11::object result = s_InspectModule.attr("signature")(function).attr("parameters");
		return pybind11::len(result);
	}
	
	static Ref<Texture2D> GetPluginTexture(AssetHandle handle)
	{
		if (!handle)
			return nullptr;

		if (s_PluginTextures.find(handle) != s_PluginTextures.end())
			return s_PluginTextures[handle];

		return nullptr;
	}

	struct PythonFileAction
	{
		PythonFileAction(const std::string& label, const std::string& group, const std::function<void()>& callback)
			: Label(label), Group(group), Callback(callback) {}

		std::string Label;
		std::string Group;
		std::function<void()> Callback;
	};

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
				DY_CORE_ERROR("Specified python plugin path '{}' does not exist.", m_Path.string());
				return;
			}

			if (std::filesystem::is_directory(m_Path))
			{
				DY_CORE_ERROR("Specified python plugin path '{}' is a directory, not a file.", m_Path.string());
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
		}

		~PythonPlugin()
		{	
			// Free all of the textures allocated for this plugin that are associated with its lifetime.
			for (auto& handle : m_TextureHandles)
				s_PluginTextures.erase(handle);

			// Destroy all created taskbar buttons
			for (auto& handle : m_TaskbarButtonHandles)
				Taskbar::RemoveThumbnailButton(handle);

			// Release the python module and interpreter.
			m_Module.release();
		}

		// On load is triggered after the constructor to ensure that this plugin has been added to
		// the s_Plugins map (in case we want to any tied asset loading etc).
		void OnLoad()
		{
			if (pybind11::function onLoad = GetFunction("DymaticOnLoad"))
			{
				uint32_t argumentCount = GetPythonArgumentCount(onLoad);
				if (argumentCount == 0)
					onLoad();
				else if (argumentCount == 1)
				{
					onLoad((uint64_t)m_Handle);
				}
			}
		}

		// OnUnload is triggered before the destructor for similar reasons to OnLoad above.
		void OnUnload()
		{
			if (pybind11::function onUnload = GetFunction("DymaticOnUnload"))
				if (GetPythonArgumentCount(onUnload) == 0)
					onUnload();
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

		inline UUID GetHandle() const { return m_Handle; }

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

		AssetHandle LoadTexture(const std::filesystem::path& filepath)
		{
			std::filesystem::path localPath = (m_Path.parent_path() / filepath).lexically_normal();

			if (!std::filesystem::exists(localPath))
			{
				DY_CORE_ERROR("Specified texture '{}' does not exist.", localPath.string());
				return 0;
			}

			Ref<Texture2D> texture = Texture2D::Create(localPath);
			if (texture)
			{
				s_PluginTextures[texture->Handle] = texture;
				m_TextureHandles.push_back(texture->Handle);
				return texture->Handle;
			}

			return AssetHandle();
		}

		void AddTaskbarButton(UUID handle)
		{
			m_TaskbarButtonHandles.push_back(handle);
		}

		void AddFileAction(const std::string& extension, const std::string& label, const std::function<void()>& callback, const std::string& group)
		{
			m_FileActions[extension].emplace_back(label, group, callback);
		}

	private:
		UUID m_Handle;
		
		std::filesystem::path m_Path;
		pybind11::module_ m_Module;
		
		pybind11::function m_OnUpdate;
		uint32_t m_OnUpdateArgumentCount;
		
		pybind11::function m_OnUIRender;
		uint32_t m_OnUIRenderArgumentCount;

		// Keep track of all our allocated and managed resources
		std::vector<AssetHandle> m_TextureHandles;
		std::vector<UUID> m_TaskbarButtonHandles;
		std::unordered_map<std::string, std::vector<PythonFileAction>> m_FileActions;

		friend class PythonTools;
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
		// Ensure all python OnUnload events are handled before unloading.
		for (auto& [handle, plugin] : s_Plugins)
			plugin->OnUnload();

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
		Ref<PythonPlugin> plugin = CreateRef<PythonPlugin>(path);
		s_Plugins[plugin->GetHandle()] = plugin;
		plugin->OnLoad();
	}

	void PythonTools::UnloadPlugin(std::filesystem::path path)
	{
		path.make_preferred();
		for (auto& [handle, plugin] :s_Plugins)
		{
			if (plugin->GetPath() == path)
			{
				plugin->OnUnload();
				s_Plugins.erase(handle);
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
		for (auto& [handle, plugin] : s_Plugins)
		{
			plugin->OnUpdate(ts);
		}
	}

	void PythonTools::OnImGuiRender(PythonUIRenderStage stage)
	{
		for (auto& [handle, plugin] : s_Plugins)
		{
			plugin->OnUIRender(stage);
		}
	}

	void PythonTools::OnImGuiRender(const std::filesystem::path& path, PythonUIRenderStage stage)
	{
		// Find the specified plugin and call the render command.
		// Note: This lookup is significantly slower than the global render call (Use Sparingly!)
		for (auto& [handle, plugin] : s_Plugins)
			if (plugin->GetPath() == path)
				plugin->OnUIRender(stage);
	}

	void PythonTools::OnImGuiRenderContentBrowser(const std::string& extension)
	{
		for (const auto& [handle, plugin] : s_Plugins)
		{
			const auto& fileActions = plugin->m_FileActions;
			if (fileActions.find(extension) == fileActions.end())
				continue;

			const auto& actions = fileActions.at(extension);

			for (const auto& action : actions)
			{
				const bool grouped = !action.Group.empty();

				const bool visible = !grouped || ImGui::BeginMenu(fmt::format(FA_FOLDER_GEAR " {}", action.Group).c_str());

				if (visible && ImGui::MenuItem(fmt::format(FA_PYTHON " {}", action.Label).c_str()) && action.Callback)
					action.Callback();

				if (grouped && visible)
					ImGui::EndMenu();
			}
		}
	}

}

// Embedded Python Module allowing for an Engine interface.
PYBIND11_EMBEDDED_MODULE(Dymatic, module)
{
	module.doc() = "Dymatic Engine Python Tool Interface Module";
	
	// Dymatic Python data types
	
	// Vec2
	pybind11::class_<glm::vec2>(module, "Vector2")
		.def(pybind11::init<>())
		.def(pybind11::init<float>())
		.def(pybind11::init<float, float>())
		.def("__repr__", [](const glm::vec2& vec2) 
		{
			std::ostringstream oss;
			oss << "Vector2(" << vec2.x << ", " << vec2.y << ")";
			return oss.str();
			})
		.def_readwrite("x", &glm::vec2::x)
		.def_readwrite("y", &glm::vec2::y)
		.def_readwrite("r", &glm::vec2::r)
		.def_readwrite("g", &glm::vec2::g)
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
		.def("__ge__", [](const glm::vec2& v1, const glm::vec2& v2) { return v1.x >= v2.x && v1.y >= v2.y; })
	;
		
	// Vec3
	pybind11::class_<glm::vec3>(module, "Vector3")
		.def(pybind11::init<>())
		.def(pybind11::init<float>())
		.def(pybind11::init<float, float, float>())
		.def("__repr__", [](const glm::vec3& vec3) 
		{
			std::ostringstream oss;
			oss << "Vector3(" << vec3.x << ", " << vec3.y << ", " << vec3.z << ")";
			return oss.str();
		})
		.def_readwrite("x", &glm::vec3::x)
		.def_readwrite("y", &glm::vec3::y)
		.def_readwrite("z", &glm::vec3::z)
		.def_readwrite("r", &glm::vec3::r)
		.def_readwrite("g", &glm::vec3::g)
		.def_readwrite("b", &glm::vec3::b)
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
		.def("__ge__", [](const glm::vec3& v1, const glm::vec3& v2) { return v1.x >= v2.x && v1.y >= v2.y && v1.z >= v2.z; })
	;
		
	// Vec4
	pybind11::class_<glm::vec4>(module, "Vector4")
		.def(pybind11::init<>())
		.def(pybind11::init<float>())
		.def(pybind11::init<float, float, float, float>())
		.def("__repr__", [](const glm::vec4& vec4) 
		{
			std::ostringstream oss;
			oss << "Vector4(" << vec4.x << ", " << vec4.y << ", " << vec4.z << ", " << vec4.w << ")";
			return oss.str();
		})
		.def_readwrite("x", &glm::vec4::x)
		.def_readwrite("y", &glm::vec4::y)
		.def_readwrite("z", &glm::vec4::z)
		.def_readwrite("w", &glm::vec4::w)
		.def_readwrite("r", &glm::vec4::r)
		.def_readwrite("g", &glm::vec4::g)
		.def_readwrite("b", &glm::vec4::b)
		.def_readwrite("a", &glm::vec4::a)
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
		.def("__ge__", [](const glm::vec4& v1, const glm::vec4& v2) { return v1.x >= v2.x && v1.y >= v2.y && v1.z >= v2.z && v1.w >= v2.w; })
	;
		
	// Button
	pybind11::class_<Dymatic::ButtonData>(module, "Button")
		.def(pybind11::init<>())
		.def(pybind11::init<std::string, std::function<void()>>())
		.def_readwrite("Name", &Dymatic::ButtonData::Name)
		.def_readwrite("OnPressed", &Dymatic::ButtonData::OnPressedFunction);
		
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

	// Taskbar
	module.def_submodule("Taskbar", "Dymatic Taskbar Submodule")
		.def("AddThumbnailButton", [](PythonContext context, Dymatic::AssetHandle thumbnail, std::string tooltip, std::function<void()> callback, bool enabled)
		{
			if (Dymatic::s_Plugins.find(context) == Dymatic::s_Plugins.end())
			{
				DY_CORE_ERROR("Could not find Python Plugin with context.");
				return (uint64_t)0;
			}

			Dymatic::Ref<Dymatic::Texture2D> thumbnailAsset = Dymatic::GetPluginTexture(thumbnail);

			Dymatic::Taskbar::ThumbnailButton thumbnailButton;
			thumbnailButton.Icon = thumbnailAsset;
			thumbnailButton.Tooltip = tooltip;
			thumbnailButton.Enabled = enabled;

			if (callback)
				thumbnailButton.Callback = [callback](Dymatic::Taskbar::ThumbnailButton&) { callback(); };

			// Ensure the created button is registered.
			Dymatic::s_Plugins[context]->AddTaskbarButton(thumbnailButton.Handle);

			// Return the handle of the created button after creation.
			Dymatic::Taskbar::AddThumbnailButton(thumbnailButton);

			return (uint64_t)thumbnailButton.Handle;
		}, pybind11::arg("context"), pybind11::arg("thumbnail") = 0, pybind11::arg("tooltip") = std::string(), pybind11::arg("callback") = std::function<void()>(), pybind11::arg("enabled") = true)
		.def("RemoveThumbnailButton", [](uint64_t handle)
		{
			Dymatic::Taskbar::RemoveThumbnailButton(handle);
		}, pybind11::arg("handle"))
		.def("FlashIcon", &Dymatic::Taskbar::FlashIcon)
		.def("SetLoadingStatus", &Dymatic::Taskbar::SetLoading, pybind11::arg("is_loading"))
		.def("SetLoadingProgress", &Dymatic::Taskbar::SetProgress, pybind11::arg("progress"))
	;

	// Content Browser
	module.def_submodule("ContentBrowser", "Dymatic Content Browser Submodule")
		.def("RegisterFileAction", [](PythonContext context, std::string extension, std::string label, std::function<void()> callback, std::string group)
		{
			if (Dymatic::s_Plugins.find(context) == Dymatic::s_Plugins.end())
			{
				DY_CORE_ERROR("Could not find Python Plugin with context.");
				return;
			}

			Dymatic::s_Plugins[context]->AddFileAction(extension, label, callback, group);
		}, pybind11::arg("context"), pybind11::arg("extension"), pybind11::arg("label"), pybind11::arg("callback") = nullptr, pybind11::arg("group") = std::string())
	;

	// Input Commands/Polling
#define KEYCODE(x) keyModule.attr(#x) = (uint16_t)Dymatic::Key::x

	auto keyModule = module.def_submodule("Key", "Dymatic Key Code Submodule");
	// From Input.h
	KEYCODE(Invalid);
	KEYCODE(Space);
	KEYCODE(Apostrophe);
	KEYCODE(Comma);
	KEYCODE(Minus);
	KEYCODE(Period);
	KEYCODE(Slash);
	KEYCODE(D0);
	KEYCODE(D1);
	KEYCODE(D2);
	KEYCODE(D3);
	KEYCODE(D4);
	KEYCODE(D5);
	KEYCODE(D6);
	KEYCODE(D7);
	KEYCODE(D8);
	KEYCODE(D9);
	KEYCODE(Semicolon);
	KEYCODE(Equal);
	KEYCODE(A);
	KEYCODE(B);
	KEYCODE(C);
	KEYCODE(D);
	KEYCODE(E);
	KEYCODE(F);
	KEYCODE(G);
	KEYCODE(H);
	KEYCODE(I);
	KEYCODE(J);
	KEYCODE(K);
	KEYCODE(L);
	KEYCODE(M);
	KEYCODE(N);
	KEYCODE(O);
	KEYCODE(P);
	KEYCODE(Q);
	KEYCODE(R);
	KEYCODE(S);
	KEYCODE(T);
	KEYCODE(U);
	KEYCODE(V);
	KEYCODE(W);
	KEYCODE(X);
	KEYCODE(Y);
	KEYCODE(Z);
	KEYCODE(LeftBracket);
	KEYCODE(Backslash);
	KEYCODE(RightBracket);
	KEYCODE(GraveAccent);
	KEYCODE(World1);
	KEYCODE(World2);
	KEYCODE(Escape);
	KEYCODE(Enter);
	KEYCODE(Tab);
	KEYCODE(Backspace);
	KEYCODE(Insert);
	KEYCODE(Delete);
	KEYCODE(Right);
	KEYCODE(Left);
	KEYCODE(Down);
	KEYCODE(Up);
	KEYCODE(PageUp);
	KEYCODE(PageDown);
	KEYCODE(Home);
	KEYCODE(End);
	KEYCODE(CapsLock);
	KEYCODE(ScrollLock);
	KEYCODE(NumLock);
	KEYCODE(PrintScreen);
	KEYCODE(Pause);
	KEYCODE(F1);
	KEYCODE(F2);
	KEYCODE(F3);
	KEYCODE(F4);
	KEYCODE(F5);
	KEYCODE(F6);
	KEYCODE(F7);
	KEYCODE(F8);
	KEYCODE(F9);
	KEYCODE(F10);
	KEYCODE(F11);
	KEYCODE(F12);
	KEYCODE(F13);
	KEYCODE(F14);
	KEYCODE(F15);
	KEYCODE(F16);
	KEYCODE(F17);
	KEYCODE(F18);
	KEYCODE(F19);
	KEYCODE(F20);
	KEYCODE(F21);
	KEYCODE(F22);
	KEYCODE(F23);
	KEYCODE(F24);
	KEYCODE(F25);
	KEYCODE(KP0);
	KEYCODE(KP1);
	KEYCODE(KP2);
	KEYCODE(KP3);
	KEYCODE(KP4);
	KEYCODE(KP5);
	KEYCODE(KP6);
	KEYCODE(KP7);
	KEYCODE(KP8);
	KEYCODE(KP9);
	KEYCODE(KPDecimal);
	KEYCODE(KPDivide);
	KEYCODE(KPMultiply);
	KEYCODE(KPSubtract);
	KEYCODE(KPAdd);
	KEYCODE(KPEnter);
	KEYCODE(KPEqual);
	KEYCODE(LeftShift);
	KEYCODE(LeftControl);
	KEYCODE(LeftAlt);
	KEYCODE(LeftSuper);
	KEYCODE(RightShift);
	KEYCODE(RightControl);
	KEYCODE(RightAlt);
	KEYCODE(RightSuper);
	KEYCODE(Menu);
	
	pybind11::class_<class Dymatic::Input>(module, "Input")
		.def_static("IsKeyPressed", &Dymatic::Input::IsKeyPressed)
	;
	
	// Editor Commands
	pybind11::class_<Dymatic::EditorPythonInterface>(module, "Editor")
		.def_static("NewScene", &Dymatic::EditorPythonInterface::NewScene)
		.def_static("OpenScene", &Dymatic::EditorPythonInterface::OpenScene)
		.def_static("SaveScene", &Dymatic::EditorPythonInterface::SaveScene)
		.def_static("SaveSceneAs", &Dymatic::EditorPythonInterface::SaveSceneAs)
		.def_static("Compile", &Dymatic::EditorPythonInterface::Compile)

		.def_static("Popup", [](std::string title, std::string message, std::vector<Dymatic::ButtonData> buttons, bool is_loading, Dymatic::AssetHandle texture, std::function<void()> on_render_callback, glm::vec2 on_render_size)
		{
			Dymatic::Ref<Dymatic::Texture2D> textureAsset = Dymatic::GetPluginTexture(texture);
			Dymatic::Popup::Create((FA_PYTHON " ") + title, message, buttons, textureAsset, is_loading, on_render_callback, on_render_size);
		}, pybind11::arg("title"), pybind11::arg("message"), pybind11::arg("buttons") = std::vector<Dymatic::ButtonData>(), pybind11::arg("is_loading") = false, pybind11::arg("texture") = 0, pybind11::arg("on_render_callback") = std::function<void()>(), pybind11::arg("on_render_size") = glm::vec2(0.0f))
		
		.def_static("Notification", [](std::string title, std::string message, std::vector<Dymatic::ButtonData> buttons, float display_time, bool is_loading, Dymatic::AssetHandle texture, glm::vec3 taskbar_color)
		{
			Dymatic::Ref<Dymatic::Texture2D> textureAsset = Dymatic::GetPluginTexture(texture);
			Dymatic::Notification::Create(title, message, buttons, display_time, is_loading, textureAsset ? textureAsset : Dymatic::EditorResources::PythonLogo, taskbar_color);
		}, pybind11::arg("title"), pybind11::arg("message"), pybind11::arg("buttons") = std::vector<Dymatic::ButtonData>(), pybind11::arg("display_time") = 0.0f, pybind11::arg("is_loading") = false, pybind11::arg("texture") = 0, pybind11::arg("taskbar_color") = glm::vec3(1.0f, 0.84f, 0.28f))
	
		.def_static("Transaction", [](std::string name, std::function<void()> execute, std::function<void()> undo)
		{
			const Dymatic::Ref<Dymatic::PythonTransaction> transaction = Dymatic::CreateRef<Dymatic::PythonTransaction>(name, execute, undo);
			Dymatic::TransactionManager::Execute(transaction);
		}, pybind11::arg("name"), pybind11::arg("execute_callback") = nullptr, pybind11::arg("redo_callback") = nullptr)
		.def_static("Undo", &Dymatic::EditorPythonInterface::Undo)
		.def_static("Redo", &Dymatic::EditorPythonInterface::Redo)
	;

	// Entity Logic
	pybind11::class_<Dymatic::PythonEntity>(module, "Entity")
		.def(pybind11::init<uint64_t>())
		.def_readonly("Handle", &Dymatic::PythonEntity::Handle)
		.def_property("Tag", &Dymatic::PythonEntity::GetTag, &Dymatic::PythonEntity::SetTag)
		.def_property("Translation", &Dymatic::PythonEntity::GetTranslation, &Dymatic::PythonEntity::SetTranslation)
		.def_property("Rotation", &Dymatic::PythonEntity::GetRotation, &Dymatic::PythonEntity::SetRotation)
		.def_property("Scale", &Dymatic::PythonEntity::GetScale, &Dymatic::PythonEntity::SetScale)
		
		.def("GetComponent", &Dymatic::PythonEntity::GetComponent)
		.def("AddComponent", &Dymatic::PythonEntity::AddComponent)
		.def("RemoveComponent", &Dymatic::PythonEntity::RemoveComponent)
		.def("HasComponent", &Dymatic::PythonEntity::HasComponent)

		.def("HasChildren", &Dymatic::PythonEntity::HasChildren)
		.def("HasParent", &Dymatic::PythonEntity::HasParent)
		.def("GetChildren", &Dymatic::PythonEntity::GetChildren)
		.def("GetParent", &Dymatic::PythonEntity::GetParent)
		.def("Parent", &Dymatic::PythonEntity::Parent)
		.def("Unparent", &Dymatic::PythonEntity::Unparent)
		
		.def("GetTag", &Dymatic::PythonEntity::GetTag)
		.def("SetTag", &Dymatic::PythonEntity::SetTag)
		.def("GetTranslation", &Dymatic::PythonEntity::GetTranslation)
		.def("SetTranslation", &Dymatic::PythonEntity::SetTranslation)
		.def("GetRotation", &Dymatic::PythonEntity::GetRotation)
		.def("SetRotation", &Dymatic::PythonEntity::SetRotation)
		.def("GetScale", &Dymatic::PythonEntity::GetScale)
		.def("SetScale", &Dymatic::PythonEntity::SetScale)
	;

	// Component System
	pybind11::class_<Dymatic::PythonSpriteRendererComponent>(module, "SpriteRendererComponent")
		.def(pybind11::init<uint64_t>())
		.def_property("Color", &Dymatic::PythonSpriteRendererComponent::GetColor, &Dymatic::PythonSpriteRendererComponent::SetColor)
	;

	pybind11::class_<Dymatic::PythonStaticMeshComponent>(module, "StaticMeshComponent")
		.def(pybind11::init<uint64_t>())
		.def_property("Mesh", &Dymatic::PythonStaticMeshComponent::GetMesh, &Dymatic::PythonStaticMeshComponent::SetMesh)
	;

	// Scene Logic
	pybind11::class_<Dymatic::ScenePythonInterface>(module, "Scene")
		.def_static("CreateEntity", &Dymatic::ScenePythonInterface::CreateEntity)
		.def_static("DuplicateEntity", &Dymatic::ScenePythonInterface::DuplicateEntity)
		.def_static("DestroyEntity", &Dymatic::ScenePythonInterface::DestroyEntity)
		.def_static("FindEntityByName", &Dymatic::ScenePythonInterface::FindEntityByName)
	;

	// Asset Logic
	
	// Asset Manager
	pybind11::class_<Dymatic::AssetManagerPythonInterface>(module, "AssetManager")
		.def_static("GetAsset", &Dymatic::AssetManagerPythonInterface::GetAsset)
		.def_static("DoesAssetExist", &Dymatic::AssetManagerPythonInterface::DoesAssetExist)
	;

	// Asset Type
	pybind11::module_ assetTypeModule = module.def_submodule("AssetTypeModule", "Dymatic Asset Type Submodule");
	pybind11::enum_<Dymatic::AssetType>(assetTypeModule, "AssetTypeEnum")
		.value("None", Dymatic::AssetType::None)
		.value("Scene", Dymatic::AssetType::Scene)
		.value("Prefab", Dymatic::AssetType::Prefab)
		.value("MeshSource", Dymatic::AssetType::MeshSource)
		.value("Mesh", Dymatic::AssetType::Mesh)
		.value("Material", Dymatic::AssetType::Material)
		.value("Texture", Dymatic::AssetType::Texture)
		.value("EnvironmentMap", Dymatic::AssetType::EnvironmentMap)
		.value("VirtualTexture", Dymatic::AssetType::VirtualTexture)
		.value("Font", Dymatic::AssetType::Font)
		.value("Audio", Dymatic::AssetType::Audio)
		.value("ParticleSystem", Dymatic::AssetType::ParticleSystem)
		.value("Skeleton", Dymatic::AssetType::Skeleton)
		.value("Animation", Dymatic::AssetType::Animation)
		.value("AnimationGraph", Dymatic::AssetType::AnimationGraph)
		.value("PhysicsMaterial", Dymatic::AssetType::PhysicsMaterial)
		.value("Video", Dymatic::AssetType::Video)
		.value("Subtitle", Dymatic::AssetType::Subtitle)
		.value("VideoPlayer", Dymatic::AssetType::VideoPlayer)
		.export_values()
		.def_static("ToString", &Dymatic::AssetManager::AssetTypeToString)
	;
	// Create an alias so we can access AssetType directly
	module.attr("AssetType") = assetTypeModule.attr("AssetTypeEnum");


	// Asset Object
	pybind11::class_<Dymatic::PythonAsset>(module, "Asset")
		.def(pybind11::init<>())
		.def(pybind11::init<uint64_t>())
		.def_readonly("Handle", &Dymatic::PythonAsset::Handle)
		.def_property_readonly("Type", &Dymatic::PythonAsset::GetType)
		.def_property_readonly("Path", &Dymatic::PythonAsset::GetPath)
		.def("IsAlive", &Dymatic::PythonAsset::IsAlive)
		.def("Serialize", &Dymatic::PythonAsset::Serialize)
		.def("Retarget", &Dymatic::PythonAsset::Retarget)
	;

	// Plugin API Commands
	pybind11::class_<Dymatic::PythonTools>(module, "Plugin")
		.def_static("LoadTexture", [](PythonContext context, const char* file_path)
		{
			if (Dymatic::s_Plugins.find(context) == Dymatic::s_Plugins.end())
			{
				DY_CORE_ERROR("Could not find Python Plugin with context.");
				return (Dymatic::AssetHandle)0;
			}

			return Dymatic::s_Plugins[context]->LoadTexture(file_path);
		}, pybind11::arg("context"), pybind11::arg("file_path"))
	;

	//========================================================================================================//
	//                                              USER INTERFACE										      //
	//========================================================================================================//

	pybind11::enum_<Dymatic::PythonUIRenderStage>(module, "UIRenderStage")
		.value("Main", Dymatic::PythonUIRenderStage::Main)
		.value("PluginPreferences", Dymatic::PythonUIRenderStage::PluginPreferences)
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
		.value("AlwaysInsertMode", ImGuiInputTextFlags_AlwaysOverwrite)
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

		.def("Image", [](Dymatic::AssetHandle texture, glm::vec2 size, glm::vec4 tint_color, glm::vec4 border_color)
		{ 
			Dymatic::Ref<Dymatic::Texture2D> textureAsset = Dymatic::GetPluginTexture(texture);
			if (textureAsset)
				ImGui::Image((ImTextureID)textureAsset->GetRendererID(), size, { 0, 1 }, { 1, 0 }, tint_color, border_color);
		}, pybind11::arg("texture") = 0, pybind11::arg("size") = glm::vec2(0.0f), pybind11::arg("tint_color") = glm::vec4(1.0f), pybind11::arg("border_color") = glm::vec4(0.0f))
			
		.def("AssetSelectionDropdown", [](const Dymatic::AssetType type, const Dymatic::PythonAsset current, std::function<void(Dymatic::PythonAsset)> callback)
		{
			Dymatic::UI::DrawAssetSelectionDropdown(type, current.Handle, [&](Dymatic::UUID handle)
			{
				if (callback)
					callback(Dymatic::PythonAsset(handle));
			});
		}, pybind11::arg("type") = Dymatic::AssetType::None, pybind11::arg("current") = Dymatic::PythonAsset(0), pybind11::arg("callback") = nullptr)

		.def("AssetSelectionDropdown", [](const Dymatic::AssetType type, const Dymatic::PythonAsset current)
		{
			bool modified = false;
			Dymatic::PythonAsset asset;

			Dymatic::UI::DrawAssetSelectionDropdown(type, current.Handle, [&](Dymatic::UUID handle)
			{
				modified = true;
				asset = Dymatic::PythonAsset(handle);
			});

			return pybind11::make_tuple(modified, asset);

		}, pybind11::arg("type") = Dymatic::AssetType::None, pybind11::arg("current") = Dymatic::PythonAsset(0))

		.def("InputVector", [](const char* label, glm::vec3 vector, glm::vec3 resetValue)
		{
			Dymatic::UI::DrawVec3Control(label, vector, resetValue);
			return vector;

		}, pybind11::arg("label"), pybind11::arg("vector"), pybind11::arg("resetValue") = glm::vec3(0.0f))

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
