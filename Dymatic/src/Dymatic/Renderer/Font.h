#pragma once
#include "Dymatic/Core/Base.h"

#include "Dymatic/Asset/Asset.h"

#include "Dymatic/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace Dymatic {

	class Font : public Asset
	{
	public:
		static AssetType GetStaticType() { return AssetType::Font; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		
	public:
		struct Glyph
		{
			uint32_t Codepoint;
			
			glm::vec2 Min;
			glm::vec2 Max;
			double Left, Bottom, Right, Top;
			glm::vec2 Size;
			float Advance;
			bool IsWhitespace;
		};
		
		
	public:
		static Ref<Font> Create(const std::string& filepath) { return CreateRef<Font>(filepath); }
		Font(const std::string& filepath);

		inline Ref<Texture2D> GetAtlas() const { return m_Atlas; }
		inline float GetLineHeight() const { return m_LineHeight; }

		const Glyph* GetGlyph(uint32_t codepoint) const; 
		
	private:
		Ref<Texture2D> m_Atlas;
		std::unordered_map<uint32_t, Glyph> m_Glyphs;
		float m_LineHeight;
	};

}