# Projekt Autonomiczne – Robot Mobilny „MonsterTruck”

Projekt został zrealizowany w ramach zajęć z **Programowania systemów autonomicznych**. Celem było stworzenie działającego prototypu robota mobilnego, który może działać zarówno w trybie **autonomicznym**, jak i **zdalnym**, a przy tym gromadzi i archiwizuje dane z czujników.

## Osiągnięte rezultaty
- Robot „MonsterTruck” działający w dwóch trybach:  
  - Autonomiczny – samodzielna eksploracja i omijanie przeszkód przy użyciu LIDAR-a (VL53L5CX) i MPU6050.  
  - Zdalny – sterowanie przez przeglądarkę w sieci lokalnej (bez potrzeby internetu).  
- Lekki interfejs WWW (HTML/CSS/JS) z funkcjami:  
  - regulacja prędkości,  
  - przełączanie trybów,  
  - wirtualny joystick,  
  - konfiguracja adresów IP,  
  - podgląd i czyszczenie zapisanych odczytów.  
- Zbieranie danych LIDAR-a i zapisywanie ich:  
  - w chmurze (MongoDB Atlas),  
  - w lokalnej bazie SQLite.  
- Możliwość przeglądania historii pomiarów w tabeli.

## Architektura systemu
- Centralna jednostka sterująca: ESP32-C3-ZeroW.  
- Czujniki: LiDAR (VL53L5CX), żyroskop i akcelerometr (MPU6050).  
- Układ napędowy: dwa silniki DC z przekładniami + sterownik TB6612FNG.  
- Zasilanie: bateria + przetwornica boost 5V + regulator 3,3V.  
- Komunikacja: lokalna sieć Wi-Fi hostowana przez ESP32, HTTP GET/POST.  
- Interfejs: prosta strona HTML (desktop + mobilna).

## Technologie
- Platforma: ESP32-C3.  
- Języki programowania: C/C++, Arduino IDE.  
- Biblioteki:  
  - Wire (I2C),  
  - VL53L5CX (LiDAR) – SparkFun,  
  - MPU6050_light – Electronic Cats,  
  - TB6612FNG – autorska,  
  - ArduinoOTA (aktualizacje OTA),  
  - WebServer (ESP32).  
- Bazy danych: MongoDB Atlas, SQLite.  
- System kontroli wersji: GitHub.

## Przykład działania
- Interfejs HTML: sterowanie i podgląd danych.  
- Sterowanie mobilne: wirtualny joystick w przeglądarce.  
- Robot: samodzielna eksploracja + reakcja na komendy użytkownika.  
[Film z działania robota](https://drive.google.com/drive/folders/1qSsjkXssr3BIvSTFcuZnZurtBkeST_fl?usp=sharing)

## Kod źródłowy
[Repozytorium kodu i plików](https://drive.google.com/drive/folders/1hNmyajBKfpIUuRQ2Dvikjvw1qeH3RM5o?usp=sharing)

## Podział prac
- Jakub Szaraj – sterowanie robota, serwer strony, system autonomiczny, zarys projektu.  
- Kateryna Kuzmenko – projekt i implementacja strony HTML, debugging, dokumentacja.  
- Jakub Wiatr – integracja bazy MongoDB, przetwarzanie danych z czujnika LIDAR.
