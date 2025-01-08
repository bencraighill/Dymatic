#pragma once
#include "Dymatic/Core/Base.h"

#include "Dymatic/Asset/Asset.h"

#include "Dymatic/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace Dymatic {

	enum class TextAlignment
	{
		Left,
		Center,
		Right,
		Justify
	};

	struct CharsetRange
	{
		uint32_t Begin, End;
	};

	struct FontSpecification
	{
		std::filesystem::path Path;
		std::vector<CharsetRange> CharsetRanges = { { 0x0020, 0x00FF } };
	};

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
		static Ref<Font> Create(const std::filesystem::path& filepath, bool load = true) { return CreateRef<Font>(filepath, load); }
		static Ref<Font> Create(const FontSpecification& specification, bool load = true) { return CreateRef<Font>(specification, load); }
		static Ref<Font> Create(const float lineHeight, const std::unordered_map<uint32_t, Glyph>& glyphs, Ref<Texture2D> atlas) { return CreateRef<Font>(lineHeight, glyphs, atlas); }

	public:
		Font(const std::filesystem::path& filepath, bool load);
		Font(const FontSpecification& specification, bool load);
		Font(const float lineHeight, const std::unordered_map<uint32_t, Glyph>& glyphs, Ref<Texture2D> atlas);

		inline Ref<Texture2D> GetAtlas() const { return m_Atlas; }
		inline float GetLineHeight() const { return m_LineHeight; }

		const Glyph* GetGlyph(uint32_t codepoint) const; 
		const std::unordered_map<uint32_t, Glyph>& GetGlyphs() const { return m_Glyphs; }

		void Load();
		void CreateAtlas();

		inline bool IsLoaded() const { return m_IsLoaded; }
		
	private:
		FontSpecification m_Specification;
		bool m_IsLoaded = false;

		Ref<Texture2D> m_Atlas;
		std::unordered_map<uint32_t, Glyph> m_Glyphs;
		float m_LineHeight;

		// Staging Only
		uint32_t m_Width, m_Height;
		Buffer m_StagingBuffer;
	};

}