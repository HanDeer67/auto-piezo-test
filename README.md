# auto-piezo-test
A Qt-based automated testing system for piezoelectric actuator performance evaluation

> 一套基于Qt的压电产品参数自动化测试系统，目前被应用于商业航天压电产品。主控制面板灵活设置时序、电压偏置、步进、重复次数等配置，一键启动测试流程，全程无需人工干预，直接输出测试结果。测试参数包括GB/T 38614-2020国标下的重复定位精度和线性度。
> A Qt-based automated testing system for piezoelectric product parameters, currently applied to commercial aerospace piezoelectric devices. The main control panel allows flexible configuration of timing sequences, voltage bias, step increments, repetition counts, and other settings. The entire testing process can be initiated with a single click, requiring no manual intervention, and directly outputs the test results. Tested parameters include repeatability and linearity in accordance with the GB/T 38614-2020 national standard.

---

## 📌 Overview

Auto Piezo Test System is a modular and automated performance evaluation platform designed for piezoelectric actuators.

The system enables:

* Automated test sequence control
* Voltage bias and stepping configuration
* Real-time motion control via UDP
* Data acquisition and computation
* Automatic result export (CSV)
* Compliance evaluation according to GB/T 38614-2020

It is designed for laboratory validation, production quality inspection, and aerospace-grade component verification.

---

## ✨ Key Features

### Automated Test Control

* Configurable timing sequence
* Voltage bias setting
* Step size control
* Repeat cycle configuration
* One-click start of full test procedure

### Performance Evaluation (GB/T 38614-2020)

* Repeat positioning accuracy
* Linearity analysis
* Multi-cycle statistical processing
* Automatic pass/fail determination

### Data Processing & Export

* Real-time data acquisition
* Mathematical post-processing
* CSV report generation
* Structured result output for traceability

### Communication & Control

* UDP-based motion communication
* Closed-loop command interaction
* Modular communication interface abstraction

---

## 📦 Dependencies

Instrument Drivers (Required)

This system communicates with external instruments via VISA interface.
The following driver packages must be installed before running the software:

* OpenChoiceTekVisa_Deployment_Package_066093813.exe
  (Tektronix VISA driver)

* IOLibrariesSuite-21.2.207-windows-x64.exe
  (Keysight IO Libraries Suite)

Make sure both packages are installed successfully and VISA communication is functioning properly before launching the system..

---

## 🔬 Test Instruments Used

The current Piezo Automatic Test System is configured with the following instruments:

🎛 Signal Generator

* Manufacturer: Tektronix

* Model: AFG1062

* Function: Provides excitation voltage and waveform control for piezo devices.

🔭 Autocollimator

* Manufacturer: 上海光学仪器五厂有限公司

* Model: 1X5-10G (1")

* Type: Dual-axis photoelectric autocollimator

* Function: Measures angular displacement of the piezo-driven platform.

---

## 🏗 System Architecture

```
+--------------------------------------------------+
|                    UI Layer                      |
|        (Parameter Setting / Status Display)     |
+--------------------------------------------------+
|              Test Control Engine                |
|     (Sequence Scheduler / State Machine)        |
+--------------------------------------------------+
|          Data Processing & Math Module          |
|   (Linearity / Repeatability / Statistics)      |
+--------------------------------------------------+
|        Communication Abstraction Layer          |
|                   (UDP)                         |
+--------------------------------------------------+
|             Piezo Motion Platform               |
+--------------------------------------------------+
```

---

## 🛠 Development Environment

| Component | Version            |
| --------- | ------------------ |
| Framework | Qt 6.7.3          |
| Compiler  | MSVC 2019 (64-bit) |
| IDE       | Qt Creator 14.0.2   |
| OS        | Windows            |

> ⚠ The project is built with MSVC 64-bit toolchain.

---

## 📁 Project Structure

```
PZT_testing_system/
│
├── PZT_testing_system.pro        # Main Qt project file
│
├── CsvModule/                    # CSV export module
├── GeneralTools/                 # Utility functions
├── MathModule/                   # Mathematical analysis module
├── UICustom/                     # Custom UI components
│
├── 头文件/
│   ├── mainwindow.h
│   ├── Structs.h
│   ├── udphelper.h
│   └── udpsimulator.h
│
├── 源文件/
│   ├── main.cpp
│   ├── mainwindow.cpp
│   ├── udphelper.cpp
│   └── udpsimulator.cpp
│
├── 界面文件/
│   └── mainwindow.ui
│
└── 资源/
    └── resource.qrc
```

---

## 🚀 Build Instructions

### Using Qt Creator

1. Open the .pro file in Qt Creator 14.0.2
2. Select Qt 6.7.3 (MSVC 2019 64-bit Kit)
3. Configure build settings
4. Build and run


---

## ⚠ Known Constraints

* Windows only
* Hardware-dependent communication layer
* Cross-platform not verified

---

## 👤 Author
XuXiaohan ​
Email: iridescenthan@gmail.com

---

## 📄 License

This project is intended for technical demonstration purposes.
Copyright © 2026 XuXiaohan.  
All rights reserved.  
