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

    float getTemperatureC(){
        ensureFresh();
        return _lastTC;
    }

    float getTemperatureF(){
        ensureFresh();
        return static_cast<float>(_lastTC * 9.0 / 5.0 + 32.0);
    }

    float getHumidityRH(){
        ensureFresh();
        return _lastRH;
    }

    std::string lastError() const {return _lastErr;} //exposing the last error string for debug purposes

private:
    uint8_t _addr;
    int     _busnum;
    int     _fd;
    std::string _devpath;
    std::string _lastErr;

    float _lastTC, lastRH;
    bool _haveReading;
    std::chrono::steady_clock::time_point _lastReadTime;

    static uint8_t crc8(const uint8_t* data, size_t len){
        uint8_t crc=0xFF;
        for(size_t i=0; i<len; i++){
            crc ^=data[i];
            for (int b=0; b<8; b++){
                crc = (crc&0x80) ? static_cast<uint8_t>((crc<<1)) ^ 0x31 : static_cast<uint8_t>(crc <<1);
            }
        }
        return crc;
    }

    bool setSlave(int addr7){
        if (_fd < 0) return false;
        if (ioctl(_fd, I2C_SLAVE, addr7) < 0){
            _lastErr = "ioctl(I2C_SLAVE,0x" + toHex(addr7) + ") failed: " + std::string(std::strerror(errno));
            return false;
        }
        return true;
    }
    
    static std::string toHex(int v){
        std::ostringstream oss; oss << std::hex <<v; return oss.str();
    }
    
    bool measureOnce(float& tC, float& rh) {
        if(!setSlave(_addr)) return false;

        const uint8_t cmd[2] = {0x24, 0x00};
        if(::write(_fd, cmd, 2) != 2){
            _lastErr = "measure write failed: " + std::string(std::strerror(errno));
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        uint8_t buf[6] = {0};
        int n = ::read(_fd, buf, 6);
        if(n!=6){
            _lastErr = "measure read failed: " + std::string(std::strerror(errno));
            return false;
        }

        if (crc8(buf, 2) != buf[2]) {_lastErr = "temp CRC mismatch"; return false}
        if (crc8(buf +3, 2) != buf[5]) {_lastErr = "RH CRC mismatch"; return false}

        uint16_t rawT = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
        uint16_t rawRH = (static_cast<uint16_t>(buf[3]) << 8) | buf[4];

        double t_c = -45 + 175.0 * (static_cast<uint16_t>(rawT) / 65535.0);
        double rhp = 100.0*(static_cast<double>(rawRH) / 65535.0);

        if (rhp < 0.0) rhp=0.0; if (rhp > 100.0) rhp = 100.0;

        tC = static_cast<float>(t_c);
        rh = static_cast<float(rhp);
        return true;
    }
    
    void ensureFresh(){
        auto now = std::chrono::steady_clock::now();
        bool need = !_haveReading || (std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastReadTime).count() > 500);

        if (!need) return;

        float tc=0, rh=0;

        if(measureOnce(tc, rh)){
            _lastTC = tc; _lastRH = rh; _haveReading=true; _lastReadTime = now;
        } else {
            if(!_haveReading) {_lastTC=_lastRH = std::numeric_limits<float>::quiet_NaN();}
        }
    }
};

int main(int argc, char** argv){
    int bus = 7;        //default to /dev/i2c-7 (Radxa X4 pins 3/5)
    int addr = 0x44;    //default sensor
    if (argc >= 2) bus = std::stoi(argv[1]);
    if (argc >= 3) addr = (std::strncmp(argv[2], "0x", 2) == 0 || std::strcmp(argv[2], "0X", 2) == 0) ? static_cast<int>(std::strtol(argv[2] + 2, nullptr, 16)): std::stoi(argv[2]);


    DFRobot_SHT3x sht3x(nullptr, static_cast<uint8_t>(addr), bus);

    while (sht3x.begin() != 0){
        std::cerr << "Failed to initilaze the chip on /dev/i2c-" << bus;
                  << " addr 0x" << std::hex << addr << std::dec
                  << " - " << sht3x.lastError() << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Chip serial number: 0X" << std::hex << sht3x.readSerialNumber() << std::dec << "\n";
    if (!sht3x.softReset()) std::cout << "WARNING: softReset failed!!!";

    std::cout << "-------------------- Read in Single Measurement Mode --------------------";

    for(;;){
        float c = sht3x.getTemperatureC();
        float f = sht3x.getTemperatureF();
        float rh = sht3x.getHumidityRH();

        std::cout << std::fixed << std::setprecision(2)
                  << "Ambient Temperature(C/F): " << c << " /C " << f << " /F "
                  << "Relative Humidity(%RH): " << rh << " %RH\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}