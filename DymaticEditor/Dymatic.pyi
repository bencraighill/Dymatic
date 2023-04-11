# Dymatic module

def GetVersionString() -> str:
    """
    Returns the current Dymatic version as a string in the format '{MAJOR}.{MINOR}.{PATCH}'.
    """

def GetVersionMajor() -> int:
    """
    Returns the current Dymatic major version as an integer.
    """
   
def GetVersionMinor() -> int:
    """
    Returns the current Dymatic minor version as an integer.
    """
    
def GetVersionPatch() -> int:
    """
    Returns the current Dymatic patch version as an integer.
    """
    
class Editor:
    def SaveScene() -> None:
        """
        Saves the current scene.
        """

class Platform:
    def OpenFileDialogue(filter: str) -> str:
        """
        Opens a open file dialogue and returns the selected file path.
        """
    def SaveFileDialogue(filter: str) -> str:
        """
        Opens a save file dialogue and returns the selected file path.
        """
        
    def SelectFolderDialogue() -> str:
        """
        Opens a select folder dialogue and returns the selected folder path.
        """

class UI:
    def Begin(label: str) -> bool:
        """
        Begins a new UI window.
        """
    def End():
        """
        Ends the current UI window.
        """
    
    def Text(text: str):
        """
        Draws text.
        """
        
    def TextDisabled(text: str):
        """
        Draws disabled text.
        """
        
    def Button(label: str, size: vec2) -> bool:
        """
        Draws a button with a label and size
        Returns true if clicked
        """
    def SameLine(offsetFromStart: float, spacing: float):
        """
        Ensures that the next item draw will remain on the same line
        as the previous item.
        """
        
    def Seperator():
        """
        Draws a horizontal line.
        """
        
    def BeginTree(label: str, leaf: bool = False):
        """
        Begins a tree node.
        """
        
    def EndTree():
        """
        Ends a tree node.
        """