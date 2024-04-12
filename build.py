import requests
import subprocess
import os

def download_file(url: str, file_name: str) -> None:
    if not os.path.exists(file_name):
        response = requests.get(url)
        if response.status_code == 200:
            with open(file_name, 'w') as file:
                file.write(response.text)
            print(f"{file_name} saved successfully.")
        else:
            print(f"Couldn't connect to {url}")
    else:
        print(f"{file_name} already exists.")

def download_dependencies() -> None:
    dependencies = {
        "stb_image.h": "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h",
        "stb_image_write.h": "https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h",
        "omp.h": "https://raw.githubusercontent.com/OpenMP/sources/main/include/omp.h"
    }
    
    for file_name, url in dependencies.items():
        download_file(url, file_name)

def check_for_dependencies() -> bool:
    required_files = ['stb_image.h', 'stb_image_write.h', 'omp.h']

    for file_name in required_files:
        if not os.path.exists(file_name):
            print(f"{file_name} is missing")
            return False

    return True

download_dependencies()

if check_for_dependencies():
    print("All required dependencies are present.")
else:
    print("Some dependencies are missing.")


is_clang_installed = subprocess.run(["clang", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE).returncode == 0
print("Clang is installed on your system.") if is_clang_installed else print("There is no Clang installed on your system.")

cli_compilation_command = "clang cli_palette_shift.c -o cli_palette_shift -lm -fopenmp -s"
cli_result = subprocess.run(cli_compilation_command, shell=True, capture_output=True, text=True);
print("CLI compilation successful." if cli_result.returncode == 0 else f"Compilation failed.\nError: {cli_result.stderr}")

gui_compilation_command = "clang `pkg-config --cflags gtk+-3.0` -o gui_palette_shift gui_palette_shift.c `pkg-config --libs gtk+-3.0`"
gui_result = subprocess.run(gui_compilation_command, shell=True, capture_output=True, text=True);
print("GUI compilation successful." if gui_result.returncode == 0 else f"Compilation failed.\nError: {gui_result.stderr}")

