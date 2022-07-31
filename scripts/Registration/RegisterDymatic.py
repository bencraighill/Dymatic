import os, winshell, sys, subprocess, platform, ctypes
import win32com.shell.shell as shell
from win32com.client import Dispatch
import win32event

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

if is_admin():
    # Install Neccisary Submodules
    subprocess.call([os.path.abspath("InstallPythonSubmodules.bat"), "nopause"])

    # Get Required Paths
    print("Getting Required Paths...")
    cwd = os.getcwd()
    desktop = winshell.desktop()
    startmenu = winshell.start_menu(False)
    appdata = winshell.application_data(False)

    # Setup Desktop Shortcut
    print("Setting up Desktop Shortcut...")
    path = os.path.join(desktop, "Dymatic Engine.lnk")
    target = cwd + r"\..\..\bin\Debug-windows-x86_64\DymaticEditor\DymaticEditor.exe"
    wDir = cwd + r"\..\..\DymaticEditor"
    icon = cwd + r"\..\..\bin\Debug-windows-x86_64\DymaticEditor\DymaticEditor.exe"

    shell = Dispatch('WScript.Shell')
    shortcut = shell.CreateShortCut(path)
    shortcut.TargetPath = target
    shortcut.WorkingDirectory = wDir
    shortcut.IconLocation = icon
    shortcut.save()

    # Setup Start Menu Shortcut
    print("Setting up Start Menu Shortcut...")
    path = os.path.join(startmenu, "Programs", "Dymatic Engine.lnk")
    shortcut = shell.CreateShortCut(path)
    shortcut.TargetPath = target
    shortcut.WorkingDirectory = wDir
    shortcut.IconLocation = icon
    shortcut.save()

    # Write To Registry
    print("Writing to Registry...")
    import winreg
    path = winreg.HKEY_CLASSES_ROOT

    dymatic_key = winreg.CreateKey(path, "Dymatic")
    winreg.SetValueEx(dymatic_key, "", 0, winreg.REG_SZ, "Dymatic Scene")

    dymatic_filetype_key = winreg.CreateKey(path, ".dymatic")
    winreg.SetValueEx(dymatic_filetype_key, "", 0, winreg.REG_SZ, "Dymatic")
    winreg.SetValueEx(dymatic_filetype_key, "Content Type", 0, winreg.REG_SZ, "DymaticScene")
    winreg.SetValueEx(dymatic_filetype_key, "PerceivedType", 0, winreg.REG_SZ, "Dymatic Scene")

    default_icon_key = winreg.CreateKey(dymatic_filetype_key, "DefaultIcon")
    winreg.SetValueEx(default_icon_key, "", 0, winreg.REG_SZ, cwd + "\..\..\Resources\Branding\Logo\DymaticLogo.ico")

    shell_new_key = winreg.CreateKey(dymatic_filetype_key, "ShellNew")
    winreg.SetValueEx(shell_new_key, "FileName", 0, winreg.REG_SZ, cwd + "\..\..\DymaticEditor\saved\presets\scenes\EmptySceneTemplate.dymatic")

    print("Dymatic registration with Windows complete!")
else:
    # Re-run the program with admin rights
    ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, " ".join(sys.argv), None, 1)