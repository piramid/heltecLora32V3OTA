# Heltec LoRa32 V3 — OTA Template & Upload Tool

โปรเจกต์เทมเพลตสำหรับพัฒนาโปรแกรมบน **Heltec WiFi LoRa32 V3** ที่รองรับการอัปเดตเฟิร์มแวร์แบบไร้สาย (OTA) พร้อมแสดงสถานะบนจอ OLED ในตัวและไฟ LED บอกสถานะ พร้อมเครื่องมือช่วยอัปโหลดผ่าน IP โดยตรงสำหรับกรณีที่เครือข่ายไม่รองรับ mDNS

---

## ไฟล์ในโปรเจกต์

| ไฟล์ | หน้าที่ |
|---|---|
| `heltec_ota_template.ino` | เทมเพลตตั้งต้นสำหรับเริ่มโปรเจกต์ใหม่ มี OTA + WiFi + OLED + splash screen เครดิตพร้อมใช้งาน |
| `ota_upload_tool.bat` | เครื่องมืออัปโหลดเฟิร์มแวร์ผ่าน OTA แบบ IP ตรง (ใช้เมื่อ mDNS ในเครือข่ายใช้งานไม่ได้) |
| `ota_config.ini` | ไฟล์ตั้งค่าที่ `ota_upload_tool.bat` สร้างขึ้นเองอัตโนมัติ (เก็บ IP, path ต่างๆ) ไม่ต้องแก้ไขเอง |

---

## คุณสมบัติของเทมเพลต (`heltec_ota_template.ino`)

- เชื่อมต่อ WiFi และเปิดใช้งาน ArduinoOTA อัตโนมัติ
- แสดงสถานะบนจอ OLED ตลอดการทำงาน: กำลังเชื่อม WiFi, พร้อมใช้งาน, กำลังอัปโหลด (พร้อม progress bar), อัปโหลดสำเร็จ/ล้มเหลว
- ไฟ LED กระพริบ/ติดค้างตามสถานะ OTA แต่ละขั้น
- หน้าจอ splash screen แสดงเครดิตผู้พัฒนาตอนเปิดเครื่อง
- โครงสร้างโค้ดใช้ `millis()` แทน `delay()` ยาวๆ เพื่อไม่ให้บล็อกการทำงานของ OTA
- มีจุดที่ทำเครื่องหมาย `===== YOUR CODE HERE =====` ไว้ชัดเจน สำหรับใส่โค้ดฟีเจอร์ใหม่โดยไม่ต้องเขียน OTA boilerplate ซ้ำ

---

## ขั้นตอนติดตั้งเครื่องมือ (ทำครั้งเดียวต่อเครื่อง)

### 1. ติดตั้ง Driver

Heltec V3 ใช้ชิป **CP2102** ถ้า Windows ไม่รู้จักพอร์ต USB โหลด driver จาก:
https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers

### 2. ติดตั้ง Arduino IDE

https://www.arduino.cc/en/software (แนะนำเวอร์ชัน 2.x)

### 3. เพิ่ม Board Package

**File > Preferences > Additional Boards Manager URLs** ใส่ URL ทั้งสอง (คั่นด้วยจุลภาค):
```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
https://resource.heltec.cn/download/package_heltec_esp32_index.json
```

จากนั้น **Tools > Board > Boards Manager** ค้นหา "Heltec ESP32" แล้วติดตั้ง

### 4. ติดตั้ง Library

**Sketch > Include Library > Manage Libraries** ค้นหาและติดตั้ง:
- **ESP8266 and ESP32 OLED driver for SSD1306 displays** (by ThingPulse)

### 5. เลือกบอร์ดให้ถูกต้อง

**Tools > Board > Heltec ESP32 Series Dev-boards > Heltec WiFi LoRa32(V3)**

---

## การใช้งานเทมเพลต

### เริ่มโปรเจกต์ใหม่

1. Copy `heltec_ota_template.ino` ไปโฟลเดอร์โปรเจกต์ใหม่
2. เปลี่ยนชื่อไฟล์ให้ตรงกับชื่อโฟลเดอร์ (กฎบังคับของ Arduino)
3. แก้ค่าในไฟล์:
   - `ssid`, `password` — ข้อมูล WiFi
   - `OTA_HOSTNAME` — ชื่อเฉพาะของโปรเจกต์นี้
4. เขียนโค้ดฟีเจอร์ใหม่เฉพาะในจุดที่ทำเครื่องหมาย `YOUR CODE HERE` เท่านั้น

### อัปโหลดครั้งแรก (ต้องผ่านสาย USB เสมอ)

กด **Upload** ตามปกติใน Arduino IDE — จำเป็นสำหรับฝัง ArduinoOTA เข้าไปในชิปก่อน จะได้อัปเดตไร้สายได้ในครั้งต่อๆ ไป

หลังอัปโหลดสำเร็จ เปิด **Serial Monitor** (115200 baud) หรือดูจอ OLED เพื่อดู IP ที่บอร์ดได้รับ

---

## การอัปเดตไร้สาย (OTA)

มี 2 วิธี เลือกใช้ตามสถานการณ์เครือข่าย

### วิธีที่ 1: ผ่าน Arduino IDE โดยตรง (ถ้า mDNS ใช้งานได้)

**Tools > Port** ถ้าเห็น network port ชื่อ `<OTA_HOSTNAME> at [IP]` ให้เลือกแล้วกด Upload ได้ตามปกติ

> mDNS อาจใช้งานไม่ได้ในบางเครือข่าย (router บางรุ่น, เครือข่ายองค์กร/สถานศึกษาที่บล็อก multicast) ถ้าไม่เจอ network port ให้ใช้วิธีที่ 2 แทน

### วิธีที่ 2: ผ่าน `ota_upload_tool.bat` (อัปโหลดผ่าน IP ตรง ใช้ได้ทุกกรณี)

1. เปิด Arduino IDE > **Sketch > Export Compiled Binary** เพื่อสร้างไฟล์ `.bin`
2. Double-click `ota_upload_tool.bat`
3. **ตั้งค่าครั้งแรกบนเครื่องนี้** (ทำครั้งเดียว):
   - เลือกเมนู **2** ใส่ IP ของบอร์ด (ดูจาก Serial Monitor หรือจอ OLED)
   - เลือกเมนู **3** ใส่ path โฟลเดอร์ sketch (โฟลเดอร์ที่มีไฟล์ .ino อยู่ข้างใน) — สคริปต์จะค้นหา `espota.py` ให้อัตโนมัติ
4. เลือกเมนู **1** เพื่ออัปโหลด — สคริปต์จะเช็ค Python, IP, espota.py, และไฟล์ .bin ให้อัตโนมัติก่อนอัปโหลดจริง

ครั้งต่อๆ ไปแค่ export .bin ใหม่แล้วกดเมนู 1 ได้เลย ไม่ต้องตั้งค่าซ้ำ (ค่าจะถูกจำไว้ในไฟล์ `ota_config.ini`)

**เมนูอื่นๆ ในสคริปต์:**
- เมนู **4** — เปิดไฟล์ .ino ด้วย Notepad เพื่อแก้ไขเร็วๆ (เช่น WiFi SSID/password)
- เมนู **5** — คำแนะนำวิธี export .bin

---

## Troubleshooting

| อาการ | สาเหตุที่เป็นไปได้ | วิธีแก้ |
|---|---|---|
| Tools > Port ไม่เห็น network port | mDNS ถูกบล็อกโดยเครือข่าย/router | ใช้ `ota_upload_tool.bat` แทน (ไม่ต้องพึ่ง mDNS) |
| `ping <hostname>.local` ไม่เจอ แต่ ping IP เจอ | เครื่อง PC หรือ router ไม่รองรับ mDNS reflection | ติดตั้ง Bonjour, เช็ค Windows Firewall/Network profile (Private), หรือใช้ IP ตรงแทน |
| สคริปต์ .bat บอกไม่เจอ Python | ยังไม่ได้ติดตั้ง หรือไม่ได้ติ๊ก "Add to PATH" ตอนติดตั้ง | ติดตั้งจาก https://www.python.org/downloads/ แล้วติ๊กช่อง Add to PATH |
| สคริปต์บอกไม่เจอไฟล์ .bin | ยังไม่ได้ export | Arduino IDE > Sketch > Export Compiled Binary |
| อัปโหลดโปรแกรมใหม่แล้ว OTA ใช้ไม่ได้อีก | โปรแกรมที่อัปโหลดไปไม่มีโค้ด ArduinoOTA อยู่ในตัว | ใช้ `heltec_ota_template.ino` เป็นฐานเสมอเวลาจะเขียนโปรแกรมใหม่ |
| OTA หลุด/ค้างระหว่างอัปโหลด | มี `delay()` ยาวๆ ใน `loop()` บล็อกไม่ให้ `ArduinoOTA.handle()` ทำงานทัน | เปลี่ยนไปใช้ `millis()` แทน `delay()` ตามแบบในเทมเพลต |

---

## เครดิต

**Powered by** Mr. Pornchai Thong-in
Computer Technology Faculty
Mahasarakham Technical College
Email: pornchai.tom@gmail.com
