#include <stdio.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_task.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "max77972.hpp"

static const char* TAG = "MAX77972_EXAMPLE";

void max77972_demo_task(void* arg) {
    using namespace max77972;

    // Create MAX77972 instance
    // Using default pins: GPIO8 (SCL), GPIO9 (SDA), I2C_NUM_0
    MAX77972 charger(I2C_NUM_0, I2C_ADDR_0x36, GPIO_NUM_9, GPIO_NUM_8, 400000);

    // Initialize the driver
    ESP_LOGI(TAG, "Initializing MAX77972...");
    if (charger.init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MAX77972");
        vTaskDelete(NULL);
        return;
    }

    // Check if device is present
    if (charger.probe() != ESP_OK) {
        ESP_LOGE(TAG, "MAX77972 device not found on I2C bus");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "MAX77972 device initialized successfully!");

    // Configure some initial settings
    ESP_LOGI(TAG, "Configuring charger settings...");
    charger.set_charge_enable(true);
    charger.set_fast_charge_current(ChargingCurrent::I_500MA);
    charger.set_charge_voltage(ChargingVoltage::V_4200MV);
    charger.set_input_current_limit(1000);
    charger.set_alert_enable(true);

    // Main monitoring loop
    ESP_LOGI(TAG, "Starting periodic monitoring...");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000)); // Update every 2 seconds

        // ==================================================================
        // READ BATTERY INFORMATION
        // ==================================================================
        BatteryInfo bat_info;
        if (charger.get_battery_info(&bat_info) == ESP_OK) {
            ESP_LOGI(TAG, "\n=== BATTERY STATUS ===");
            ESP_LOGI(TAG, "Voltage:        %5u mV", bat_info.voltage_mv);
            ESP_LOGI(TAG, "Avg Voltage:    %5u mV", bat_info.avg_voltage_mv);
            ESP_LOGI(TAG, "Current:        %6d mA", bat_info.current_ma);
            ESP_LOGI(TAG, "Avg Current:    %6d mA", bat_info.avg_current_ma);
            ESP_LOGI(TAG, "Capacity:       %5u mAh", bat_info.capacity_mah);
            ESP_LOGI(TAG, "State of Charge:%5u %%", bat_info.state_of_charge);
            ESP_LOGI(TAG, "Temperature:    %5d °C", bat_info.temperature_c);
            ESP_LOGI(TAG, "Die Temp:       %5d °C", bat_info.die_temperature_c);
            ESP_LOGI(TAG, "Cycle Count:    %5u", bat_info.cycles);
        } else {
            ESP_LOGW(TAG, "Failed to read battery info");
        }

        // ==================================================================
        // READ CHARGER STATUS
        // ==================================================================
        ChargerStatus chg_status;
        if (charger.get_charger_status(&chg_status) == ESP_OK) {
            ESP_LOGI(TAG, "\n=== CHARGER STATUS ===");
            
            const char* state_str = "UNKNOWN";
            switch (chg_status.state) {
                case ChargerState::DEAD_BATTERY:
                    state_str = "DEAD_BATTERY";
                    break;
                case ChargerState::PRECHARGING:
                    state_str = "PRECHARGING";
                    break;
                case ChargerState::FAST_CHARGING:
                    state_str = "FAST_CHARGING";
                    break;
                case ChargerState::CONSTANT_VOLTAGE:
                    state_str = "CONSTANT_VOLTAGE";
                    break;
                case ChargerState::TOP_OFF:
                    state_str = "TOP_OFF";
                    break;
                case ChargerState::DONE:
                    state_str = "DONE";
                    break;
                case ChargerState::TIMER_EXPIRED:
                    state_str = "TIMER_EXPIRED";
                    break;
                case ChargerState::TEMP_SUSPEND:
                    state_str = "TEMP_SUSPEND";
                    break;
                case ChargerState::OFF:
                    state_str = "OFF";
                    break;
                default:
                    state_str = "RESERVED";
            }
            
            const char* temp_str = "UNKNOWN";
            switch (chg_status.temp_zone) {
                case TemperatureZone::COLD:
                    temp_str = "COLD";
                    break;
                case TemperatureZone::COOL:
                    temp_str = "COOL";
                    break;
                case TemperatureZone::ROOM:
                    temp_str = "ROOM";
                    break;
                case TemperatureZone::WARM:
                    temp_str = "WARM";
                    break;
                case TemperatureZone::HOT:
                    temp_str = "HOT";
                    break;
                default:
                    temp_str = "RESERVED";
            }

            ESP_LOGI(TAG, "State:          %s", state_str);
            ESP_LOGI(TAG, "Temp Zone:      %s", temp_str);
            ESP_LOGI(TAG, "Is Charging:    %s", chg_status.is_charging ? "YES" : "NO");
            ESP_LOGI(TAG, "Dead Battery:   %s", chg_status.is_dead_battery ? "YES" : "NO");
        } else {
            ESP_LOGW(TAG, "Failed to read charger status");
        }

        // ==================================================================
        // READ SYSTEM VOLTAGES
        // ==================================================================
        SystemVoltages sys_vol;
        if (charger.get_system_voltages(&sys_vol) == ESP_OK) {
            ESP_LOGI(TAG, "\n=== SYSTEM VOLTAGES ===");
            ESP_LOGI(TAG, "BYP Voltage:    %5u mV", sys_vol.vbyp_mv);
            ESP_LOGI(TAG, "SYS Voltage:    %5u mV", sys_vol.vsys_mv);
            ESP_LOGI(TAG, "CHGIN Current:  %5u mA", sys_vol.ichgin_ma);
        } else {
            ESP_LOGW(TAG, "Failed to read system voltages");
        }

        // ==================================================================
        // READ FUEL GAUGE STATUS
        // ==================================================================
        uint8_t rep_soc;
        uint16_t rep_cap;
        uint16_t full_cap_nom;
        
        if (charger.read_rep_soc(&rep_soc) == ESP_OK &&
            charger.read_rep_cap(&rep_cap) == ESP_OK &&
            charger.read_full_cap_nom(&full_cap_nom) == ESP_OK) {
            
            ESP_LOGI(TAG, "\n=== FUEL GAUGE ===");
            ESP_LOGI(TAG, "Reported SOC:   %5u %%", rep_soc);
            ESP_LOGI(TAG, "Remaining Cap:  %5u mAh", rep_cap);
            ESP_LOGI(TAG, "Nominal Capacity:%5u mAh", full_cap_nom);
            
            if (full_cap_nom > 0) {
                uint8_t health = (rep_cap * 100) / full_cap_nom;
                ESP_LOGI(TAG, "Health:         %5u %%", health);
            }
        }

        // ==================================================================
        // READ ALERTS
        // ==================================================================
        AlertStatus alert_status;
        if (charger.get_alert_status(&alert_status) == ESP_OK) {
            ESP_LOGI(TAG, "\n=== ALERTS ===");
            if (alert_status.voltage_alert) {
                ESP_LOGW(TAG, "Voltage Alert!");
            }
            if (alert_status.current_alert) {
                ESP_LOGW(TAG, "Current Alert!");
            }
            if (alert_status.temperature_alert) {
                ESP_LOGW(TAG, "Temperature Alert!");
            }
            if (alert_status.soc_alert) {
                ESP_LOGW(TAG, "SOC Alert!");
            }
            if (alert_status.soc_change_1pct) {
                ESP_LOGI(TAG, "SOC changed by 1%%");
            }
        }

        ESP_LOGI(TAG, "\n" "-------------------------------------------\n");
    }

    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "MAX77972 Library Example Started");
    ESP_LOGI(TAG, "=================================\n");

    // Create a task to run the MAX77972 demo
    xTaskCreate(max77972_demo_task, "max77972_demo", 4096, NULL, 5, NULL);
}
