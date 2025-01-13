#pragma once

#include <string>

namespace Dymatic {

	class GraphicsVendor
	{
	public:
		enum GraphicsVendorType
		{
			Unknown = 0,

			NVIDIA,
			AMD,
			Intel,
			Microsoft,
			Mesa,
			Apple,
			Imagination,
			Qualcomm,
			ARM
		};
		
	public:
		static GraphicsVendorType GraphicsVendorFromString(const std::string& vendor);
		static const char* GraphicsVendorToString(const GraphicsVendorType vendor);
	};

}