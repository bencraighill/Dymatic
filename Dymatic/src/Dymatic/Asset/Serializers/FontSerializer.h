#pragma once

#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Renderer/Font.h"

#include "Dymatic/Asset/AssetThread.h"

namespace Dymatic {

	class FontSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const override
		{
			const std::string filepath = AssetManager::GetFileSystemPathString(metadata);

			if (multithreaded)
			{
				Ref<Font> font = Font::Create(filepath, false);
				AssetThread::QueueWork({ [font]() { font->Load(); }, [font]() { font->CreateAtlas(); } });
				asset = font;
			}
			else
			{
				asset = Font::Create(filepath);
			}

			return true;
		}

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
		{
			Ref<Font> font = AssetManager::GetAsset<Font>(handle);
			
			// Write basic properties
			stream.WriteRaw<float>(font->GetLineHeight());

			// Write Glyphs
			const auto& glyphs = font->GetGlyphs();
			stream.WriteRaw<uint32_t>(glyphs.size());
			for (const auto& [codepoint, glyph] : glyphs)
			{
				stream.WriteRaw<uint32_t>(codepoint);
				stream.WriteRaw<glm::vec2>(glyph.Min);
				stream.WriteRaw<glm::vec2>(glyph.Max);
				stream.WriteRaw<double>(glyph.Left);
				stream.WriteRaw<double>(glyph.Bottom);
				stream.WriteRaw<double>(glyph.Right);
				stream.WriteRaw<double>(glyph.Top);
				stream.WriteRaw<glm::vec2>(glyph.Size);
				stream.WriteRaw<float>(glyph.Advance);
				stream.WriteRaw<bool>(glyph.IsWhitespace);
			}

			// Write Atlas Data
			const Ref<Texture2D> atlas = font->GetAtlas();
			stream.WriteRaw<uint32_t>(atlas->GetWidth());
			stream.WriteRaw<uint32_t>(atlas->GetHeight());

			Buffer data = atlas->GetData();
			stream.WriteBuffer(data);
			data.Release();

			return true;
		}

		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
		{
			// Basic Properties
			const float lineHeight = stream.ReadRaw<float>();

			// Glyphs
			const uint32_t glyphCount = stream.ReadRaw<uint32_t>();
			std::unordered_map<uint32_t, Font::Glyph> glyphs(glyphCount);
			for (uint32_t glyphIndex = 0; glyphIndex < glyphCount; glyphIndex++)
			{
				const uint32_t codepoint = stream.ReadRaw<uint32_t>();
				auto& glyph = glyphs[codepoint];

				glyph.Codepoint = codepoint;
				stream.ReadRaw<glm::vec2>(glyph.Min);
				stream.ReadRaw<glm::vec2>(glyph.Max);
				stream.ReadRaw<double>(glyph.Left);
				stream.ReadRaw<double>(glyph.Bottom);
				stream.ReadRaw<double>(glyph.Right);
				stream.ReadRaw<double>(glyph.Top);
				stream.ReadRaw<glm::vec2>(glyph.Size);
				stream.ReadRaw<float>(glyph.Advance);
				stream.ReadRaw<bool>(glyph.IsWhitespace);
			}

			// Atlas Data
			TextureSpecification atlasSpecification;
			atlasSpecification.Format = TextureFormat::RGB8;
			stream.ReadRaw<uint32_t>(atlasSpecification.Width);
			stream.ReadRaw<uint32_t>(atlasSpecification.Height);

			Buffer atlasData;
			stream.ReadBuffer(atlasData);
			Ref<Texture2D> atlas = Texture2D::Create(atlasSpecification, atlasData);
			atlasData.Release();

			Ref<Font> font = Font::Create(lineHeight, glyphs, atlas);
			asset = font;

			return true;
		}
	};

}