# MAX77972 I2C Library for ESP32-S3

A comprehensive C++ library for interfacing with the Maxim Integrated MAX77972 AccuCharge + ModelGauge 1-Cell Fuel Gauge and 3A Charger IC on the ESP32-S3 using the ESP-IDF framework.

## Features

### Charger Control
- **Enable/Disable charging** with independent charger control
- **Fast charge current control** - Configurable from 100mA to 3.15A
- **Charge voltage regulation** - Configurable from 3.4V to 4.66V
- **Input current limiting** - AICL (Adaptive Input Current Limit) support
- **OTG (Reverse Boost) mode** - Up to 5.1V/1.5A output
- **Prequalification mode** - Trickle and precharge support
- **Temperature-based charging** - 9-zone JEITA temperature control
- **USB Type-C detection** - Automatic USB type detection (SDP/DCP/CDP)
- **BC1.2 detection** - Legacy USB charger detection

### Fuel Gauge Monitoring
- **ModelGauge m5 EZ algorithm** - Industry-leading accuracy
- **Cell voltage measurement** - 78.125 µV resolution
- **Battery current measurement** - 0.15625 mA resolution (10mΩ sense)
- **Temperature monitoring** - Internal die temp & external thermistor
- **State of Charge (SOC)** - Accurate remaining charge percentage
- **Remaining capacity** - Predicted mAh remaining
- **Cycle counting** - Battery cycle lifetime tracking
- **Battery health** - Age estimation as percentage
- **Power calculation** - Instant and averaged battery power
- **Alert system** - Voltage, current, temperature, and SOC alerts

### System Monitoring
- **Input voltage/current** - Monitor power source characteristics
- **System voltages** - VByp, VSys measurements
- **Thermal regulation** - Junction temperature monitoring
- **Watchdog timer** - Configurable timeout control
- **Ship mode** - Minimal power consumption state

## Hardware Connection (ESP32-S3)

```
ESP32-S3          MAX77972
---------         --------
GPIO8 (SCL)  <--> SCL
GPIO9 (SDA)  <--> SDA
GND          <--> GND
3V3          <--> VIO (optional, for logic supply reference)

Battery          MAX77972
-------          --------
+                 BATT
-                 GND

Power Input      MAX77972
-----------      --------
4.5-13.7V   <--> CHGIN
GND         <--> GND
```

### Optional Components
- **Thermistor**: Connect to THM pin for precise temperature monitoring
- **Pull-up resistors**: 4.7kΩ on SCL and SDA (built-in on most development boards)
- **Boost capacitors**: Follow datasheet recommendations for BYP, CHGIN filtering

## Installation

1. Copy the `MAX77972_Library` directory into your ESP-IDF project's `components` directory
2. In your project's `CMakeLists.txt`, add the dependency:
   ```cmake
   set(EXTRA_COMPONENT_DIRS components/MAX77972_Library)
   ```

## I2C Configuration

The library supports two I2C addresses:
- **0x36** (primary) - Default and recommended
- **0x37** (secondary) - Alternative address if 0x36 is occupied

### Custom I2C Pins and Frequency

```cpp
using namespace max77972;

// Create with custom pins and frequency
MAX77972 charger(
    I2C_NUM_0,           // I2C port
    I2C_ADDR_0x36,       // 7-bit address
    GPIO_NUM_9,          // SDA pin
    GPIO_NUM_8,          // SCL pin
    400000               // Clock frequency in Hz
);

charger.init();
charger.probe();
```

## Quick Start Example

```cpp
#include "max77972.hpp"

using namespace max77972;

void setup() {
    // Create and initialize charger
    MAX77972 charger(I2C_NUM_0, I2C_ADDR_0x36);
    charger.init();
    
    // Configure charging parameters
    charger.set_charge_enable(true);
    charger.set_fast_charge_current(ChargingCurrent::I_1000MA);
    charger.set_charge_voltage(ChargingVoltage::V_4200MV);
    charger.set_input_current_limit(2000);  // 2A input limit
    
    // Enable alerts
    charger.set_alert_enable(true);
    charger.set_soc_alert_threshold(10);  // Alert below 10%
}

void loop() {
    BatteryInfo battery;
    charger.get_battery_info(&battery);
    
    printf("Battery: %u%% | %umV | %dmA\n",
           battery.state_of_charge,
           battery.voltage_mv,
           battery.current_ma);
    
    delay(1000);
}
```

## API Reference

### Initialization & Probe

```cpp
esp_err_t init();                           // Initialize I2C and device
esp_err_t probe();                          // Check device presence
```

### Register Operations

```cpp
esp_err_t read_register(uint8_t reg, uint8_t* data);
esp_err_t write_register(uint8_t reg, uint8_t data);
esp_err_t read_registers(uint8_t reg, uint8_t* data, uint8_t len);
esp_err_t write_registers(uint8_t reg, const uint8_t* data, uint8_t len);
esp_err_t modify_register(uint8_t reg, uint8_t mask, uint8_t value);
```

### Charger Control

```cpp
esp_err_t set_charger_enable(bool enable);
esp_err_t set_fast_charge_current(ChargingCurrent current);
esp_err_t get_fast_charge_current(uint16_t* current_ma);
esp_err_t set_charge_voltage(ChargingVoltage voltage);
esp_err_t get_charge_voltage(uint16_t* voltage_mv);
esp_err_t set_input_current_limit(uint16_t current_ma);
esp_err_t get_input_current_limit(uint16_t* current_ma);
esp_err_t set_otg_enable(bool enable);
esp_err_t set_otg_current_limit(uint16_t current_ma);
esp_err_t set_aicl_enable(bool enable);
esp_err_t set_prequalification_enable(bool enable);
esp_err_t set_trickle_charge_current(uint16_t current_ma);
```

### Charger Status

```cpp
esp_err_t get_charger_status(ChargerStatus* status);
esp_err_t get_charger_state(ChargerState* state);
esp_err_t get_temperature_zone(TemperatureZone* zone);
esp_err_t get_usb_type(USBDetectionType* usb_type);
esp_err_t is_charging(bool* is_charging);
```

### Battery Measurements

```cpp
esp_err_t get_battery_info(BatteryInfo* info);
esp_err_t read_vcell(uint16_t* voltage_mv);
esp_err_t read_avg_vcell(uint16_t* voltage_mv);
esp_err_t read_current(int16_t* current_ma);           // Discharge is negative
esp_err_t read_avg_current(int16_t* current_ma);
esp_err_t read_power(int16_t* power_mw);
esp_err_t read_avg_power(int16_t* avg_power_mw);
esp_err_t read_temp(int8_t* temperature_c);
esp_err_t read_die_temp(int8_t* temperature_c);
```

### State of Charge & Capacity

```cpp
esp_err_t read_rep_soc(uint8_t* soc);               // 0-100%
esp_err_t read_mix_soc(uint8_t* soc);
esp_err_t read_av_soc(uint8_t* soc);                // Average SOC
esp_err_t read_rep_cap(uint16_t* capacity_mah);
esp_err_t read_mix_cap(uint16_t* capacity_mah);
esp_err_t read_av_cap(uint16_t* capacity_mah);
esp_err_t read_full_cap_nom(uint16_t* capacity_mah); // Rated capacity
esp_err_t read_cycles(uint16_t* cycles);
esp_err_t read_age(uint8_t* age_pct);
```

### Alerts & Thresholds

```cpp
esp_err_t get_alert_status(AlertStatus* status);
esp_err_t set_alert_enable(bool enable);
esp_err_t set_voltage_alert_threshold(uint16_t threshold_mv);
esp_err_t set_current_alert_threshold(int16_t threshold_ma);
esp_err_t set_temperature_alert_threshold(int8_t threshold_c);
esp_err_t set_soc_alert_threshold(uint8_t threshold_pct);
```

### Configuration

```cpp
esp_err_t soft_reset();
esp_err_t enter_ship_mode();
esp_err_t enter_deep_sleep_mode();
esp_err_t set_jeita_zone_config(uint8_t zone_idx, uint8_t reg_value);
esp_err_t set_thermistor_config(uint16_t therm_cfg);
```

### Utilities

```cpp
esp_err_t dump_registers();
esp_err_t clear_interrupts();
esp_err_t manual_usb_detection();
esp_err_t get_cc_pin_status(uint8_t* cc1, uint8_t* cc2);
```

## Data Structures

### BatteryInfo
```cpp
struct BatteryInfo {
    uint16_t voltage_mv;           // Cell voltage in mV
    uint16_t avg_voltage_mv;       // Average cell voltage
    int16_t current_ma;            // Battery current (-=discharge)
    int16_t avg_current_ma;        // Average battery current
    uint16_t capacity_mah;         // Remaining capacity
    uint8_t state_of_charge;       // SOC 0-100%
    int8_t temperature_c;          // Battery temperature
    int8_t die_temperature_c;      // Die temperature
    uint16_t cycles;               // Cycle count
};
```

### ChargerStatus
```cpp
struct ChargerStatus {
    ChargerState state;            // Current charger mode
    TemperatureZone temp_zone;     // Temperature region
    USBDetectionType usb_type;     // USB power source type
    bool is_charging;              // Actively charging
    bool is_dead_battery;          // Dead battery mode
    bool is_temp_suspended;        // Thermal suspend
};
```

## Enumerations

### ChargerState
```cpp
enum class ChargerState : uint8_t {
    DEAD_BATTERY = 0x00,
    PRECHARGING = 0x01,
    FAST_CHARGING = 0x02,
    CONSTANT_VOLTAGE = 0x03,
    TOP_OFF = 0x04,
    DONE = 0x05,
    TIMER_EXPIRED = 0x06,
    TEMP_SUSPEND = 0x07,
    OFF = 0x08
};
```

### TemperatureZone
```cpp
enum class TemperatureZone : uint8_t {
    COLD = 0x00,
    COOL = 0x01,
    ROOM = 0x02,
    WARM = 0x03,
    HOT = 0x04
};
```

### ChargingCurrent
Predefined values from 100mA to 3150mA in 25mA increments
```cpp
I_100MA, I_125MA, I_150MA, ... I_3000MA, I_3150MA
```

### ChargingVoltage
Predefined values from 3.4V to 4.66V
```cpp
V_3400MV, V_3425MV, ... V_4600MV, V_4650MV, V_4660MV
```

## Error handling

All functions return `esp_err_t`:
- `ESP_OK` - Operation successful
- `ESP_ERR_INVALID_STATE` - Device not initialized
- `ESP_ERR_INVALID_ARG` - Invalid argument
- Other ESP-IDF error codes for I2C communication failures

Example:
```cpp
esp_err_t ret = charger.read_vcell(&voltage);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read voltage: %s", esp_err_to_name(ret));
}
```

## Register Map Reference

The library provides convenient access to all MAX77972 registers via named constants:

**Charger Registers:**
- `CHG_INT` (0x00) - Interrupt status
- `CHG_DTLS_00` (0x01) - Charger details
- `CHG_CTRL` (0x04) - Charger control
- `CHG_CNFG_00-15` (0x20-0x2F) - Configuration registers
- `CHGIN_CTRL_0-1` (0x13-0x14) - Input control

**Fuel Gauge Registers:**
- `VCell` (0xA8) - Cell voltage
- `Current` (0xAC) - Battery current
- `RepSOC` (0xB2) - Reported state of charge
- `RepCap` (0xB3) - Reported capacity
- `Cycles` (0xBA) - Cycle count
- `Temp` (0xAE) - Temperature
- And many more...

See the header file for the complete list.

## Troubleshooting

### Device Not Found
1. Verify hardware connections (SCL/SDA/GND)
2. Check I2C address: default is 0x36
3. Enable I2C pull-up resistors (4.7kΩ typical)
4. Verify 3.3V power supply to device

### Incorrect Readings
1. Ensure sense resistor is 10mΩ (affects current/capacity calculations)
2. Verify thermistor configuration if using external temperature
3. Check register conversion functions for custom sense resistor values
4. Allow time for fuel gauge algorithm to stabilize after power-up

### Cannot Enable Charging
1. Check battery voltage isn't below minimum
2. Verify thermal conditions are appropriate
3. Check charger interrupts for fault flags
4. Review input power source voltage/current

## Building and Flashing

```bash
# Set the target
idf.py set-target esp32s3

# Build
idf.py build

# Flash to ESP32-S3
idf.py flash

# Monitor output
idf.py monitor
```

## Performance Characteristics

- I2C Clock Rate: 20kHz - 400kHz (400kHz recommended)
- Typical I2C transaction time: <2ms
- Register read/write latency: ~1ms
- Fuel gauge update rate: 1.4s (voltage/current/temp)
- Battery voltage update: 1.4s intervals
- System voltage update: 2.8s intervals
- USB detection response: <200ms

## Power Consumption

- Active monitoring: ~1.5mA (MAX77972 only, not including ESP32)
- Ship mode: <1µA
- Deep sleep mode: <5µA

## Limitations and Notes

1. **Sense Resistor**: Default calculations assume 10mΩ external sense resistor. Adjust conversion factors for different values.

2. **Temperature Measurement**: Thermistor type must be configured in `nThermCfg` register. Supported types documented in datasheet Table 9.

3. **JEITA Charging**: 9 temperature zones can be independently configured. Defaults provided for common scenarios.

4. **Fuel Gauge Learning**: Algorithm improves accuracy over multiple charge/discharge cycles. Initial SOC may have ±5% error.

5. **Clock Stretching**: MAX77972 does not support I2C clock stretching.

6. **Register Addressing**: Two I2C addresses available - 0x36 accesses 0x00-0xFF, 0x37 accesses 0x80-0xFF.

## References

- MAX77972 Datasheet: See `datasheets/MAX77972.pdf`
- ESP-IDF Documentation: https://docs.espressif.com/projects/esp-idf/
- I2C Specification: https://www.nxp.com/docs/en/user-manual/UM10204.pdf

## License

This library is provided for educational and commercial use with the MAX77972 IC.

## Support

For issues or feature requests:
1. Check the example code in `main/main.cpp`
2. Review register definitions in `include/max77972.hpp`
3. Consult MAX77972 datasheet for register details
4. Enable debug logging: Set log level to DEBUG

## Version History

- **1.0.0** (March 2026) - Initial release
  - Complete charger control API
  - Full fuel gauge monitoring
  - USB detection support
  - Temperature and alert functionality
  - ESP32-S3 support

---

**Maintained for ESP32-S3 with ESP-IDF Framework**
