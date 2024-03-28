# LightLoRaAPRS
ESP32-S3 based LoRa APRS (433 MHz) Firmware for LightTracker Plus. Check out the hardware -> https://github.com/lightaprs/LightTracker-Plus-1.0

![image](https://github.com/lightaprs/LightLoRaAPRS/assets/48382675/505ebd86-4a61-43f0-999b-e3fc780d13f3)

The main purpose of this basic software is to demonstrate capabilities of [LightTracker Plus 1.0](https://github.com/lightaprs/LightTracker-Plus-1.0) Every LightTracker Plus module is shipped with this firmware so you can start playing with your device immediately.

![image](https://github.com/lightaprs/LightLoRaAPRS/assets/48382675/a0017bbe-c8ea-4921-a755-40861533240e)

LightLoRaAPRS is a single firmware that contains Tracker, iGate (Gateway), and Digi (Router) features. Therefore, you don't have to install different firmware to switch from Tracker to iGate or Digipeater. All you need to do is activate configuration mode and access the configuration page using your computer or smartphone via Wi-Fi.

Additionally, I am not planning to improve or add new features to this software, except for maybe some experimental features or bug fixes. Ricardo Guzman is doing a great job developing a comprehensive LoRa APRS software. I hope he supports LightTracker Plus soon using this repository, so we can focus on designing new modules :)

## WiKi

* [F.A.Q.](https://github.com/lightaprs/LightLoRaAPRS/wiki/F.A.Q.)
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
