#pragma once

namespace Dymatic {

	enum FileType
	{
		FileTypeDirectory = 0,
		FileTypeFile,
		FileTypeScene,
		FileTypePrefab,
		FileTypeScript,
		FileTypeMeshSource,
		FileTypeMesh,
		FileTypeMaterial,
		FileTypeMaterialInstance,
		FileTypeTexture,
		FileTypeEnvironmentMap,
		FileTypeVirtualTexture,
		FileTypeFont,
		FileTypeAudio,
		FileTypeParticleSystem,
		FileTypeSkeleton,
		FileTypeAnimation,
		FileTypeAnimationGraph,
		FileTypePhysicsMaterial,
		FileTypeVideo,
		FileTypeVideoPlayer,
		FileTypeSubtitle,
		FileTypeZipArchive,
		FileTypeSolution,

		FILE_TYPE_SIZE
	};

}