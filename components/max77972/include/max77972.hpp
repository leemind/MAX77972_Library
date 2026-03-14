#pragma once

#include <cstdint>
#include <cstring>
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

/**
 * @file max77972.hpp
 * @brief MAX77972 AccuCharge + ModelGauge Driver for ESP32-S3
 * 
 * This library provides comprehensive I2C communication with the MAX77972
 * integrated charger with fuel gauge, USB Type-C and BC1.2 detection.
 */

namespace max77972 {

// ============================================================================
// I2C CONFIGURATION
// ============================================================================

constexpr uint8_t I2C_ADDR_0x36 = 0x36;  // Primary I2C address (7-bit)
constexpr uint8_t I2C_ADDR_0x37 = 0x37;  // Secondary I2C address (7-bit)

// ============================================================================
// REGISTER ADDRESSES - CHARGER REGISTERS (0x00h - 0x7Fh)
// ============================================================================

// Interrupt and Status Registers
constexpr uint8_t CHG_INT = 0x00;            // Charger Interrupt
constexpr uint8_t CHG_DTLS_00 = 0x01;        // Charger Details 00
constexpr uint8_t CHG_DTLS_01 = 0x02;        // Charger Details 01
constexpr uint8_t CHG_DTLS_02 = 0x03;        // Charger Details 02
constexpr uint8_t CHG_CTRL = 0x04;           // Charger Control
constexpr uint8_t CHG_INT_MASK = 0x05;       // Charger Interrupt Mask
constexpr uint8_t CHG_INT_OK = 0x10;         // Charger Interrupt OK
constexpr uint8_t INOK_INT = 0x0B;           // Input OK Interrupt
constexpr uint8_t INOK_INT_MASK = 0x0C;      // Input OK Interrupt Mask

// USB Detection and Configuration
constexpr uint8_t USBC_CTRL_1 = 0x06;        // USB Type-C Control 1
constexpr uint8_t USBC_CTRL_2 = 0x07;        // USB Type-C Control 2
constexpr uint8_t USBC_CTRL_3 = 0x08;        // USB Type-C Control 3
constexpr uint8_t BC_STATUS = 0x09;          // BC Detection Status
constexpr uint8_t BC_CTRL = 0x0A;            // BC Detection Control

// Power Input Registers
constexpr uint8_t CHGIN_CTRL_0 = 0x13;       // CHGIN Control 0
constexpr uint8_t CHGIN_CTRL_1 = 0x14;       // CHGIN Control 1
constexpr uint8_t CHGIN_CTRL_2 = 0x15;       // CHGIN Control 2

// Thermistor and Die Temperature
constexpr uint8_t THM_CNFG = 0x0D;           // Thermistor Configuration
constexpr uint8_t THM_CNFG2 = 0x0E;          // Thermistor Configuration 2
constexpr uint8_t ADCCFG = 0x0F;             // ADC Configuration
constexpr uint8_t ADCCFG2 = 0x10;            // ADC Configuration 2

// Charger Configuration Registers
constexpr uint8_t CHG_CNFG_00 = 0x20;        // Charger Configuration 00
constexpr uint8_t CHG_CNFG_01 = 0x21;        // Charger Configuration 01
constexpr uint8_t CHG_CNFG_02 = 0x22;        // Charger Configuration 02 (Fast Charge Current)
constexpr uint8_t CHG_CNFG_03 = 0x23;        // Charger Configuration 03 (Regulation Voltage)
constexpr uint8_t CHG_CNFG_04 = 0x24;        // Charger Configuration 04 (AICL & Charging Current)
constexpr uint8_t CHG_CNFG_05 = 0x25;        // Charger Configuration 05 (Deep Sleep)
constexpr uint8_t CHG_CNFG_06 = 0x26;        // Charger Configuration 06 (Thermistor)
constexpr uint8_t CHG_CNFG_07 = 0x27;        // Charger Configuration 07 (Thermal Regulation 1)
constexpr uint8_t CHG_CNFG_08 = 0x28;        // Charger Configuration 08 (Thermal Regulation 2)
constexpr uint8_t CHG_CNFG_09 = 0x29;        // Charger Configuration 09
constexpr uint8_t CHG_CNFG_10 = 0x2A;        // Charger Configuration 10 (JEITA Protection 1)
constexpr uint8_t CHG_CNFG_11 = 0x2B;        // Charger Configuration 11 (JEITA Protection 2)
constexpr uint8_t CHG_CNFG_12 = 0x2C;        // Charger Configuration 12 (Input Voltage Regulation)
constexpr uint8_t CHG_CNFG_13 = 0x2D;        // Charger Configuration 13 (Charge Termination)
constexpr uint8_t CHG_CNFG_14 = 0x2E;        // Charger Configuration 14 (Watchdog Timer)
constexpr uint8_t CHG_CNFG_15 = 0x2F;        // Charger Configuration 15 (SYS Minimum Voltage)

// Boost Converter Registers
constexpr uint8_t CHG_CNFG_16 = 0x30;        // Charger Configuration 16 (Boost Mode)
constexpr uint8_t CHG_CNFG_17 = 0x31;        // Charger Configuration 17 (Boost Voltage)
constexpr uint8_t CHG_CNFG_18 = 0x32;        // Charger Configuration 18 (Boost Current Limit)

// System Registers
constexpr uint8_t COMMAND = 0xFE;            // Command Register (Reset)

// ============================================================================
// FUEL GAUGE / ModelGauge m5 REGISTERS (0x80h - 0xFFh)
// ============================================================================

// Status and Configuration
constexpr uint8_t Status = 0xB0;             // Status Register
constexpr uint8_t Status2 = 0xB1;            // Status 2 Register
constexpr uint8_t Config = 0xBD;             // Configuration Register
constexpr uint8_t Config2 = 0xBB;            // Configuration 2 Register
constexpr uint8_t FilterCfg = 0xC0;          // Filter Configuration
constexpr uint8_t ADCCfg = 0xD0;             // ADC Configuration
constexpr uint8_t ADCCfg2 = 0xD1;            // ADC Configuration 2
constexpr uint8_t ADCCfg3 = 0xD2;            // ADC Configuration 3
constexpr uint8_t RelaxCfg = 0xC2;           // Relaxation Configuration
constexpr uint8_t LearnCfg = 0xC3;           // Learn Configuration
constexpr uint8_t MaskSOC = 0xC4;            // Mask SOC Register
constexpr uint8_t PackCfg = 0xBB;            // Pack Configuration
constexpr uint8_t HibCfg = 0xBA;             // Hibernate Configuration
constexpr uint8_t NRCfg = 0xCF;              // NR Configuration

// Current Gain and Offset
constexpr uint8_t CGain = 0xD3;              // Current Gain
constexpr uint8_t COff = 0xD4;               // Current Offset
constexpr uint8_t RCOMP0 = 0xD5;             // RCOMP0 Register
constexpr uint8_t TempCo = 0xD6;             // Temperature Coefficient
constexpr uint8_t IChgTerm = 0xD7;           // Charge Termination Current
constexpr uint8_t NVCfg0 = 0xD8;             // NV Configuration 0
constexpr uint8_t NVCfg1 = 0xD9;             // NV Configuration 1
constexpr uint8_t NVCfg2 = 0xDA;             // NV Configuration 2

// Real-time Measurements
constexpr uint8_t VCell = 0xA8;              // Cell Voltage
constexpr uint8_t AvgVCell = 0xA9;           // Average Cell Voltage
constexpr uint8_t VByp = 0xAA;               // BYP Voltage
constexpr uint8_t VSys = 0xAB;               // System Voltage
constexpr uint8_t Current = 0xAC;            // Battery Current
constexpr uint8_t AvgCurrent = 0xAD;         // Average Battery Current
constexpr uint8_t Temp = 0xAE;               // Temperature
constexpr uint8_t AvgTA = 0xAF;              // Average Temperature
constexpr uint8_t ICHGIN = 0xB1;             // CHGIN Current
constexpr uint8_t DieTemp = 0xB9;            // Die Temperature
constexpr uint8_t Power = 0xBE;              // Power Register
constexpr uint8_t AvgPower = 0xBF;           // Average Power

// Voltage/Current Registers
constexpr uint8_t VOCVTab = 0xC8;            // VOCV Table
constexpr uint8_t VAlertTh = 0xC5;           // Voltage Alert Threshold
constexpr uint8_t IAlertTh = 0xC7;           // Current Alert Threshold

// Temperature and Thermistor
constexpr uint8_t TAlertTh = 0xC6;           // Temperature Alert Threshold
constexpr uint8_t ThermCfg = 0xD3;           // Thermistor Configuration
constexpr uint8_t NThermCfg = 0xC9;          // NTC Thermistor Configuration

// State of Charge and Capacity
constexpr uint8_t RepSOC = 0xB2;             // Reported State of Charge
constexpr uint8_t RepCap = 0xB3;             // Reported Capacity
constexpr uint8_t AvSOC = 0xB4;              // Average State of Charge
constexpr uint8_t AvCap = 0xB5;              // Available Capacity
constexpr uint8_t MixSOC = 0xB6;             // Mixed State of Charge
constexpr uint8_t MixCap = 0xB7;             // Mixed Capacity
constexpr uint8_t FullCapNom = 0xB8;         // Nominal Full Capacity
constexpr uint8_t FullCap = 0xB8;            // Full Capacity (same as FullCapNom)
constexpr uint8_t SAlertTh = 0xC6;           // State of Charge Alert Threshold

// Cell Characterization
constexpr uint8_t Design = 0xD8;             // Design Capacity
constexpr uint8_t OCVTable0 = 0x12;          // OCV Table 0
constexpr uint8_t OCVTable1 = 0x22;          // OCV Table 1
constexpr uint8_t OCVTable2 = 0x32;          // OCV Table 2
constexpr uint8_t OCVTable3 = 0x42;          // OCV Table 3
constexpr uint8_t OCVTable4 = 0x52;          // OCV Table 4
constexpr uint8_t OCVTable5 = 0x62;          // OCV Table 5
constexpr uint8_t RComp0 = 0xD5;             // R-Comp 0
constexpr uint8_t Mn = 0xD4;                 // Mn Register
constexpr uint8_t Mh = 0xD3;                 // Mh Register

// Cycle and Age Tracking
constexpr uint8_t Cycles = 0xBA;             // Cycle Count
constexpr uint8_t Age = 0xBC;                // Battery Age
constexpr uint8_t Qh = 0xCD;                 // Charge Count
constexpr uint8_t Ql = 0xCC;                 // Discharge Count
constexpr uint8_t JAGA = 0xCB;               // JAG Register A
constexpr uint8_t JAGBTop = 0xCA;            // JAG Register B Top (MSB)
constexpr uint8_t JAGBBot = 0xCB;            // JAG Register B Bottom (LSB)

// Time and Learning
constexpr uint8_t Time = 0xC9;               // Time Register
constexpr uint8_t Timer = 0xC9;              // Timer (same as Time)
constexpr uint8_t TTF = 0xCE;                // Time To Full
constexpr uint8_t TimeScaler = 0xCD;         // Time Scaler
constexpr uint8_t HistC = 0xCF;              // History Capacity
constexpr uint8_t HistSOC = 0xD0;            // History SOC
constexpr uint8_t HistT = 0xD1;              // History Temperature

// Learned Data Registers
constexpr uint8_t nRComp0 = 0xD5;            // nRComp0 Register
constexpr uint8_t nTempCo = 0xD6;            // nTempCo Register
constexpr uint8_t nIChgTerm = 0xD7;          // nIChgTerm Register
constexpr uint8_t nQRTable00 = 0x12;         // nQRTable for 4.4V steps
constexpr uint8_t nQRTable10 = 0x22;         // nQRTable for 4.3V steps
constexpr uint8_t nQRTable20 = 0x32;         // nQRTable for 4.2V steps
constexpr uint8_t nQRTable30 = 0x42;         // nQRTable for 4.1V steps

// Additional Cell Characterization
constexpr uint8_t CFastInt = 0xD3;           // CFastInt Register (Charge Fast Intercept)
constexpr uint8_t CFastSlope = 0xD4;         // CFastSlope Register
constexpr uint8_t Mx = 0xD5;                 // Mx Register (Cell Max Voltage)
constexpr uint8_t Mi = 0xD6;                 // Mi Register (Cell Min Voltage)

// Non-Volatile Charger Configuration (Learned Parameters)
constexpr uint8_t nChgCfg0 = 0xE0;           // nChgCfg 0 - Charger Config Block 0
constexpr uint8_t nChgCfg1 = 0xE1;           // nChgCfg 1 - Charger Config Block 1
constexpr uint8_t nChgCfg2 = 0xE2;           // nChgCfg 2 - Charger Config Block 2
constexpr uint8_t nChgCfg3 = 0xE3;           // nChgCfg 3 - Charger Config Block 3
constexpr uint8_t nChgCfg4 = 0xE4;           // nChgCfg 4 - Charger Config Block 4
constexpr uint8_t nChgCfg5 = 0xE5;           // nChgCfg 5 - Thermal Zone Boundary 1
constexpr uint8_t nChgCfg6 = 0xE6;           // nChgCfg 6 - Thermal Zone Boundary 2
constexpr uint8_t nChgCfg7 = 0xE7;           // nChgCfg 7 - Thermal Zone Boundary 3
constexpr uint8_t nChgCfg8 = 0xE8;           // nChgCfg 8 - Boost Converter Calibration
constexpr uint8_t nChgCfg9 = 0xE9;           // nChgCfg 9 - Input Current Limit Calibration
constexpr uint8_t nChgCfg10 = 0xEA;          // nChgCfg 10 - Charge Current Offset
constexpr uint8_t nChgCfg11 = 0xEB;          // nChgCfg 11 - IC Offset & Trim
constexpr uint8_t nChgCfg12 = 0xEC;          // nChgCfg 12 - Reserved for future use

// ============================================================================
// BIT FIELD DEFINITIONS
// ============================================================================

// CHG_INT Register Bits
struct ChgInt {
    uint8_t CHG_INT_BYP : 1;          // Bit 0: BYP Interrupt
    uint8_t CHG_INT_BAT : 1;          // Bit 1: Battery Interrupt
    uint8_t CHG_INT_CHG : 1;          // Bit 2: Charger Interrupt
    uint8_t CHG_INT_USBC : 1;         // Bit 3: USB Type-C Interrupt
    uint8_t CHG_INT_OTG : 1;          // Bit 4: OTG Interrupt
    uint8_t CHG_INT_THM : 1;          // Bit 5: Thermistor Interrupt
    uint8_t reserved : 2;             // Bits 6-7: Reserved
};

// CHG_DTLS_00 Register Bits - Charger State
struct ChgDtls00 {
    uint8_t CHG_DTLS : 4;             // Bits 0-3: Charger State
    uint8_t CHG_DEAD_BAT : 1;         // Bit 4: Dead Battery Mode
    uint8_t reserved : 3;             // Bits 5-7: Reserved
};

// CHG_DTLS_01 Register Bits - Temperature Zone
struct ChgDtls01 {
    uint8_t CHG_DTLS : 4;             // Bits 0-3: Temperature Zone
    uint8_t DC_MIN : 1;               // Bit 4: DC Module Minimum
    uint8_t reserved : 3;             // Bits 5-7: Reserved
};

// CHG_DTLS_02 Register Bits - Voltage and Current
struct ChgDtls02 {
    uint8_t BATT_DTL : 3;             // Bits 0-2: Battery Details
    uint8_t reserved : 5;             // Bits 3-7: Reserved
};

// CHG_CTRL Register - Charger Control
struct ChgCtrl {
    uint8_t CHG_EN : 1;               // Bit 0: Charger Enable
    uint8_t OTG_EN : 1;               // Bit 1: OTG Enable
    uint8_t reserved : 6;             // Bits 2-7: Reserved
};

// ============================================================================
// ENUMERATIONS
// ============================================================================

enum class ChargerState : uint8_t {
    DEAD_BATTERY = 0x00,
    PRECHARGING = 0x01,
    FAST_CHARGING = 0x02,
    CONSTANT_VOLTAGE = 0x03,
    TOP_OFF = 0x04,
    DONE = 0x05,
    TIMER_EXPIRED = 0x06,
    TEMP_SUSPEND = 0x07,
    OFF = 0x08,
    RESERVED = 0x0F
};

enum class TemperatureZone : uint8_t {
    COLD = 0x00,
    COOL = 0x01,
    ROOM = 0x02,
    WARM = 0x03,
    HOT = 0x04,
    RESERVED = 0x0F
};

enum class USBDetectionType : uint8_t {
    UNKNOWN = 0x00,
    SDP = 0x01,                      // Standard Downstream Port
    DCP = 0x02,                      // Dedicated Charging Port
    CDP = 0x03,                      // Charging Downstream Port
    OTG = 0x04
};

enum class ChargingCurrent : uint16_t {
    I_100MA = 0x00,
    I_125MA = 0x01,
    I_150MA = 0x02,
    I_175MA = 0x03,
    I_200MA = 0x04,
    I_225MA = 0x05,
    I_250MA = 0x06,
    I_275MA = 0x07,
    I_300MA = 0x08,
    I_325MA = 0x09,
    I_350MA = 0x0A,
    I_400MA = 0x0B,
    I_450MA = 0x0C,
    I_500MA = 0x0D,
    I_550MA = 0x0E,
    I_600MA = 0x0F,
    I_650MA = 0x10,
    I_700MA = 0x11,
    I_800MA = 0x12,
    I_900MA = 0x13,
    I_1000MA = 0x14,
    I_1100MA = 0x15,
    I_1200MA = 0x16,
    I_1300MA = 0x17,
    I_1400MA = 0x18,
    I_1500MA = 0x19,
    I_1600MA = 0x1A,
    I_1700MA = 0x1B,
    I_1800MA = 0x1C,
    I_1900MA = 0x1D,
    I_2000MA = 0x1E,
    I_2100MA = 0x1F,
    I_2200MA = 0x20,
    I_2400MA = 0x21,
    I_2600MA = 0x22,
    I_2800MA = 0x23,
    I_3000MA = 0x24,
    I_3150MA = 0x25
};

enum class ChargingVoltage : uint8_t {
    V_3400MV = 0x00,
    V_3425MV = 0x01,
    V_3450MV = 0x02,
    V_3475MV = 0x03,
    V_3500MV = 0x04,
    V_3600MV = 0x0E,
    V_4000MV = 0x20,
    V_4100MV = 0x28,
    V_4200MV = 0x30,
    V_4300MV = 0x38,
    V_4400MV = 0x40,
    V_4500MV = 0x48,
    V_4600MV = 0x50,
    V_4650MV = 0x54,
    V_4660MV = 0x55
};

// ============================================================================
// DATA STRUCTURES FOR MEASUREMENTS AND STATUS
// ============================================================================

struct BatteryInfo {
    uint16_t voltage_mv;              // Cell voltage in mV
    uint16_t avg_voltage_mv;          // Average cell voltage in mV
    int16_t current_ma;               // Battery current in mA (negative = discharge)
    int16_t avg_current_ma;           // Average battery current in mA
    uint16_t capacity_mah;            // Remaining capacity in mAh
    uint8_t state_of_charge;          // State of charge (0-100%)
    int8_t temperature_c;             // Battery temperature in °C
    int8_t die_temperature_c;         // Die temperature in °C
    uint16_t cycles;                  // Cycle count
};

struct ChargerStatus {
    ChargerState state;               // Current charger state
    TemperatureZone temp_zone;        // Current temperature zone
    USBDetectionType usb_type;        // Detected USB type
    bool is_charging;                 // Is charger active
    bool is_dead_battery;             // Dead battery mode
    bool is_temp_suspended;           // Charging suspended due to temperature
    uint16_t ichg_mv;                 // Charge voltage
    uint16_t ichg_ma;                 // Charge current
};

struct SystemVoltages {
    uint16_t vbyp_mv;                 // BYP pin voltage
    uint16_t vsys_mv;                 // SYS pin voltage
    uint16_t ichgin_ma;               // CHGIN current measurement
};

struct AlertStatus {
    bool voltage_alert;
    bool current_alert;
    bool temperature_alert;
    bool soc_alert;
    bool soc_change_1pct;
    uint8_t alert_reason;
};

// ============================================================================
// CLASS DEFINITION
// ============================================================================

class MAX77972 {
public:
    /**
     * @brief Constructor for MAX77972
     * @param port I2C port number
     * @param i2c_addr I2C slave address (typically 0x36 or 0x37)
     * @param sda_pin GPIO pin for SDA
     * @param scl_pin GPIO pin for SCL
     * @param freq_hz I2C clock frequency
     */
    MAX77972(i2c_port_t port, uint8_t i2c_addr = I2C_ADDR_0x36, 
             gpio_num_t sda_pin = GPIO_NUM_9, gpio_num_t scl_pin = GPIO_NUM_8,
             uint32_t freq_hz = 400000);

    /**
     * @brief Destructor
     */
    ~MAX77972();

    /**
     * @brief Initialize the MAX77972 device
     * @return ESP_OK on success
     */
    esp_err_t init();

    /**
     * @brief Check if device is present on I2C bus
     * @return ESP_OK if device responds
     */
    esp_err_t probe();

    // ========================================================================
    // GENERAL I2C OPERATIONS
    // ========================================================================

    /**
     * @brief Read a single 8-bit register
     * @param reg Register address
     * @param data Pointer to store read value
     * @return ESP_OK on success
     */
    esp_err_t read_register(uint8_t reg, uint8_t* data);

    /**
     * @brief Write to a single 8-bit register
     * @param reg Register address
     * @param data Value to write
     * @return ESP_OK on success
     */
    esp_err_t write_register(uint8_t reg, uint8_t data);

    /**
     * @brief Read multiple registers
     * @param reg Starting register address
     * @param data Pointer to buffer for read data
     * @param len Number of bytes to read
     * @return ESP_OK on success
     */
    esp_err_t read_registers(uint8_t reg, uint8_t* data, uint8_t len);

    /**
     * @brief Write to multiple registers
     * @param reg Starting register address
     * @param data Pointer to data to write
     * @param len Number of bytes to write
     * @return ESP_OK on success
     */
    esp_err_t write_registers(uint8_t reg, const uint8_t* data, uint8_t len);

    /**
     * @brief Perform bit field read and write
     * @param reg Register address
     * @param mask Mask for bits to modify
     * @param value New value for masked bits
     * @return ESP_OK on success
     */
    esp_err_t modify_register(uint8_t reg, uint8_t mask, uint8_t value);

    // ========================================================================
    // CHARGER CONTROL FUNCTIONS
    // ========================================================================

    /**
     * @brief Enable/disable charger
     * @param enable true to enable, false to disable
     * @return ESP_OK on success
     */
    esp_err_t set_charger_enable(bool enable);

    /**
     * @brief Set fast charge current
     * @param current Current setting (see ChargingCurrent enum)
     * @return ESP_OK on success
     */
    esp_err_t set_fast_charge_current(ChargingCurrent current);

    /**
     * @brief Get fast charge current
     * @param current Pointer to store current setting
     * @return ESP_OK on success
     */
    esp_err_t get_fast_charge_current(uint16_t* current_ma);

    /**
     * @brief Set charge termination voltage
     * @param voltage Voltage setting (see ChargingVoltage enum)
     * @return ESP_OK on success
     */
    esp_err_t set_charge_voltage(ChargingVoltage voltage);

    /**
     * @brief Get charge termination voltage
     * @param voltage_mv Pointer to store voltage in mV
     * @return ESP_OK on success
     */
    esp_err_t get_charge_voltage(uint16_t* voltage_mv);

    /**
     * @brief Enable/disable OTG (reverse charge) mode
     * @param enable true to enable OTG, false to disable
     * @return ESP_OK on success
     */
    esp_err_t set_otg_enable(bool enable);

    /**
     * @brief Set OTG current limit
     * @param current_ma Current limit in mA
     * @return ESP_OK on success
     */
    esp_err_t set_otg_current_limit(uint16_t current_ma);

    /**
     * @brief Set input current limit
     * @param current_ma Current limit in mA (100-3500mA)
     * @return ESP_OK on success
     */
    esp_err_t set_input_current_limit(uint16_t current_ma);

    /**
     * @brief Get input current limit
     * @param current_ma Pointer to store current limit
     * @return ESP_OK on success
     */
    esp_err_t get_input_current_limit(uint16_t* current_ma);

    /**
     * @brief Enable/disable Adaptive Input Current Limit (AICL)
     * @param enable true to enable AICL
     * @return ESP_OK on success
     */
    esp_err_t set_aicl_enable(bool enable);

    /**
     * @brief Enable/disable prequalification mode
     * @param enable true to enable prequalification
     * @return ESP_OK on success
     */
    esp_err_t set_prequalification_enable(bool enable);

    /**
     * @brief Set trickle charge current
     * @param current_ma Current in mA
     * @return ESP_OK on success
     */
    esp_err_t set_trickle_charge_current(uint16_t current_ma);

    /**
     * @brief Enable/disable charging
     * @param enable true to enable, false to disable
     * @return ESP_OK on success
     */
    esp_err_t set_charge_enable(bool enable);

    // ========================================================================
    // CHARGER STATUS FUNCTIONS
    // ========================================================================

    /**
     * @brief Get charger status
     * @param status Pointer to ChargerStatus structure
     * @return ESP_OK on success
     */
    esp_err_t get_charger_status(ChargerStatus* status);

    /**
     * @brief Get current charger state
     * @param state Pointer to store charger state
     * @return ESP_OK on success
     */
    esp_err_t get_charger_state(ChargerState* state);

    /**
     * @brief Get current temperature zone
     * @param zone Pointer to store temperature zone
     * @return ESP_OK on success
     */
    esp_err_t get_temperature_zone(TemperatureZone* zone);

    /**
     * @brief Get USB detection result
     * @param usb_type Pointer to store USB type
     * @return ESP_OK on success
     */
    esp_err_t get_usb_type(USBDetectionType* usb_type);

    /**
     * @brief Check if charger is actively charging
     * @param is_charging Pointer to store charging status
     * @return ESP_OK on success
     */
    esp_err_t is_charging(bool* is_charging);

    /**
     * @brief Get charger interrupt status
     * @param status Pointer to store interrupt status
     * @return ESP_OK on success
     */
    esp_err_t get_charger_interrupt(uint8_t* status);

    /**
     * @brief Clear charger interrupt flags
     * @param mask Interrupt flags to clear
     * @return ESP_OK on success
     */
    esp_err_t clear_charger_interrupt(uint8_t mask);

    // ========================================================================
    // FUEL GAUGE FUNCTIONS - VOLTAGE & CURRENT MEASUREMENTS
    // ========================================================================

    /**
     * @brief Get battery information (comprehensive measurement)
     * @param info Pointer to BatteryInfo structure
     * @return ESP_OK on success
     */
    esp_err_t get_battery_info(BatteryInfo* info);

    /**
     * @brief Read cell voltage
     * @param voltage_mv Pointer to store voltage in mV
     * @return ESP_OK on success
     */
    esp_err_t read_vcell(uint16_t* voltage_mv);

    /**
     * @brief Read average cell voltage
     * @param voltage_mv Pointer to store average voltage in mV
     * @return ESP_OK on success
     */
    esp_err_t read_avg_vcell(uint16_t* voltage_mv);

    /**
     * @brief Read battery current
     * @param current_ma Pointer to store current in mA (negative = discharge)
     * @return ESP_OK on success
     */
    esp_err_t read_current(int16_t* current_ma);

    /**
     * @brief Read average battery current
     * @param current_ma Pointer to store average current in mA
     * @return ESP_OK on success
     */
    esp_err_t read_avg_current(int16_t* current_ma);

    /**
     * @brief Read battery pack power
     * @param power_mw Pointer to store power in mW
     * @return ESP_OK on success
     */
    esp_err_t read_power(int16_t* power_mw);

    /**
     * @brief Read average power
     * @param avg_power_mw Pointer to store average power in mW
     * @return ESP_OK on success
     */
    esp_err_t read_avg_power(int16_t* avg_power_mw);

    /**
     * @brief Get system voltages (VByp, VSys, ICHGIN)
     * @param voltages Pointer to SystemVoltages structure
     * @return ESP_OK on success
     */
    esp_err_t get_system_voltages(SystemVoltages* voltages);

    /**
     * @brief Read BYP pin voltage
     * @param voltage_mv Pointer to store voltage in mV
     * @return ESP_OK on success
     */
    esp_err_t read_vbyp(uint16_t* voltage_mv);

    /**
     * @brief Read system voltage
     * @param voltage_mv Pointer to store voltage in mV
     * @return ESP_OK on success
     */
    esp_err_t read_vsys(uint16_t* voltage_mv);

    /**
     * @brief Read CHGIN input current
     * @param current_ma Pointer to store current in mA
     * @return ESP_OK on success
     */
    esp_err_t read_ichgin(uint16_t* current_ma);

    /**
     * @brief Read die temperature
     * @param temperature_c Pointer to store temperature in °C
     * @return ESP_OK on success
     */
    esp_err_t read_die_temp(int8_t* temperature_c);

    // ========================================================================
    // FUEL GAUGE FUNCTIONS - STATE OF CHARGE & CAPACITY
    // ========================================================================

    /**
     * @brief Read reported state of charge
     * @param soc Pointer to store SOC (0-100%)
     * @return ESP_OK on success
     */
    esp_err_t read_rep_soc(uint8_t* soc);

    /**
     * @brief Read mixed state of charge
     * @param soc Pointer to store SOC (0-100%)
     * @return ESP_OK on success
     */
    esp_err_t read_mix_soc(uint8_t* soc);

    /**
     * @brief Read average state of charge
     * @param soc Pointer to store average SOC (0-100%)
     * @return ESP_OK on success
     */
    esp_err_t read_av_soc(uint8_t* soc);

    /**
     * @brief Read reported remaining capacity
     * @param capacity_mah Pointer to store capacity in mAh
     * @return ESP_OK on success
     */
    esp_err_t read_rep_cap(uint16_t* capacity_mah);

    /**
     * @brief Read mixed remaining capacity
     * @param capacity_mah Pointer to store capacity in mAh
     * @return ESP_OK on success
     */
    esp_err_t read_mix_cap(uint16_t* capacity_mah);

    /**
     * @brief Read available capacity
     * @param capacity_mah Pointer to store capacity in mAh
     * @return ESP_OK on success
     */
    esp_err_t read_av_cap(uint16_t* capacity_mah);

    /**
     * @brief Read full capacity nominal
     * @param capacity_mah Pointer to store capacity in mAh
     * @return ESP_OK on success
     */
    esp_err_t read_full_cap_nom(uint16_t* capacity_mah);

    /**
     * @brief Read full capacity
     * @param capacity_mah Pointer to store capacity in mAh
     * @return ESP_OK on success
     */
    esp_err_t read_full_cap(uint16_t* capacity_mah);

    /**
     * @brief Read cycle count
     * @param cycles Pointer to store cycle count
     * @return ESP_OK on success
     */
    esp_err_t read_cycles(uint16_t* cycles);

    /**
     * @brief Read battery age
     * @param age_pct Pointer to store age as percentage (0-100%)
     * @return ESP_OK on success
     */
    esp_err_t read_age(uint8_t* age_pct);

    // ========================================================================
    // FUEL GAUGE FUNCTIONS - TEMPERATURE & ALERTS
    // ========================================================================

    /**
     * @brief Read temperature measurement
     * @param temperature_c Pointer to store temperature in °C
     * @return ESP_OK on success
     */
    esp_err_t read_temp(int8_t* temperature_c);

    /**
     * @brief Read average temperature
     * @param temperature_c Pointer to store average temperature in °C
     * @return ESP_OK on success
     */
    esp_err_t read_avg_temp(int8_t* temperature_c);

    /**
     * @brief Set thermistor configuration
     * @param therm_cfg Thermistor configuration value
     * @return ESP_OK on success
     */
    esp_err_t set_thermistor_config(uint16_t therm_cfg);

    /**
     * @brief Get fuel gauge alerts status
     * @param status Pointer to AlertStatus structure
     * @return ESP_OK on success
     */
    esp_err_t get_alert_status(AlertStatus* status);

    /**
     * @brief Enable alerting
     * @param enable true to enable alerts
     * @return ESP_OK on success
     */
    esp_err_t set_alert_enable(bool enable);

    /**
     * @brief Set voltage alert threshold
     * @param threshold_mv Threshold voltage in mV
     * @return ESP_OK on success
     */
    esp_err_t set_voltage_alert_threshold(uint16_t threshold_mv);

    /**
     * @brief Set current alert threshold
     * @param threshold_ma Threshold current in mA
     * @return ESP_OK on success
     */
    esp_err_t set_current_alert_threshold(int16_t threshold_ma);

    /**
     * @brief Set temperature alert threshold
     * @param threshold_c Threshold temperature in °C
     * @return ESP_OK on success
     */
    esp_err_t set_temperature_alert_threshold(int8_t threshold_c);

    /**
     * @brief Set SOC alert threshold
     * @param threshold_pct Threshold SOC in percentage (0-100%)
     * @return ESP_OK on success
     */
    esp_err_t set_soc_alert_threshold(uint8_t threshold_pct);

    // ========================================================================
    // ADVANCED FUEL GAUGE CONFIGURATION
    // ========================================================================

    /**
     * @brief Configure filter settings for averaging
     * @param voltage_filter Voltage filter setting
     * @param current_filter Current filter setting
     * @param temp_filter Temperature filter setting
     * @return ESP_OK on success
     */
    esp_err_t set_filter_config(uint8_t voltage_filter, uint8_t current_filter, uint8_t temp_filter);

    /**
     * @brief Configure relaxation detection
     * @param relax_cfg Relaxation configuration register value
     * @return ESP_OK on success
     */
    esp_err_t set_relaxation_config(uint8_t relax_cfg);

    /**
     * @brief Configure learning parameters
     * @param learn_cfg Learning configuration register value
     * @return ESP_OK on success
     */
    esp_err_t set_learning_config(uint8_t learn_cfg);

    /**
     * @brief Configure hibernation mode
     * @param hib_threshold Hibernation threshold in mA
     * @param hib_enable Enable hibernation mode
     * @return ESP_OK on success
     */
    esp_err_t set_hibernation_config(uint16_t hib_threshold, bool hib_enable);

    /**
     * @brief Configure ADC sampling
     * @param adc_cfg ADC configuration register value
     * @return ESP_OK on success
     */
    esp_err_t set_adc_config(uint8_t adc_cfg);

    /**
     * @brief Read ADC configuration
     * @param adc_cfg Pointer to store ADC configuration
     * @return ESP_OK on success
     */
    esp_err_t get_adc_config(uint8_t* adc_cfg);

    // ========================================================================
    // CELL CHARACTERIZATION & LEARNING DATA
    // ========================================================================

    /**
     * @brief Set charge termination current
     * @param iterm_ma Termination current in mA
     * @return ESP_OK on success
     */
    esp_err_t set_charge_termination_current(uint16_t iterm_ma);

    /**
     * @brief Get charge termination current
     * @param iterm_ma Pointer to store termination current in mA
     * @return ESP_OK on success
     */
    esp_err_t get_charge_termination_current(uint16_t* iterm_ma);

    /**
     * @brief Set resistance compensation (RComp)
     * @param rcomp New RComp value
     * @return ESP_OK on success
     */
    esp_err_t set_rcomp(uint8_t rcomp);

    /**
     * @brief Get resistance compensation (RComp)
     * @param rcomp Pointer to store RComp value
     * @return ESP_OK on success
     */
    esp_err_t get_rcomp(uint8_t* rcomp);

    /**
     * @brief Set temperature coefficient
     * @param tempco Temperature coefficient value
     * @return ESP_OK on success
     */
    esp_err_t set_temp_coefficient(uint8_t tempco);

    /**
     * @brief Get temperature coefficient
     * @param tempco Pointer to store temperature coefficient
     * @return ESP_OK on success
     */
    esp_err_t get_temp_coefficient(uint8_t* tempco);

    /**
     * @brief Set current gain and offset
     * @param cgain Current gain register value
     * @param coff Current offset register value
     * @return ESP_OK on success
     */
    esp_err_t set_current_calibration(uint8_t cgain, uint8_t coff);

    /**
     * @brief Get current gain and offset
     * @param cgain Pointer to store current gain
     * @param coff Pointer to store current offset
     * @return ESP_OK on success
     */
    esp_err_t get_current_calibration(uint8_t* cgain, uint8_t* coff);

    /**
     * @brief Read design capacity
     * @param capacity_mah Pointer to store design capacity in mAh
     * @return ESP_OK on success
     */
    esp_err_t read_design_capacity(uint16_t* capacity_mah);

    /**
     * @brief Write design capacity
     * @param capacity_mah Design capacity in mAh
     * @return ESP_OK on success
     */
    esp_err_t write_design_capacity(uint16_t capacity_mah);

    /**
     * @brief Read OCV table entry
     * @param table_index Table index (0-5)
     * @param data Pointer to store OCV table data (16 bytes)
     * @return ESP_OK on success
     */
    esp_err_t read_ocv_table(uint8_t table_index, uint8_t* data);

    /**
     * @brief Write OCV table entry
     * @param table_index Table index (0-5)
     * @param data OCV table data (16 bytes)
     * @return ESP_OK on success
     */
    esp_err_t write_ocv_table(uint8_t table_index, const uint8_t* data);

    // ========================================================================
    // ADVANCED CHARGER CONFIGURATION
    // ========================================================================

    /**
     * @brief Set CHGIN current measurement offset
     * @param offset Offset value
     * @return ESP_OK on success
     */
    esp_err_t set_chgin_offset(uint8_t offset);

    /**
     * @brief Set external input voltage regulation
     * @param voltage_mv Input regulation voltage in mV
     * @return ESP_OK on success
     */
    esp_err_t set_chgin_regulation_voltage(uint16_t voltage_mv);

    /**
     * @brief Get external input voltage regulation
     * @param voltage_mv Pointer to store regulation voltage in mV
     * @return ESP_OK on success
     */
    esp_err_t get_chgin_regulation_voltage(uint16_t* voltage_mv);

    /**
     * @brief Set system minimum voltage
     * @param voltage_mv Minimum system voltage in mV (default 3500mV)
     * @return ESP_OK on success
     */
    esp_err_t set_sys_min_voltage(uint16_t voltage_mv);

    /**
     * @brief Set watchdog timer
     * @param timeout_sec Watchdog timeout in seconds (0 to disable)
     * @return ESP_OK on success
     */
    esp_err_t set_watchdog_timer(uint8_t timeout_sec);

    /**
     * @brief Clear watchdog timer
     * @return ESP_OK on success
     */
    esp_err_t clear_watchdog();

    /**
     * @brief Enable OTG (Reverse Boost) mode
     * @param enable true to enable OTG
     * @return ESP_OK on success
     */
    esp_err_t set_otg_mode(bool enable);

    /**
     * @brief Set OTG voltage (when in reverse boost)
     * @param voltage_mv OTG output voltage in mV (typically 5100mV)
     * @return ESP_OK on success
     */
    esp_err_t set_otg_voltage(uint16_t voltage_mv);

    /**
     * @brief Configure boost converter
     * @param boost_enable Enable boost converter
     * @param boost_voltage Boost voltage in mV
     * @return ESP_OK on success
     */
    esp_err_t configure_boost_converter(bool boost_enable, uint16_t boost_voltage);

    /**
     * @brief Set precharge configuration
     * @param vprechg_mv Precharge voltage threshold
     * @param iprechg_ma Precharge current
     * @return ESP_OK on success
     */
    esp_err_t set_precharge_config(uint16_t vprechg_mv, uint16_t iprechg_ma);

    /**
     * @brief Set hysteresis for fast charge restart
     * @param hysteresis_mv Charger restart hysteresis in mV
     * @return ESP_OK on success
     */
    esp_err_t set_fast_charge_hysteresis(uint16_t hysteresis_mv);

    // ========================================================================
    // NON-VOLATILE CHARGER CONFIGURATION (nChgConfig)
    // ========================================================================

    /**
     * @brief Read nChgConfig register block
     * @param start_index Starting block index (0-12)
     * @param data Pointer to buffer for read data
     * @param len Number of bytes to read (max 13 for all blocks)
     * @return ESP_OK on success
     */
    esp_err_t read_nchg_config(uint8_t start_index, uint8_t* data, uint8_t len);

    /**
     * @brief Write nChgConfig register block
     * @param start_index Starting block index (0-12)
     * @param data Pointer to data to write
     * @param len Number of bytes to write
     * @return ESP_OK on success
     */
    esp_err_t write_nchg_config(uint8_t start_index, const uint8_t* data, uint8_t len);

    /**
     * @brief Read all nChgConfig blocks (complete configuration)
     * @param data Pointer to buffer for all 13 bytes of nChgConfig
     * @return ESP_OK on success
     */
    esp_err_t read_all_nchg_config(uint8_t* data);

    /**
     * @brief Write all nChgConfig blocks (complete configuration)
     * @param data Pointer to all 13 bytes of nChgConfig data
     * @return ESP_OK on success
     */
    esp_err_t write_all_nchg_config(const uint8_t* data);

    /**
     * @brief Get learned charger configuration block 0
     * @param config Pointer to store nChgCfg0 value
     * @return ESP_OK on success
     */
    esp_err_t get_nchg_config_0(uint8_t* config);

    /**
     * @brief Set learned charger configuration block 0
     * @param config nChgCfg0 value to write
     * @return ESP_OK on success
     */
    esp_err_t set_nchg_config_0(uint8_t config);

    /**
     * @brief Get learned charger configuration block 1
     * @param config Pointer to store nChgCfg1 value
     * @return ESP_OK on success
     */
    esp_err_t get_nchg_config_1(uint8_t* config);

    /**
     * @brief Set learned charger configuration block 1
     * @param config nChgCfg1 value to write
     * @return ESP_OK on success
     */
    esp_err_t set_nchg_config_1(uint8_t config);

    /**
     * @brief Get learned charger configuration block 2
     * @param config Pointer to store nChgCfg2 value
     * @return ESP_OK on success
     */
    esp_err_t get_nchg_config_2(uint8_t* config);

    /**
     * @brief Set learned charger configuration block 2
     * @param config nChgCfg2 value to write
     * @return ESP_OK on success
     */
    esp_err_t set_nchg_config_2(uint8_t config);

    /**
     * @brief Get boost converter calibration
     * @param calib Pointer to store boost calibration value (nChgCfg8)
     * @return ESP_OK on success
     */
    esp_err_t get_boost_calibration(uint8_t* calib);

    /**
     * @brief Set boost converter calibration
     * @param calib Boost calibration value (nChgCfg8)
     * @return ESP_OK on success
     */
    esp_err_t set_boost_calibration(uint8_t calib);

    /**
     * @brief Get input current limit calibration
     * @param calib Pointer to store input current calibration (nChgCfg9)
     * @return ESP_OK on success
     */
    esp_err_t get_input_current_calib(uint8_t* calib);

    /**
     * @brief Set input current limit calibration
     * @param calib Input current calibration value (nChgCfg9)
     * @return ESP_OK on success
     */
    esp_err_t set_input_current_calib(uint8_t calib);

    /**
     * @brief Get charge current offset
     * @param offset Pointer to store charge current offset (nChgCfg10)
     * @return ESP_OK on success
     */
    esp_err_t get_charge_current_offset(uint8_t* offset);

    /**
     * @brief Set charge current offset
     * @param offset Charge current offset value (nChgCfg10)
     * @return ESP_OK on success
     */
    esp_err_t set_charge_current_offset(uint8_t offset);

    /**
     * @brief Get IC offset and trim values
     * @param offset_trim Pointer to store IC offset/trim value (nChgCfg11)
     * @return ESP_OK on success
     */
    esp_err_t get_ic_offset_trim(uint8_t* offset_trim);

    /**
     * @brief Set IC offset and trim values
     * @param offset_trim IC offset/trim value (nChgCfg11)
     * @return ESP_OK on success
     */
    esp_err_t set_ic_offset_trim(uint8_t offset_trim);

    /**
     * @brief Get thermal zone boundary 1 (nChgCfg5)
     * @param boundary Pointer to store boundary temperature setting
     * @return ESP_OK on success
     */
    esp_err_t get_thermal_zone_boundary_1(uint8_t* boundary);

    /**
     * @brief Set thermal zone boundary 1 (nChgCfg5)
     * @param boundary Boundary temperature setting
     * @return ESP_OK on success
     */
    esp_err_t set_thermal_zone_boundary_1(uint8_t boundary);

    /**
     * @brief Get thermal zone boundary 2 (nChgCfg6)
     * @param boundary Pointer to store boundary temperature setting
     * @return ESP_OK on success
     */
    esp_err_t get_thermal_zone_boundary_2(uint8_t* boundary);

    /**
     * @brief Set thermal zone boundary 2 (nChgCfg6)
     * @param boundary Boundary temperature setting
     * @return ESP_OK on success
     */
    esp_err_t set_thermal_zone_boundary_2(uint8_t boundary);

    /**
     * @brief Get thermal zone boundary 3 (nChgCfg7)
     * @param boundary Pointer to store boundary temperature setting
     * @return ESP_OK on success
     */
    esp_err_t get_thermal_zone_boundary_3(uint8_t* boundary);

    /**
     * @brief Set thermal zone boundary 3 (nChgCfg7)
     * @param boundary Boundary temperature setting
     * @return ESP_OK on success
     */
    esp_err_t set_thermal_zone_boundary_3(uint8_t boundary);

    // ========================================================================
    // CONFIGURATION & RESET FUNCTIONS
    // ========================================================================

    /**
     * @brief Perform soft reset of the device
     * @return ESP_OK on success
     */
    esp_err_t soft_reset();

    /**
     * @brief Enter ship mode (low power state)
     * @return ESP_OK on success
     */
    esp_err_t enter_ship_mode();

    /**
     * @brief Enter deep sleep mode
     * @return ESP_OK on success
     */
    esp_err_t enter_deep_sleep_mode();

    /**
     * @brief Set JEITA temperature zone configuration
     * @param zone_idx Zone index (0-8)
     * @param reg_value Register value for the zone
     * @return ESP_OK on success
     */
    esp_err_t set_jeita_zone_config(uint8_t zone_idx, uint8_t reg_value);

    /**
     * @brief Get device status register
     * @param status Pointer to store status
     * @return ESP_OK on success
     */
    esp_err_t get_status(uint8_t* status);

    /**
     * @brief Clear all interrupt flags
     * @return ESP_OK on success
     */
    esp_err_t clear_interrupts();

    // ========================================================================
    // USB DETECTION FUNCTIONS
    // ========================================================================

    /**
     * @brief Manually trigger USB detection
     * @return ESP_OK on success
     */
    esp_err_t manual_usb_detection();

    /**
     * @brief Get USB Type-C CC pin status
     * @param cc1_status Pointer to store CC1 status
     * @param cc2_status Pointer to store CC2 status
     * @return ESP_OK on success
     */
    esp_err_t get_cc_pin_status(uint8_t* cc1_status, uint8_t* cc2_status);

    // ========================================================================
    // UTILITY FUNCTIONS
    // ========================================================================

    /**
     * @brief Dump all critical registers to log
     * @return ESP_OK on success
     */
    esp_err_t dump_registers();

    /**
     * @brief Get firmware version
     * @param version Pointer to store version
     * @return ESP_OK on success
     */
    esp_err_t get_firmware_version(uint8_t* version);

    /**
     * @brief Convert raw 16-bit register value to signed value
     * @param raw Raw register value
     * @return Converted signed value
     */
    static int16_t convert_raw_to_signed(uint16_t raw);

    /**
     * @brief Convert raw voltage register value to mV
     * @param raw Raw register value
     * @return Voltage in mV
     */
    static uint16_t convert_raw_voltage(uint16_t raw);

    /**
     * @brief Convert raw current register value to mA
     * @param raw Raw register value
     * @return Current in mA (signed)
     */
    static int16_t convert_raw_current(uint16_t raw);

    /**
     * @brief Convert raw temperature register value to °C
     * @param raw Raw register value
     * @return Temperature in °C
     */
    static int8_t convert_raw_temperature(uint16_t raw);

    /**
     * @brief Convert raw capacity register value to mAh
     * @param raw Raw register value
     * @return Capacity in mAh
     */
    static uint16_t convert_raw_capacity(uint16_t raw);

private:
    i2c_port_t i2c_port;
    uint8_t device_addr;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    uint32_t i2c_freq;
    bool initialized;
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
};

} // namespace max77972
