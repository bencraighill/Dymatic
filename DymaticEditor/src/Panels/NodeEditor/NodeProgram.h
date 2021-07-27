#pragma once
#include <string>

#include "Dymatic/Events/KeyEvent.h"

namespace Dymatic {

	struct NodeCompilerLoopBackOutput
	{
		std::string Declarations = "";
		std::string Definitions = "";
	};

	class NodeEditorPannel
	{
	public:
		NodeEditorPannel();
	
		void OnEvent(Event& e);

		void Application_Initialize();
		void Application_Finalize();
	
		void CompileNodes();
	
		void CopyNodes();
		void PasteNodes();
		void DuplicateNodes();
		void DeleteNodes();
	
		void Application_Frame();
	private:
		

		int stackIndex;
	};

}
