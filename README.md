# LightLoRaAPRS
ESP32-S3 based LoRa APRS (433 MHz) Firmware for LightTracker Plus. Check out the hardware -> https://github.com/lightaprs/LightTracker-Plus-1.0

![image](https://github.com/lightaprs/LightLoRaAPRS/assets/48382675/505ebd86-4a61-43f0-999b-e3fc780d13f3)

Main purpose of this basic software is to demonstrate capabilities of [LightTracker Plus 1.0](https://github.com/lightaprs/LightTracker-Plus-1.0) Every LightTracker Plus module is shipped with this firmware so you can start playing with your device immediately.

![image](https://github.com/lightaprs/LightLoRaAPRS/assets/48382675/a0017bbe-c8ea-4921-a755-40861533240e)

LightLoRaAPRS is an single firmware contains Tracker, iGate (Gateway) and Digi (Router) features. So you don't have to install a different firmware to switch from Tracker to iGate or Digipeater. All you need is to activate configuration mode and access to configuration page using your computer or smartphone via Wi-Fi. You will find more info about the software soon on Wiki page.

Also I am not planing to improve or add new features to this software, maybe add some experimental features or bugfixes. Ricardo Guzman is doing a great job and developing a great LoRa APRS software. So I hope he supports LightTracker Plus soon using this repo and so we can focus on designing new modules :)

## WiKi

* [Setup (Configuration) Mode](https://github.com/lightaprs/LightLoRaAPRS/wiki/Setup-(Configuration)-Mode)
* [Common Configuration](https://github.com/lightaprs/LightLoRaAPRS/wiki/Common-Configuration)
* [Tracker Configuration](https://github.com/lightaprs/LightLoRaAPRS/wiki/Tracker-Configuration)
* [High Altitude Balloon (HAB) Tracker Mode](https://github.com/lightaprs/LightLoRaAPRS/wiki/High-Altitude-Balloon-(HAB)-Tracker-Mode)
* [Web (OTA) Firmware Update](https://github.com/lightaprs/LightLoRaAPRS/wiki/Web-(OTA)-Firmware-Update)
* [Putting the Device into Programming Mode](https://github.com/lightaprs/LightLoRaAPRS/wiki/Putting-the-Device-into-Programming-Mode)
* [ESPTool (USB) Firmware Update Guide](https://github.com/lightaprs/LightLoRaAPRS/wiki/ESPTool-Firmware-Update-Guide)

## This code was based on the work of :

* https://github.com/lora-aprs/LoRa_APRS_Tracker : Peter - OE5BPA LoRa Tracker
* https://github.com/lora-aprs/LoRa_APRS_iGate : Peter - OE5BPA LoRa iGate
* https://github.com/richonguzman/LoRa_APRS_Tracker : Ricardo Guzman - CA2RXU Tracker
* https://github.com/richonguzman/LoRa_APRS_iGate : Ricardo Guzman - CA2RXU iGate
