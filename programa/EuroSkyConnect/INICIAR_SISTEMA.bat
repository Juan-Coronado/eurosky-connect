@echo off
cd /d "%~dp0"
title EuroSky Connect - Inicio

echo Iniciando EuroSky Connect...
curl.exe --silent --fail --max-time 1 "http://127.0.0.1:8080/api/aeropuertos" >nul 2>nul
if errorlevel 1 start "EuroSky Backend" /min "%~dp0EuroSkyConnectWeb.exe"

echo Esperando al backend C++...
for /L %%i in (1,1,20) do (
  curl.exe --silent --fail --max-time 1 "http://127.0.0.1:8080/api/aeropuertos" >nul 2>nul
  if not errorlevel 1 goto listo
  timeout /t 1 /nobreak >nul
)

echo No se pudo iniciar el backend. Revisa que el puerto 8080 este disponible.
pause
exit /b 1

:listo
echo Backend listo. Abriendo el sistema...
start "" "http://127.0.0.1:8080"
exit /b 0
