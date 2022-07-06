#include <string.h>
#include "ThingModel.h"
#include "core/ble_qiot_template.h"
#include "core/ble_qiot_log.h"
#include "core/ble_qiot_llsync_data.h"

static const char *VERSION = "version";
static const char *TYPE = "type";
static const char *NAME = "name";
static const char *ID = "id";
static const char *MIN = "min";
static const char *MAX = "max";
static const char *START = "start";
static const char *STEP = "step";
static const char *DEFINE = "define";
static const char *PROPERTIES = "properties";
static const char *EVENTS = "events";
static const char *ACTIONS = "actions";
static const char *MAPPING = "mapping";
static const char *UNIT = "unit";
static const char *UNITDESC = "unitDesc";
static const char *REQUIRED = "required";
static const char *MODE = "mode";
static const char *SPECS = "specs";
static const char *BOOL = "bool";
static const char *INT = "int";
static const char *STRING = "string";
static const char *FLOAT = "float";
static const char *ENUM = "enum";
static const char *TIMESTAMP = "timestamp";
static const char *STRUCT = "struct";
static const char *DATATYPE = "dataType";
static const char *ARRAY = "array";
static const char *ARRAYINFO = "arrayInfo";
static const char *PARAMS = "params";
static const char *INPUT = "input";
static const char *OUTPUT = "output";

using std::string;
using neb::CJsonObject;

static int StrTypeToInt(string &s_type)
{
    if (s_type == BOOL) {
        return BLE_QIOT_DATA_TYPE_BOOL;
    } else if (s_type == INT) {
        return BLE_QIOT_DATA_TYPE_INT;
    } else if (s_type == STRING) {
        return BLE_QIOT_DATA_TYPE_STRING;
    } else if (s_type == FLOAT) {
        return BLE_QIOT_DATA_TYPE_FLOAT;
    } else if (s_type == ENUM) {
        return BLE_QIOT_DATA_TYPE_ENUM;
    } else if (s_type == TIMESTAMP) {
        return BLE_QIOT_DATA_TYPE_TIME;
    } else if (s_type == STRUCT) {
        return BLE_QIOT_DATA_TYPE_STRUCT;
    } else if (s_type == ARRAY) {
        return BLE_QIOT_DATA_TYPE_ARRAY;
    }
    return BLE_QIOT_DATA_TYPE_BUTT;
}

static const char *IntTypeToStr(int type)
{
    switch (type) {
    case BLE_QIOT_DATA_TYPE_BOOL:
        return BOOL;
    case BLE_QIOT_DATA_TYPE_INT:
        return INT;
    case BLE_QIOT_DATA_TYPE_STRING:
        return STRING;
    case BLE_QIOT_DATA_TYPE_FLOAT:
        return FLOAT;
    case BLE_QIOT_DATA_TYPE_ENUM:
        return ENUM;
    case BLE_QIOT_DATA_TYPE_TIME:
        return TIMESTAMP;
    case BLE_QIOT_DATA_TYPE_STRUCT:
        return STRUCT;
    case BLE_QIOT_DATA_TYPE_ARRAY:
        return ARRAY;
    }
    return "errorType";
}

void QiotData::Dump(const char *preFormat)
{
    BLE_QIOT_LOG_PRINT("%sid:%s | type:%s | value:", preFormat, _id.c_str(), IntTypeToStr(_type));
    switch(_type) {
    case BLE_QIOT_DATA_TYPE_BOOL:
    case BLE_QIOT_DATA_TYPE_INT:
    case BLE_QIOT_DATA_TYPE_ENUM:
    case BLE_QIOT_DATA_TYPE_TIME:
        BLE_QIOT_LOG_PRINT("%d", _val);
        break;
    case BLE_QIOT_DATA_TYPE_FLOAT:
        BLE_QIOT_LOG_PRINT("%f", (float)_val);
        break;
    case BLE_QIOT_DATA_TYPE_STRING:
        for (char c : _strVal)
            BLE_QIOT_LOG_PRINT("%c", c);
        break;
    case BLE_QIOT_DATA_TYPE_STRUCT:
    case BLE_QIOT_DATA_TYPE_ARRAY:
        BLE_QIOT_LOG_PRINT(" | childs: %d", ChildsCount());
        break;
    }
    BLE_QIOT_LOG_PRINT("\n");
    string childFormat("    ");
    childFormat += preFormat;
    for (auto &child : _childs) {
        child.Dump(childFormat.c_str());
    }
}

QiotData::QiotData(const char *id, int type)
    : _id(id),
      _type(type)
{
}

QiotData::QiotData(string &id, int type)
    : _id(id),
      _type(type)
{
}

bool QiotData::SetValue(uint32_t val)
{
    switch (_type) {
    case BLE_QIOT_DATA_TYPE_BOOL:
    case BLE_QIOT_DATA_TYPE_INT:
    case BLE_QIOT_DATA_TYPE_FLOAT:
    case BLE_QIOT_DATA_TYPE_ENUM:
    case BLE_QIOT_DATA_TYPE_TIME:
        _val = val;
        return true;
    }
    return false;
}

bool QiotData::SetValue(const char *str)
{
    if (_type != BLE_QIOT_DATA_TYPE_STRING)
        return false;
    int len = strlen(str) + 1;
    _strVal.resize(len);
    memcpy(_strVal.data(), str, len);
    return true;
}

bool QiotData::SetValue(const char *dat, int len)
{
    switch (_type) {
    case BLE_QIOT_DATA_TYPE_INT:
    case BLE_QIOT_DATA_TYPE_TIME:
    case BLE_QIOT_DATA_TYPE_FLOAT: {
        if (len != BLE_QIOT_DATA_INT_TYPE_LEN)
            return false;
        uint32_t val = ((uint32_t)dat[0] << 24) | (dat[1] << 16) | (dat[2] << 8) | dat[3];
        _val = val;
        return true;
    }
    case BLE_QIOT_DATA_TYPE_BOOL: {
        if (len != BLE_QIOT_DATA_BOOL_TYPE_LEN)
            return false;
        _val = dat[0];
        return true;
    }
    case BLE_QIOT_DATA_TYPE_STRING:
        _strVal.resize(len);
        memcpy(_strVal.data(), dat, len);
        return true;
    case BLE_QIOT_DATA_TYPE_ENUM: {
        if (len != BLE_QIOT_DATA_ENUM_TYPE_LEN)
            return false;
        uint16_t val = ((uint16_t)dat[0] << 8) | dat[1];
        _val = val;
        return true;
    }
    }
    ble_qiot_log_e("qiot_data type %s cannot set val\n", IntTypeToStr(_type));
    return false;
}

bool QiotData::SetValue(unsigned int index, uint32_t val)
{
    if (index >= _childs.size())
        return false;
    return _childs[index].SetValue(val);
}

bool QiotData::SetValue(unsigned int index, const char *val, int len)
{
    if (index >= _childs.size())
        return false;

    return _childs[index].SetValue(val, len);
}

int QiotData::GetValue(char *buf, uint16_t buf_len)
{
    switch (_type) {
    case BLE_QIOT_DATA_TYPE_INT:
    case BLE_QIOT_DATA_TYPE_FLOAT:
    case BLE_QIOT_DATA_TYPE_TIME: {
        uint32_t val = _val;
        buf[0] = (val >> 24) & 0xff;
        buf[1] = (val >> 16) & 0xff;
        buf[2] = (val >> 8) & 0xff;
        buf[3] = (val >> 0) & 0xff;
        return BLE_QIOT_DATA_INT_TYPE_LEN;
    }
    case BLE_QIOT_DATA_TYPE_ENUM: {
        uint16_t val = _val;
        buf[0] = (val >> 8) & 0xff;
        buf[1] = (val >> 0) & 0xff;
        return BLE_QIOT_DATA_ENUM_TYPE_LEN;
    }
    case BLE_QIOT_DATA_TYPE_BOOL: {
        if (_val)
            buf[0] = 1;
        else
            buf[0] = 0;
        return BLE_QIOT_DATA_BOOL_TYPE_LEN;
    }
    case BLE_QIOT_DATA_TYPE_STRING: {
        if (buf_len < _strVal.size()) {
            return -1;
        } else {
            memcpy(buf, _strVal.data(), _strVal.size());
        }
        return _strVal.size();
    }
    }
    ble_qiot_log_e("qiot_data type %s cannot get val\n", IntTypeToStr(_type));
    return -1;
}

int QiotData::GetValue(uint8_t index, char *buf, uint16_t buf_len)
{
    if (index >= _childs.size())
        return -1;
    return _childs[index].GetValue(buf, buf_len);
}

uint8_t QiotData::GetType()
{
    int type = _type;
    if (type == BLE_QIOT_DATA_TYPE_ARRAY) {
        if (!_childs.empty()) {
            switch (_childs[0]._type) {
            case BLE_QIOT_DATA_TYPE_INT:
                type |= BLE_QIOT_ARRAY_INT_BIT_MASK;
                break;
            case BLE_QIOT_DATA_TYPE_STRING:
                type |= BLE_QIOT_ARRAY_STRING_BIT_MASK;
                break;
            case BLE_QIOT_DATA_TYPE_FLOAT:
                type |= BLE_QIOT_ARRAY_FLOAT_BIT_MASK;
                break;
            case BLE_QIOT_DATA_TYPE_STRUCT:
                type |= BLE_QIOT_ARRAY_STRUCT_BIT_MASK;
                break;
            }
        } else {
            type = BLE_QIOT_DATA_TYPE_BUTT;
        }
    }
    return type;
}

QiotData* QiotData::GetChildCtx(uint8_t index)
{
    if (index >= _childs.size())
        return NULL;
    return _childs.data() + index;
}

extern "C" void *ble_struct_array_get_elem_ctx(void *ctx, uint8_t id)
{
    QiotData *h = (QiotData *)ctx;
    return h->GetChildCtx(id);
}

extern "C" uint8_t ble_qiot_data_type_get(void *ctx)
{
    QiotData *h = (QiotData *)ctx;
    return h->GetType();
}

extern "C" uint8_t ble_struct_array_get_elem_cnt(void *ctx)
{
    QiotData *h = (QiotData *)ctx;
    return h->ChildsCount();
}

extern "C" int ble_struct_array_elem_set(void *ctx, uint8_t id, const char *val, int len)
{
    QiotData *h = (QiotData *)ctx;
    return !h->SetValue(id, val, len);
}

extern "C" int ble_qiot_data_set(void *ctx, const char *buf, uint16_t buf_len)
{
    QiotData *h = (QiotData *)ctx;
    return !h->SetValue(buf, buf_len);
}

extern "C" int ble_qiot_data_get(void *ctx, char *buf, uint16_t buf_len)
{
    QiotData *h = (QiotData *)ctx;
    return h->GetValue(buf, buf_len);
}

void ThingModel::Dump()
{
    if (!_valid) {
        BLE_QIOT_LOG_PRINT("ThingModel is inValid\n");
        return;
    }
    BLE_QIOT_LOG_PRINT("ThingModel Dump:\n");
    if (_properties) {
        BLE_QIOT_LOG_PRINT("Properties:\n");
        _properties->Dump();
    } else {
        BLE_QIOT_LOG_PRINT("Properties is null\n");
    }
    if (_events) {
        BLE_QIOT_LOG_PRINT("Events:\n");
        _events->Dump();
    } else {
        BLE_QIOT_LOG_PRINT("Events is null\n");
    }
    if (_actions) {
        BLE_QIOT_LOG_PRINT("Actions:\n");
        _actions->Dump();
    } else {
        BLE_QIOT_LOG_PRINT("Actions is null\n");
    }
}

bool ThingModel::Load(const char *jsonStr)
{
    CJsonObject model(jsonStr);
    if (!ParseProperties(model[PROPERTIES])) {
        _valid = false;
        if (_properties) {
            delete _properties;
            _properties = NULL;
        }
        ble_qiot_log_e("parse properties fail\n");
        return false;
    }
    if (!ParseEvents(model[EVENTS])) {
        if (_events) {
            delete _events;
            _events = NULL;
        }
        ble_qiot_log_e("parse events fail\n");
    }
    if (!ParseActions(model[ACTIONS])) {
        if (_actions) {
            delete _actions;
            _actions = NULL;
        }
        ble_qiot_log_e("parse actions fail\n");
    }
    _valid = true;
    return true;
}

bool ThingModel::ParseActions(CJsonObject &actions)
{
    if (_actions) {
        ble_qiot_log_e("actions is already exist, not null\n");
        return false;
    }

    int size = actions.GetArraySize();
    if (size <= 0) {
        ble_qiot_log_e("actions is null or not a array\n");
        return false;
    }

    _actions = new QiotData("", BLE_QIOT_DATA_TYPE_ARRAY);

    for (int i = 0; i < size; ++i) {
        CJsonObject &action = actions[i];
        string id;
        if (!action.Get(ID, id)) {
            ble_qiot_log_e("action no id param\n%s\n", action.ToFormattedString().c_str());
            return false;
        }
        _actions->_childs.push_back(QiotData(id, BLE_QIOT_DATA_TYPE_STRUCT));
        QiotData &val = _actions->_childs.back();
        CJsonObject &input = action[INPUT];
        val._childs.push_back(QiotData(INPUT, BLE_QIOT_DATA_TYPE_STRUCT));
        if (!ParseStruct(input, val._childs.back(), DEFINE, true)) {
            ble_qiot_log_e("action input parse fail\n%s\n", input.ToFormattedString().c_str());
            return false;
        }
        CJsonObject &output = action[OUTPUT];
        val._childs.push_back(QiotData(OUTPUT, BLE_QIOT_DATA_TYPE_STRUCT));
        if (!ParseStruct(output, val._childs.back(), DEFINE, true)) {
            ble_qiot_log_e("action output parse fail\n%s\n", output.ToFormattedString().c_str());
            return false;
        }
    }
    return true;
}

bool ThingModel::ParseEvents(CJsonObject &events)
{
    if (_events) {
        ble_qiot_log_e("events is already exist, not null\n");
        return false;
    }

    int size = events.GetArraySize();
    if (size <= 0) {
        ble_qiot_log_e("events is null or not a array\n");
        return false;
    }

    _events = new QiotData("", BLE_QIOT_DATA_TYPE_ARRAY);

    for (int i = 0; i < size; ++i) {
        CJsonObject &event = events[i];
        string id;
        if (!event.Get(ID, id)) {
            ble_qiot_log_e("event no id param\n%s\n", event.ToFormattedString().c_str());
            return false;
        }
        _events->_childs.push_back(QiotData(id, BLE_QIOT_DATA_TYPE_STRUCT));
        CJsonObject &params = event[PARAMS];
        if (!ParseStruct(params, _events->_childs.back(), DEFINE, true)) {
            ble_qiot_log_e("event params parse fail\n%s\n", params.ToFormattedString().c_str());
            return false;
        }
    }
    return true;
}

bool ThingModel::ParseProperties(CJsonObject &properties)
{
    if (_properties) {
        ble_qiot_log_e("properties is already exist, not null\n");
        return false;
    }

    int size = properties.GetArraySize();
    if (size <= 0) {
        ble_qiot_log_e("properties is null or not a array\n");
        return false;
    }

    _properties = new QiotData("", BLE_QIOT_DATA_TYPE_ARRAY);

    for (int i = 0; i < size; ++i) {
        CJsonObject &property = properties[i];
        string id, s_type;
        if (!property.Get(ID, id) || !property[DEFINE].Get(TYPE, s_type)) {
            ble_qiot_log_e("property no id or define.type param\n%s\n", property.ToFormattedString().c_str());
            return false;
        }
        int type = StrTypeToInt(s_type);
        if (type == BLE_QIOT_DATA_TYPE_BUTT) {
            ble_qiot_log_e("property type %s is unknown\n%s\n", s_type.c_str(), property.ToFormattedString().c_str());
            return false;
        }
        _properties->_childs.push_back(QiotData(id, type));
        if (type == BLE_QIOT_DATA_TYPE_STRUCT) {
            CJsonObject &specs = property[DEFINE][SPECS];
            if (!ParseStruct(specs, _properties->_childs.back(), DATATYPE, false)) {
                ble_qiot_log_e("property struct specs parse fail\n%s\n", specs.ToFormattedString().c_str());
                return false;
            }
        } else if (type == BLE_QIOT_DATA_TYPE_ARRAY) {
            CJsonObject &arrayInfo = property[DEFINE][ARRAYINFO];
            if (!ParseArray(arrayInfo, _properties->_childs.back(), DATATYPE, false)) {
                ble_qiot_log_e("property array arrayInfo parse fail\n%s\n", arrayInfo.ToFormattedString().c_str());
                return false;
            }
        }
    }
    return true;
}

bool ThingModel::ParseStruct(CJsonObject &Struct, QiotData &dat, const char* typeKey, bool noArray)
{
    int size = Struct.GetArraySize();
    if (size <= 0)
        return false;
    for (int i = 0; i < size; i++) {
        CJsonObject &item = Struct[i];
        string id, s_type;
        if (!item.Get(ID, id) || !item[typeKey].Get(TYPE, s_type)) {
            ble_qiot_log_e("struct member no id or %s.type param\n%s\n", typeKey, item.ToFormattedString().c_str());
            return false;
        }
        int type = StrTypeToInt(s_type);
        if (type == BLE_QIOT_DATA_TYPE_BUTT) {
            ble_qiot_log_e("struct member type %s is unknow\n%s\n", s_type.c_str(), item.ToFormattedString().c_str());
            return false;
        }
        if ((type == BLE_QIOT_DATA_TYPE_STRUCT) || ((type == BLE_QIOT_DATA_TYPE_ARRAY) && (noArray))) {
            ble_qiot_log_e("struct member is not allowed nesting type %s\n%s\n", s_type.c_str(), item.ToFormattedString().c_str());
            return false;
        }
        dat._childs.push_back(QiotData(id, type));
        if (type == BLE_QIOT_DATA_TYPE_ARRAY) {
            CJsonObject &child = item[typeKey][ARRAYINFO];
            if (!ParseArray(child, dat._childs.back(), typeKey, true)) {
                ble_qiot_log_e("struct member array parase arrayInfo fail\n%s\n", child.ToFormattedString().c_str());
                return false;
            }
        }
    }
    return true;
}

bool ThingModel::ParseArray(CJsonObject &array, QiotData &dat, const char *typeKey, bool noStruct)
{
    string s_type;
    if (!array.Get(TYPE, s_type)) {
        ble_qiot_log_e("array no type param\n%s\n", array.ToFormattedString().c_str());
        return false;
    }
    int type = StrTypeToInt(s_type);
    if (type == BLE_QIOT_DATA_TYPE_BUTT) {
        ble_qiot_log_e("array type %s is unknow\n%s\n", s_type.c_str(), array.ToFormattedString().c_str());
        return false;
    }
    if ((type == BLE_QIOT_DATA_TYPE_ARRAY) || ((type == BLE_QIOT_DATA_TYPE_STRUCT) && (noStruct))) {
        ble_qiot_log_e("array is not allowed nesting type %s\n%s\n", s_type.c_str(), array.ToFormattedString().c_str());
        return false;
    }
    dat._childs.push_back(QiotData("", type));
    if (type == BLE_QIOT_DATA_TYPE_STRUCT) {
        CJsonObject &child = array[SPECS];
        if (!ParseStruct(child, dat._childs.back(), typeKey, true)) {
            ble_qiot_log_e("array parase struct specs fail\n%s\n", child.ToFormattedString().c_str());
            return false;
        }
    }
    dat._childs.resize(BLE_QIOT_PROPERTY_DEFAULT_ARRAY_SIZE, dat._childs[0]);
    return true;
}

bool ThingModel::PropertyValid(uint8_t index)
{
    if (!_valid || !_properties || (index >= _properties->_childs.size())) {
        ble_qiot_log_e("property index %d is not valid\n", index);
        return false;
    }
    return true;
}

uint8_t ThingModel::GetPropertyType(uint8_t index)
{
    if (!PropertyValid(index))
        return BLE_QIOT_DATA_TYPE_BUTT;
    return _properties->_childs[index].GetType();
}

QiotData* ThingModel::GetPropertyCtx(uint8_t index)
{
    if (!PropertyValid(index))
        return NULL;
    return (_properties->_childs.data() + index);
}

bool ThingModel::ReportProperty(const char *id, const char *val)
{
    unsigned int i;
    QiotData *property;
    for (i = 0; i < _properties->ChildsCount(); ++ i) {
        property = _properties->_childs.data() + i;
        if (property->_id == id) {
            if (property->SetValue(val)) {
                return !ble_user_property_get_report_data(i, i + 1);
            }
            return false;
        }
    }
    return false;
}

QiotData* ThingModel::GetEventCtx(uint8_t index)
{
    if (!_valid || !_events || (index >= _events->_childs.size())) {
        ble_qiot_log_e("_events index %d is not valid\n", index);
        return NULL;
    }
    return (_events->_childs.data() + index);
}

QiotData* ThingModel::GetActionInputCtx(uint8_t index)
{
    if (!_valid || !_actions || (index >= _actions->_childs.size())) {
        ble_qiot_log_e("_actions index %d is not valid\n", index);
        return NULL;
    }
    return _actions->_childs[index].GetChildCtx(0);
}

QiotData* ThingModel::GetActionOutputCtx(uint8_t index)
{
    if (!_valid || !_actions || (index >= _actions->_childs.size())) {
        ble_qiot_log_e("_actions index %d is not valid\n", index);
        return NULL;
    }
    return _actions->_childs[index].GetChildCtx(1);
}
