# MAX77972 Library - Configuration & Porting Guide

## Table of Contents
1. [I2C Configuration](#i2c-configuration)
2. [Hardware Setup](#hardware-setup)
3. [Porting to Different Microcontrollers](#porting-guide)
4. [Advanced Configuration](#advanced-configuration)
5. [Calibration](#calibration)

## I2C Configuration

### Default Configuration
```cpp
// ESP32-S3 Default Pin Assignment
GPIO8  = SCL
GPIO9  = SDA
I2C0   = Port
400kHz = Frequency
0x36   = Slave Address
```

### Custom I2C Setup

#### Option 1: Constructor Parameters
```cpp
MAX77972 charger(
    I2C_NUM_0,              // Use I2C port 0
    I2C_ADDR_0x36,          // Use 0x36 address
    GPIO_NUM_19,            // Custom SDA pin
    GPIO_NUM_20,            // Custom SCL pin
    400000                  // 400kHz clock
);
```

#### Option 2: Manual I2C Initialization
If you have existing I2C configuration, you can modify the library
to use an external I2C handle:

```cpp
// Modify max77972.hpp to accept pre-configured I2C devices:
class MAX77972 {
public:
    // Option: Constructor with external device handle
    MAX77972(i2c_master_dev_handle_t existing_handle);
```

### I2C Clock Speed Selection

| Speed | Best For | Notes |
|-------|----------|-------|
| 20kHz minimum | Very long I2C lines | Not recommended unless necessary |
| 100kHz | Reliable, EMI-resistant | Good for noisy environments |
| 400kHz | **Recommended** | Balance of speed and reliability |
| 1MHz | Fast, short lines only | Careful with noise and capacitance |

**Important**: Do not go below 20kHz - the device internal bus will be occupied.

### Pull-up Resistor Configuration

The MAX77972 operates with open-drain I2C. Pull-ups are required:

```
3V3
 |
R(Pullup)  Typical 4.7kΩ (2.2kΩ for fastest speeds)
 |
------- SCL/SDA -------
 |
Device (internal pull-down to GND through open-drain driver)
```

**Calculation for 400kHz:**
- Bus Capacitance: ~100pF per 30cm of trace
- Pull-up: ~5.6kΩ for reasonable time constant
- Common value: 4.7kΩ per 100pF

Many ESP32 development boards include 10k pull-ups, which are adequate.

## Hardware Setup

### Minimal Board with Battery Protection

```
      3V3 ─┬─ 100µF ── GND
           ├─ 10µF  ── GND
           |
        ┌──┴───────────────────┐
        |                       |
        ├─ VIO ───┬─ 1µF ── GND |
        |         |            |
        ├─ VDD ─┬─┴── GND      |
        |       |              |
        │       └─ 10µF ── GND |
        |                      |
MAX77972|                     (device)
        |    I2C Interface     |
        ├─ SCL ───────┬── 4.7k─┤ (SCL)
        |             ├──────┤└─ SCL
        ├─ SDA ───────┬── 4.7k─┤ (SDA)
        |             └──────────── SDA
        ├─ GND ─────────────────── GND
        |
        ├─ BATT ──────── Battery +
        ├─ BST  ──────── Boost Output (if using OTG)
        ├─ CSP  ──────── Current Sense+ (via 0.01Ω)
        ├─ CSN  ──────── Current Sense-
        └─ THM  ────┬─── GND (or thermistor divider)
                    |
                   10kΩ or 100kΩ resistor (depends on thermistor)
                    |
                    GND
```

### LCD Display Integration Example

```cpp
// Typical application with battery monitoring display
struct {
    uint16_t battery_v;
    int16_t battery_i;
    uint8_t soc;
    uint8_t temp;
} battery_display;

void update_display() {
    charger.read_vcell(&battery_display.battery_v);
    charger.read_current(&battery_display.battery_i);
    charger.read_rep_soc(&battery_display.soc);
    charger.read_temp(&battery_display.temp);
    
    // Update LCD...
}
```

### Thermal Monitoring Setup

For devices requiring thermal control:

```cpp
void thermal_protection_task(void* arg) {
    MAX77972* charger = (MAX77972*)arg;
    
    while (1) {
        int8_t die_temp, battery_temp;
        charger->read_die_temp(&die_temp);
        charger->read_temp(&battery_temp);
        
        if (die_temp > 60) {
            ESP_LOGW(TAG, "Die overtemp, suspending charge");
            charger->set_charge_enable(false);
        } else if (die_temp < 50) {
            charger->set_charge_enable(true);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // Check every 5 seconds
    }
}
```

## Porting Guide

### Porting to Other Microcontrollers

The library is designed around ESP-IDF's I2C HAL. To port to other platforms:

#### Step 1: Abstraction Layer
Create a platform-specific I2C wrapper:

```cpp
// File: i2c_platform.hpp
#ifndef I2C_PLATFORM_HPP
#define I2C_PLATFORM_HPP

#include <cstdint>
#include <cstring>

class I2CInterface {
public:
    virtual ~I2CInterface() = default;
    
    virtual int write(uint8_t reg, uint8_t data) = 0;
    virtual int read(uint8_t reg, uint8_t* data) = 0;
    virtual int write_bytes(uint8_t reg, const uint8_t* data, size_t len) = 0;
    virtual int read_bytes(uint8_t reg, uint8_t* data, size_t len) = 0;
};
```

#### Step 2: Implement for Target Platform

**For STM32 HAL:**
```cpp
class STM32I2C : public I2CInterface {
private:
    I2C_HandleTypeDef* hi2c;
    uint16_t device_addr;
    
public:
    STM32I2C(I2C_HandleTypeDef* i2c, uint16_t addr) 
        : hi2c(i2c), device_addr(addr << 1) {}
    
    int read(uint8_t reg, uint8_t* data) override {
        return HAL_I2C_Mem_Read(hi2c, device_addr, reg, I2C_MEMADD_SIZE_8BIT,
                                data, 1, HAL_MAX_DELAY);
    }
    
    int write(uint8_t reg, uint8_t data) override {
        return HAL_I2C_Mem_Write(hi2c, device_addr, reg, I2C_MEMADD_SIZE_8BIT,
                                 &data, 1, HAL_MAX_DELAY);
    }
};
```

**For Arduino:**
```cpp
class ArduinoI2C : public I2CInterface {
private:
    uint8_t device_addr;
    
public:
    ArduinoI2C(uint8_t addr) : device_addr(addr) {}
    
    int read(uint8_t reg, uint8_t* data) override {
        Wire.beginTransmission(device_addr);
        Wire.write(reg);
        Wire.endTransmission();
        Wire.requestFrom(device_addr, (uint8_t)1);
        *data = Wire.read();
        return 0;  // Success
    }
    
    int write(uint8_t reg, uint8_t data) override {
        Wire.beginTransmission(device_addr);
        Wire.write(reg);
        Wire.write(data);
        return Wire.endTransmission();
    }
};
```

#### Step 3: Modify MAX77972 Class

Replace I2C-specific code in `max77972.cpp`:

```cpp
// Instead of:
esp_err_t MAX77972::read_register(uint8_t reg, uint8_t* data) {
    i2c_master_transmit_receive(dev_handle, &reg, 1, data, 1, -1);
}

// Use:
esp_err_t MAX77972::read_register(uint8_t reg, uint8_t* data) {
    return i2c_interface->read(reg, data) == 0 ? ESP_OK : ESP_FAIL;
}
```

#### Step 4: Add Platform-Agnostic Error Codes

Create a compatibility header:
```cpp
// error_compat.hpp
#ifdef ESP_IDF
    #include "esp_err.h"
#else
    enum esp_err_t {
        ESP_OK = 0,
        ESP_FAIL = -1,
        ESP_ERR_INVALID_ARG = -2,
        ESP_ERR_INVALID_STATE = -3
    };
#endif
```

### Platform-Specific Example: STM32

Complete example for STM32 microcontroller:

```cpp
// STM32 main.c
#include "i2c.h"
#include "stm32_i2c.hpp"
#include "max77972.hpp"

int main() {
    HAL_Init();
    SystemClock_Config();
    MX_I2C1_Init();  // Init I2C1
    
    // Create platform-specific I2C interface
    STM32I2C i2c_platform(&hi2c1, 0x36);
    
    // Create MAX77972 with that interface
    // (requires modifying MAX77972 constructor to accept I2CInterface*)
    MAX77972 charger(&i2c_platform);
    charger.init();
    
    while (1) {
        BatteryInfo info;
        charger.get_battery_info(&info);
        printf("Battery: %d%%\n", info.state_of_charge);
        
        HAL_Delay(1000);
    }
}
```

### Considerations for Different Architectures

| Aspect | Arduino | STM32 HAL | nRF52 | RP2040 |
|--------|---------|-----------|-------|--------|
| Typical I2C Speed | 100kHz | 400kHz | 400kHz | 400kHz |
| Interrupt Support | GPIO interrupt | EXTI | Events | GPIO-based |
| Float Support | Minimal | Full | Full | Full |
| Memory Constraints | Very Limited | Moderate | Good | Good |
| Real-time Requirements | Low | Medium | High | Medium |

## Advanced Configuration

### Using External Flash for Learned Data

The fuel gauge learns battery characteristics over time. Save this learning data:

```cpp
// Save learned data when battery is full
void save_learned_data() {
    uint8_t learned_data[32];
    
    // Read all learning-related registers
    charger.read_registers(0xC0, learned_data, 32);
    
    // Save to EEPROM or Flash
    save_to_nvs("battery_learned", learned_data, 32);
}

// Restore on startup
void restore_learned_data() {
    uint8_t learned_data[32];
    
    if (!load_from_nvs("battery_learned", learned_data, 32)) {
        return;  // No saved data
    }
    
    // Write back to device
    charger.write_registers(0xC0, learned_data, 32);
}
```

### Interrupt-Driven Monitoring

For responsive applications, use ALRT pin:

```cpp
static MAX77972* g_charger = nullptr;

void IRAM_ATTR alrt_isr_handler(void* arg) {
    // ISR context - just set a flag
    BaseType_t high_task_awoken = pdFALSE;
    xSemaphoreGiveFromISR(alert_semaphore, &high_task_awoken);
    portYIELD_FROM_ISR(high_task_awoken);
}

void alert_monitor_task(void* arg) {
    while (1) {
        if (xSemaphoreTake(alert_semaphore, portMAX_DELAY)) {
            // Handle alert outside ISR context
            AlertStatus alerts;
            g_charger->get_alert_status(&alerts);
            // React to alerts...
        }
    }
}

void setup_alrt_interrupt() {
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << ALRT_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ALRT_PIN, alrt_isr_handler, nullptr);
}
```

### Custom Sense Resistor Calibration

If using non-standard sense resistor:

```cpp
// Default: 10mΩ sense resistor
// If using 5mΩ, current readings will be 2x
// If using 20mΩ, current readings will be 0.5x

class MAX77972Calibrated : public MAX77972 {
private:
    float sense_resistor_ohms;
    
public:
    MAX77972Calibrated(float sense_r) : sense_resistor_ohms(sense_r) {}
    
protected:
    // Override conversion function
    int16_t convert_raw_current(uint16_t raw) {
        // Standard formula for 10mΩ:
        // current_ma = raw * 0.15625
        
        // Adjust for custom sense resistor:
        float calibration_factor = 10.0f / sense_resistor_ohms;
        int16_t signed_val = MAX77972::convert_raw_to_signed(raw);
        return (signed_val * 15625) / (100000 * calibration_factor);
    }
};
```

### Battery Profile Support

Different battery chemistry requires different configurations:

```cpp
enum class BatteryChemistry {
    LIPO,      // Lithium Polymer
    LIION,     // Lithium Ion
    LIR,       // Lithium Iron
    NIMH       // Nickel Metal Hydride
};

void configure_battery(BatteryChemistry chemistry) {
    switch (chemistry) {
        case BatteryChemistry::LIPO:
            charger.set_charge_voltage(ChargingVoltage::V_4200MV);
            charger.set_trickle_charge_current(300);
            break;
            
        case BatteryChemistry::LIION:
            charger.set_charge_voltage(ChargingVoltage::V_4200MV);
            charger.set_trickle_charge_current(300);
            break;
            
        case BatteryChemistry::LIR:
            charger.set_charge_voltage(ChargingVoltage::V_3600MV);
            charger.set_trickle_charge_current(200);
            break;
            
        case BatteryChemistry::NIMH:
            // NIMH charging typically not supported by this IC
            break;
    }
}
```

## Calibration

### Current Sense Calibration

If precise current measurement is required:

```cpp
// Method 1: DCR (DC Resistance) Calibration
void calibrate_current_sense() {
    // With battery fully charged (no load):
    // 1. Measure actual charge current with multimeter
    // 2. Read from device
    // 3. Calculate correction factor
    
    int16_t device_reading;
    charger.read_current(&device_reading);
    
    // Actual current = 2.0A (measured with multimeter)
    float correction = 2000.0f / device_reading;  // 2000mA / device reading
    
    // Store for later use
    save_calibration("current_cal", correction);
}
```

### Voltage Measurement Calibration

```cpp
void calibrate_voltage() {
    // With stable known voltage applied:
    uint16_t device_reading;
    charger.read_vcell(&device_reading);
    
    // Actual voltage = 3.700V (measured with DVM)
    float correction = 3700.0f / device_reading;
    save_calibration("voltage_cal", correction);
}
```

### Temperature Calibration

For external thermistor:

1. Place thermistor at known temperatures (0°C, 25°C, 50°C)
2. Read device and actual temperature
3. Create lookup table or linear fit

```cpp
struct TempCalPoint {
    float actual_c;
    int16_t raw_reading;
};

TempCalPoint cal_points[] = {
    {0.0f, 1234},    // At 0°C, raw = 1234
    {25.0f, 5678},   // At 25°C, raw = 5678
    {50.0f, 9012}    // At 50°C, raw = 9012
};

int8_t calibrated_temp(int16_t raw) {
    // Linear interpolation between calibration points
    for (int i = 0; i < 2; i++) {
        if (raw >= cal_points[i].raw_reading && raw <= cal_points[i+1].raw_reading) {
            float t = (float)(raw - cal_points[i].raw_reading) / 
                      (cal_points[i+1].raw_reading - cal_points[i].raw_reading);
            float temp = cal_points[i].actual_c + 
                        t * (cal_points[i+1].actual_c - cal_points[i].actual_c);
            return (int8_t)temp;
        }
    }
    return 0;  // Out of range
}
```

---

For additional help, consult the MAX77972 datasheet and ESP-IDF documentation.
