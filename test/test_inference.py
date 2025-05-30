import os
import subprocess
import builtins

# Save original print function
original_print = print

# Define a new print function that adds color
def print(*args, **kwargs):
    COLOR = '\033[92m'  # Green text; change to any ANSI color code you want
    RESET = '\033[0m'   # Reset to default color
    # Add color before the text, then reset after
    colored_args = [f"{COLOR}{arg}{RESET}" for arg in args]
    original_print(*colored_args, **kwargs)

# Override the built-in print
builtins.print = print

print("If you are running on a non arm64 machine, please remove 'arch -arm64' from the instruction below")
# run dune exec ../bin/main.exe -- <filename> --runtime-dir ./runtime from home for each file in should_fail. Make sure it gives a compiler error
os.chdir(os.path.dirname(os.path.abspath(__file__)))
print(f"Current working directory: {os.getcwd()}")
for filename in os.listdir("./should_fail"):
    if filename.endswith(".qdbp"):
        print(f"Testing {filename}")
        ret = os.system(f"arch -arm64 dune exec ../bin/main.exe -- ./should_fail/{filename} --runtime-dir ../runtime")
        assert ret != 0, f"Expected a compiler error for {filename}, but got success"
        print("Done")
    else:
        print(f"Skipping {filename}")

# No file in ../samples should fail
# CD to the parent
os.chdir(os.path.dirname(os.path.abspath(__file__)) + "/..")
# Print the current working directory
for filename in os.listdir("./samples"):
    if filename.endswith(".qdbp") and "phantom" not in filename and "test" not in filename:
        print(f"Testing {filename}")
        command = [f"arch -arm64 dune exec ./bin/main.exe -- ./samples/{filename} --runtime-dir ./runtime"]
        ret = subprocess.run(command, shell=True)
        assert ret.returncode == 0, f"Expected success for {filename}, but got a compiler error"
        print("Done")
    else:
        print(f"Skipping {filename}")