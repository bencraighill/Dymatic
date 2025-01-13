#pragma once

#include <string>

#include "Dymatic/Core/Base.h"
#include "Dymatic/Core/Buffer.h"
#include "Dymatic/Renderer/TextureFormat.h"
#include "Dymatic/Renderer/RendererSpecification.h"
#include "Dymatic/Renderer/RendererResource.h"

namespace Dymatic {

	enum class TextureType
	{
		Texture2D,
		TextureCube,
		Texture3D
	};

	enum class TextureWrap
	{
		None = 0,
		Repeat,
		MirroredRepeat,
		ClampToEdge,
		ClampToBorder,
		MirrorClampToEdge
	};

	enum class TextureFilter
	{
		None = 0,
		Linear,
		Nearest,
		LinearMipmapLinear,
		LinearMipmapNearest,
		NearestMipmapLinear,
		NearestMipmapNearest
	};

	struct TextureSpecification : public RendererSpecification
	{
		TextureFormat Format = TextureFormat::RGBA8;
		bool UseFileChannels = true;

		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Depth = 1;
		TextureWrap SamplerWrap = TextureWrap::Repeat;
		TextureFilter SamplerFilter = TextureFilter::Linear;
		bool GenerateMips = false;
	};

	class Texture : public RendererResource
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual uint32_t GetRendererID() const = 0;
		virtual uint64_t GetHandle() const = 0;

		virtual const TextureSpecification& GetSpecification() const = 0;

		virtual void GetData(void* data, uint32_t size) = 0;
		virtual Buffer GetData() = 0;
		virtual void SetData(const void* data, uint32_t size) = 0;
		virtual void SetData(const Buffer& buffer) = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;
		virtual void BindTexture(uint32_t slot = 0, int level = 0, bool layered = false, int layer = 0) const = 0;

		virtual uint32_t GetMipLevels() const = 0;

		virtual bool IsLoaded() const = 0;

		virtual bool operator==(const Texture& other) const = 0;

		static AssetType GetStaticType() { return AssetType::Texture; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		virtual TextureType GetType() const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(const TextureSpecification& specification);
		static Ref<Texture2D> Create(const std::filesystem::path& filepath, const TextureSpecification& specification = {});
		static Ref<Texture2D> Create(const TextureSpecification& specification, const Buffer& imageData);
		static Ref<Texture2D> Create(const Buffer& fileData, const TextureSpecification& specification = {});

		virtual void StageData(const Buffer& buffer) = 0;
		virtual void UploadData() = 0;

		inline glm::vec2 GetSize() { return glm::vec2( GetWidth(), GetHeight() ); }

		virtual void Copy(Ref<Texture2D> target) const = 0;

		// Used by virtual/memory-only textures
		virtual void SetSpecification(const TextureSpecification& specification) = 0;
		virtual void Clear() = 0;
		virtual void Resize(const uint32_t width, const uint32_t height) = 0;
		inline void Resize(const glm::uvec2 size) { Resize(size.x, size.y); }

		inline size_t GetDataSize() const { return GetWidth() * GetHeight() * Utils::GetDymaticTextureFormatBPP(GetSpecification().Format); }

		virtual TextureType GetType() const override { return TextureType::Texture2D; }
		virtual AssetType GetAssetType() const override { return IsMemoryOnly() ? AssetType::VirtualTexture : AssetType::Texture; }

		virtual bool IsMemoryOnly() const = 0;
	};

	class TextureCube : public Texture
	{
	public:
		static Ref<TextureCube> Create(const TextureSpecification& specification, const Buffer& imageData = Buffer());

		virtual TextureType GetType() const override { return TextureType::TextureCube; }

		static AssetType GetStaticType() { return AssetType::EnvironmentMap; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};

	class Texture3D : public Texture
	{
	public:
		static Ref<Texture3D> Create(const TextureSpecification& specification);
		static Ref<Texture3D> Create(const TextureSpecification& specification, const Buffer& imageData);

		virtual glm::uvec3 GetMipmapLevelSize(uint32_t level) const = 0;

		virtual TextureType GetType() const override { return TextureType::Texture3D; }
	};

}