#include "max77972.hpp"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "MAX77972";

namespace max77972 {

// ============================================================================
// CONSTRUCTOR & DESTRUCTOR
// ============================================================================

MAX77972::MAX77972(i2c_port_t port, uint8_t i2c_addr, 
                   gpio_num_t sda, gpio_num_t scl,
                   uint32_t freq_hz)
    : i2c_port(port), device_addr(i2c_addr), sda_pin(sda), scl_pin(scl),
      i2c_freq(freq_hz), initialized(false), bus_handle{nullptr}, dev_handle{nullptr} {
}

MAX77972::~MAX77972() {
    if (initialized) {
        if (dev_handle != nullptr) {
            i2c_master_bus_rm_device(dev_handle);
        }
        if (bus_handle != nullptr) {
            i2c_del_master_bus(bus_handle);
        }
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

esp_err_t MAX77972::init() {
    if (initialized) {
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;

    // Configure I2C master bus
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = i2c_port,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = true,
        }
    };

    ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure I2C device
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_addr,
        .scl_speed_hz = i2c_freq,
        .scl_wait_high_cycles = 0,
        .flags = {
            .disable_ack_check = false,
        }
    };

    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(ret));
        i2c_del_master_bus(bus_handle);
        return ret;
    }

    initialized = true;
    ESP_LOGI(TAG, "MAX77972 initialized successfully at address 0x%02X", device_addr);

    return ESP_OK;
}

esp_err_t MAX77972::probe() {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data;
    esp_err_t ret = read_register(0x00, &data);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "MAX77972 device discovered at 0x%02X", device_addr);
    } else {
        ESP_LOGE(TAG, "Failed to communicate with MAX77972 at 0x%02X", device_addr);
    }

    return ret;
}

// ============================================================================
// BASIC I2C OPERATIONS
// ============================================================================

esp_err_t MAX77972::read_register(uint8_t reg, uint8_t* data) {
    if (!initialized || dev_handle == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    if (data == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg, 1, data, 1, -1);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Read register 0x%02X failed: %s", reg, esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t MAX77972::write_register(uint8_t reg, uint8_t data) {
    if (!initialized || dev_handle == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t write_buf[2] = {reg, data};
    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, 2, -1);

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Write register 0x%02X failed: %s", reg, esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t MAX77972::read_registers(uint8_t reg, uint8_t* data, uint8_t len) {
    if (!initialized || dev_handle == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    if (data == nullptr || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, -1);

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Read registers 0x%02X failed: %s", reg, esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t MAX77972::write_registers(uint8_t reg, const uint8_t* data, uint8_t len) {
    if (!initialized || dev_handle == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    if (data == nullptr || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t write_buf[256];
    if (len + 1 > sizeof(write_buf)) {
        return ESP_ERR_INVALID_ARG;
    }

    write_buf[0] = reg;
    std::memcpy(&write_buf[1], data, len);

    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, len + 1, -1);

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Write registers 0x%02X failed: %s", reg, esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t MAX77972::modify_register(uint8_t reg, uint8_t mask, uint8_t value) {
    uint8_t data;
    esp_err_t ret = read_register(reg, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    data = (data & ~mask) | (value & mask);
    return write_register(reg, data);
}

// ============================================================================
// CHARGER CONTROL FUNCTIONS
// ============================================================================

esp_err_t MAX77972::set_charger_enable(bool enable) {
    uint8_t value = enable ? 0x01 : 0x00;
    return modify_register(CHG_CTRL, 0x01, value);
}

esp_err_t MAX77972::set_fast_charge_current(ChargingCurrent current) {
    // Fast charge current is typically in CHG_CNFG_02 or similar
    // Register layout varies - this is a placeholder for the actual register
    uint8_t reg = CHG_CNFG_02;
    uint8_t value = static_cast<uint8_t>(current);
    return modify_register(reg, 0xFF, value);
}

esp_err_t MAX77972::get_fast_charge_current(uint16_t* current_ma) {
    if (current_ma == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data;
    esp_err_t ret = read_register(CHG_CNFG_02, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    // Convert register value to mA (step size is 50mA typically)
    *current_ma = 100 + (data * 50);
    return ESP_OK;
}

esp_err_t MAX77972::set_charge_voltage(ChargingVoltage voltage) {
    uint8_t reg = CHG_CNFG_03;
    uint8_t value = static_cast<uint8_t>(voltage);
    return modify_register(reg, 0xFF, value);
}

esp_err_t MAX77972::get_charge_voltage(uint16_t* voltage_mv) {
    if (voltage_mv == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data;
    esp_err_t ret = read_register(CHG_CNFG_03, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    // Convert register value to mV (step size is 12.5mV)
    *voltage_mv = 3400 + (data * 12);
    return ESP_OK;
}

esp_err_t MAX77972::set_otg_enable(bool enable) {
    uint8_t value = enable ? 0x02 : 0x00;
    return modify_register(CHG_CTRL, 0x02, value);
}

esp_err_t MAX77972::set_otg_current_limit(uint16_t current_ma) {
    // OTG current limit typically in range 500-1500mA
    // Register value is usually (current_ma - 500) / 100
    if (current_ma < 500 || current_ma > 1500) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t value = (current_ma - 500) / 100;
    return modify_register(CHG_CNFG_04, 0x0F, value);
}

esp_err_t MAX77972::set_input_current_limit(uint16_t current_ma) {
    if (current_ma < 100 || current_ma > 3500) {
        return ESP_ERR_INVALID_ARG;
    }

    // Input current limit step is typically 100mA
    uint8_t value = (current_ma - 100) / 100;
    if (value > 0x3F) {
        value = 0x3F;
    }

    return modify_register(CHGIN_CTRL_1, 0x3F, value);
}

esp_err_t MAX77972::get_input_current_limit(uint16_t* current_ma) {
    if (current_ma == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data;
    esp_err_t ret = read_register(CHGIN_CTRL_1, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    data &= 0x3F;
    *current_ma = 100 + (data * 100);
    return ESP_OK;
}

esp_err_t MAX77972::set_aicl_enable(bool enable) {
    uint8_t value = enable ? 0x80 : 0x00;
    return modify_register(CHG_CNFG_04, 0x80, value);
}

esp_err_t MAX77972::set_prequalification_enable(bool enable) {
    uint8_t value = enable ? 0x10 : 0x00;
    return modify_register(CHG_CNFG_01, 0x10, value);
}

esp_err_t MAX77972::set_trickle_charge_current(uint16_t current_ma) {
    // Trickle charge current typically 200-400mA
    if (current_ma < 200 || current_ma > 400) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t value = (current_ma - 200) / 50;
    return modify_register(CHG_CNFG_01, 0x0F, value);
}

esp_err_t MAX77972::set_charge_enable(bool enable) {
    uint8_t value = enable ? 0x01 : 0x00;
    return modify_register(CHG_CNFG_00, 0x01, value);
}

// ============================================================================
// CHARGER STATUS FUNCTIONS
// ============================================================================

esp_err_t MAX77972::get_charger_status(ChargerStatus* status) {
    if (status == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    uint8_t data;

    // Read charger state
    ret = read_register(CHG_DTLS_00, &data);
    if (ret != ESP_OK) {
        return ret;
    }
    status->state = static_cast<ChargerState>(data & 0x0F);
    status->is_dead_battery = (data & 0x10) != 0;

    // Read temperature zone
    ret = read_register(CHG_DTLS_01, &data);
    if (ret != ESP_OK) {
        return ret;
    }
    status->temp_zone = static_cast<TemperatureZone>(data & 0x0F);

    // Read control register for enable status
    ret = read_register(CHG_CTRL, &data);
    if (ret != ESP_OK) {
        return ret;
    }
    status->is_charging = (data & 0x01) != 0;

    return ESP_OK;
}

esp_err_t MAX77972::get_charger_state(ChargerState* state) {
    if (state == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data;
    esp_err_t ret = read_register(CHG_DTLS_00, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    *state = static_cast<ChargerState>(data & 0x0F);
    return ESP_OK;
}

esp_err_t MAX77972::get_temperature_zone(TemperatureZone* zone) {
    if (zone == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data;
    esp_err_t ret = read_register(CHG_DTLS_01, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    *zone = static_cast<TemperatureZone>(data & 0x0F);
    return ESP_OK;
}

esp_err_t MAX77972::get_usb_type(USBDetectionType* usb_type) {
    if (usb_type == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data;
    esp_err_t ret = read_register(CHG_DTLS_02, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    // Extract USB type from battery detail bits
    uint8_t usb_bits = (data >> 2) & 0x03;
    *usb_type = static_cast<USBDetectionType>(usb_bits);
    return ESP_OK;
}

esp_err_t MAX77972::is_charging(bool* is_charging) {
    if (is_charging == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data;
    esp_err_t ret = read_register(CHG_CTRL, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    *is_charging = (data & 0x01) != 0;
    return ESP_OK;
}

esp_err_t MAX77972::get_charger_interrupt(uint8_t* status) {
    if (status == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    return read_register(CHG_INT, status);
}

esp_err_t MAX77972::clear_charger_interrupt(uint8_t mask) {
    uint8_t data;
    esp_err_t ret = read_register(CHG_INT, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    // Writing 1 typically clears the bit
    return write_register(CHG_INT, data | mask);
}

// ============================================================================
// FUEL GAUGE - VOLTAGE & CURRENT MEASUREMENTS
// ============================================================================

esp_err_t MAX77972::get_battery_info(BatteryInfo* info) {
    if (info == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    // Read all voltage and current values
    ret = read_vcell(&info->voltage_mv);
    if (ret != ESP_OK) return ret;

    ret = read_avg_vcell(&info->avg_voltage_mv);
    if (ret != ESP_OK) return ret;

    ret = read_current(&info->current_ma);
    if (ret != ESP_OK) return ret;

    ret = read_avg_current(&info->avg_current_ma);
    if (ret != ESP_OK) return ret;

    ret = read_rep_cap(&info->capacity_mah);
    if (ret != ESP_OK) return ret;

    ret = read_rep_soc(&info->state_of_charge);
    if (ret != ESP_OK) return ret;

    ret = read_temp(&info->temperature_c);
    if (ret != ESP_OK) return ret;

    ret = read_die_temp(&info->die_temperature_c);
    if (ret != ESP_OK) return ret;

    ret = read_cycles(&info->cycles);
    if (ret != ESP_OK) return ret;

    return ESP_OK;
}

esp_err_t MAX77972::read_vcell(uint16_t* voltage_mv) {
    if (voltage_mv == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(VCell, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 4) | (data[1] >> 4);
    *voltage_mv = convert_raw_voltage(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_avg_vcell(uint16_t* voltage_mv) {
    if (voltage_mv == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(AvgVCell, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 4) | (data[1] >> 4);
    *voltage_mv = convert_raw_voltage(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_current(int16_t* current_ma) {
    if (current_ma == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(Current, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    *current_ma = convert_raw_current(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_avg_current(int16_t* current_ma) {
    if (current_ma == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(AvgCurrent, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    *current_ma = convert_raw_current(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_power(int16_t* power_mw) {
    if (power_mw == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(Power, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    // Power LSB is typically 12.5mW
    int16_t raw = (int16_t)((data[0] << 8) | data[1]);
    *power_mw = raw * 12.5;
    return ESP_OK;
}

esp_err_t MAX77972::read_avg_power(int16_t* avg_power_mw) {
    if (avg_power_mw == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(AvgPower, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    int16_t raw = (int16_t)((data[0] << 8) | data[1]);
    *avg_power_mw = raw * 12.5;
    return ESP_OK;
}

esp_err_t MAX77972::get_system_voltages(SystemVoltages* voltages) {
    if (voltages == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    ret = read_vbyp(&voltages->vbyp_mv);
    if (ret != ESP_OK) return ret;

    ret = read_vsys(&voltages->vsys_mv);
    if (ret != ESP_OK) return ret;

    ret = read_ichgin(&voltages->ichgin_ma);
    if (ret != ESP_OK) return ret;

    return ESP_OK;
}

esp_err_t MAX77972::read_vbyp(uint16_t* voltage_mv) {
    if (voltage_mv == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(VByp, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 4) | (data[1] >> 4);
    *voltage_mv = convert_raw_voltage(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_vsys(uint16_t* voltage_mv) {
    if (voltage_mv == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(VSys, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 4) | (data[1] >> 4);
    *voltage_mv = convert_raw_voltage(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_ichgin(uint16_t* current_ma) {
    if (current_ma == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(ICHGIN, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    // ICHGIN is typically 0.39mA per LSB
    *current_ma = raw * 39 / 100;
    return ESP_OK;
}

esp_err_t MAX77972::read_die_temp(int8_t* temperature_c) {
    if (temperature_c == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data;
    esp_err_t ret = read_register(DieTemp, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    // Die temperature is typically 1°C per LSB with -40°C offset
    *temperature_c = (int8_t)data - 40;
    return ESP_OK;
}

// ============================================================================
// FUEL GAUGE - STATE OF CHARGE & CAPACITY
// ============================================================================

esp_err_t MAX77972::read_rep_soc(uint8_t* soc) {
    if (soc == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(RepSOC, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    // SOC is in upper 8 bits (1% resolution)
    *soc = data[0];
    return ESP_OK;
}

esp_err_t MAX77972::read_mix_soc(uint8_t* soc) {
    if (soc == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(MixSOC, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    *soc = data[0];
    return ESP_OK;
}

esp_err_t MAX77972::read_av_soc(uint8_t* soc) {
    if (soc == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(AvSOC, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    *soc = data[0];
    return ESP_OK;
}

esp_err_t MAX77972::read_rep_cap(uint16_t* capacity_mah) {
    if (capacity_mah == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(RepCap, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    *capacity_mah = convert_raw_capacity(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_mix_cap(uint16_t* capacity_mah) {
    if (capacity_mah == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(MixCap, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    *capacity_mah = convert_raw_capacity(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_av_cap(uint16_t* capacity_mah) {
    if (capacity_mah == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(AvCap, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    *capacity_mah = convert_raw_capacity(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_full_cap_nom(uint16_t* capacity_mah) {
    if (capacity_mah == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(FullCapNom, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    *capacity_mah = convert_raw_capacity(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_full_cap(uint16_t* capacity_mah) {
    if (capacity_mah == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    // Note: FullCap may not exist in this model - using Age as fallback
    uint8_t data[2];
    esp_err_t ret = read_registers(FullCapNom, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    *capacity_mah = convert_raw_capacity(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_cycles(uint16_t* cycles) {
    if (cycles == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(Cycles, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    *cycles = (data[0] << 8) | data[1];
    return ESP_OK;
}

esp_err_t MAX77972::read_age(uint8_t* age_pct) {
    if (age_pct == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(Age, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    // Age is percentage (0-100%)
    *age_pct = data[0];
    return ESP_OK;
}

// ============================================================================
// FUEL GAUGE - TEMPERATURE & ALERTS
// ============================================================================

esp_err_t MAX77972::read_temp(int8_t* temperature_c) {
    if (temperature_c == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(Temp, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    *temperature_c = convert_raw_temperature(raw);
    return ESP_OK;
}

esp_err_t MAX77972::read_avg_temp(int8_t* temperature_c) {
    if (temperature_c == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];
    esp_err_t ret = read_registers(AvgTA, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    *temperature_c = convert_raw_temperature(raw);
    return ESP_OK;
}

esp_err_t MAX77972::set_thermistor_config(uint16_t therm_cfg) {
    uint8_t data[2] = {
        static_cast<uint8_t>((therm_cfg >> 8) & 0xFF),
        static_cast<uint8_t>(therm_cfg & 0xFF)
    };
    return write_registers(THM_CNFG, data, 2);
}

esp_err_t MAX77972::get_alert_status(AlertStatus* status) {
    if (status == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data;
    esp_err_t ret = read_register(Status, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    status->voltage_alert = (data & 0x02) != 0;
    status->current_alert = (data & 0x04) != 0;
    status->temperature_alert = (data & 0x08) != 0;
    status->soc_alert = (data & 0x10) != 0;
    status->soc_change_1pct = (data & 0x20) != 0;
    status->alert_reason = data;

    return ESP_OK;
}

esp_err_t MAX77972::set_alert_enable(bool enable) {
    uint8_t value = enable ? 0x01 : 0x00;
    return modify_register(Config, 0x01, value);
}

esp_err_t MAX77972::set_voltage_alert_threshold(uint16_t threshold_mv) {
    uint16_t raw = threshold_mv / 78; // 78.125 µV per LSB (approx 78)
    uint8_t data[2] = {
        static_cast<uint8_t>((raw >> 4) & 0xFF),
        static_cast<uint8_t>((raw & 0x0F) << 4)
    };
    return write_registers(VAlertTh, data, 2);
}

esp_err_t MAX77972::set_current_alert_threshold(int16_t threshold_ma) {
    uint16_t raw = (threshold_ma * 1000) / 156; // 0.15625 mA per LSB (approx 156 per A)
    uint8_t data[2] = {
        static_cast<uint8_t>((raw >> 8) & 0xFF),
        static_cast<uint8_t>(raw & 0xFF)
    };
    return write_registers(IAlertTh, data, 2);
}

esp_err_t MAX77972::set_temperature_alert_threshold(int8_t threshold_c) {
    uint8_t data[2] = {
        static_cast<uint8_t>(threshold_c),
        0x00
    };
    return write_registers(TAlertTh, data, 2);
}

esp_err_t MAX77972::set_soc_alert_threshold(uint8_t threshold_pct) {
    if (threshold_pct > 100) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2] = {
        threshold_pct,
        0x00
    };
    return write_registers(SAlertTh, data, 2);
}

// ============================================================================
// CONFIGURATION & RESET FUNCTIONS
// ============================================================================

esp_err_t MAX77972::soft_reset() {
    // Command register at 0xFE typically contains reset command
    return write_register(0xFE, 0x0F);
}

esp_err_t MAX77972::enter_ship_mode() {
    return modify_register(Config, 0x80, 0x80);
}

esp_err_t MAX77972::enter_deep_sleep_mode() {
    return modify_register(CHG_CNFG_05, 0x01, 0x01);
}

esp_err_t MAX77972::set_jeita_zone_config(uint8_t zone_idx, uint8_t reg_value) {
    if (zone_idx > 8) {
        return ESP_ERR_INVALID_ARG;
    }

    // JEITA zones are typically in registers CHG_CNFG_10 through CHG_CNFG_11
    uint8_t reg = CHG_CNFG_10 + (zone_idx / 2);
    uint8_t mask = (zone_idx % 2) ? 0xF0 : 0x0F;
    uint8_t value = (zone_idx % 2) ? (reg_value << 4) : reg_value;

    return modify_register(reg, mask, value);
}

esp_err_t MAX77972::get_status(uint8_t* status) {
    if (status == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    return read_register(Status, status);
}

esp_err_t MAX77972::clear_interrupts() {
    // Clear charger interrupts
    esp_err_t ret = clear_charger_interrupt(0xFF);
    if (ret != ESP_OK) {
        return ret;
    }

    // Clear input OK interrupts
    return write_register(INOK_INT, 0xFF);
}

// ============================================================================
// USB DETECTION FUNCTIONS
// ============================================================================

esp_err_t MAX77972::manual_usb_detection() {
    return modify_register(CHG_CNFG_00, 0x01, 0x01);
}

esp_err_t MAX77972::get_cc_pin_status(uint8_t* cc1_status, uint8_t* cc2_status) {
    if (cc1_status == nullptr || cc2_status == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data;
    esp_err_t ret = read_register(USBC_CTRL_2, &data);
    if (ret != ESP_OK) {
        return ret;
    }

    *cc1_status = (data & 0x0F);
    *cc2_status = ((data >> 4) & 0x0F);

    return ESP_OK;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

esp_err_t MAX77972::dump_registers() {
    ESP_LOGI(TAG, "=== MAX77972 Register Dump ===");

    uint8_t data;
    for (uint8_t i = 0; i <= 0x0F; i++) {
        if (read_register(i, &data) == ESP_OK) {
            ESP_LOGI(TAG, "0x%02X: 0x%02X", i, data);
        }
    }

    for (uint8_t i = 0x20; i <= 0x2F; i++) {
        if (read_register(i, &data) == ESP_OK) {
            ESP_LOGI(TAG, "0x%02X: 0x%02X", i, data);
        }
    }

    return ESP_OK;
}

esp_err_t MAX77972::get_firmware_version(uint8_t* version) {
    if (version == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    // Firmware version is typically at a reserved register
    return read_register(0x21, version);
}

// ============================================================================
// CONVERSION FUNCTIONS
// ============================================================================

int16_t MAX77972::convert_raw_to_signed(uint16_t raw) {
    if (raw & 0x8000) {
        return -((~raw & 0xFFFF) + 1);
    }
    return raw;
}

uint16_t MAX77972::convert_raw_voltage(uint16_t raw) {
    // Voltage LSB is 78.125 µV (approx 0.078125 mV)
    return (raw * 78) / 100;
}

int16_t MAX77972::convert_raw_current(uint16_t raw) {
    // Current LSB is 0.15625 mA (for 10mΩ sense resistor)
    int16_t signed_val = convert_raw_to_signed(raw);
    return (signed_val * 15625) / 100000;
}

int8_t MAX77972::convert_raw_temperature(uint16_t raw) {
    // Temperature is in upper byte, 1/256°C per LSB in lower byte
    int8_t temp = static_cast<int8_t>(raw >> 8);
    return temp;
}

uint16_t MAX77972::convert_raw_capacity(uint16_t raw) {
    // Capacity LSB is 0.5 mAh (for 10mΩ sense resistor)
    return (raw * 5) / 10;
}

} // namespace max77972
