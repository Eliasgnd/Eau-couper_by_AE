import subprocess
import os
import sys

os.chdir(r"C:\Users\elias\Desktop\codeQT\Eau-couper_by_AE\build\Desktop_Qt_6_10_2_MSVC2022_64bit-Debug")

# Set up environment
env = os.environ.copy()
env['PATH'] = r"C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Tools\MSVC\14.43.34621\bin\Hostx64\x64;" + env.get('PATH', '')
env['LIB'] = r"C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Tools\MSVC\14.43.34621\lib\x64;"
env['INCLUDE'] = r"C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Tools\MSVC\14.43.34621\include;"

print("Building project...")
result = subprocess.run([r"C:\Qt\6.10.2\msvc2022_64\bin\nmake.exe", "-f", "Makefile.Debug"], 
                       capture_output=True, text=True, env=env, timeout=300)

print("STDOUT:")
print(result.stdout[-2000:] if len(result.stdout) > 2000 else result.stdout)

if result.stderr:
    print("\nSTDERR:")
    print(result.stderr[-1000:] if len(result.stderr) > 1000 else result.stderr)

print(f"\nReturn code: {result.returncode}")

if result.returncode == 0:
    print("BUILD SUCCESSFUL!")
else:
    print("BUILD FAILED!")

sys.exit(result.returncode)
