#pragma once
#include <string>

#include "Dymatic/Events/KeyEvent.h"

namespace Dymatic {

	class ScriptEditorInternal;

	class ScriptEditorPanel
	{
	public:
		ScriptEditorPanel();
		~ScriptEditorPanel();
	
		void OnEvent(Event& e);
	
		void CompileNodes();
	
		void CopyNodes();
		void PasteNodes();
		void DuplicateNodes();
		void DeleteNodes();
	
		void OnImGuiRender();
	private:
		ScriptEditorInternal* m_InternalEditor;
	};

}
