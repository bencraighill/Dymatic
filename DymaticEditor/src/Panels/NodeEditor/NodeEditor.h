#pragma once
#include <string>

#include "Dymatic/Events/KeyEvent.h"

namespace Dymatic {

	class NodeEditorPannel
	{
	public:
		NodeEditorPannel();

		bool& GetNodeEditorVisible();
	
		void OnEvent(Event& e);

		void Application_Initialize();
		void Application_Finalize();
	
		void CompileNodes();
	
		void CopyNodes();
		void PasteNodes();
		void DuplicateNodes();
		void DeleteNodes();
	
		void OnImGuiRender();
	private:
		

		int stackIndex;
	};

}
