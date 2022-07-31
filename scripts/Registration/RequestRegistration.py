import subprocess, os

permissionGranted = False
while not permissionGranted:
    print("Registering Dymatic with Windows will add it to the start menu, desktop, and setup Windows to recognise Dymatic files.")
    reply = str(input("Would you like to register Dymatic with Windows? [Y/N]: ")).lower().strip()[:1]
    if reply == 'n':
        exit()
    permissionGranted = (reply == 'y')
subprocess.call([os.path.abspath("RegisterDymatic.bat"), "nopause"])