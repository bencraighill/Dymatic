#include "Dymatic/Core/Base.h"
#include "WinToastHandler.h"
#include "WinToast/wintoastlib.h"

#include <string>
#include <codecvt>
#include <locale>

namespace WinToastLib
{

	using namespace WinToastLib;

	class CustomHandler : public IWinToastHandler {
	public:

		CustomHandler(std::string ID)
		{
			m_ToastID = ID;
		}

		INT64 GetUniqueID()
		{
			return m_UniqueID;
		}

		void SetUniqueID(INT64 id)
		{
			m_UniqueID = id;
		}

		bool ReturnToastClickedValue()
		{
			return m_ButtonClicked;
		}

		int ReturnToastButtonPressedValue()
		{
			return m_ButtonPressed;
		}

		void toastActivated() {
			m_ButtonClicked = true;
		}

		void toastActivated(int actionIndex) {
			m_ButtonPressed = actionIndex;
		}

		void toastFailed() {
		}
		void toastDismissed(WinToastDismissalReason state) {
			switch (state) {
			case UserCanceled:
				break;
			case ApplicationHidden:
				break;
			case TimedOut:
				break;
			default:
				break;
			}
		}
	private:
		std::string m_ToastID;
		int m_ButtonPressed = -1;
		bool m_ButtonClicked = false;
		INT64 m_UniqueID = -1;
	};

	std::vector<CustomHandler*> handlers;
	std::vector<INT64> m_ToastList;

	Toast::Toast()
	{
		WinToast::instance()->setAppName(L"Dymatic Editor");
		WinToast::instance()->setAppUserModelId(WinToast::configureAUMI(L"BAS Solutions", L"Dymatic Engine", L"Dymatic Editor", L"V1.2.1"));
		if (!WinToast::instance()->initialize()) {
			DY_CORE_ASSERT(false, "System is not compatible");
		}
		//WinToastTemplate::AudioOption::Default;
		m_AudioOption = WinToastTemplate::Alarm9;
	}

	INT64 Toast::ShowToast(std::string id, std::string title, std::string message, std::vector<std::string> buttons, int displayTime, std::string imagePath)
	{
		using convert_t = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_t, wchar_t> strconverter;

		WinToastTemplate templ = WinToastTemplate(WinToastTemplate::ImageAndText04);
		templ.setAttributionText(strconverter.from_bytes(message));
		templ.setTextField(L"Dymatic Editor", WinToastTemplate::FirstLine);
		templ.setTextField(std::wstring(strconverter.from_bytes(title)), WinToastTemplate::SecondLine);
		templ.setDuration(WinToastTemplate::Duration::System);
		templ.setExpiration(displayTime);
		templ.setImagePath(std::wstring(imagePath == "" ? L"C:/dev/Dymatic/DymaticEditor/assets/icons/DymaticLogoTransparent.png" : strconverter.from_bytes(imagePath)));
		templ.setAudioPath(static_cast<WinToastTemplate::AudioSystemFile>(m_AudioOption));
		templ.setAudioOption(static_cast<WinToastTemplate::AudioOption>(m_AudioOption));
		for (std::string button : buttons)
		{
			if (true) templ.addAction(strconverter.from_bytes(button));
		}

		auto newHandler = WinToast::instance();
		CustomHandler* a = (new CustomHandler(id));
		handlers.push_back(a);
		auto ids = newHandler->showToast(templ, a);
		if ( ids < 0) {
			DY_CORE_ASSERT(false, "Could not launch toast notification!");
		}
		m_ToastList.push_back(ids);
		a->SetUniqueID(ids);

		return ids;

		//Sleep(1000);
		//WinToast::instance()->hideToast(m_ToastList[0]);
	}

	void Toast::HideToast(INT64 id)
	{
		WinToast::instance()->hideToast(id);
	}

	bool Toast::GetToastClicked(INT64 id)
	{
		for (CustomHandler* handler : handlers)
		{
			CustomHandler a = *handler;
			if (a.GetUniqueID() == id)
			{
				return a.ReturnToastClickedValue();
			}
		}
		return false;
	}


	int Toast::GetToastButtonPressed(INT64 id)
	{
		for (CustomHandler* handler : handlers)
		{
			CustomHandler a = *handler;
			if (a.GetUniqueID() == id)
			{
				return a.ReturnToastButtonPressedValue();
			}
		}
		return -1;
	}

}
