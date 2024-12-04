#include <stdio.h>
#include <string.h>
#include <esp_bt.h>
#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>
#include <esp_bt_main.h>
#include <esp_gatt_common_api.h>

#define DEVICE_NAME "ESP32_BLE_SERVER"
#define SERVICE_UUID "0000180A-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID "00002A57-0000-1000-8000-00805F9B34FB"

static uint8_t char_value[100]; // Giá trị lưu trong đặc tính (characteristic)
static esp_gatt_char_prop_t char_property = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;

// GATT server profile
static struct {
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
} gatts_profile;

// Hàm callback khi nhận sự kiện từ GATT server
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

// Hàm xử lý khi nhận dữ liệu từ Client
void gatts_write_event_handler(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    char received_data[100];
    memcpy(received_data, param->write.value, param->write.len);
    received_data[param->write.len] = '\0';

    // Kiểm tra tên và mật khẩu
    if (strcmp(received_data, "username:password") == 0) {
        printf("Authentication success!\n");
    } else {
        printf("Authentication failed!\n");
    }
}

// Hàm callback khi nhận sự kiện GAP
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                printf("Advertising start failed\n");
            } else {
                printf("Advertising started successfully\n");
            }
            break;
        default:
            break;
    }
}

// Khởi tạo dịch vụ GATT
void create_gatt_service(esp_gatt_if_t gatts_if) {
    // Cấu hình ID dịch vụ
    gatts_profile.service_id.is_primary = true;
    gatts_profile.service_id.id.inst_id = 0x00;
    gatts_profile.service_id.id.uuid.len = ESP_UUID_LEN_16;
    gatts_profile.service_id.id.uuid.uuid.uuid16 = 0x180A; // UUID của dịch vụ

    // Tạo dịch vụ
    esp_ble_gatts_create_service(gatts_if, &gatts_profile.service_id, 4);
}

// Hàm xử lý các sự kiện GATT server
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            printf("GATT server registered\n");
            create_gatt_service(gatts_if);
            break;

        case ESP_GATTS_CREATE_EVT:
            gatts_profile.service_handle = param->create.service_handle;
            printf("Service created\n");

            // Thêm đặc tính (Characteristic) vào dịch vụ
            esp_bt_uuid_t char_uuid = {
                .len = ESP_UUID_LEN_16,
                .uuid.uuid16 = 0x2A57 // UUID của đặc tính
            };
            esp_ble_gatts_add_char(
                gatts_profile.service_handle,
                &char_uuid,
                ESP_GATT_PERM_WRITE | ESP_GATT_PERM_READ,
                char_property,
                NULL,
                NULL
            );
            break;

        case ESP_GATTS_ADD_CHAR_EVT:
            gatts_profile.char_handle = param->add_char.attr_handle;
            printf("Characteristic added\n");

            // Bắt đầu quảng cáo
            esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
                .adv_int_min = 0x20,
                .adv_int_max = 0x40,
                .adv_type = ADV_TYPE_IND,
                .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
                .channel_map = ADV_CHNL_ALL,
                .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
            });
            break;

        case ESP_GATTS_WRITE_EVT:
            gatts_write_event_handler(gatts_if, param);
            break;

        default:
            break;
    }
}

// Hàm main
void app_main() {
    esp_err_t ret;

    // Khởi tạo Bluetooth controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        printf("Bluetooth controller initialize failed\n");
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        printf("Bluetooth enable failed\n");
        return;
    }

    // Khởi tạo và bật Bluetoot stack
    esp_bluedroid_init();
    esp_bluedroid_enable();

    // Đăng ký callback cho GATT server và GAP
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);

    // Đăng ký ứng dụng GATT server
    esp_ble_gatts_app_register(0);
    esp_ble_gap_set_device_name(DEVICE_NAME);
}