@echo off

set EsptoolPath=esptool_v4.7.0.exe

set BaseArgs=--chip esp32 --baud 921600
set SetupArgs=--before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect

set firmware=0x0 ..\docs\firmware\ocs2_esp32_latest.bin

echo %EsptoolPath% %BaseArgs% %SetupArgs% %firmware%
%EsptoolPath% %BaseArgs% %SetupArgs% %firmware%

pause
