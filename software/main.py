import os
import sys
import subprocess
import platform

def check_dependencies():
    """Check and install required dependencies for the GUI."""
    required_packages = {"tkinter": "tk", "Pillow": "pillow", "matplotlib": "matplotlib", "pyserial": "pyserial"}
    for package, import_name in required_packages.items():
        try:
            __import__(import_name)
        except ImportError:
            print(f"Package '{package}' is not installed. Installing...")
            try:
                subprocess.check_call([sys.executable, "-m", "pip", "install", package])
            except subprocess.CalledProcessError:
                print(f"Failed to install package '{package}'. Please install it manually.")
                sys.exit(1)

if platform.system() == "Linux" and os.geteuid() != 0:
    print("This script requires root permissions to access serial ports.")
    print("Please run the script with 'sudo'.")
    sys.exit(1)

def launch_gui():
    """Launch the GUI script."""
    gui_script = os.path.join(os.path.dirname(__file__), "gui.py")
    if not os.path.exists(gui_script):
        print(f"Error: {gui_script} not found.")
        sys.exit(1)
    
    print("Launching GUI...")
    os.system(f"{sys.executable} {gui_script}")

if __name__ == "__main__":
    print("Setting up the environment for the GUI...")
    check_dependencies()
    launch_gui()