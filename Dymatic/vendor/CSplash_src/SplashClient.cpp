#include "dypch.h"

//  ===========================================================================
//  File    SplashClient.cpp
//  Desc    Test stub for the CSplash class
//  ===========================================================================
#include "stdafx.h"
#include "splash.h"


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    //  =======================================================================
    //  Display the splash screen using the overloaded construcutor
    //  =======================================================================
    //  Launch splash screen
    CSplash splash1(TEXT(".\\Splash.bmp"), RGB(128, 128, 128));
    splash1.ShowSplash();

    // your start up code here
    Sleep(3000); //  simulate using a 5 second delay

    //  Close the splash screen
    splash1.CloseSplash();

    //  =======================================================================
    //  Display the splash screen using the various CSplash methods
    //  =======================================================================
    CSplash splash2;
    splash2.SetBitmap(TEXT(".\\about.bmp"));
    splash2.SetTransparentColor(RGB(128, 128, 128));
    splash2.ShowSplash();

    Sleep(5000);

    splash2.CloseSplash();

	return 0;
}
