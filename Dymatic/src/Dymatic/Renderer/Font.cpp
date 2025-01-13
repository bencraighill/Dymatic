#include "dypch.h"
#include "Dymatic/Renderer/Font.h"
#include "Dymatic/Renderer/Renderer.h"

#pragma push_macro("INFINITE")
#undef INFINITE
#include <msdf-atlas-gen.h>
#pragma pop_macro("INFINITE")

#define THREAD_COUNT 8

namespace Dymatic {

	namespace Utils {

		static const double GetFontQualitySize()
		{
			switch (Renderer::GetTiering().FontQuality)
			{
			case Tiering::Renderer::FontQuality::Low:		return 24.0;
			case Tiering::Renderer::FontQuality::Medium:	return 40.0;
			case Tiering::Renderer::FontQuality::High:		return 128.0;
			case Tiering::Renderer::FontQuality::VeryHigh:	return 256.0;
			}

			return 40.0;
		}

	}

	Font::Font(const std::filesystem::path& filepath, bool load)
		: Font(FontSpecification{ filepath }, load)
	{
	}

	Font::Font(const FontSpecification& specification, bool load)
		: m_Specification(specification)
	{
		if (load)
		{
			Load();
			CreateAtlas();
		}
	}

	Font::Font(const float lineHeight, const std::unordered_map<uint32_t, Glyph>& glyphs, Ref<Texture2D> atlas)
		: m_LineHeight(lineHeight), m_Glyphs(glyphs), m_Atlas(atlas), m_IsLoaded(true)
	{}

	const Font::Glyph* Font::GetGlyph(uint32_t codepoint) const
	{
		auto it = m_Glyphs.find(codepoint);
		if (it != m_Glyphs.end())
			return &it->second;
		return nullptr;
	}

	void Font::Load()
	{
		msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
		DY_CORE_ASSERT(ft);

		std::string pathString = m_Specification.Path.string();

		// Load font file
		msdfgen::FontHandle* font = msdfgen::loadFont(ft, pathString.c_str());

		if (!font)
		{
			DY_CORE_ERROR("Failed to load font: '{}'", pathString);
			return;
		}

		std::vector<msdf_atlas::GlyphGeometry> glyphs;

		// Load the charset as specified by the GlyphRanges.
		// Note: We originally used to just simply use the built in msdf_atlas::Charset::ASCII
		msdf_atlas::Charset charset;
		for (CharsetRange range : m_Specification.CharsetRanges)
		{
			for (uint32_t c = range.Begin; c <= range.End; c++)
				charset.add(c);
		}

		// Note: to load a specific glyph indices, use loadGlyphs instead.
		msdf_atlas::FontGeometry fontGeometry = msdf_atlas::FontGeometry(&glyphs);
		int glyphsLoaded = fontGeometry.loadCharset(font, 1.0, charset);
		DY_CORE_INFO("Loaded {} glyphs from font '{}' (out of {})", glyphsLoaded, pathString, charset.size());

		const double emSize = Utils::GetFontQualitySize();

		msdf_atlas::TightAtlasPacker atlasPacker;
		//packer.setScale(emSize); // TODO: Probably should use this instead of below!!!
		//atlasPacker.setDimensionsConstraint(TightAtlasPacker::DimensionsConstraint::SQUARE);
		atlasPacker.setMinimumScale(emSize);
		atlasPacker.setPixelRange(2.0); // If this is adjusted, the shader values also need to be updated
		atlasPacker.setMiterLimit(1.0);
		atlasPacker.setPadding(0);
		int remaining = atlasPacker.pack(glyphs.data(), glyphs.size());
		DY_CORE_ASSERT(remaining == 0);

		// Get final atlas dimensions
		int width = 0, height = 0;
		atlasPacker.getDimensions(width, height);

		// TODO: Move bitmap related code below to a helper function
		// NOTE: For asset packing we can generate multiple downsamples of the font atlas to support lower end devices.

		// 
#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull

		const uint64_t coloringSeed = 0;
		const bool expensiveColoring = false;
		if (expensiveColoring)
		{
			msdf_atlas::Workload([&glyphs, &coloringSeed](int i, int threadNo) -> bool
				{
					unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
					glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
					return true;
				}, glyphs.size()).finish(THREAD_COUNT);
		}
		else
		{
			unsigned long long glyphSeed = coloringSeed;
			for (msdf_atlas::GlyphGeometry& glyph : glyphs)
			{
				glyphSeed *= LCG_MULTIPLIER;
				glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
			}
		}

		// The ImmediateAtlasGenerator class facilitates the generation of the atlas bitmap.
		msdf_atlas::ImmediateAtlasGenerator<
			float, // pixel type of buffer for individual glyphs depends on generator function
			3, // number of atlas color channels
			&msdf_atlas::msdfGenerator, // function to generate bitmaps for individual glyphs
			msdf_atlas::BitmapAtlasStorage<msdf_atlas::byte, 3> // class that stores the atlas bitmap
			// For example, a custom atlas storage class that stores it in VRAM can be used.
		> generator(width, height);

		// Set the generator attributes
		msdf_atlas::GeneratorAttributes attributes;
		attributes.config.overlapSupport = true;
		attributes.scanlinePass = true;

		generator.setAttributes(attributes);
		generator.setThreadCount(THREAD_COUNT);

		// Generate the final bitmap
		generator.generate(glyphs.data(), glyphs.size());
		msdfgen::BitmapConstRef<msdf_atlas::byte, 3> bitmap = generator.atlasStorage();

		m_Width = bitmap.width;
		m_Height = bitmap.height;
		m_StagingBuffer.Set(Buffer(bitmap.pixels, bitmap.width * bitmap.height * 3));

		// Get line height
		m_LineHeight = fontGeometry.getMetrics().lineHeight;

		// Record the glyph metadata
		m_Glyphs.reserve(glyphs.size());
		for (auto& atlasGlyph : glyphs)
		{
			double minX, minY, maxX, maxY;
			atlasGlyph.getQuadAtlasBounds(minX, minY, maxX, maxY);

			double left, bottom, right, top;
			atlasGlyph.getQuadPlaneBounds(left, bottom, right, top);

			Glyph glyph;
			glyph.Codepoint = atlasGlyph.getCodepoint();
			glyph.Min = glm::vec2((float)minX / (float)width, (float)minY / (float)height);
			glyph.Max = glm::vec2((float)maxX / (float)width, (float)maxY / (float)height);
			glyph.Left = (float)left;
			glyph.Bottom = (float)bottom;
			glyph.Right = (float)right;
			glyph.Top = (float)top;
			glyph.Size = glm::vec2(right - left, top - bottom);
			glyph.Advance = atlasGlyph.getAdvance();
			glyph.IsWhitespace = atlasGlyph.isWhitespace();

			m_Glyphs[atlasGlyph.getCodepoint()] = glyph;
		}

		// Cleanup
		msdfgen::destroyFont(font);
		msdfgen::deinitializeFreetype(ft);
	}

	void Font::CreateAtlas()
	{
		// Create a GPU texture and copy the pixel data to VRAM
		TextureSpecification textureSpecification;
		textureSpecification.Width = m_Width;
		textureSpecification.Height = m_Height;
		textureSpecification.Format = TextureFormat::RGB8;
		textureSpecification.GenerateMips = false;
		m_Atlas = Texture2D::Create(textureSpecification, m_StagingBuffer);

		m_StagingBuffer.Release();

		m_IsLoaded = true;
	}

}