#include "dypch.h"
#include "Dymatic/Renderer/Utils/ShaderCompressor.h"

#include "Dymatic/Renderer/Utils/BitBuffer.h"

#include <queue>

namespace Dymatic {

	// OpenGL 4.6 compatible GLSL shader minifier for Dymatic Engine
	// Note: Vulkan keyword minification is also supported along with specific extension keywords

	// This is based on the OpenGL Shading Language 4.60.8 specification outlined by Khronos here:
	// https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.pdf

	namespace Utils {
	
		// Configurable entry point (this will not be stripped from the minification result)
		static std::string s_EntryPointKeyword = "main";

		// Set of built in keywords outlined in specification
		static const std::unordered_set<std::string> s_Keywords = {

			// Keywords (actively used in specification)
			"const","uniform", "buffer", "shared", "attribute", "varying", "coherent",
			"volatile", "restrict", "readonly", "writeonly", "atomic_uint", "layout",
			"centroid", "flat", "smooth", "noperspective", "patch", "sample", "invariant",
			"precise", "break", "continue", "do", "for", "while", "switch", "case",
			"default", "if", "else", "subroutine", "in", "out", "inout", "int", "void",
			"bool", "true", "false", "float", "double", "discard", "return", "vec2",
			"vec3", "vec4",  "ivec2", "ivec3", "ivec4", "bvec2", "bvec3", "bvec4", "uint",
			"uvec2", "uvec3", "uvec4", "dvec2", "dvec3", "dvec4", "mat2", "mat3",
			"mat4", "mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4","mat4x2",
			"mat4x3", "mat4x4", "dmat2", "dmat3", "dmat4", "dmat2x2", "dmat2x3",
			"dmat2x4", "dmat3x2", "dmat3x3", "dmat3x4", "dmat4x2", "dmat4x3", "dmat4x4",
			"lowp", "mediump", "highp", "precision", "sampler1D", "sampler1DShadow",
			"sampler1DArray", "sampler1DArrayShadow", "isampler1D", "isampler1DArray",
			"usampler1D", "usampler1DArray", "sampler2D", "sampler2DShadow", "sampler2DArray",
			"sampler2DArrayShadow", "isampler2D", "isampler2DArray", "usampler2D",
			"usampler2DArray", "sampler2DRect", "sampler2DRectShadow", "isampler2DRect",
			"usampler2DRect", "sampler2DMS", "isampler2DMS", "usampler2DMS", "sampler2DMSArray",
			"isampler2DMSArray", "usampler2DMSArray", "sampler3D", "isampler3D", "usampler3D",
			"samplerCube", "samplerCubeShadow", "isamplerCube", "usamplerCube", "samplerCubeArray",
			"samplerCubeArrayShadow", "isamplerCubeArray", "usamplerCubeArray", "samplerBuffer",
			"isamplerBuffer", "usamplerBuffer", "image1D", "iimage1D", "uimage1D", "image1DArray",
			"iimage1DArray", "uimage1DArray", "image2D", "iimage2D", "uimage2D", "image2DArray",
			"iimage2DArray", "uimage2DArray", "image2DRect", "iimage2DRect", "uimage2DRect",
			"image2DMS", "iimage2DMS", "uimage2DMS", "image2DMSArray", "iimage2DMSArray",
			"uimage2DMSArray", "image3D", "iimage3D", "uimage3D", "imageCube", "iimageCube",
			"uimageCube", "imageCubeArray", "iimageCubeArray", "uimageCubeArray", "imageBuffer",
			"iimageBuffer", "uimageBuffer", "struct",

			// Vulkan compatibility keywords
			"set", "texture1D", "texture1DArray", "itexture1D", "itexture1DArray", "utexture1D",
			"utexture1DArray", "texture2D", "texture2DArray", "itexture2D", "itexture2DArray",
			"utexture2D", "utexture2DArray", "texture2DRect", "itexture2DRect", "utexture2DRect",
			"texture2DMS", "itexture2DMS", "utexture2DMS", "texture2DMSArray", "itexture2DMSArray",
			"utexture2DMSArray", "texture3D", "itexture3D", "utexture3D", "textureCube",
			"itextureCube", "utextureCube", "textureCubeArray", "itextureCubeArray",
			"utextureCubeArray", "textureBuffer", "itextureBuffer", "utextureBuffer",
			"sampler", "samplerShadow", "subpassInput", "isubpassInput", "usubpassInput",
			"subpassInputMS", "isubpassInputMS", "usubpassInputMS",

			// Keywords (reserved for future usage by specification)
			"common", "partition", "active", "asm", "class", "union", "enum", "typedef",
			"template", "this", "resource", "goto", "inline", "noinline", "public",
			"static", "extern", "external", "interface", "long", "short", "half", "fixed",
			"unsigned", "superp", "input", "output", "hvec2", "hvec3", "hvec4", "fvec2",
			"fvec3", "fvec4", "filter", "sizeof", "cast", "namespace", "using", "sampler3DRect",

			// Layout Qualifiers (could be handled separately to avoid minifying in other places)
			"shared", "packed", "std140", "std430", "row_major", "column_major",
			"binding", "offset", "align", "set", "push_constant", "input_attachment_index",
			"location", "component", "index", "triangles", "quads", "isolines", "equal_spacing",
			"fractional_even_spacing", "fractional_odd_spacing", "cw", "ccw", "point_mode",
			"points", "lines", "lines_adjacency", "invocations", "origin_upper_left",
			"pixel_center_integer", "early_fragment_tests", "local_size_x", "local_size_y",
			"local_size_z", "local_size_x_id", "local_size_y_id", "local_size_z_id",
			"xfb_buffer", "xfb_stride", "xfb_offset", "vertices", "line_strip", "triangle_strip",
			"max_vertices", "stream", "depth_any", "depth_greater", "depth_less", "depth_unchanged",
			"constant_id",

			// Format Layout Qualifiers (uniform interface only)
			"rgba32f", "rgba16f", "rg32f", "rg16f", "r11f_g11f_b10f", "r32f", "r16f",
			"rgba16", "rgb10_a2", "rgba8", "rg16", "rg8", "r16", "r8", "rgba16_snorm",
			"rgba8_snorm", "rg16_snorm", "rg8_snorm", "r16_snorm", "r8_snorm", "rgba32i",
			"rgba16i", "rgba8i", "rg32i", "rg16i", "rg8i", "r32i", "r16i", "r8i", "rgba32ui",
			"rgba16ui", "rgb10_a2ui", "rgba8ui", "rg32ui", "rg16ui", "rg8ui", "r32ui",
			"r16ui", "r8ui",

			// Angle and Trigonometry Functions
			"radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan",
			"sinh", "cosh", "tanh", "asinh", "acosh", "atanh",

			// Exponential Functions
			"pow", "exp", "log", "exp2", "log2", "sqrt", "inversesqrt",

			// Common Functions
			"abs", "sign", "floor", "trunc", "round", "roundEven", "ceil", "fract",
			"mod", "modf", "min", "max", "clamp", "mix", "step", "smoothstep", "isnan",
			"isinf", "floatBitsToInt", "floatBitsToUint", "intBitsToFloat", "uintBitsToFloat",
			"fma", "frexp", "ldexp",

			// Floating-Point Pack and Unpack Functions
			"packUnorm2x16", "packSnorm2x16", "packUnorm4x8", "packSnorm4x8",
			"unpackUnorm2x16", "unpackSnorm2x16", "unpackUnorm4x8", "unpackSnorm4x8",
			"packHalf2x16", "unpackHalf2x16", "packDouble2x32", "unpackDouble2x32",

			// Geometric Functions
			"length", "distance", "dot", "cross", "normalize", "ftransform", "faceforward",
			"reflect", "refract",

			// Matrix Functions
			"matrixCompMult", "outerProduct", "transpose", "determinant", "inverse",

			// Vector Relational Functions
			"lessThan", "lessThanEqual", "greaterThan", "greaterThanEqual", "equal",
			"notEqual", "any", "all", "not",

			// Integer Functions
			"uaddCarry", "usubBorrow", "umulExtended", "imulExtended", "bitfieldExtract",
			"bitfieldInsert", "bitfieldReverse", "bitCount", "findLSB", "findMSB",

			// Texture Query Functions
			"textureSize", "textureQueryLod", "textureQueryLevels", "textureSamples",

			// Texel Lookup Functions
			"texture", "textureProj", "textureLod",
			"textureOffset", "texelFetch", "texelFetchOffset", "textureProjOffset",
			"textureLodOffset", "textureProjLod", "textureProjLodOffset",
			"textureGrad", "textureGradOffset", "textureProjGrad", "textureProjGradOffset",

			// Texture Gather Functions
			"textureGather", "textureGatherOffset", "textureGatherOffsets",

			// Compatibility Profile Texture Functions
			"texture1D", "texture1DProj", "texture1DLod", "texture1DProjLod",
			"texture2D", "texture2DProj", "texture2DLod", "texture2DProjLod",
			"texture3D", "texture3DProj", "texture3DLod", "texture3DProjLod",
			"textureCube", "textureCubeLod",
			"shadow1D", "shadow2D", "shadow1DProj", "shadow2DProj",
			"shadow1DLod", "shadow2DLod", "shadow1DProjLod", "shadow2DProjLod",

			// Atomic Counter Functions
			"atomicCounterIncrement", "atomicCounterDecrement", "atomicCounter",
			"atomicCounterAdd", "atomicCounterSubtract", "atomicCounterMin",
			"atomicCounterMax", "atomicCounterAnd","atomicCounterOr", "atomicCounterXor",
			"atomicCounterExchange", "atomicCounterCompSwap",

			// Atomic Functions
			"atomicAdd", "atomicMin", "atomicMax", "atomicAnd", "atomicOr", "atomicXor",
			"atomicExchange", "atomicCompSwap",

			// Image Functions
			"imageSize", "imageSamples", "imageLoad", "imageStore", "imageAtomicAdd",
			"imageAtomicMin", "imageAtomicMax", "imageAtomicAnd", "imageAtomicOr",
			"imageAtomicXor", "imageAtomicExchange", "imageAtomicCompSwap",

			// Geometry Shader Functions
			"EmitStreamVertex", "EndStreamPrimitive", "EmitVertex", "EndPrimitive",

			// Fragment Processing Functions
			"dFdx", "dFdy", "dFdxFine", "dFdyFine", "dFdxCoarse", "dFdyCoarse",
			"fwidth", "fwidthFine", "fwidthCoarse",

			// Interpolation Functions
			"interpolateAtCentroid", "interpolateAtSample", "interpolateAtOffset",

			// Noise Functions
			"noise1", "noise2", "noise3", "noise4",

			// Shader Invocation Control Functions
			"barrier",

			// Shader Memory Control Functions
			"memoryBarrier", "memoryBarrierAtomicCounter", "memoryBarrierBuffer",
			"memoryBarrierShared", "memoryBarrierImage", "groupMemoryBarrier",

			// Subpass-Input Functions
			"subpassLoad",

			// Shader Invocation Group Functions
			"anyInvocation", "allInvocations", "allInvocationsEqual",

			// Optional: Extension Support

			// GL_NV_geometry_shader_passthrough
			"passthrough",

			// GL_NV_shader_atomic_fp16_vector
			"f16vec4",
		};

		static bool IsValidLiteralCharacter(const char character, const char previous, const char next)
		{
			if (isalnum(character) || character == '.')
				return true;

			if ((character == '-' || character == '+') && isalpha(previous) && isdigit(next))
				return true;

			return false;
		}

		static bool IsComponentAccess(const std::string& identifier)
		{
			// A keyword is for component access if it is a combination of xyzw, rgba or stpq up to 4 letters
			if (identifier.empty() || identifier.length() > 4)
				return false;

			static const std::unordered_set<char> xyzw = { 'x', 'y', 'z', 'w' };
			static const std::unordered_set<char> rgba = { 'r', 'g', 'b', 'a' };
			static const std::unordered_set<char> stpq = { 's', 't', 'p', 'q' };

			bool allXyzw = true, allRgba = true, allStpq = true;

			for (const char c : identifier)
			{
				if (xyzw.find(c) == xyzw.end()) allXyzw = false;
				if (rgba.find(c) == rgba.end()) allRgba = false;
				if (stpq.find(c) == stpq.end()) allStpq = false;
			}

			return (allXyzw || allRgba || allStpq);
		}

		static bool IsKeyword(const std::string& identifier)
		{
			// Check if identifier is the shader entry point
			if (identifier == s_EntryPointKeyword)
				return true;

			// Verify that the identifier is not a reserved word beginning with gl_ or __
			if (identifier.rfind("gl_", 0) == 0 || identifier.rfind("__", 0) == 0)
				return true;

			// Check if the identifier is some form of component
			if (IsComponentAccess(identifier))
				return true;

			// Otherwise check if the identifier is an explicit built in type/method
			return s_Keywords.find(identifier) != s_Keywords.end();
		}

		static std::string GenerateShortenedIdentifierName(int index)
		{
			const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
			const int charCount = characters.size();
			const int alphaCount = 2 * 26;
			std::string result;

			// Ensure that the first character is an alphabetical one
			result += characters[index % alphaCount];
			index /= alphaCount;

			// Handle all subsequent alphanumeric characters
			while (index > 0)
			{
				result += characters[index % charCount];
				index /= charCount;
			}

			return result;
		}

	}

	void ShaderCompressor::Minify(const std::unordered_map<uint32_t, std::string>& sources, std::unordered_map<uint32_t, std::string>& minifiedSources)
	{
		minifiedSources.clear();

		// Identify keyword character usage across ALL stages (we do this to avoid issues with incompatible buffer naming across stages where glsl yells at us)
		std::unordered_map<std::string, size_t> identifierUsage;

		// For each shader stage we gather identifiers used
		for (const auto& [stage, source] : sources)
		{
			bool inSingleLineComment = false;
			bool inMultiLineComment = false;
			bool inString = false;
			bool inPreprocessorDirective = false;
			bool inIdentifier = false;
			bool inLiteral = false;
			std::string currentIdentifier;

			char previous = 0;
			for (size_t i = 0; i < source.length(); ++i)
			{
				const char c = source[i];
				const char next = (i + 1 < source.length()) ? source[i + 1] : '\0';

				if (inSingleLineComment) {
					if (c == '\n')
						inSingleLineComment = false;
					continue;
				}

				if (inMultiLineComment) {
					if (c == '*' && next == '/')
					{
						inMultiLineComment = false;
						i++; // Skip the '/'
					}
					continue;
				}

				if (c == '#')
					inPreprocessorDirective = true;

				if (inPreprocessorDirective) {
					if (c == '\n')
						inPreprocessorDirective = false;

					continue;
				}

				if (inString) {
					if (c == '"' && previous != '\\')
						inString = false;
					continue;
				}

				if (c == '/' && next == '/')
				{
					inSingleLineComment = true;
					i++; // Skip the second '/'
					continue;
				}

				if (c == '/' && next == '*')
				{
					inMultiLineComment = true;
					i++; // Skip the '*'
					continue;
				}

				if (c == '"')
					inString = true;

				// Add characters, skipping unnecessary whitespace
				if (!isspace(c) || (isspace(c) &&
					(previous && ((isalnum(previous) || previous == '_') && (isalnum(next) || next == '_')))))
				{
					if (!inIdentifier && !inLiteral)
					{
						// Check if we have reached the beginning of a new identifier
						if ((isalpha(c) || c == '_'))
						{
							inIdentifier = true;
							currentIdentifier.clear();
						}

						// Check if we have reached the beginning of a new constant
						if (isdigit(c))
						{
							inLiteral = true;
							currentIdentifier.clear();
						}
					}

					if (inIdentifier)
					{
						currentIdentifier += c;

						// Check if this is the end of the identifier
						if (!isalnum(next) && next != '_')
						{
							identifierUsage[currentIdentifier] += currentIdentifier.length();
							inIdentifier = false;
							continue;
						}

						continue;
					}

					if (inLiteral)
					{
						// Check if we should stop being in a literal
						if (!Utils::IsValidLiteralCharacter(next, c, source[i + 2]))
							inLiteral = false;
					}
				}

				previous = c;
			}
		}

		// Extract all existing macro defines
		std::unordered_set<std::string> existingDefines;
		for (const auto& [stage, source] : sources)
		{
			size_t defineSearchPos = 0;
			const std::string defineString = "#define ";
			while (source.find(defineString, defineSearchPos) != std::string::npos)
			{
				size_t start = source.find(defineString, defineSearchPos);
				start += defineString.length();
				start = source.find_first_not_of(" \t", start);

				const size_t end = source.find_first_of(" \r\n\t(", start);
				existingDefines.insert(source.substr(start, end - start));
				defineSearchPos = end + 1;
			}
		}

		// Sort identifiers into descending order of usage
		std::vector<std::pair<std::string, size_t>> sortedIdentifiers(identifierUsage.begin(), identifierUsage.end());

		std::sort(sortedIdentifiers.begin(), sortedIdentifiers.end(), [](const auto& a, const auto& b)
		{
			return a.second > b.second;
		});

		// Assign identifiers minified names (only if they will actually reduce character count)
		int nameIndex = 0;
		std::unordered_set<std::string> usedMinifiedNames;
		std::unordered_map<std::string, std::string> identifierNames;
		for (const auto& [identifier, usage] : sortedIdentifiers)
		{
			// Generate a minified name (and ensure it is not used as a keyword or an existing define)
			std::string minifiedName;
			do
			{
				minifiedName = Utils::GenerateShortenedIdentifierName(nameIndex);
				nameIndex++;
			} while (Utils::IsKeyword(minifiedName) || existingDefines.find(minifiedName) != existingDefines.end());

			// Check if name will benefit from minification (however if we have already used the original we must enforce minification)
			if (minifiedName.length() > identifier.length() && usedMinifiedNames.find(identifier) == usedMinifiedNames.end())
				continue;

			// If identifier is a macro, do not modify it
			if (existingDefines.find(identifier) != existingDefines.end())
				continue;

			// If the identifier is a keyword check if it will benefit from being added
			if (Utils::IsKeyword(identifier))
			{
				// Note: Each shader stage will require its own #define (with the current setup) so we need to weigh up the total space saved
				const int defineLength = minifiedName.length() + identifier.length() + 9;
				const int identifierInstanceCount = usage / identifier.length();
				const int newUsage = identifierInstanceCount * minifiedName.length() + (defineLength * sources.size());

				if (newUsage > usage)
					continue;
			}

			// Otherwise, use the minified name
			identifierNames[identifier] = minifiedName;
			usedMinifiedNames.insert(minifiedName);
		}

		// Generate #define preprocessor macros for keywords
		std::string keywordDefineString;
		for (const auto& [identifer, name] : identifierNames)
			if (Utils::IsKeyword(identifer))
				keywordDefineString += fmt::format("#define {} {}", name, identifer) + "\n";

		// Begin Minification
		for (const auto& [stage, source] : sources)
		{
			std::string& result = minifiedSources[stage];

			bool inSingleLineComment = false;
			bool inMultiLineComment = false;
			bool inString = false;
			bool inPreprocessorDirective = false;
			bool inIdentifier = false;
			bool inLiteral = false;
			std::string currentIdentifier;
			std::string currentPreprocessor;
			int preprocessorDefineStage = 0;

			for (size_t i = 0; i < source.length(); ++i)
			{
				char c = source[i];
				char next = (i + 1 < source.length()) ? source[i + 1] : '\0';

				if (c == '\r')
				{
					if (next == '\n')
						continue;

					c = '\n';
				}

				if (inSingleLineComment) {
					if (c == '\n')
						inSingleLineComment = false;
					continue;
				}

				if (inMultiLineComment) {
					if (c == '*' && next == '/')
					{
						inMultiLineComment = false;
						i++; // Skip the '/'
					}
					continue;
				}

				if (c == '#')
				{
					if (!result.empty() && result.back() != '\n')
						result += '\n';
					inPreprocessorDirective = true;

					currentPreprocessor.clear();
					preprocessorDefineStage = 0;
				}

				// Note: The previous character loop control section does not check defines like the following.
				// This means that identifiers in macros will not contribute to overall weighting of variable assignment (but are still correctly replaced here)
				// This is not an issue as there are tons of optimal identifiers and macros are only likely to have a couple instances of a given one
				if (inPreprocessorDirective) {
					result += c;
					currentPreprocessor += c;

					// Wait for define keyword
					if (currentPreprocessor == "#define" && preprocessorDefineStage == 0)
					{
						preprocessorDefineStage = 1;
						continue;
					}

					// Handle arbitrary number of spaces/tabs after define
					if (preprocessorDefineStage == 1 && (c != '\t' || c != ' '))
					{
						preprocessorDefineStage = 2;
						continue;
					}

					// Handle end of any preprocessor directive (new line) as well as define/macro name endings
					if (c == '\n' || (preprocessorDefineStage == 2 && (c == '\t' || c == ' ')))
					{
						// After the name has ended, we can end the #define directive so inside functions/symbols can also be minified
						preprocessorDefineStage = 3;
						inPreprocessorDirective = false;
					}

					continue;
				}

				// Ensure we insert a new line at the end of a #define line (as the expression for these are treated line normal minified code and as such will not auto insert)
				if (preprocessorDefineStage == 3 && c == '\n')
				{
					preprocessorDefineStage = 4;
					result += c;
					continue;
				}

				if (inString) {
					result += c;
					if (c == '"' && source[i - 1] != '\\')
						inString = false;
					continue;
				}

				if (c == '/' && next == '/')
				{
					inSingleLineComment = true;
					i++; // Skip the second '/'
					continue;
				}

				if (c == '/' && next == '*')
				{
					inMultiLineComment = true;
					i++; // Skip the '*'
					continue;
				}

				if (c == '"')
					inString = true;

				// Add characters, skipping unnecessary whitespace
				if (!isspace(c) || (isspace(c) &&
					(!result.empty() && ((isalnum(result.back()) || result.back() == '_') && (isalnum(next) || next == '_')))))
				{
					if (!inIdentifier && !inLiteral)
					{
						// Check if we have reached the beginning of a new identifier
						if ((isalpha(c) || c == '_'))
						{
							inIdentifier = true;
							currentIdentifier.clear();
						}

						// Check if we have reached the beginning of a new constant
						if (isdigit(c))
						{
							inLiteral = true;
							currentIdentifier.clear();
						}
					}

					if (inIdentifier)
					{
						currentIdentifier += c;

						// Check if this is the end of the identifier
						if (!isalnum(next) && next != '_')
						{
							result += identifierNames.find(currentIdentifier) != identifierNames.end() ? identifierNames.at(currentIdentifier) : currentIdentifier;
							inIdentifier = false;
							continue;
						}

						continue;
					}

					if (inLiteral)
					{
						// Check if we should stop being in a literal
						if (!Utils::IsValidLiteralCharacter(next, c, source[i + 2]))
							inLiteral = false;
					}

					// All other constants, operands etc will get added unaltered
					result += c;
				}
			}
		}

		// Insert hash defines AFTER version declaration (if it exists)
		const std::string versionString = "#version";
		for (auto& [stage, minified] : minifiedSources)
		{
			const size_t versionPos = minified.find(versionString);
			const size_t insertionPos = versionPos == std::string::npos ? 0 : (minified.find_first_of("\r\n", versionPos) + 1);
			minified.insert(insertionPos, keywordDefineString);
		}
	}

	// Huffman Encoding System

	struct HuffmanNode
	{
		char Character;
		size_t Frequency;

		HuffmanNode* Left;
		HuffmanNode* Right;

		HuffmanNode(char character, size_t frequency, HuffmanNode* left = nullptr, HuffmanNode* right = nullptr)
			: Character(character), Frequency(frequency), Left(left), Right(right)
		{}
	};

	struct HuffmanCompare
	{
		bool operator()(HuffmanNode* left, HuffmanNode* right)
		{
			return left->Frequency > right->Frequency;
		}
	};

	namespace Utils {
	
		static void GenerateHuffmanCodes(HuffmanNode* root, std::vector<bool>& code, std::unordered_map<char, std::vector<bool>>& huffmanCodes)
		{
			if (!root)
				return;

			if (!root->Left && !root->Right)
				huffmanCodes[root->Character] = code;

			if (root->Left)
			{
				// Note: 0/false for left
				code.push_back(false);
				GenerateHuffmanCodes(root->Left, code, huffmanCodes);
				code.pop_back();
			}

			if (root->Right)
			{
				// Note: 1/true for left
				code.push_back(true);
				GenerateHuffmanCodes(root->Right, code, huffmanCodes);
				code.pop_back();
			}
		}

		static void SerializeHuffmanTree(HuffmanNode* root, BitBufferWriter& bitBufferWriter)
		{
			if (!root)
				return;

			if (!root->Left && !root->Right)
			{
				// Insert leaf node marker (1 bit) and character data if found
				bitBufferWriter.AddBit(1);
				bitBufferWriter.AddByte(root->Character);
				return;
			}

			// Otherwise recurse left/right with a internal node marker (0 bit)
			bitBufferWriter.AddBit(0);
			SerializeHuffmanTree(root->Left, bitBufferWriter);
			SerializeHuffmanTree(root->Right, bitBufferWriter);
		}

		static HuffmanNode* DeserializeHuffmanTree(BitBufferReader& bitBufferReader)
		{
			if (!bitBufferReader.HasBits())
				return nullptr;

			bool isLeaf = bitBufferReader.GetNextBit();

			if (isLeaf)
			{
				char character = (char)bitBufferReader.GetNextByte();
				return new HuffmanNode(character, 0);
			}
			
			HuffmanNode* left = DeserializeHuffmanTree(bitBufferReader);
			HuffmanNode* right = DeserializeHuffmanTree(bitBufferReader);
			return new HuffmanNode('\0', 0, left, right);
		}

		static void DestroyHuffmanTree(HuffmanNode* root)
		{
			if (!root)
				return;

			DestroyHuffmanTree(root->Left);
			DestroyHuffmanTree(root->Right);

			delete root;
		}

	}

	Ref<ScopedBuffer> ShaderCompressor::HuffmanEncode(const std::string& source)
	{
		// Count character frequencies
		std::unordered_map<char, size_t> frequencyMap;
		for (const char character : source)
			frequencyMap[character]++;

		// Account for end marker character
		frequencyMap['\0'] = 1;

		// Create a priority queue (min-heap) to build the Huffman tree
		std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, HuffmanCompare> minHeap;

		for (const auto& [character, frequency] : frequencyMap)
			minHeap.push(new HuffmanNode(character, frequency));

		// Build the Huffman tree
		while (minHeap.size() > 1)
		{
			HuffmanNode* left = minHeap.top();
			minHeap.pop();

			HuffmanNode* right = minHeap.top();
			minHeap.pop();

			// Merge the two nodes
			const size_t frequencySum = left->Frequency + right->Frequency;
			HuffmanNode* newNode = new HuffmanNode('\0', frequencySum);
			newNode->Left = left;
			newNode->Right = right;
			minHeap.push(newNode);
		}

		HuffmanNode* root = minHeap.top();

		// Generate codes for all characters by traversing the tree
		std::unordered_map<char, std::vector<bool>> huffmanCodes;
		std::vector<bool> code;
		Utils::GenerateHuffmanCodes(root, code, huffmanCodes);

		// Pack the Huffman tree and codes for each character into a bit buffer
		BitBufferWriter bitBufferWriter;

		Utils::SerializeHuffmanTree(root, bitBufferWriter);

		for (const char character : source)
			bitBufferWriter.AddBits(huffmanCodes.at(character));

		// Write end marker
		bitBufferWriter.AddBits(huffmanCodes.at('\0'));

		// Create scoped buffer and copy
		const std::vector<uint8_t>& finalBitBuffer = bitBufferWriter.GetFinalBuffer();
		Ref<ScopedBuffer> buffer = ScopedBuffer::Create(finalBitBuffer.size());
		buffer->Copy(finalBitBuffer.data(), finalBitBuffer.size());

		// Destroy the tree
		Utils::DestroyHuffmanTree(root);

		// Return generated buffer
		return buffer;
	}

	void ShaderCompressor::HuffmanDecode(Ref<Buffer> data, std::string& target)
	{
		// Reconstruct the Huffman tree
		BitBufferReader bitBufferReader(data->Data, data->Size);
		HuffmanNode* root = Utils::DeserializeHuffmanTree(bitBufferReader);

		// Decode data by traversing the tree
		HuffmanNode* currentNode = root;
		while (bitBufferReader.HasBits())
		{
			// Move down the tree in the direction indicated by the next bit
			bool bit = bitBufferReader.GetNextBit();
			currentNode = bit ? currentNode->Right : currentNode->Left;

			// If we reach a leaf node append the character to the target string and restart from the root for the next character
			if (!currentNode->Left && !currentNode->Right)
			{
				char character = currentNode->Character;
				
				// Check we don't have and end of stream marker (this is to avoid issues with extra 0 bits of padding when reading final byte being potentially interpreted as another character entry)
				if (character == '\0')
					break;

				target += character;
				currentNode = root;
			}
		}

		// Destroy the tree
		Utils::DestroyHuffmanTree(root);
	}

}