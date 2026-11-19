@echo off
rem Compila o simulador no Windows (gcc do WinLibs instalado pelo WinGet)
cd /d "%~dp0"
set "PATH=%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;%PATH%"
gcc -Wall -std=c99 -o fs.exe main.c src\arquivos_c\alocacao_blocos_disco.c src\arquivos_c\arvore_diretorios.c src\arquivos_c\controle_acesso_permissoes.c src\arquivos_c\operacoes_arquivos.c
if errorlevel 1 (
    echo Erro na compilacao.
    exit /b 1
)
echo Compilado: fs.exe
