#include "dypch.h"
#include "Dymatic/Renderer/GraphicsVendor.h"

namespace Dymatic {
	
	GraphicsVendor::GraphicsVendorType GraphicsVendor::GraphicsVendorFromString(const std::string& vendor)
	{

		if (vendor == "NVIDIA" || vendor == "NVIDIA Corporation")
			return GraphicsVendor::NVIDIA;
		else if (vendor == "AMD" || vendor == "ATI Technologies Inc." || vendor == "ATI")
			return GraphicsVendor::AMD;
		else if (vendor == "Intel")
			return GraphicsVendor::Intel;
		else if (vendor == "Microsoft" || vendor == "Microsoft Corporation")
			return GraphicsVendor::Microsoft;
		else if (vendor == "Mesa")
			return GraphicsVendor::Mesa;
		else if (vendor == "Apple" || vendor == "Apple Inc.")
			return GraphicsVendor::Apple;
		else if (vendor == "Imagination" || vendor == "Imagination Technologies")
			return GraphicsVendor::Imagination;
		else if (vendor == "Qualcomm")
			return GraphicsVendor::Qualcomm;
		else if (vendor == "ARM")
			return GraphicsVendor::ARM;
		
		DY_CORE_WARN("Unknown graphics vendor: {}", vendor);

		return GraphicsVendor::Unknown;
	}

	const char* GraphicsVendor::GraphicsVendorToString(const GraphicsVendorType vendor)
	{
		switch (vendor)
		{
		case GraphicsVendor::NVIDIA:		return "NVIDIA";
		case GraphicsVendor::AMD:			return "AMD";
		case GraphicsVendor::Intel:			return "Intel";
		case GraphicsVendor::Microsoft:		return "Microsoft";
		case GraphicsVendor::Mesa:			return "Mesa";
		case GraphicsVendor::Apple:			return "Apple";
		case GraphicsVendor::Imagination:	return "Imagination";
		case GraphicsVendor::Qualcomm:		return "Qualcomm";
		case GraphicsVendor::ARM:			return "ARM";
		}

		return "Unknown";
	}

}