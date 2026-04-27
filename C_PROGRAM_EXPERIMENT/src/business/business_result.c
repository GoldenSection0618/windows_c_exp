#include "business.h"
#include "business_internal.h"

BizResult mapDataResult(DataResult result)
{
    switch (result) {
    case DATA_OK:
        return BIZ_OK;
    case DATA_ERR_DUPLICATE:
        return BIZ_ERR_DUPLICATE_CARD;
    case DATA_ERR_NO_MEMORY:
        return BIZ_ERR_NO_MEMORY;
    case DATA_ERR_FILE_OPEN:
        return BIZ_ERR_FILE_OPEN;
    case DATA_ERR_FILE_NOT_FOUND:
        return BIZ_ERR_FILE_NOT_FOUND;
    case DATA_ERR_RECORD_FORMAT:
        return BIZ_ERR_RECORD_FORMAT;
    case DATA_ERR_TIME_PARSE:
        return BIZ_ERR_TIME_PARSE;
    default:
        return BIZ_ERR_SYSTEM;
    }
}


const char *bizGetMessage(BizResult result)
{
    switch (result) {
    case BIZ_OK:
        return "操作成功。";
    case BIZ_ERR_INVALID_CARD_NAME:
        return "卡号输入不合法，应为1~18位，且只能包含大小写字母、数字和 _ @ # $ % !。";
    case BIZ_ERR_INVALID_PASSWORD:
        return "密码输入不合法，应为1~8位，且只能包含大小写字母、数字和 _ @ # $ % !。";
    case BIZ_ERR_INVALID_AMOUNT:
        return "开卡金额输入不合法，应为非负金额，且最多保留两位小数。";
    case BIZ_ERR_BALANCE_TOO_LARGE:
        return "余额过大，卡内余额必须小于1000000元。";
    case BIZ_ERR_DUPLICATE_CARD:
        return "卡号已存在，不能重复添加！";
    case BIZ_ERR_CARD_NOT_FOUND:
        return "没有该卡的信息！";
    case BIZ_ERR_NO_MATCHED_CARD:
        return "没有符合关键字的卡信息！";
    case BIZ_ERR_WRONG_PASSWORD:
        return "密码错误！";
    case BIZ_ERR_CARD_UNAVAILABLE:
        return "该卡正在使用，不能上机！";
    case BIZ_ERR_CARD_CANCELED_FOR_START:
        return "该卡已注销，不能上机！";
    case BIZ_ERR_BALANCE_NOT_ENOUGH:
        return "卡号余额不足！";
    case BIZ_ERR_NO_UNSETTLED_BILLING:
        return "未找到该卡的未结算消费记录！";
    case BIZ_ERR_CARD_STATUS_INVALID_FOR_STOP:
        return "该卡当前不在上机状态，不能下机！";
    case BIZ_ERR_CARD_CANCELED_FOR_RECHARGE:
        return "已注销卡不能充值！";
    case BIZ_ERR_CARD_CANCELED_FOR_REFUND:
        return "已注销卡不能退费！";
    case BIZ_ERR_CARD_STATUS_INVALID_FOR_REFUND:
        return "该卡正在上机，不能退费！";
    case BIZ_ERR_CARD_CANCELED_FOR_CANCEL:
        return "该卡已注销，不能重复注销！";
    case BIZ_ERR_CARD_STATUS_INVALID_FOR_CANCEL:
        return "该卡正在上机，不能注销！";
    case BIZ_ERR_INVALID_TIME_RANGE:
        return "时间范围输入不合法！";
    case BIZ_ERR_BILLING_RECORD_NOT_FOUND:
        return "没有找到符合条件的消费记录！";
    case BIZ_ERR_FILE_OPEN:
        return "数据文件异常：卡信息文件打开失败。";
    case BIZ_ERR_FILE_NOT_FOUND:
        return "数据文件异常：卡信息文件不存在。";
    case BIZ_ERR_RECORD_FORMAT:
        return "数据文件内容异常：卡信息文件记录格式错误。";
    case BIZ_ERR_TIME_PARSE:
        return "数据文件内容异常：卡信息文件时间字段解析失败。";
    case BIZ_ERR_NO_MEMORY:
        return "系统内存不足，无法继续操作。";
    default:
        return "系统内部错误。";
    }
}

