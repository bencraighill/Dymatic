#pragma once
#include "Dymatic/Asset/Asset.h"

#include "Dymatic/Renderer/Texture.h"
#include "Dymatic/Renderer/Shader.h"

#include "Dymatic/Core/Buffer.h"

// Editor Only
#include "Dymatic/Editor/Material/MaterialGraph.h"

namespace Dymatic {

	// Editor Only
	namespace Editor {
		class MaterialCompiler;
	}

	class MaterialAsset : public Asset
	{
	public:
		static AssetType GetStaticType() { return AssetType::Material; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		enum AlphaBlendMode
		{
			Opaque = 0,
			Masked,
			Translucent,
			Dithered,

			ALPHA_BLEND_MODE_SIZE
		};

		enum MaterialUsage
		{
			Surface = 0,
			PostProcessing,
			Volumetric,
			Particle,

			MATERIAL_USGAE_SIZE
		};

		enum MaterialRenderStage
		{
			None = 0,
			Default,
			PreDepth,
			VXGI,
			Shadow,
			ShadowCSM,

			MATERIAL_RENDER_STAGE_SIZE
		};

		struct MaterialTexture
		{
			uint64_t CompilerNodeID;
			Ref<Texture2D> Texture;

			MaterialTexture(uint64_t compilerNodeID, Ref<Texture2D> texture)
				: CompilerNodeID(compilerNodeID), Texture(texture) {}
		};

		enum class MaterialParameterType
		{
			Bool,
			Float,
			Vector2,
			Vector3,
			Vector4,
			Texture
		};

		struct MaterialParameterData
		{
			static const char* MaterialParameterTypeToString(const MaterialParameterType type);
			static const MaterialParameterType MaterialParameterTypeFromString(const std::string& typeString);

			union Data
			{
				uint32_t Bool;
				float Float;
				glm::vec2 Vector2;
				glm::vec3 Vector3;
				glm::vec4 Vector4;
				uint64_t Texture;

				Data() = default;
				Data(bool value)			 { Bool    = value; }
				Data(float value)			 { Float   = value; }
				Data(const glm::vec2& value) { Vector2 = value; }
				Data(const glm::vec3& value) { Vector3 = value; }
				Data(const glm::vec4& value) { Vector4 = value; }
				Data(uint64_t value)		 { Texture = value; }
			};
			
			std::string Name;
			MaterialParameterType Type;
			Data Value;

			// Internal usage
			uint32_t Offset;
		};

		struct MaterialProperties
		{
			MaterialUsage Usage = MaterialAsset::MaterialUsage::Surface;
			AlphaBlendMode AlphaBlendMode = MaterialAsset::AlphaBlendMode::Opaque;
			bool CastShadows = true;
			bool TwoSided = false;
			bool Lit = true;
			bool Tessellation = false;
		};

	public:
		static const char* AlphaBlendModeToString(AlphaBlendMode mode);
		static AlphaBlendMode GetMaterialAlphaBlendMode(const std::string& string);
		
		static const char* MaterialRenderStageToString(const MaterialRenderStage stage);
		static MaterialRenderStage GetMaterialRenderStage(const std::string& string);
		
		static const char* MaterialUsageToString(const MaterialUsage usage);
		static MaterialUsage GetMaterialUsage(const std::string& string);

		virtual const bool HasShader(MaterialRenderStage renderStage) const = 0;
		virtual const Ref<Shader>& GetShader(MaterialRenderStage renderStage) const = 0;
		virtual const std::unordered_map<MaterialRenderStage, Ref<Shader>>& GetShaders() const = 0;
		virtual const std::string& GetPathTraceSource() const = 0;

		virtual const std::vector<MaterialTexture>& GetTextures() const = 0;
		virtual const Buffer& GetTextureBuffer() = 0;
		
		virtual const std::unordered_map<std::string, MaterialParameterData>& GetParameters() const = 0;
		virtual uint32_t GetParameterBufferSize() const = 0;
		virtual const Buffer& GetParameterBuffer() = 0;

		virtual const MaterialProperties& GetProperties() const = 0;

		virtual bool IsInstance() const = 0;
		virtual bool IsLoaded() const = 0;

		const UUID GetInvalidationID() const { return m_InvalidationID; }

	protected:
		UUID m_InvalidationID;
	};

	class MaterialSource : public MaterialAsset
	{
	public:
		static Ref<MaterialSource> Create() { return CreateRef<MaterialSource>(); }
		static Ref<MaterialSource> Create(const Ref<Editor::MaterialGraph> editorGraph) { return CreateRef<MaterialSource>(editorGraph); }
		static Ref<MaterialSource> Create(const std::unordered_map<MaterialRenderStage, Ref<Shader>>& shaders, const std::vector<MaterialTexture>& textures = {}, const std::unordered_map<std::string, MaterialParameterData>& parameters = {}, Ref<Editor::MaterialGraph> editorGraph = nullptr) { return CreateRef<MaterialSource>(shaders, textures, parameters, editorGraph); }

		MaterialSource() = default;
		MaterialSource(const Ref<Editor::MaterialGraph> editorGraph);
		MaterialSource(const std::unordered_map<MaterialRenderStage, Ref<Shader>>& shaders, const std::vector<MaterialTexture>& textures, const std::unordered_map<std::string, MaterialParameterData>& parameters, Ref<Editor::MaterialGraph> editorGraph);

		~MaterialSource();

		MaterialSource& operator=(const MaterialSource& other);

		virtual bool IsInstance() const override { return false; }
		virtual bool IsLoaded() const override { return m_IsLoaded; }

		virtual const bool HasShader(MaterialRenderStage renderStage) const override { return m_Shaders.find(renderStage) != m_Shaders.end(); }
		virtual const Ref<Shader>& GetShader(MaterialRenderStage renderStage) const override { return m_Shaders.at(renderStage); }
		virtual const std::unordered_map<MaterialRenderStage, Ref<Shader>>& GetShaders() const override { return m_Shaders; }
		void SetShader(MaterialRenderStage renderStage, Ref<Shader> shader) { m_Shaders[renderStage] = shader; }
		void SetShaders(const std::unordered_map<MaterialRenderStage, Ref<Shader>>& shaders) { m_Shaders = shaders; }

		virtual const std::string& GetPathTraceSource() const override { return m_PathTraceSource; }
		void SetPathTraceSource(const std::string& source) { m_PathTraceSource = source; }
		
		virtual const std::vector<MaterialTexture>& GetTextures() const override { return m_Textures; }
		virtual const Buffer& GetTextureBuffer() override { return m_TextureBuffer; }
		void SetTextures(const std::vector<MaterialTexture>& textures);

		virtual const std::unordered_map<std::string, MaterialParameterData>& GetParameters() const override { return m_Parameters; };
		virtual uint32_t GetParameterBufferSize() const { return m_ParameterBuffer.Size; }
		void SetParameters(const std::unordered_map<std::string, MaterialParameterData>& parameters);

		virtual const Buffer& GetParameterBuffer(void) override { return m_ParameterBuffer; }

		virtual const MaterialProperties& GetProperties() const override { return m_Properties; }
		void SetProperties(const MaterialProperties& properties) { m_Properties = properties; }

		void SetUsage(MaterialUsage usage) { m_Properties.Usage = usage; }
		void SetAlphaBlendMode(AlphaBlendMode blendMode) { m_Properties.AlphaBlendMode = blendMode; }
		void SetCastShadows(bool castShadows) { m_Properties.CastShadows = castShadows; }
		void SetTwoSided(bool twoSided) { m_Properties.TwoSided = twoSided; }
		void SetLit(bool lit) { m_Properties.Lit = lit; }
		void SetTessellation(bool tessellation) { m_Properties.Tessellation = tessellation; }

	private:
		bool m_IsLoaded = false;
		MaterialProperties m_Properties;

		std::unordered_map<MaterialRenderStage, Ref<Shader>> m_Shaders;
		std::string m_PathTraceSource;

		// Note: Textures are mapped based upon their internal 'Node ID', not their respective asset handles
		// This is required as two separate material nodes may share a texture in the base material but could be overridden differently in an instance.
		std::vector<MaterialTexture> m_Textures;
		std::unordered_map<std::string, MaterialParameterData> m_Parameters;
		std::unordered_map<uint64_t, uint32_t> m_TextureIndexMap;
			
		Buffer m_ParameterBuffer;
		Buffer m_TextureBuffer;

		// Editor Only
		Ref<Editor::MaterialGraph> m_EditorGraph;
		friend class MaterialSerializer;
		friend class Editor::MaterialCompiler;
		friend class MaterialPanel;
	};

	class MaterialInstance : public MaterialAsset
	{
	public:
		static Ref<MaterialInstance> Create(Ref<MaterialAsset> parent, const std::unordered_map<std::string, MaterialParameterData::Data>& parameterOverrides = {}, const std::unordered_map<uint64_t, Ref<Texture2D>>& textureOverrides = {}) { return CreateRef<MaterialInstance>(parent, parameterOverrides, textureOverrides); }
		
		MaterialInstance(Ref<MaterialAsset> parent, const std::unordered_map<std::string, MaterialParameterData::Data>& parameterOverrides, const std::unordered_map<uint64_t, Ref<Texture2D>>& textureOverrides)
			: m_Parent(parent), m_ParameterOverrides(parameterOverrides), m_TextureOverrides(textureOverrides) {}

		~MaterialInstance();

		virtual bool IsInstance() const override { return true; }
		virtual bool IsLoaded() const override { return m_Parent && m_Parent->IsLoaded(); }
		
		virtual const bool HasShader(MaterialRenderStage renderStage) const override { return m_Parent->HasShader(renderStage); }
		virtual const Ref<Shader>& GetShader(MaterialRenderStage renderStage) const override { return m_Parent->GetShader(renderStage); }
		virtual const std::unordered_map<MaterialRenderStage, Ref<Shader>>& GetShaders() const override { return m_Parent->GetShaders(); }
		
		virtual const std::string& GetPathTraceSource() const override { return m_Parent->GetPathTraceSource(); }

		virtual const std::vector<MaterialTexture>& GetTextures() const override { return m_Parent->GetTextures(); }
		const std::unordered_map<uint64_t, Ref<Texture2D>>& GetTextureOverrides() { return m_TextureOverrides; }
		virtual const Buffer& GetTextureBuffer() override;
		void SetTextureOverride(const uint64_t id, const Ref<Texture2D> texture);
		void RemoveTextureOverride(const uint64_t id);
		
		virtual const std::unordered_map<std::string, MaterialParameterData>& GetParameters() const override { return m_Parent->GetParameters(); }
		virtual uint32_t GetParameterBufferSize() const override { return m_Parent->GetParameterBufferSize(); }
		const std::unordered_map<std::string, MaterialParameterData::Data>& GetParameterOverrides() { return m_ParameterOverrides; }
		virtual const Buffer& GetParameterBuffer() override;
		void SetParameterOverride(const std::string& name, const MaterialParameterData::Data& value);
		void RemoveParameterOverride(const std::string& name);
		
		virtual const MaterialProperties& GetProperties() const override { return m_Parent->GetProperties(); }
		Ref<MaterialAsset> GetParent() const { return m_Parent; }

	private:
		void CheckParentInvalidation();
		void InvalidateParameters();
		void InvalidateTextures();
	private:
		Ref<MaterialAsset> m_Parent;
		std::unordered_map<std::string, MaterialParameterData::Data> m_ParameterOverrides;
		std::unordered_map<uint64_t, Ref<Texture2D>> m_TextureOverrides;

		Buffer m_ParameterBuffer;
		Buffer m_TextureBuffer;

		UUID m_ParentInvalidationID;
	};
	
}