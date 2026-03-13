# Getting Started with MAX77972 Library

**Last Updated**: March 13, 2026

## ⚡ 5-Minute Quick Start

### 1. Copy Library to Your Project

```bash
# Navigate to your ESP-IDF project root
cd your-esp-project

# Add the library to components
cp -r MAX77972_Library components/

# Or create symlink for easy updates
ln -s path/to/MAX77972_Library components/MAX77972_Library
```

### 2. Update Project CMakeLists.txt

If using components directory, it should be auto-discovered. Otherwise:

```cmake
# In your project's CMakeLists.txt, add:
set(EXTRA_COMPONENT_DIRS components/MAX77972_Library)
```

### 3. Configure Target

```bash
idf.py set-target esp32s3
```

### 4. Build & Flash

```bash
idf.py build
idf.py flash
idf.py monitor
```

### 5. You Should See

```
I (1234) MAX77972_EXAMPLE: MAX77972 Library Example Started
I (1235) MAX77972_EXAMPLE: =================================
I (1236) MAX77972: MAX77972 initialized successfully at address 0x36
I (1400) MAX77972_EXAMPLE: Starting periodic monitoring...

=== BATTERY STATUS ===
Voltage:         3850 mV
Current:          -250 mA
State of Charge:   75 %
Temperature:       25 °C

=== CHARGER STATUS ===
State:          FAST_CHARGING
...
```

## 🔌 Hardware Setup (5 minutes)

### Minimal Wiring

```
ESP32-S3              MAX77972        Battery
────────              ────────        ───────
GPIO8 (SCL) ────────  SCL
GPIO9 (SDA) ────────  SDA
GND ─────────────────  GND ─ ─ ─ ─ ─ ─  -
                      BATT ─ ─ ─ ─ ─ ─  +

Optional: Add 4.7kΩ pull-up resistors on SCL and SDA to 3.3V
(Most dev boards already have these)
```

### I2C Verification

```cpp
// In your code, check if device is present:
if (charger.probe() == ESP_OK) {
    ESP_LOGI(TAG, "MAX77972 found!");
} else {
    ESP_LOGE(TAG, "Device not found - check wiring");
}
```

## 📝 Basic Example (Copy & Paste Ready)

```cpp
#include "max77972.hpp"

using namespace max77972;

extern "C" void app_main(void) {
    // Create charger instance (default pins: GPIO8=SCL, GPIO9=SDA)
    MAX77972 charger(I2C_NUM_0, I2C_ADDR_0x36);
    
    // Initialize
    if (charger.init() != ESP_OK) {
        printf("Failed to init charger\n");
        return;
    }
    
    // Configure charging
    charger.set_charge_enable(true);
    charger.set_fast_charge_current(ChargingCurrent::I_1000MA);  // 1A
    charger.set_charge_voltage(ChargingVoltage::V_4200MV);        // 4.2V
    charger.set_input_current_limit(2000);                        // 2A max input
    
    // Read and print battery info every second
    while (1) {
        BatteryInfo battery;
        if (charger.get_battery_info(&battery) == ESP_OK) {
            printf("Battery: %u%% | %umV | %dmA\n",
                   battery.state_of_charge,
                   battery.voltage_mv,
                   battery.current_ma);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second
    }
}
```

Output:
```
Battery: 82% | 4150mV | -50mA
Battery: 82% | 4150mV | 0mA
Battery: 82% | 4150mV | 0mA
```

## 🎯 Common Tasks

### Read Battery Voltage

```cpp
uint16_t voltage_mv;
charger.read_vcell(&voltage_mv);
printf("Voltage: %u mV\n", voltage_mv);
```

### Read Current

```cpp
int16_t current_ma;
charger.read_current(&current_ma);
// Negative = discharging, Positive = charging
printf("Current: %d mA\n", current_ma);
```

### Get State of Charge

```cpp
uint8_t soc;
charger.read_rep_soc(&soc);
printf("Charge: %u %%\n", soc);
```

### Check Charging Status

```cpp
ChargerStatus status;
charger.get_charger_status(&status);

const char* state[] = {
    "DEAD_BAT", "PRECHARGE", "FAST_CHG", "CONST_V",
    "TOP_OFF", "DONE", "TIMER_EXP", "TEMP_SUSP", "OFF"
};

printf("State: %s\n", state[status.state]);
```

### Set Charge Current

```cpp
// Predefined values
charger.set_fast_charge_current(ChargingCurrent::I_100MA);    // 100mA
charger.set_fast_charge_current(ChargingCurrent::I_500MA);    // 500mA
charger.set_fast_charge_current(ChargingCurrent::I_1000MA);   // 1A
charger.set_fast_charge_current(ChargingCurrent::I_2000MA);   // 2A
charger.set_fast_charge_current(ChargingCurrent::I_3000MA);   // 3A
charger.set_fast_charge_current(ChargingCurrent::I_3150MA);   // 3.15A
```

### Set Charge Voltage

```cpp
// Predefined values
charger.set_charge_voltage(ChargingVoltage::V_4100MV);   // 4.1V
charger.set_charge_voltage(ChargingVoltage::V_4200MV);   // 4.2V (typical)
charger.set_charge_voltage(ChargingVoltage::V_4300MV);   // 4.3V
charger.set_charge_voltage(ChargingVoltage::V_4400MV);   // 4.4V
charger.set_charge_voltage(ChargingVoltage::V_4500MV);   // 4.5V
```

### Detect USB Power Source Type

```cpp
USBDetectionType usb_type;
charger.get_usb_type(&usb_type);

const char* type_str[] = {
    "UNKNOWN", "SDP", "DCP", "CDP", "OTG"
};
printf("USB Type: %s\n", type_str[(int)usb_type]);
```

### Monitor Temperature

```cpp
int8_t battery_temp, die_temp;
charger.read_temp(&battery_temp);
charger.read_die_temp(&die_temp);

printf("Battery Temp: %d°C\n", battery_temp);
printf("Die Temp: %d°C\n", die_temp);

// Protect from overheating
if (die_temp > 60) {
    charger.set_charge_enable(false);
    ESP_LOGW(TAG, "Temperature critical!");
}
```

### Handle Alerts

```cpp
AlertStatus alerts;
charger.get_alert_status(&alerts);

if (alerts.voltage_alert) {
    ESP_LOGW(TAG, "Voltage out of range!");
}
if (alerts.temperature_alert) {
    ESP_LOGW(TAG, "Temperature out of range!");
}
if (alerts.soc_alert) {
    uint8_t soc;
    charger.read_rep_soc(&soc);
    ESP_LOGW(TAG, "Battery low: %d%%", soc);
}
```

### Read Battery Health

```cpp
uint16_t full_nom, full_cur;
charger.read_full_cap_nom(&full_nom);
charger.read_full_cap(full_cur);

uint8_t health = (full_cur * 100) / full_nom;
printf("Battery Health: %u%%\n", health);
```

### Track Battery Cycles

```cpp
uint16_t cycles;
charger.read_cycles(&cycles);
printf("Battery Cycles: %u\n", cycles);

// Typical battery: 500-1000 cycles before replacement
uint8_t remaining = 100 - ((cycles * 100) / 500);
printf("Expected Life: %u %%\n", remaining);
```

## 🔧 Troubleshooting

### Device Not Found

**Error**: `Failed to communicate with MAX77972`

**Causes & Solutions**:
1. ❌ Wrong I2C address
   - ✅ Default is 0x36 - verify on your board
   - ✅ Try `MAX77972 charger(..., I2C_ADDR_0x37)` if default fails

2. ❌ Pin conflict
   - ✅ GPIO8/GPIO9 may be in use by other peripherals
   - ✅ See CONFIGURATION.md for custom pins

3. ❌ No pull-up resistors
   - ✅ Add 4.7kΩ resistors on SCL and SDA to 3.3V
   - ✅ Most dev boards have these built-in

4. ❌ Power not connected to device
   - ✅ Check VDD/GND connections
   - ✅ Use multimeter to verify 3.3V at device

### Incorrect Readings

**Issue**: `Voltage reading seems off by 2x or 0.5x`

**Solution**: Verify sense resistor
- Default calculations assume 10mΩ sense resistor
- Different value = multiply by 10/actual_value
- See CONFIGURATION.md for custom sense resistor support

**Issue**: `Temperature reading is way off`

**Solution**: Configure thermistor type
```cpp
charger.set_thermistor_config(0x71DE);  // Murata 10k, Beta=3435
charger.set_thermistor_config(0x48EB);  // Murata 100k, Beta=4250
// See QUICK_REFERENCE.md for your specific thermistor
```

### Cannot Enable Charging

**Possible reasons**:
- Battery voltage too low (< 2.5V)
- Temperature out of JEITA zone
- Charger in fault state
- Input power insufficient

**Debug**:
```cpp
ChargerStatus status;
charger.get_charger_status(&status);

printf("State: %d\n", status.state);
printf("Temp Zone: %d\n", status.temp_zone);
printf("Dead Batt: %d\n", status.is_dead_battery);

charger.dump_registers();  // Dump all registers for detailed analysis
```

## 📚 Documentation Guide

| Document | Use For |
|----------|---------|
| **README.md** | Overview, installation, complete API reference |
| **QUICK_REFERENCE.md** | Code examples, common tasks, reference tables |
| **CONFIGURATION.md** | Advanced setup, porting, calibration |
| **max77972.hpp** | Detailed function documentation |
| **main.cpp** | Working example application |

## 🚀 Next Steps

1. ✅ Get basic example working
2. ✅ Read QUICK_REFERENCE.md for your specific use case
3. ✅ Integrate into your application
4. ✅ See CONFIGURATION.md for advanced features
5. ✅ Refer to max77972.hpp for API details
6. ✅ Check datasheet (datasheets/MAX77972.pdf) for register details

## 📞 Quick Reference

**Default I2C Configuration**:
- Port: I2C_NUM_0
- Address: 0x36
- SDA: GPIO9
- SCL: GPIO8
- Speed: 400kHz

**Key Functions**:
```cpp
charger.init();                          // Must call first
charger.get_battery_info(&info);         // Complete battery status
charger.get_charger_status(&status);     // Charger state
charger.set_charge_enable(bool);         // Enable/disable charging
charger.set_fast_charge_current(current);// Set charge current
charger.set_charge_voltage(voltage);     // Set charge voltage
```

**Typical Charge Settings**:
- Charge Current: 1000mA (1A)
- Charge Voltage: 4200mV (4.2V for Li-ion)
- Input Limit: 2000mA (2A)
- Alert Enable: true
- SOC Alert: 10% (low battery warning)

## ✅ Prelaunch Checklist

Before deploying to production:

- [ ] Verify hardware connections (GPIO8/GPIO9/GND/Power)
- [ ] Test with simple example (see above)
- [ ] Read STRUCTURE.md for project organization
- [ ] Review CONFIGURATION.md for your specific use
- [ ] Set appropriate charge current for your battery
- [ ] Set appropriate charge voltage for your chemistry
- [ ] Enable thermistor if using external sensor
- [ ] Configure alert thresholds
- [ ] Test in expected operating conditions
- [ ] Review error handling in your integration

---

**Ready to get started?** Copy the "Basic Example" above and run it now!

For more help, see the full documentation in README.md or search the code in max77972.hpp.
