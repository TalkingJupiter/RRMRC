/*
TODOS:
sudo apt-get update
sudo apt-get install -y g++ libi2c-dev i2c-tools
# check your I²C bus (pins 3/5 are typically i2c-7 on Radxa X4):
ls /dev/i2c-*        # e.g., /dev/i2c-7
sudo i2cdetect -y 7  # should show 44 and/or 45

# save as singleMeasurementSHT31_radxa.cpp, then:
g++ -O2 -Wall -o sht31_radxa singleMeasurementSHT31_radxa.cpp

# one sensor at 0x44
sudo ./sht31_radxa /dev/i2c-7 0x44
# two sensors (0x44 & 0x45)
sudo ./sht31_radxa /dev/i2c-7 0x44 0x45
*/

/*
 * @file singleMeasurmentSHT31_radxa.cpp
 * @brief Read ambient temperature (C/F) and realtive humadity (%RH) on Radxa X4
 * @details
 *     - Soft Reset
 *     - Read serial Number
 *     - High-rep, no-clock-streatching single shot measurement
 * 
 *  Wiring (See the schematic in the file directory)
 *  @copyright (C) Texas Tech University - MIT License
 *  @author [Batuhan Sencer](batuhan.sencer@ttu.edu)
 *  @version V1.0
 *  @date 2025-10-17
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

class DFRobot_SHT3x{
public:
    DFRobot_SHT3x(void* /*pWire*/=nullptr, uint8_t address=0x44, int busnum=1): _addr(address), _busnum(busnum), _fd(-1), _lastTC(0), _lastRH(0), _haveReading(false) {}

    ~DFRobot_SHT3x(){
        if (_fd >=0) ::close(_fd);
    }

    int begin(){
        //TODO: Open `/dev/i2c-<busnum>
        _devpath = "/dev/i2c-" + std::to_string(_busnum);
        _fd = ::open(_devpath.c_str(), 0_RDWR);
        if (_fd<0){
            _lastErr = "open " + _devpath + " failed: " + std::string(std::strerror(errno));
            return 1;
        }
        if (!setSlave(_addr)) return 2;

        //Soft reset (datasheet > 1ms)
        if(!softreset()) return 3;

        //Quick Probe: Try a single shot read one
        float tc = 0, rh=0;
        if(!measureOnce(tc, rh)) return 4;

        _lastTC = tc;
        _lastRH = rh;
        _haveReading = true;
        return 0;
    }

    bool softReset(){
        if (!setSlave(_addr)) return false;
        const uint8_t cmd[2] = {0x30, 0x42};
        if(::write(_fd, cmd, 2) != 2){
            _lastErr = "softReset write failed: " + std::string(std::strerror(errno));
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return true;
    }

    uint32_t readSerialNumber(){
        if(!setSlave(_addr)) return 0;
        const uint8_t cmd[2] = {0x37, 0x80};
        if(::write(_fd, cmd, 2) != return 0);

        uint8_t buf[6] = {0};
        int n = ::read(_fd, buf, 6);
        if (n!=6) return 0;

        if(crc8(buf, 2) != buf[2]) return 0;
        if (crc8(buf + 3, 2) != buf[5]) return 0;

        uint16_t sna = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
        uint16_t snb = (static_cast<uint16_t>(buf[3]) << 8) | buf[4];
        return (static_cast<uint32_t>(sna) << 16) | snb;
    }
}