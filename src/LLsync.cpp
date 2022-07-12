
#include "LLsync.h"
#include <string.h>
#include "core/ble_qiot_template.h"

using std::string;
using neb::CJsonObject;

extern "C" void ble_qiot_service_init(void);
extern "C" void ble_qiot_ota_final_handle(uint8_t result);

void LLsync::Start(void)
{
    if (_running)
        return;
    _running = true;
    ble_ota_callback_reg(LLsync::ota_start_cb, LLsync::ota_stop_cb, LLsync::ota_valid_file_cb);
    ble_qiot_service_init();
}

void LLsync::Stop()
{
}

void LLsync::ota_start_cb()
{
    LLsync::GetInstance()->EventNotify(OTA_START);
}

void LLsync::ota_stop_cb(uint8_t result)
{
    Event evt = OTA_SUCCESS;

    ble_qiot_ota_final_handle(result);
    if (result) {
        evt = OTA_FAIL;
    }
    LLsync::GetInstance()->EventNotify(evt);
}

ble_qiot_ret_status_t LLsync::ota_valid_file_cb(uint32_t file_size, char *file_version)
{
    return BLE_QIOT_RS_OK;
}

extern "C" void llsync_connect_status_notify(int status)
{
    LLsync::Event evt = LLsync::Event::CONNECT;
    if (!status) {
        evt = LLsync::Event::DISCONNECT;
    }
    LLsync::GetInstance()->EventNotify(evt);
}

extern "C" int ble_get_product_key(char *product_secret)
{
    string &productSecret = LLsync::GetInstance()->get_product_secret();
    memcpy(product_secret, productSecret.c_str(), productSecret.length());
    return 0;
}

extern "C" int ble_get_product_id(char *product_id)
{
    string &productId = LLsync::GetInstance()->get_product_id();
    memcpy(product_id, productId.c_str(), productId.length());
    return 0;
}

extern "C" int ble_get_device_name(char *device_name)
{
    string &deviceName = LLsync::GetInstance()->get_device_name();
    memcpy(device_name, deviceName.c_str(), deviceName.length());
    return deviceName.length();
}

extern "C" int ble_get_psk(char *psk)
{
    string &device_secret = LLsync::GetInstance()->get_device_secret();
    memcpy(psk, device_secret.c_str(), device_secret.length());
    return 0;
}

extern "C" uint8_t ble_get_property_type_by_id(uint8_t id)
{
    return LLsync::GetInstance()->thingModel().GetPropertyType(id) & 0x0f;
}

extern "C" void *ble_property_ctx_get(uint8_t id)
{
    return LLsync::GetInstance()->thingModel().GetPropertyCtx(id);
}

extern "C" uint8_t ble_get_property_size()
{
    return LLsync::GetInstance()->thingModel().PropertiesSize();
}

extern "C" void ble_property_change_notify(const e_ble_tlv *tlv)
{
    QiotData *ctx = LLsync::GetInstance()->thingModel().GetPropertyCtx(tlv->id);
    if (ctx)
        LLsync::GetInstance()->thingModel().PropertyNotify(*ctx);
}

extern "C" int ble_event_get_id_array_size(uint8_t event_id)
{
    QiotData *eventCtx = LLsync::GetInstance()->thingModel().GetEventCtx(event_id);
    if (eventCtx)
        return eventCtx->ChildsCount();
    return 0;
}

extern "C" uint8_t ble_event_get_param_id_type(uint8_t event_id, uint8_t param_id)
{
    QiotData *eventCtx =  LLsync::GetInstance()->thingModel().GetEventCtx(event_id);
    if (!eventCtx)
        return BLE_QIOT_DATA_TYPE_BUTT;
    QiotData *paramCtx = eventCtx->GetChildCtx(param_id);
    if (!paramCtx)
        return BLE_QIOT_DATA_TYPE_BUTT;
    return paramCtx->GetType();
}

extern "C" int ble_event_get_data_by_id(uint8_t event_id, uint8_t param_id, char *out_buf, uint16_t buf_len)
{
    QiotData *eventCtx = LLsync::GetInstance()->thingModel().GetEventCtx(event_id);
    if (!eventCtx)
        return -1;
    QiotData *paramCtx = eventCtx->GetChildCtx(param_id);
    if (!paramCtx)
        return -1;
    return paramCtx->GetValue(out_buf, buf_len);
}

extern "C" void *ble_actions_input_ctx_get(uint8_t id)
{
    return LLsync::GetInstance()->thingModel().GetActionInputCtx(id);
}

extern "C" void ble_actions_input_notify(uint8_t id, uint8_t output_flag[])
{
    LLsync::GetInstance()->thingModel().ActionsNotify(id, output_flag);
}

extern "C" uint8_t ble_action_get_output_type_by_id(uint8_t action_id, uint8_t output_id)
{
    QiotData *action_output = LLsync::GetInstance()->thingModel().GetActionOutputCtx(action_id);
    if (!action_output)
        return BLE_QIOT_DATA_TYPE_BUTT;
    QiotData *ctx = action_output->GetChildCtx(output_id);
    if (!ctx)
        return BLE_QIOT_DATA_TYPE_BUTT;
    return ctx->GetType();
}

extern "C" int ble_action_user_handle_output_param(uint8_t action_id, uint8_t output_id, char *buf, uint16_t buf_len)
{
    QiotData *action_output = LLsync::GetInstance()->thingModel().GetActionOutputCtx(action_id);
    if (!action_output)
        return -1;
    QiotData *ctx = action_output->GetChildCtx(output_id);
    if (!ctx)
        return -1;
    return ctx->GetValue(buf, buf_len);
}
