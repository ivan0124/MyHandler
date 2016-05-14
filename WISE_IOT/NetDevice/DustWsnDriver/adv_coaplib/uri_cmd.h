// Group
#define URI_MOTE_INFO          "/dev/info"
#define URI_NET_INFO           "/net/info"
#define URI_NET_STATUS         "/net/status"
#define URI_SENSOR_INFO        "/sen/info"
#define URI_SENSOR_STATUS      "/sen/status"

// ACTIONS
#define URI_PARAM_ACTION_LIST      "action=list"
#define URI_PARAM_ACTION_OBSERVE   "action=observe"
#define URI_PARAM_ACTION_CANCEL    "action=cancel"
#define URI_PARAM_ACTION_QUERY     "action=query"

#define URI_PARAM_BOOL_VALUE_TRUE  "bv=1"
#define URI_PARAM_BOOL_VALUE_FALSE "bv=0"

#define URI_PARAM_FIELD_ID         "field=id"
#define URI_PARAM_ID(D)            "id="#D

// Return
#define URI_NAME_STR               "n"
#define URI_NAME_STR_LEN           1
#define URI_MODEL_STR              "mdl"
#define URI_MODEL_STR_LEN          3
#define URI_MOTE_INFO_HDR          "</dev>"
#define URI_MOTE_INFO_HDR_LEN      6
#define URI_SENSOR_LIST_HDR        "</sen>"
#define URI_SENSOR_LIST_HDR_LEN    6
#define URI_NET_INFO_HDR          "</net>"
#define URI_NET_INFO_HDR_LEN       6 
#define URI_SW_STR                 "sw"
#define URI_SW_STR_LEN             2
#define URI_RESET_STR              "reset"
#define URI_RESET_STR_LEN          5
#define URI_SENSOR_SID_STR         "sid"
#define URI_SENSOR_SID_STR_LEN     3
#define URI_EQUAL_STR              "=\""
#define URI_EQUAL_STR_LEN          2
#define URI_ALL_STR                "all"
#define URI_ALL_STR_LEN            3
#define URI_CANCEL_OBSERVE_STR     "cancel /sen/status?id="
#define URI_CANCEL_OBSERVE_STR_LEN 22
