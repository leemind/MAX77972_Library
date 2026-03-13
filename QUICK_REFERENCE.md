# MAX77972 Quick Reference Guide

## Typical Initialization Sequence

```cpp
#include "max77972.hpp"
using namespace max77972;

// 1. Create instance
MAX77972 charger(I2C_NUM_0, I2C_ADDR_0x36, GPIO_NUM_9, GPIO_NUM_8, 400000);

// 2. Initialize
ESP_ERROR_CHECK(charger.init());

// 3. Verify device is present
ESP_ERROR_CHECK(charger.probe());

// 4. Configure charging
ESP_ERROR_CHECK(charger.set_charge_enable(true));
ESP_ERROR_CHECK(charger.set_fast_charge_current(ChargingCurrent::I_1000MA));
ESP_ERROR_CHECK(charger.set_charge_voltage(ChargingVoltage::V_4200MV));
ESP_ERROR_CHECK(charger.set_input_current_limit(1500));

// 5. Enable monitoring
ESP_ERROR_CHECK(charger.set_alert_enable(true));
```

## Common Use Cases

### Reading Battery Status
```cpp
uint8_t soc;
uint16_t voltage;
int16_t current;

charger.read_rep_soc(&soc);
charger.read_vcell(&voltage);
charger.read_current(&current);

printf("Battery: %d%% | %umV | %dmA\n", soc, voltage, current);
```

### Detecting Charging Completion
```cpp
ChargerStatus status;
charger.get_charger_status(&status);

if (status.state == ChargerState::DONE) {
    printf("Charging complete!\n");
}
```

### Monitoring Temperature
```cpp
int8_t battery_temp, die_temp;
charger.read_temp(&battery_temp);
charger.read_die_temp(&die_temp);

if (battery_temp > 45) {
    // Too hot - may trigger thermal suspend
    charger.set_charge_enable(false);
}
```

### Handling Low Battery Warnings
```cpp
AlertStatus alerts;
charger.get_alert_status(&alerts);

if (alerts.soc_alert) {
    uint8_t soc;
    charger.read_rep_soc(&soc);
    if (soc < 10) {
        printf("Low battery warning: %d%%\n", soc);
    }
}
```

### Reading Input Power Information
```cpp
SystemVoltages voltages;
charger.get_system_voltages(&voltages);

printf("Input: %umV, %umA\n", voltages.vbyp_mv, voltages.ichgin_ma);
printf("System: %umV\n", voltages.vsys_mv);
```

### Setting Custom Alert Thresholds
```cpp
// Alert if voltage drops below 3.0V
charger.set_voltage_alert_threshold(3000);

// Alert if SOC drops below 15%
charger.set_soc_alert_threshold(15);

// Alert if temperature exceeds 50°C
charger.set_temperature_alert_threshold(50);
```

## Charge Current Reference

```cpp
// Predefined charge current values
ChargingCurrent::I_100MA    // ~100 mA
ChargingCurrent::I_200MA    // ~200 mA
ChargingCurrent::I_500MA    // ~500 mA
ChargingCurrent::I_1000MA   // ~1000 mA (1A)
ChargingCurrent::I_1500MA   // ~1500 mA (1.5A)
ChargingCurrent::I_2000MA   // ~2000 mA (2A)
ChargingCurrent::I_2500MA   // ~2500 mA (2.5A)
ChargingCurrent::I_3000MA   // ~3000 mA (3A)
ChargingCurrent::I_3150MA   // ~3150 mA (max)
```

## Charge Voltage Reference

```cpp
// Predefined charge voltages
ChargingVoltage::V_3400MV   // 3.4V  (dead battery recovery)
ChargingVoltage::V_3600MV   // 3.6V
ChargingVoltage::V_4000MV   // 4.0V
ChargingVoltage::V_4100MV   // 4.1V
ChargingVoltage::V_4200MV   // 4.2V  (typical Li-ion)
ChargingVoltage::V_4300MV   // 4.3V
ChargingVoltage::V_4400MV   // 4.4V
ChargingVoltage::V_4500MV   // 4.5V
ChargingVoltage::V_4600MV   // 4.6V  (high-energy cells)
ChargingVoltage::V_4650MV   // 4.65V
ChargingVoltage::V_4660MV   // 4.66V (max)
```

## I2C Timing

| Operation | Typical Time |
|-----------|-------------|
| Register write | 1-2 ms |
| Register read | 1-2 ms |
| Multi-register read (2 bytes) | 1-2 ms |
| Voltage/current update | 1.4 s |
| Temperature update | 1.4 s |
| System voltage update | 2.8 s |
| USB detection | < 200 ms |
| Full battery info read | 10-20 ms |

## Conversion Factors

Used internally, useful for custom conversions:

```cpp
// Voltage: 78.125 µV per LSB
// Capacity: 0.5 mAh per LSB (10mΩ sense)
// Current: 0.15625 mA per LSB (10mΩ sense)
// Temperature: 1°C per LSB (upper byte)
// SOC: 1% per LSB
// Cycles: 1 cycle per LSB
```

## Thermistor Configuration Values

For common thermistor types:

```cpp
// Murata NCP15XH103F03RC (10kΩ, Beta=3435)
charger.set_thermistor_config(0x71DE);

// Semitec 103AT-2 (10kΩ, Beta=3435)
charger.set_thermistor_config(0x91C3);

// Murata NCU15WF104F6SRC (100kΩ, Beta=4250)
charger.set_thermistor_config(0x48EB);

// TDK NTCG064EF104FTBX (100kΩ, Beta=4225)
charger.set_thermistor_config(0x58EF);
```

## USB Type Identification

| Type | Meaning | Max Current |
|------|---------|------------|
| SDP | Standard Downstream Port | 500mA |
| DCP | Dedicated Charging Port | 1500mA |
| CDP | Charging Downstream Port | 500-1500mA |
| OTG | On-The-Go (device acts as charger) | 1500mA output |

## Temperature Zone Mapping

| Zone | Range | Typical Use |
|------|-------|------------|
| COLD | < 0°C | Reduced current |
| COOL | 0-10°C | Reduced current |
| ROOM | 10-45°C | Full charging |
| WARM | 45-60°C | Reduced current |
| HOT | > 60°C | No charging |

## Charger State Machine

```
DEAD_BATTERY → PRECHARGING → FAST_CHARGING → CONSTANT_VOLTAGE → TOP_OFF → DONE
                                    ↓
                            TEMP_SUSPEND
                                    ↓
                                   OFF
```

## Diagnostic Command Sequence

```cpp
// Check device health
uint8_t status;
charger.get_status(&status);

// Get full charger state
ChargerState state;
charger.get_charger_state(&state);

// Check temperature zone
TemperatureZone zone;
charger.get_temperature_zone(&zone);

// Read all battery info
BatteryInfo info;
charger.get_battery_info(&info);

// Check for alerts
AlertStatus alerts;
charger.get_alert_status(&alerts);

// Dump all registers for debugging
charger.dump_registers();
```

## Power Input Specifications

| Parameter | Min | Typ | Max |
|-----------|-----|-----|-----|
| CHGIN Voltage | 4.5V | — | 13.7V |
| CHGIN Overvoltage Protection | — | — | 16V |
| Input Current (typical) | — | — | 3.2A RMS |
| Soft Start Time | 1.5 ms | — | — |
| Input Current Limit Range | 100mA | — | 3500mA |

## Recommended Configuration for Typical Battery

```cpp
// 3000mAh Li-ion battery
void setup_typical_battery() {
    MAX77972 charger(I2C_NUM_0, I2C_ADDR_0x36);
    charger.init();
    
    // Charge with 1.5A current
    charger.set_fast_charge_current(ChargingCurrent::I_1500MA);
    
    // To 4.2V (standard Li-ion)
    charger.set_charge_voltage(ChargingVoltage::V_4200MV);
    
    // Accept up to 2A from power supply
    charger.set_input_current_limit(2000);
    
    // Enable adaptive input current limiting
    charger.set_aicl_enable(true);
    
    // Enable all protections
    charger.set_alert_enable(true);
    charger.set_soc_alert_threshold(10);      // Low battery at 10%
    charger.set_temperature_alert_threshold(55); // Thermal warning
    
    // Enable charging
    charger.set_charge_enable(true);
}
```

## Error Recovery

```cpp
esp_err_t ret = charger.read_vcell(&voltage);
if (ret == ESP_ERR_INVALID_STATE) {
    // Device not initialized
    charger.init();
    charger.probe();
} else if (ret == ESP_FAIL) {
    // I2C communication error - retry
    vTaskDelay(pdMS_TO_TICKS(100));
    charger.read_vcell(&voltage);
} else if (ret == ESP_OK) {
    // Success
}
```

## Performance Tips

1. **Batch Reading**: Read multiple values sequentially to minimize I2C transactions
2. **Update Rate**: Battery measurements update every 1.4s, so don't poll faster
3. **Alert-Driven**: Use interrupts instead of polling for alerts if ALRT pin available
4. **Minimize Context Switches**: Keep I2C operations in same task
5. **Buffer Results**: Cache frequently-read values with sensible update intervals

## Debug Logging

Enable ESP-IDF logging to see detailed library operations:

```bash
# Set log level to DEBUG
idf.py menuconfig
# Component config → ESP System Settings → Log output → Default log verbosity → DEBUG
```

In code:
```cpp
esp_log_level_set("MAX77972", ESP_LOG_DEBUG);
```

## Lifetime Estimation

Battery cycle count provides wear estimation:

```cpp
uint16_t cycles;
charger.read_cycles(&cycles);

// Typical battery rated for 500-1000 cycles
float health = 100.0f * (500 - cycles) / 500;  // For 500-cycle battery
if (health < 80) {
    printf("Battery health degraded to %.1f%%\n", health);
}
```

---

For more details, see the full API documentation in README.md
