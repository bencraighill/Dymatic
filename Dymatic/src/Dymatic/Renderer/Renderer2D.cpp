#include "dypch.h"
#include "Dymatic/Renderer/Renderer2D.h"

#include "Dymatic/Renderer/Renderer.h"
#include "Dymatic/Renderer/RendererConstants.h"

#include "Dymatic/Renderer/VertexArray.h"
#include "Dymatic/Renderer/Shader.h"
#include "Dymatic/Renderer/UniformBuffer.h"
#include "Dymatic/Renderer/RenderCommand.h"

#include "Dymatic/Asset/EngineAsset.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Dymatic {

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;

		// Editor-only
		int EntityID;
	};

	struct CircleVertex
	{
		glm::vec3 WorldPosition;
		glm::vec3 LocalPosition;
		glm::vec4 Color;
		float Thickness;
		float Fade;

		// Editor-only
		int EntityID;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;

		// Editor-only
		int EntityID;
	};

	struct TextVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;

		// Editor-only
		int EntityID;
	};

	struct Renderer2DData
	{
		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> QuadShader;
		Ref<Texture2D> WhiteTexture;

		Ref<VertexArray> CircleVertexArray;
		Ref<VertexBuffer> CircleVertexBuffer;
		Ref<Shader> CircleShader;

		Ref<VertexArray> LineVertexArray;
		Ref<VertexBuffer> LineVertexBuffer;
		Ref<Shader> LineShader;

		Ref<VertexArray> TextVertexArray;
		Ref<VertexBuffer> TextVertexBuffer;
		Ref<Shader> TextShader;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		uint32_t CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPtr = nullptr;

		uint32_t LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;

		uint32_t PointVertexCount = 0;
		LineVertex* PointVertexBufferBase = nullptr;
		LineVertex* PointVertexBufferPtr = nullptr;

		uint32_t TextIndexCount = 0;
		TextVertex* TextVertexBufferBase = nullptr;
		TextVertex* TextVertexBufferPtr = nullptr;

		float LineWidth = 2.0f;

		std::array<Ref<Texture2D>, RendererConstants::MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		std::array<Ref<Texture2D>, RendererConstants::MaxFontSlots> FontSlots;
		uint32_t FontSlotIndex = 0;

		glm::vec4 QuadVertexPositions[4];
		glm::vec4 CubeVertexPositions[8];

		Ref<Font> DefaultFont = nullptr;

		Renderer2D::Statistics Stats;
		
		RendererSharedData::CameraData CameraBuffer;
	};

	static Renderer2DData s_Data;

	void Renderer2D::Init()
	{
		DY_PROFILE_FUNCTION();

		s_Data.QuadVertexArray = VertexArray::Create();

		s_Data.QuadVertexBuffer = VertexBuffer::Create(RendererConstants::MaxVertices * sizeof(QuadVertex));
		s_Data.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position"     },
			{ ShaderDataType::Float4, "a_Color"        },
			{ ShaderDataType::Float2, "a_TexCoord"     },
			{ ShaderDataType::Float,  "a_TexIndex"     },
			{ ShaderDataType::Float,  "a_TilingFactor" },
			{ ShaderDataType::Int,    "a_EntityID"     }
			});
		s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

		s_Data.QuadVertexBufferBase = new QuadVertex[RendererConstants::MaxVertices];

		uint32_t* quadIndices = new uint32_t[RendererConstants::MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < RendererConstants::MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, RendererConstants::MaxIndices);
		s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		// Circles
		s_Data.CircleVertexArray = VertexArray::Create();

		s_Data.CircleVertexBuffer = VertexBuffer::Create(RendererConstants::MaxVertices * sizeof(CircleVertex));
		s_Data.CircleVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_WorldPosition" },
			{ ShaderDataType::Float3, "a_LocalPosition" },
			{ ShaderDataType::Float4, "a_Color"         },
			{ ShaderDataType::Float,  "a_Thickness"     },
			{ ShaderDataType::Float,  "a_Fade"          },
			{ ShaderDataType::Int,    "a_EntityID"      }
			});
		s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
		s_Data.CircleVertexArray->SetIndexBuffer(quadIB); // Use quad IB
		s_Data.CircleVertexBufferBase = new CircleVertex[RendererConstants::MaxVertices];

		// Lines
		s_Data.LineVertexArray = VertexArray::Create();

		s_Data.LineVertexBuffer = VertexBuffer::Create(RendererConstants::MaxLineVertices * sizeof(LineVertex));
		s_Data.LineVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    },
			{ ShaderDataType::Int,    "a_EntityID" }
			});
		s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
		s_Data.LineVertexBufferBase = new LineVertex[RendererConstants::MaxLineVertices];

		// Points (shared with Lines)
		s_Data.PointVertexBufferBase = new LineVertex[RendererConstants::MaxPoints];

		// Text
		s_Data.TextVertexArray = VertexArray::Create();

		s_Data.TextVertexBuffer = VertexBuffer::Create(RendererConstants::MaxVertices * sizeof(TextVertex));
		s_Data.TextVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float,  "a_TexIndex" },
			{ ShaderDataType::Int,    "a_EntityID" }
			});
		s_Data.TextVertexArray->AddVertexBuffer(s_Data.TextVertexBuffer);
		s_Data.TextVertexArray->SetIndexBuffer(quadIB); // Use quad IB
		s_Data.TextVertexBufferBase = new TextVertex[RendererConstants::MaxVertices];

		// Setup base white texture
		TextureSpecification whiteTextureSpecifcation;
		whiteTextureSpecifcation.Width = 1;
		whiteTextureSpecifcation.Height = 1;
		s_Data.WhiteTexture = Texture2D::Create(whiteTextureSpecifcation);
		
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		s_Data.QuadShader = Renderer::GetShaderLibrary()->Get("Renderer2D_Quad");
		s_Data.CircleShader = Renderer::GetShaderLibrary()->Get("Renderer2D_Circle");
		s_Data.LineShader = Renderer::GetShaderLibrary()->Get("Renderer2D_Line");
		s_Data.TextShader = Renderer::GetShaderLibrary()->Get("Renderer2D_Text");

		// Set first texture slot to 0
		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

		s_Data.CubeVertexPositions[0] = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[1] = { 0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[2] = { 0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[3] = { -0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[4] = { -0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[5] = { 0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[6] = { 0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[7] = { -0.5f,  0.5f,  0.5f, 1.0f };

		s_Data.DefaultFont = AssetManager::GetAsset<Font>((AssetHandle)EngineAsset::DefaultFont);

		// Note: RendererSharedData now handles the camera uniform buffer.
	}

	void Renderer2D::Shutdown()
	{
		DY_PROFILE_FUNCTION();

		delete[] s_Data.QuadVertexBufferBase;
	}

	void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		DY_PROFILE_FUNCTION();
		
		s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
		s_Data.CameraBuffer.ViewPosition = transform[3];
		
		UpdateCamera();

		StartBatch();
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		DY_PROFILE_FUNCTION();
		
		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_Data.CameraBuffer.ViewPosition = glm::vec4(camera.GetPosition(), 1.0f);

		UpdateCamera();

		StartBatch();
	}

	void Renderer2D::EndScene()
	{
		DY_PROFILE_FUNCTION();

		Flush();

		// Clear all texture and font slots (excluding white texture) to allow asset references to die if unused
		std::fill(s_Data.TextureSlots.begin() + 1, s_Data.TextureSlots.begin() + s_Data.TextureSlotIndex, nullptr);
		std::fill(s_Data.FontSlots.begin(), s_Data.FontSlots.begin() + s_Data.FontSlotIndex, nullptr);
	}

	void Renderer2D::StartBatch()
	{
		StartQuadBatch();
		StartCircleBatch();
		StartLineBatch();
		StartPointBatch();
		StartTextBatch();
	}

	void Renderer2D::Flush()
	{
		FlushQuads();
		FlushCircles();
		FlushLines();
		FlushPoints();
		FlushText();
	}

	void Renderer2D::FlushQuads()
	{
		if (!s_Data.QuadIndexCount)
			return;

		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
		s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

		// Bind textures
		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			s_Data.TextureSlots[i]->Bind(i);

		s_Data.QuadShader->Bind();
		RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
		s_Data.Stats.DrawCalls++;
	}

	void Renderer2D::StartQuadBatch()
	{
		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;
		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D::NextQuadBatch()
	{
		FlushQuads();
		StartQuadBatch();
	}

	void Renderer2D::FlushCircles()
	{
		if (!s_Data.CircleIndexCount)
			return;

		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase);
		s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

		s_Data.CircleShader->Bind();
		RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
		s_Data.Stats.DrawCalls++;
	}

	void Renderer2D::StartCircleBatch()
	{
		s_Data.CircleIndexCount = 0;
		s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;
	}

	void Renderer2D::NextCircleBatch()
	{
		FlushCircles();
		StartCircleBatch();
	}

	void Renderer2D::FlushLines()
	{
		if (!s_Data.LineVertexCount)
			return;

		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
		s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

		s_Data.LineShader->Bind();
		RenderCommand::SetLineWidth(s_Data.LineWidth);
		RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
		s_Data.Stats.DrawCalls++;
	}

	void Renderer2D::StartLineBatch()
	{
		s_Data.LineVertexCount = 0;
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;
	}

	void Renderer2D::NextLineBatch()
	{
		FlushLines();
		StartLineBatch();
	}

	void Renderer2D::FlushPoints()
	{
		if (!s_Data.PointVertexCount)
			return;

		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.PointVertexBufferPtr - (uint8_t*)s_Data.PointVertexBufferBase);
		s_Data.LineVertexBuffer->SetData(s_Data.PointVertexBufferBase, dataSize);

		s_Data.LineShader->Bind();
		RenderCommand::SetPointSize(10.0f);
		RenderCommand::DrawPoints(s_Data.LineVertexArray, s_Data.PointVertexCount);
		s_Data.Stats.DrawCalls++;
	}

	void Renderer2D::StartPointBatch()
	{
		s_Data.PointVertexCount = 0;
		s_Data.PointVertexBufferPtr = s_Data.PointVertexBufferBase;
	}

	void Renderer2D::NextPointBatch()
	{
		FlushPoints();
		StartPointBatch();
	}

	void Renderer2D::FlushText()
	{
		if (!s_Data.TextIndexCount)
			return;

		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.TextVertexBufferPtr - (uint8_t*)s_Data.TextVertexBufferBase);
		s_Data.TextVertexBuffer->SetData(s_Data.TextVertexBufferBase, dataSize);

		// Bind textures
		for (uint32_t i = 0; i < s_Data.FontSlotIndex; i++)
			s_Data.FontSlots[i]->Bind(i);

		s_Data.TextShader->Bind();
		RenderCommand::DrawIndexed(s_Data.TextVertexArray, s_Data.TextIndexCount);
		s_Data.Stats.DrawCalls++;
	}

	void Renderer2D::StartTextBatch()
	{
		s_Data.TextIndexCount = 0;
		s_Data.TextVertexBufferPtr = s_Data.TextVertexBufferBase;
		s_Data.FontSlotIndex = 0;
	}

	void Renderer2D::NextTextBatch()
	{
		FlushText();
		StartTextBatch();
	}

	void Renderer2D::UpdateCamera()
	{
		// The Renderer2D only actually sets the first two uniforms in the buffer so we specify the size here.
		const size_t bufferUploadSize = sizeof(glm::mat4) + sizeof(glm::vec4);
		Renderer::SetCameraData(s_Data.CameraBuffer, bufferUploadSize);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		DY_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DY_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		DY_PROFILE_FUNCTION();

		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f; // White Texture
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (s_Data.QuadIndexCount >= RendererConstants::MaxIndices)
			NextQuadBatch();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;

			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		DY_PROFILE_FUNCTION();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.QuadIndexCount >= RendererConstants::MaxIndices)
			NextQuadBatch();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= RendererConstants::MaxTextureSlots)
				NextQuadBatch();

			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;

			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DY_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DY_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness /*= 1.0f*/, float fade /*= 0.005f*/, int entityID /*= -1*/)
	{
		DY_PROFILE_FUNCTION();

		if (s_Data.CircleIndexCount >= RendererConstants::MaxIndices)
			NextCircleBatch();

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.CircleVertexBufferPtr->WorldPosition = transform * s_Data.QuadVertexPositions[i];
			s_Data.CircleVertexBufferPtr->LocalPosition = s_Data.QuadVertexPositions[i] * 2.0f;
			s_Data.CircleVertexBufferPtr->Color = color;
			s_Data.CircleVertexBufferPtr->Thickness = thickness;
			s_Data.CircleVertexBufferPtr->Fade = fade;
			s_Data.CircleVertexBufferPtr->EntityID = entityID;
			s_Data.CircleVertexBufferPtr++;
		}

		s_Data.CircleIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID)
	{
		s_Data.LineVertexBufferPtr->Position = p0;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr->EntityID = entityID;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexBufferPtr->Position = p1;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr->EntityID = entityID;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexCount += 2;
	}

	void Renderer2D::DrawLineDashed(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, float dashRatio, float scale, int entityID)
	{
		// Calculate line/pattern lengths
		const float lineLength = glm::distance(p0, p1);
		const float dashLength = dashRatio * scale;
		const float spaceLength = (1.0f - dashRatio) * scale;
		const float patternLength = dashLength + spaceLength;

		// Calculate the number of full patterns that fit
		const uint32_t fullPatternCount = lineLength / patternLength;
		const glm::vec3 lineDirection = glm::normalize(p1 - p0);

		glm::vec3 currentPoint = p0;

		// Draw all full line segments
		for (uint32_t i = 0; i < fullPatternCount; i++)
		{
			const glm::vec3 dashEndPoint = currentPoint + lineDirection * dashLength;
			DrawLine(currentPoint, dashEndPoint, color, entityID);
			currentPoint = dashEndPoint + lineDirection * spaceLength;
		}

		// Draw the remaining partial segment
		const float remainingLength = glm::length(p1 - currentPoint);
		if (remainingLength > 0.0f)
		{
			if (remainingLength > dashLength)
				DrawLine(currentPoint, currentPoint + lineDirection * dashLength, color, entityID);
			else
				DrawLine(currentPoint, p1, color, entityID);
		}
	}

	void Renderer2D::DrawPoint(const glm::vec3& point, const glm::vec4& color, int entityID)
	{
		if (s_Data.PointVertexCount >= RendererConstants::MaxPoints)
			NextPointBatch();

		s_Data.PointVertexBufferPtr->Position = point;
		s_Data.PointVertexBufferPtr->Color = color;
		s_Data.PointVertexBufferPtr->EntityID = entityID;
		s_Data.PointVertexBufferPtr++;

		s_Data.PointVertexCount++;
	}

	void Renderer2D::DrawText(const glm::mat4& transform, const std::string& text, const TextAlignment alignment, Ref<Font> font, const glm::vec4& color, float kerning, float lineSpacing, float maxWidth, int entityID)
	{
		DY_PROFILE_FUNCTION();

		if (!font || !font->IsLoaded())
			font = s_Data.DefaultFont;

		if (!font)
			return;

		constexpr size_t quadVertexCount = 4;

		const float scale_x = glm::length(glm::vec3(transform[0][0], transform[1][0], transform[2][0]));
		const float scaledMaxWidth = maxWidth / scale_x;

		glm::vec2 cursor = glm::vec2(0.0f);
		float lineLength;
		int lineMaxIndex;
		int spaceCount;

		const size_t textLength = text.size();
		for (uint32_t index = 0; index < textLength; index++)
		{
			const char character = text[index];

			// If we hit a new line, reset the cursor and move on to the next character.
			if (character == '\n')
			{
				cursor.x = 0.0f;
				cursor.y -= font->GetLineHeight() + lineSpacing;
				continue;
			}

			// When the cursor is situated at the beginning of a new line, we can calculate the length of the line.
			if (cursor.x == 0.0f)
			{
				lineLength = 0;
				spaceCount = 0;
				float previousLength = 0.0f;
				float currentLength = 0.0f;
				for (uint32_t i = index; i < textLength; i++)
				{
					const char c = text[i];

					previousLength = currentLength;

					const Font::Glyph* glyph = font->GetGlyph(c);
					if (glyph)
					{
						currentLength += glyph->Advance + kerning;
					}

					if (currentLength > maxWidth && maxWidth != 0.0f && lineLength != 0)
						break;

					if (c == '\n')
					{
						lineLength = previousLength;
						lineMaxIndex = i;
						break;
					}

					if (i == textLength - 1)
					{
						lineLength = currentLength;
						lineMaxIndex = i;
						break;
					}

					if (c == ' ')
					{
						lineLength = previousLength;
						lineMaxIndex = i;

						if (alignment == TextAlignment::Justify)
							spaceCount++;
					}
				}
			}

			const Font::Glyph* glyph = font->GetGlyph(character);
			if (!glyph)
				continue;

			if (!glyph->IsWhitespace)
			{

				const glm::vec2 textureCoords[] = { glyph->Min, { glyph->Max.x, glyph->Min.y }, glyph->Max, { glyph->Min.x, glyph->Max.y } };

				if (s_Data.TextIndexCount >= RendererConstants::MaxIndices)
					NextTextBatch();

				int fontIndex = 0;
				for (uint32_t i = 1; i < s_Data.FontSlotIndex; i++)
				{
					if (*s_Data.FontSlots[i] == *font->GetAtlas())
					{
						fontIndex = i;
						break;
					}
				}

				if (fontIndex == 0)
				{
					if (s_Data.FontSlotIndex >= RendererConstants::MaxFontSlots)
						NextTextBatch();

					fontIndex = s_Data.FontSlotIndex;
					s_Data.FontSlots[s_Data.FontSlotIndex] = font->GetAtlas();
					s_Data.FontSlotIndex++;
				}

				float offset;

				switch (alignment)
				{
				case TextAlignment::Left:		offset = 0.0f; break;
				case TextAlignment::Center:		offset = lineLength * 0.5f; break;
				case TextAlignment::Right:		offset = lineLength; break;
				case TextAlignment::Justify:	offset = 0.0f; break;
				}

				for (size_t i = 0; i < quadVertexCount; i++)
				{
					s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(glyph->Size * glm::vec2(s_Data.QuadVertexPositions[i]) + cursor + glm::vec2(glyph->Left + (glyph->Size.x * 0.5f), glyph->Bottom + (glyph->Size.y * 0.5f)) - glm::vec2(offset, 0.0f), 0.0f, 1.0f);
					s_Data.TextVertexBufferPtr->Color = color;
					s_Data.TextVertexBufferPtr->TexCoord = textureCoords[i];
					s_Data.TextVertexBufferPtr->TexIndex = fontIndex;
					s_Data.TextVertexBufferPtr->EntityID = entityID;

					s_Data.TextVertexBufferPtr++;
				}

				s_Data.TextIndexCount += 6;

				s_Data.Stats.QuadCount++;
			}

			// Move the cursor along by the space required by the glyph and any additional kerning,
			// only if the character is not a space at the end of the line.
			if (character != ' ' || index != lineMaxIndex)
				cursor.x += glyph->Advance + kerning;

			if (alignment == TextAlignment::Justify && character == ' ')
				cursor.x += std::fmax(maxWidth - lineLength, 0.0f) / (float)spaceCount;

			if (index >= lineMaxIndex)
			{
				cursor.x = 0.0f;
				cursor.y -= font->GetLineHeight() + lineSpacing;
				continue;
			}
		}
	}

	void Renderer2D::DrawTextComponent(const glm::mat4& transform, TextComponent& tc, int entityID)
	{
		DY_PROFILE_FUNCTION();

		DrawText(transform, tc.TextString, tc.Alignment, tc.Font, tc.Color, tc.Kerning, tc.LineSpacing, tc.MaxWidth, entityID);
	}

	void Renderer2D::DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID)
	{
		glm::vec3 p0 = glm::vec3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p1 = glm::vec3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p2 = glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
		glm::vec3 p3 = glm::vec3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

		DrawLine(p0, p1, color, entityID);
		DrawLine(p1, p2, color, entityID);
		DrawLine(p2, p3, color, entityID);
		DrawLine(p3, p0, color, entityID);
	}

	void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		glm::vec3 lineVertices[4];
		for (size_t i = 0; i < 4; i++)
			lineVertices[i] = transform * s_Data.QuadVertexPositions[i];

		DrawLine(lineVertices[0], lineVertices[1], color, entityID);
		DrawLine(lineVertices[1], lineVertices[2], color, entityID);
		DrawLine(lineVertices[2], lineVertices[3], color, entityID);
		DrawLine(lineVertices[3], lineVertices[0], color, entityID);
	}

	void Renderer2D::DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, int entityID /*= -1*/)
	{
		glm::vec3 p0 = glm::vec3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z - size.z * 0.5f);
		glm::vec3 p1 = glm::vec3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z - size.z * 0.5f);
		glm::vec3 p2 = glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z - size.z * 0.5f);
		glm::vec3 p3 = glm::vec3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z - size.z * 0.5f);
		glm::vec3 p4 = glm::vec3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z + size.z * 0.5f);
		glm::vec3 p5 = glm::vec3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z + size.z * 0.5f);
		glm::vec3 p6 = glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z + size.z * 0.5f);
		glm::vec3 p7 = glm::vec3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z + size.z * 0.5f);

		DrawLine(p0, p1, color, entityID);
		DrawLine(p1, p2, color, entityID);
		DrawLine(p2, p3, color, entityID);
		DrawLine(p3, p0, color, entityID);

		DrawLine(p4, p5, color, entityID);
		DrawLine(p5, p6, color, entityID);
		DrawLine(p6, p7, color, entityID);
		DrawLine(p7, p4, color, entityID);
		
		DrawLine(p0, p4, color, entityID);
		DrawLine(p1, p5, color, entityID);
		DrawLine(p2, p6, color, entityID);
		DrawLine(p3, p7, color, entityID);
	}

	void Renderer2D::DrawCube(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		glm::vec3 lineVertices[8];
		for (size_t i = 0; i < 8; i++)
			lineVertices[i] = transform * s_Data.CubeVertexPositions[i];

		DrawLine(lineVertices[0], lineVertices[1], color, entityID);
		DrawLine(lineVertices[1], lineVertices[2], color, entityID);
		DrawLine(lineVertices[2], lineVertices[3], color, entityID);
		DrawLine(lineVertices[3], lineVertices[0], color, entityID);

		DrawLine(lineVertices[4], lineVertices[5], color, entityID);
		DrawLine(lineVertices[5], lineVertices[6], color, entityID);
		DrawLine(lineVertices[6], lineVertices[7], color, entityID);
		DrawLine(lineVertices[7], lineVertices[4], color, entityID);

		DrawLine(lineVertices[0], lineVertices[4], color, entityID);
		DrawLine(lineVertices[1], lineVertices[5], color, entityID);
		DrawLine(lineVertices[2], lineVertices[6], color, entityID);
		DrawLine(lineVertices[3], lineVertices[7], color, entityID);
	}

	void Renderer2D::DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int entityID)
	{
		if (src.Texture)
			DrawQuad(transform, src.Texture, src.TilingFactor, src.Color, entityID);
		else
			DrawQuad(transform, src.Color, entityID);
	}

	float Renderer2D::GetLineWidth()
	{
		return s_Data.LineWidth;
	}

	void Renderer2D::SetLineWidth(float width)
	{
		s_Data.LineWidth = width;
	}

	void Renderer2D::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_Data.Stats;
	}

}