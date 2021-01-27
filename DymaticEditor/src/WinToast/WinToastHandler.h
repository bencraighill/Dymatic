#ifndef MAINWINDOW_H
#define MAINWINDOW_H

namespace WinToastLib
{
	class Toast
	{
	public:
		Toast();
		INT64 ShowToast(std::string id, std::string title, std::string message, std::vector<std::string> buttons, int displayTime = 0, std::string imagePath = "");
		void HideToast(INT64 id);
		bool GetToastClicked(INT64 id);
		int GetToastButtonPressed(INT64 id);
	private:
		int m_AudioOption;
	};
}

#endif // MAINWINDOW_H
