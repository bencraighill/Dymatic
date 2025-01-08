#pragma once

namespace Dymatic::Editor {

	enum class PinKind
	{
		Output,
		Input
	};

	enum class NodeType
	{
		Blueprint = 0,
		Simple,
		Comment,
		Tree,
		Point,
	};

}