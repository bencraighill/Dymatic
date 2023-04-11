#include "dypch.h"
#include "Dymatic/Renderer/Font.h"

#pragma push_macro("INFINITE")
#undef INFINITE
#include <msdf-atlas-gen.h>
#pragma pop_macro("INFINITE")

namespace Dymatic {

	Font::Font(const std::string& filepath)
	{
		using namespace msdf_atlas;
		
		if (msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype()) 
		{
			// Load font file
			if (msdfgen::FontHandle* font = msdfgen::loadFont(ft, filepath.c_str())) 
			{
				std::vector<GlyphGeometry> glyphs;
				// FontGeometry is a helper class that loads a set of glyphs from a single font.
				// It can also be used to get additional font metrics, kerning information, etc.
				FontGeometry fontGeometry(&glyphs);
				// In the last argument, you can specify a charset other than ASCII.
				// To load specific glyph indices, use loadGlyphs instead.
				fontGeometry.loadCharset(font, 1.0, Charset::ASCII);

				const double maxCornerAngle = 3.0;
				for (GlyphGeometry& glyph : glyphs)
					glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);
				TightAtlasPacker packer;
				packer.setDimensionsConstraint(TightAtlasPacker::DimensionsConstraint::SQUARE);
				packer.setMinimumScale(24.0);
				packer.setPixelRange(2.0); // If this is adjusted, the shader values also need to be updated
				packer.setMiterLimit(1.0);
				packer.pack(glyphs.data(), glyphs.size());
				// Get final atlas dimensions
				int width = 0, height = 0;
				packer.getDimensions(width, height);
				// The ImmediateAtlasGenerator class facilitates the generation of the atlas bitmap.
				ImmediateAtlasGenerator<
					float, // pixel type of buffer for individual glyphs depends on generator function
					3, // number of atlas color channels
					&msdfGenerator, // function to generate bitmaps for individual glyphs
					BitmapAtlasStorage<byte, 3> // class that stores the atlas bitmap
					// For example, a custom atlas storage class that stores it in VRAM can be used.
				> generator(width, height);
				GeneratorAttributes attributes;
				generator.setAttributes(attributes);
				generator.setThreadCount(4);

				// Generate the final bitmap
				generator.generate(glyphs.data(), glyphs.size());
				msdfgen::BitmapConstRef<byte, 3> bitmap = generator.atlasStorage();

				// Copy bitmap data
				uint32_t size = bitmap.width * bitmap.height * 4;
				unsigned char* data = new unsigned char[size];
				for (uint32_t i = 0; i < bitmap.width * bitmap.height; i++)
				{
					data[i * 4 + 0] = bitmap.pixels[i * 3 + 0];
					data[i * 4 + 1] = bitmap.pixels[i * 3 + 1];
					data[i * 4 + 2] = bitmap.pixels[i * 3 + 2];
					data[i * 4 + 3] = 255;
				}
				m_Atlas = Texture2D::Create(bitmap.width, bitmap.height, TextureFormat::RGBA8);
				m_Atlas->SetData(data, size);
				delete[] data;

				// Get line height
				m_LineHeight = fontGeometry.getMetrics().lineHeight;
				
				// Set glyph metadata
				m_Glyphs.reserve(glyphs.size());
				for (auto& glyph : glyphs)
				{
					double minX, minY, maxX, maxY;
					glyph.getQuadAtlasBounds(minX, minY, maxX, maxY);

					double left, bottom, right, top;
					glyph.getQuadPlaneBounds(left, bottom, right, top);

					m_Glyphs[glyph.getCodepoint()] = {
						glyph.getCodepoint(),
						{ (float)minX / (float)width, (float)minY / (float)height },
						{ (float)maxX / (float)width, (float)maxY / (float)height },
						(float)left,
						(float)bottom,
						(float)right,
						(float)top,
						{ right - left, top - bottom },
						{ (float)glyph.getAdvance() },
						glyph.isWhitespace()
					};
				}
				
				msdfgen::destroyFont(font);
			}
			
			msdfgen::deinitializeFreetype(ft);
		}
	}

	const Font::Glyph* Font::GetGlyph(uint32_t codepoint) const
	{
		auto it = m_Glyphs.find(codepoint);
		if (it != m_Glyphs.end())
			return &it->second;
		return nullptr;
	}

}