/*
 * Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <string.h>

#include "ble_qiot_export.h"
#include "ble_qiot_template.h"
#include "ble_qiot_param_check.h"
#include "ble_qiot_common.h"

static bool ble_check_space_enough_by_type(uint8_t type, uint16_t left_size)
{
    switch(type)
    {
        case BLE_QIOT_DATA_TYPE_BOOL:
            return left_size >= sizeof(uint8_t);
        case BLE_QIOT_DATA_TYPE_INT:
        case BLE_QIOT_DATA_TYPE_FLOAT:
        case BLE_QIOT_DATA_TYPE_TIME:
            return left_size >= sizeof(uint32_t);
        case BLE_QIOT_DATA_TYPE_ENUM:
            return left_size >= sizeof(uint16_t);
        default:
            // string length is unknow, default true
            return true;
    }
}

static uint16_t ble_check_ret_value_by_type(uint8_t type, uint16_t buf_len, uint16_t ret_val)
{
    switch(type)
    {
        case BLE_QIOT_DATA_TYPE_BOOL:
            return ret_val <= sizeof(uint8_t);
        case BLE_QIOT_DATA_TYPE_INT:
        case BLE_QIOT_DATA_TYPE_FLOAT:
        case BLE_QIOT_DATA_TYPE_TIME:
            return ret_val <= sizeof(uint32_t);
        case BLE_QIOT_DATA_TYPE_ENUM:
            return ret_val <= sizeof(uint16_t);
        default:
            // string length is unknow, default true
            return ret_val <= buf_len;
    }
}

int ble_user_property_report_reply_handle(uint8_t result)
{
    ble_qiot_log_d("report reply result %d", result);
    return BLE_QIOT_RS_OK;
}

static int ble_qiot_data_array_get(void *ctx, char *in_buf, int buf_len, int struct_member);
static int ble_qiot_data_struct_get(void *ctx, char *in_buf, int buf_len, int array_member)
{
    if (ctx == NULL) {
        ble_qiot_log_e("invalid ctx");
        return BLE_QIOT_RS_ERR;
    }
    if ((ble_qiot_data_type_get(ctx) & 0x0f) != BLE_QIOT_DATA_TYPE_STRUCT) {
        ble_qiot_log_e("property type is not struct");
        return BLE_QIOT_RS_ERR;
    }

    uint16_t index = 0;
    int16_t data_len = 0;
    int ret_len;
    void *member_ctx = ble_struct_array_get_elem_ctx(ctx, index);
    while (member_ctx) {
        int length_param = 0;
        int type = ble_qiot_data_type_get(member_ctx) & 0x0f;
        in_buf[data_len ++] = BLE_QIOT_PACKAGE_TLV_HEAD(type, index);
        switch (type) {
        case BLE_QIOT_DATA_TYPE_BOOL:
        case BLE_QIOT_DATA_TYPE_INT:
        case BLE_QIOT_DATA_TYPE_FLOAT:
        case BLE_QIOT_DATA_TYPE_ENUM:
        case BLE_QIOT_DATA_TYPE_TIME:
            ret_len = ble_qiot_data_get(member_ctx, in_buf + data_len, buf_len - data_len);
            break;
        case BLE_QIOT_DATA_TYPE_STRING:
            ret_len = ble_qiot_data_get(member_ctx, in_buf + data_len + 2, buf_len - data_len - 2);
            length_param = 1;
            break;
        case BLE_QIOT_DATA_TYPE_ARRAY:
            if (!array_member) {
                ret_len = -1;
                break;
            }
            ret_len = ble_qiot_data_array_get(member_ctx, in_buf + data_len + 2, buf_len - data_len - 2, 0);
            length_param = 1;
            break;
        default:
            ret_len = -1;
        }
        if (ret_len < 0) {
            ble_qiot_log_e("too long data, member id %d, data length %d", index, data_len);
            return BLE_QIOT_RS_ERR;
        } else if (ret_len == 0) {
            // no data to post
            data_len--;
            in_buf[data_len] = '0';
            ble_qiot_log_d("member id %d no data to post", index);
        } else {
            if (length_param) {
                uint16_t param_len = ret_len;
                param_len = HTONS(param_len);
                memcpy(in_buf + data_len, &param_len, sizeof(uint16_t));
                data_len += sizeof(uint16_t);
            }
            data_len += ret_len;
        }
        ++ index;
        member_ctx = ble_struct_array_get_elem_ctx(ctx, index);
    }
    return data_len;
}

static int ble_qiot_data_array_get(void *ctx, char *in_buf, int buf_len, int struct_member)
{
    if (ctx == NULL) {
        ble_qiot_log_e("invalid ctx");
        return BLE_QIOT_RS_ERR;
    }
    int type = ble_qiot_data_type_get(ctx);
    if ((type & 0x0f) != BLE_QIOT_DATA_TYPE_ARRAY) {
        ble_qiot_log_e("property type is not array");
        return BLE_QIOT_RS_ERR;
    }

    uint16_t index = 0;
    int16_t data_len = 0;
    int ret_len;

    void *member_ctx = ble_struct_array_get_elem_ctx(ctx, index);
    while (member_ctx) {
        in_buf[data_len ++] = BLE_QIOT_PACKAGE_TLV_HEAD(type, index);
        switch (type & 0xf0) {
        case BLE_QIOT_ARRAY_INT_BIT_MASK:
        case BLE_QIOT_ARRAY_FLOAT_BIT_MASK:
            ret_len = ble_qiot_data_get(member_ctx, in_buf + data_len, buf_len - data_len);
            data_len += sizeof(uint32_t);
            break;
        case BLE_QIOT_ARRAY_STRING_BIT_MASK:
        case BLE_QIOT_ARRAY_STRUCT_BIT_MASK: {
            if ((type & 0xf0) == BLE_QIOT_ARRAY_STRING_BIT_MASK) {
                ret_len = ble_qiot_data_get(member_ctx, in_buf + data_len + 2, buf_len - data_len - 2);
            } else {
                if (!struct_member) {
                    ret_len = -1;
                    break;
                }
                ret_len = ble_qiot_data_struct_get(member_ctx, in_buf + data_len + 2, buf_len - data_len - 2, 0);
            }
            uint16_t param_len = ret_len;
            param_len = HTONS(param_len);
            memcpy(in_buf + data_len, &param_len, sizeof(uint16_t));
            data_len += sizeof(uint16_t);
            data_len += ret_len;
        } break;
        default:
            ret_len = -1;
        }

        if (ret_len <= 0) {
            ble_qiot_log_e("too long data, member id %d, data length %d", index, data_len);
            return BLE_QIOT_RS_ERR;
        }

        ++ index;
        member_ctx = ble_struct_array_get_elem_ctx(ctx, index);
    }
    return data_len;
}

//.................
static int ble_qiot_data_struct_set(void *ctx, const e_ble_tlv *tlv, int array_member);
static int ble_qiot_data_array_set(void *ctx, const e_ble_tlv *tlv, int struct_member)
{
    if (ctx == NULL) {
        ble_qiot_log_e("invalid ctx");
        return BLE_QIOT_RS_ERR;
    }
    int type = ble_qiot_data_type_get(ctx);
    if ((type & 0x0f) != BLE_QIOT_DATA_TYPE_ARRAY) {
        ble_qiot_log_e("property type is not array");
        return BLE_QIOT_RS_ERR;
    }

    uint16_t parse_len = 0;
    uint16_t index = 0;
    uint16_t member_len;
    int ret = BLE_QIOT_RS_ERR;

    while (parse_len < tlv->len) {
        switch (type & 0xF0) {
        case BLE_QIOT_ARRAY_INT_BIT_MASK:
        case BLE_QIOT_ARRAY_FLOAT_BIT_MASK:
            ret = ble_struct_array_elem_set(ctx, index, tlv->val + parse_len, sizeof(uint32_t));
            parse_len += sizeof(uint32_t);
            break;
        case BLE_QIOT_ARRAY_STRING_BIT_MASK:
            memcpy(&member_len, &tlv->val[parse_len], sizeof(uint16_t));
            member_len = NTOHS(member_len);
            parse_len += sizeof(uint16_t);
            ret = ble_struct_array_elem_set(ctx, index, tlv->val + parse_len, member_len);
            parse_len += member_len;
            break;
        case BLE_QIOT_ARRAY_STRUCT_BIT_MASK:
            if (struct_member) {
                memcpy(&member_len, &tlv->val[parse_len], sizeof(uint16_t));
                member_len = NTOHS(member_len);
                parse_len += sizeof(uint16_t);
                void *member_ctx = ble_struct_array_get_elem_ctx(ctx, index);
                if (member_ctx == NULL) {
                    ble_qiot_log_e("member property id error");
                    return BLE_QIOT_RS_ERR;
                }
                e_ble_tlv child_tlv;
                child_tlv.type = BLE_QIOT_DATA_TYPE_STRUCT;
                child_tlv.id = index;
                child_tlv.val = tlv->val + parse_len;
                child_tlv.len = member_len;
                ret = ble_qiot_data_struct_set(member_ctx, &child_tlv, 0);
                parse_len += member_len;
            } else {
                ret = BLE_QIOT_RS_ERR;
            }
            break;
        }
        if (ret != BLE_QIOT_RS_OK) {
            ble_qiot_log_e("set property id %d failed", tlv->id);
            return ret;
        }
        index++;
    }
    return parse_len == tlv->len ? BLE_QIOT_RS_OK : BLE_QIOT_RS_ERR;
}

static int ble_qiot_data_struct_set(void *ctx, const e_ble_tlv *tlv, int array_member)
{
    if (ctx == NULL) {
        ble_qiot_log_e("invalid ctx");
        return BLE_QIOT_RS_ERR;
    }
    if (ble_qiot_data_type_get(ctx) != BLE_QIOT_DATA_TYPE_STRUCT) {
        ble_qiot_log_e("property type is not struct");
        return BLE_QIOT_RS_ERR;
    }

    int ret;
    int ret_len;
    uint16_t parse_len = 0;
    e_ble_tlv child_tlv;
    void *member_ctx;

    while (parse_len < tlv->len) {
        memset(&child_tlv, 0, sizeof(e_ble_tlv));
        ret_len = ble_lldata_parse_tlv(tlv->val + parse_len, tlv->len - parse_len, &child_tlv);
        if (ret_len <= 0) {
            ble_qiot_log_e("parse struct failed");
            return BLE_QIOT_RS_ERR;
        }
        if (parse_len + ret_len > tlv->len) {
            ble_qiot_log_e("parse struct failed");
            return BLE_QIOT_RS_ERR;
        }
        member_ctx = ble_struct_array_get_elem_ctx(ctx, child_tlv.id);
        if ((member_ctx == NULL) || ((ble_qiot_data_type_get(member_ctx) & 0x0f) != child_tlv.type)) {
            ble_qiot_log_e("member property id or type error");
            return BLE_QIOT_RS_ERR;
        }

        switch(child_tlv.type) {
        case BLE_QIOT_DATA_TYPE_BOOL:
        case BLE_QIOT_DATA_TYPE_INT:
        case BLE_QIOT_DATA_TYPE_STRING:
        case BLE_QIOT_DATA_TYPE_FLOAT:
        case BLE_QIOT_DATA_TYPE_ENUM:
        case BLE_QIOT_DATA_TYPE_TIME:
            ret = ble_qiot_data_set(member_ctx, child_tlv.val, child_tlv.len);
            break;
        case BLE_QIOT_DATA_TYPE_ARRAY:
            if (array_member)
                ret = ble_qiot_data_array_set(member_ctx, &child_tlv, 0);
            else
                ret = BLE_QIOT_RS_ERR;
            break;
        default:
            ret = BLE_QIOT_RS_ERR;
            ble_qiot_log_e("tlv type %d error", child_tlv.type);
        }
        if (BLE_QIOT_RS_OK != ret) {
            ble_qiot_log_e("user ctx property error, member id %d, type %d, len %d", child_tlv.id, child_tlv.type, child_tlv.len);
            return ret;
        }
        parse_len += ret_len;
    }

    return (parse_len == tlv->len) ? BLE_QIOT_RS_OK : BLE_QIOT_RS_ERR;
}


int ble_user_property_set_data(const e_ble_tlv *tlv)
{
    POINTER_SANITY_CHECK(tlv, BLE_QIOT_RS_ERR_PARA);
    void *ctx = ble_property_ctx_get(tlv->id);
    if (ctx == NULL)
        return BLE_QIOT_RS_ERR;

    switch (tlv->type & 0x0f) {
    case BLE_QIOT_DATA_TYPE_ARRAY:
        return ble_qiot_data_array_set(ctx, tlv, 1);
    case BLE_QIOT_DATA_TYPE_STRUCT:
        return ble_qiot_data_struct_set(ctx, tlv, 1);
    case BLE_QIOT_DATA_TYPE_BOOL:
    case BLE_QIOT_DATA_TYPE_INT:
    case BLE_QIOT_DATA_TYPE_STRING:
    case BLE_QIOT_DATA_TYPE_FLOAT:
    case BLE_QIOT_DATA_TYPE_ENUM:
    case BLE_QIOT_DATA_TYPE_TIME:
        return ble_qiot_data_set(ctx, tlv->val, tlv->len);
    }
    return BLE_QIOT_RS_ERR;
}

int ble_user_property_get_data_by_id(uint8_t id, char *buf, uint16_t buf_len)
{
    int ret_len = 0;

    POINTER_SANITY_CHECK(buf, BLE_QIOT_RS_ERR_PARA);
    void *ctx = ble_property_ctx_get(id);
    if (ctx == NULL)
        return -1;

    uint8_t type = ble_qiot_data_type_get(ctx) & 0x0f;
    if (!ble_check_space_enough_by_type(type, buf_len)) {
        ble_qiot_log_e("not enough space get property id %d data", id);
        return -1;
    }
    switch (type) {
    case BLE_QIOT_DATA_TYPE_ARRAY:
        return ble_qiot_data_array_get(ctx, buf, buf_len, 1);
    case BLE_QIOT_DATA_TYPE_STRUCT:
        return ble_qiot_data_struct_get(ctx, buf, buf_len, 1);
    case BLE_QIOT_DATA_TYPE_BOOL:
    case BLE_QIOT_DATA_TYPE_INT:
    case BLE_QIOT_DATA_TYPE_STRING:
    case BLE_QIOT_DATA_TYPE_FLOAT:
    case BLE_QIOT_DATA_TYPE_ENUM:
    case BLE_QIOT_DATA_TYPE_TIME:
        return ble_qiot_data_get(ctx, buf, buf_len);
    }

    ble_qiot_log_e("invalid callback, property id %d", id);
    return -1;
}

#ifdef BLE_QIOT_INCLUDE_EVENT

int ble_user_event_reply_handle(uint8_t event_id, uint8_t result)
{
    ble_qiot_log_d("event id %d, reply result %d", event_id, result);

    return BLE_QIOT_RS_OK;
}
#endif

#ifdef BLE_QIOT_INCLUDE_ACTION

int ble_user_actions_input_set(const e_ble_tlv *tlv)
{
    POINTER_SANITY_CHECK(tlv, BLE_QIOT_RS_ERR_PARA);
    void *ctx = ble_actions_input_ctx_get(tlv->id);
    if (ctx == NULL)
        return BLE_QIOT_RS_ERR;

    return ble_qiot_data_struct_set(ctx, tlv, 0);
}

#endif
