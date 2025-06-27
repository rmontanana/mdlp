
# read the version from the CMakeLists.txt file
import re
import sys
from pathlib import Path
 
def get_version_from_cmakelists(cmakelists_path):
    # Read the CMakeLists.txt file
    try:
        with open(cmakelists_path, 'r') as file:
            content = file.read()
    except IOError as e:
        print(f"Error reading {cmakelists_path}: {e}")
        sys.exit(1)
    # Use regex to find the version line
    # The regex pattern looks for a line that starts with 'project' and captures the version number
    # in the format VERSION x.y.z where x, y, and z are digits.
    # It allows for optional whitespace around the parentheses and the version number.
    version_pattern = re.compile(
        r'project\s*\([^\)]*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)', re.IGNORECASE | re.DOTALL
    )
    match = version_pattern.search(content)
    if match:
        return match.group(1)
    else:
        return None
    
def main():
    # Get the path to the CMakeLists.txt file
    cmakelists_path = Path(__file__).parent / "CMakeLists.txt"
    
    # Check if the file exists
    if not cmakelists_path.exists():
        print(f"Error: {cmakelists_path} does not exist.")
        sys.exit(1)
    
    # Get the version from the CMakeLists.txt file
    version = get_version_from_cmakelists(cmakelists_path)
    
    if version:
        print(f"Version: {version}")
    else:
        print("Version not found in CMakeLists.txt.")
        sys.exit(1)

if __name__ == "__main__":
    main()
