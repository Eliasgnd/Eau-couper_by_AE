import subprocess
import os

os.chdir(r"C:\Users\elias\Desktop\codeQT\Eau-couper_by_AE\build\Desktop_Qt_6_10_2_MSVC2022_64bit-Debug")

nmake_path = r"C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\nmake.exe"
vcvars_path = r"C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"

# Call vcvars first, then nmake in a shell command
cmd = f'"{vcvars_path}" > /dev/null && "{nmake_path}" -f Makefile.Debug'

print("Starting build...")
result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=300, cwd=os.getcwd())

# Print last 100 lines of output
output_lines = result.stdout.split('\n')
print("BUILD OUTPUT (last 100 lines):")
print('\n'.join(output_lines[-100:]))

if result.stderr:
    print("\nERRORS:")
    print(result.stderr[-1000:])

print(f"\n{'='*60}")
print(f"Build exit code: {result.returncode}")

if os.path.exists(r"debug\machineDecoupeIHM.exe"):
    import datetime
    exe_time = os.path.getmtime(r"debug\machineDecoupeIHM.exe")
    print(f"Executable updated: {datetime.datetime.fromtimestamp(exe_time)}")
    if result.returncode == 0:
        print("✓ BUILD SUCCESSFUL")
    else:
        print("✗ Executable exists but build had errors")
else:
    print("✗ Executable not found")

