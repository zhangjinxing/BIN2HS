/***********************************************************************************************
_____________________________________杭州大仁科技有限公司_______________________________________
@文 件 名: SRecord.h
@日    期: 2016.09.16
@作    者: 张诗星

@文件说明:

    SRecord是用于处理MOTOROL的S记录文件的操作库

@修订说明:

2016.09.16  初始版本

***********************************************************************************************/
#ifndef _S_RECORD_H_
#define _S_RECORD_H_

/*----------------------------------------头文件----------------------------------------------*/
#include <stdint.h>

/*---------------------------------------常量定义---------------------------------------------*/
#define SREC_MAX_BINDATA_NUM    (255-3)                         /* 单个记录最大数据大小 */
#define SREC_RECORD_STR_LEN     (4+IHEX_MAX_BINDATA_NUM*2+4)    /* 最大行记录长度 */
#define IHEX_STRING_BUFF_SIZE   (IHEX_RECORD_STR_LEN+IHEX_EXTADDR_STR_LEN)


/*---------------------------------------枚举定义---------------------------------------------*/
//错误码枚举
typedef enum
{
    SREC_RESULT_NOERROR = 0,            /* 无错误 */
    SREC_RESULT_INVALID_RECORD,         /* 首字符非S，非法的记录 */
    SREC_RESULT_INVALID_CHAR,           /* 16进制存在非法的字符 */
    SREC_RESULT_INVALID_TYPE,           /* 非法的记录类型 */
    SREC_RESULT_ERROR_CHECKSUM,         /* 校验和错误 */
    SREC_RESULT_ERROR_DATA_COUNT,       /* 数据数量错误 */
}SRecResult;

//记录类型
typedef enum
{
    SREC_TYPE_DATA  = 0,    /* 数据记录类型 */
    SREC_TYPE_HEAD,         /* 记录头类型 */
    SREC_TYPE_ENDBOOT,      /* 启动地址记录类型 */
    SREC_TYPE_COUNT,        /* 计数记录 */
}SRecType;

/*---------------------------------------类型定义---------------------------------------------*/
typedef struct
{
    uint8_t count;          /* 数据字节数 */
    uint32_t address;       /* 记录数据地址 */
    SRecType type;          /* 记录类型 */
    uint8_t *pbuff;         /* 内存指针 */
    uint16_t reclines;      /* 数据记录行数量 */

    uint32_t minAddr;       /* 最小地址 */
    uint32_t maxAddr;       /* 最大地址 */
}SRecRecord;

//写SREC描述结构体
typedef struct
{
    uint8_t* pbuff;         /* bin内存区 */
    uint32_t size;          /* bin文件大小 */
    uint32_t offsetAddr;    /* bin偏移地址 */
    uint16_t linewidth;     /* 每行数量 */

    uint32_t index;         /* 当前索引 */
}SRecWriteDescribe;

/*---------------------------------------接口函数---------------------------------------------*/
//兼容C C++混合编程
#ifdef __cplusplus
extern "C" {
#endif

void SRec_InitRecord(SRecRecord* precord, uint8_t* pbuff);
void SRec_InitWriteDesc(SRecWriteDescribe* pdes);
SRecResult SRec_GetNextHeadInfo(SRecRecord* precord, const char * str);
SRecResult SRec_GetNextBindata(SRecRecord* precord, const char * str);
uint32_t SRec_GetNextlinestr(SRecWriteDescribe* pinfo, char* str);

//兼容C C++混合编程
#ifdef __cplusplus 
}
#endif

#endif

