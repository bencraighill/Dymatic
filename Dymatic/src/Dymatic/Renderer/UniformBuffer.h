#pragma once

#include "Dymatic/Core/Base.h"
#include "Dymatic/Core/Buffer.h"
#include "Dymatic/Renderer/RendererResource.h"

namespace Dymatic {

	class UniformBuffer : public RendererResource
	{
	public:
		virtual ~UniformBuffer() {}
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void SetData(const Buffer& buffer, uint32_t offset = 0) = 0;

		static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding);

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};
	
}