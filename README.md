# Reliable Management of Repacss Cluster
## 1 Intoduction
The purpose of this repository is to document the source code used, or intended to be used, for the development and deployment of the second monitoring layer of the REPACSS Cluster. This documentation serves as a technical reference to ensure clarity, reproducibility, and maintainability of the system’s components.

## 2 Document Overview
### 2.1 `singleMeasurementSHT31/`
This directory contains the Arduino source code for operating the SHT31 temperature and humidity sensor. The implementation utilizes the DFRobot_SHT3x library. However, due to compatibility issues between this library and the Radxa platform, a custom library was developed to ensure proper integration and functionality.

### 2.2 `singleMeasurmentSHT31_radxa.cpp`
This C++ source file serves as a **prototype** for running the SHT31 sensor on the Radxa X4 platform. Upon successful validation of its functionality, the code will be refactored into separate header (`.h`) and implementation (`.cpp`) files to enhance modularity, reusability, and maintainability.
