#pragma once

#include "Dymatic/Events/KeyEvent.h"

namespace Dymatic {

	class MaterialEditorInternal;

	class MaterialEditor
	{
	public:
		MaterialEditor();
		~MaterialEditor();
		
		void OnImGuiRender();

	private:
		MaterialEditorInternal* m_InternalEditor;
	};
	
}