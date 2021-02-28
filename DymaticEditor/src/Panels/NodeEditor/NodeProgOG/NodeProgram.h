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

		bool OnKeyPressed(KeyPressedEvent& e);

		void Application_Initialize();
		void Application_Finalize();

		void CompileNodes();

		void CopyNodes();
		void PasteNodes();
		void DuplicateNodes();
		void DeleteNodes();

		void Application_Frame();
	private:
		std::string CallNode(int* inNode, bool declaration = true);
		std::string RetrievePin(int* inPin);
		
		NodeCompilerLoopBackOutput CreateLoopBackVariables(int* inNode);

		bool ResetSearchArea = false;
	};

}
