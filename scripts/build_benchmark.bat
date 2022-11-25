:: 작성자: 윤정도
:: 작성일: 2022/11/24

@echo off

cd ..
git submodule update --init --recursive

:: 32비트 빌드
cmake -S . -B build/Win32 -G"Visual Studio 17 2022" -A Win32
cmake --build build/Win32 --config Debug
cmake --build build/Win32 --config Release

:: 64비트 빌드
cmake -S . -B build/x64 -G"Visual Studio 17 2022" -A x64
cmake --build build/x64 --config Debug
cmake --build build/x64 --config Release

echo "빌드가 모두 완료되었습니다"
timeout /t 2