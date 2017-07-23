/***********************************************************************************************
_____________________________________杭州大仁科技有限公司_______________________________________
@文 件 名: SRecord.c
@日    期: 2016.09.16
@作    者: 张诗星

@文件说明:

SRecord是用于处理MOTOROL的S记录文件的操作库

@修订说明:

2016.09.16  初始版本

***********************************************************************************************/

/*----------------------------------------头文件----------------------------------------------*/
#include <assert.h>
#include "Srecord.h"

/*---------------------------------------常量定义---------------------------------------------*/
//字段偏移量定义
#define OFFSET_TYPE             (1)     /* 记录类型偏移 1字符 */
#define OFFSET_BYTE_COUNT       (2)     /* 字节数量偏移 1字节 */
#define OFFSET_ADDRESS          (4)     /* 地址字段偏移 2、3、4字节 */

/*---------------------------------------内部变量---------------------------------------------*/

typedef struct
{
    uint8_t addrwidth;      // 地址字段宽度字节数
    SRecType rectype;       // 记录类型
}SRecSTypeStruct;

static const SRecSTypeStruct typeTable[10] =
{
    { 2, SREC_TYPE_HEAD },   // S0
    { 2, SREC_TYPE_DATA },   // S1
    { 3, SREC_TYPE_DATA },   // S2
    { 4, SREC_TYPE_DATA },   // S3
    { 0, (SRecType)-1 },     // S4
    { 2, SREC_TYPE_COUNT },  // S5
    { 3, SREC_TYPE_COUNT },  // S6
    { 4, SREC_TYPE_ENDBOOT },// S7
    { 3, SREC_TYPE_ENDBOOT },// S8
    { 2, SREC_TYPE_ENDBOOT } // S9
};

/*---------------------------------------内部函数---------------------------------------------*/

// 外部引用函数
uint32_t Bcv_Hexstring2Uint32(const char* pchr, uint8_t len, uint8_t* presult);
void Bcv_Byte2Hexstring(char pdes[], uint8_t adata);
char* Bcv_strcpy(char* dest, const char* source);
char* Bcv_ConvertHexAndSum(char* deststr, void* binbuff, uint8_t count, uint8_t* sum);

/******************************************************************************************
说明:         解析记录头数据
参数:
// @precd       记录指针
// @pchr        字符数组
返回值:  SRecResult  结果
******************************************************************************************/
static SRecResult ParseHead(SRecRecord* precd, const char pchr[])
{
    uint8_t count,res;
    uint8_t typeindex;      // 类型索引
    uint8_t addrwidth;      // 地址字段宽度
    
    //S 数据起始标志
    if ('S' != pchr[0])
        return SREC_RESULT_INVALID_RECORD;
    //解析记录格式
    typeindex = pchr[OFFSET_TYPE]-'0';   /* S0 S1 S2 S3... */
    if (typeindex > 9u || (SRecType)-1 == (precd->type = typeTable[typeindex].rectype))
        return SREC_RESULT_INVALID_TYPE;

    //字节数
    count = (uint8_t)Bcv_Hexstring2Uint32(&pchr[OFFSET_BYTE_COUNT], 2, &res);
    if (!res)
        return SREC_RESULT_INVALID_CHAR;

    //获取并检查记录字节数
    addrwidth = typeTable[typeindex].addrwidth;   // n字节地址+1字节crc字段
    switch (precd->type)
    {
    case SREC_TYPE_DATA:
    case SREC_TYPE_HEAD:
        if (count <= addrwidth + 1)
            return SREC_RESULT_ERROR_DATA_COUNT;
        //获取记录地址
        precd->address = Bcv_Hexstring2Uint32(&pchr[OFFSET_ADDRESS], addrwidth * 2, &res);
        if (!res)
            return SREC_RESULT_INVALID_CHAR;
        break;
    case SREC_TYPE_COUNT:
    case SREC_TYPE_ENDBOOT:
        if (count != addrwidth + 1)
            return SREC_RESULT_ERROR_DATA_COUNT;
        //获取计数值
        precd->address = Bcv_Hexstring2Uint32(&pchr[OFFSET_ADDRESS], addrwidth * 2, &res);
        if (!res)
            return SREC_RESULT_INVALID_CHAR;
        break;
    default:
        assert(0);
        break;
    }

    precd->count = count - (addrwidth + 1);    /* 所包含的数据字节数 */
  
    //解析成功
    return SREC_RESULT_NOERROR;
}
/******************************************************************************************
说明:         解析并校验数据
参数:
// @precd       记录指针
// @str         字符数组
返回值:  SRecResult  结果
******************************************************************************************/
static SRecResult ParseData(SRecRecord* precd, const char* str)
{
    uint8_t sum, bytedata,res;
    uint32_t temp;

    //数据记录计数
    if (SREC_TYPE_DATA == precd->type)
        ++precd->reclines;

    //计算字节数、地址字段校验和
    sum = (uint8_t)Bcv_Hexstring2Uint32(&str[OFFSET_BYTE_COUNT], 2, &res);
    if (!res)
        return SREC_RESULT_INVALID_TYPE;
    for (temp = precd->address; temp; temp >>= 8)
        sum += (uint8_t)temp;   /* 地址字段 */

    //数据字段起始地址
    bytedata = typeTable[str[OFFSET_TYPE] - '0'].addrwidth;
    str = str + OFFSET_ADDRESS + (bytedata * 2);

    //转换数据并计算校验和
    for (uint8_t i = 0; i < precd->count; i++, str += 2)
    {
        bytedata = (uint8_t)Bcv_Hexstring2Uint32(str, 2, &res);
        if (!res)
            return SREC_RESULT_INVALID_TYPE;
        precd->pbuff[i] = bytedata;            /* 保存数据 */
        sum += bytedata;                       /* 计算校验和 */
    }
    //计算校验和
    bytedata = (uint8_t)Bcv_Hexstring2Uint32(str, 2, &res);
    if (!res)
        return SREC_RESULT_INVALID_TYPE;
    if (0xffu != (sum + bytedata))
        return SREC_RESULT_ERROR_CHECKSUM;

    return SREC_RESULT_NOERROR;
}
/******************************************************************************************
说明:         转换记录为HEX文件字符串[s][3][字节数][地址][数据]
参数:
// @str         字符串
// @precord     记录指针
返回值:  char*   返回字符串末尾指针
******************************************************************************************/
static char* ConvertHexline(char pchr[], SRecRecord* precord)
{
    uint8_t i, sum = 0;
    uint32_t temp;
    static const char endline[] = "S9030000FC";

    //参数断言
    assert(pchr);
    assert(precord);
    assert(SREC_TYPE_DATA == precord->type || 
        SREC_TYPE_ENDBOOT == precord->type);

    if (SREC_TYPE_DATA == precord->type)
    {   /* 数据记录类型 */
        //开始转换为记录字符串
        pchr[0] = 'S';              /* :标志 */

        pchr[OFFSET_TYPE] = '3';
        //字节数
        Bcv_Byte2Hexstring(&pchr[OFFSET_BYTE_COUNT], precord->count+5); /* 字节数量 */
        pchr = &pchr[OFFSET_ADDRESS];                   /* 地址字段 */
        sum += precord->count+5;                        /* 数据字节数+4字节地址+1字节CRC */
        //地址
        for (i = 0, temp = precord->address;i < 4;++i)
        {
            Bcv_Byte2Hexstring(pchr, (uint8_t)(temp >> 24));
            sum += (uint8_t)(temp >> 24);   /* 校验和 */                 
            pchr += 2;                      /* 下个保存位置 */
            temp <<= 8;
        }
        //记录数据
        pchr = Bcv_ConvertHexAndSum(pchr, precord->pbuff, precord->count, &sum);
        //计算校验和
        Bcv_Byte2Hexstring(pchr, 0xff - sum);
        pchr += 2;
    }
    else if (SREC_TYPE_ENDBOOT == precord->type)
    {   /* 记录结束 */
        pchr = Bcv_strcpy(pchr, endline);
    }

    //添加换行和\0字符
    pchr[0] = '\n';
    pchr[1] = '\0';
    return &pchr[1];
}
/*---------------------------------------接口函数---------------------------------------------*/

/******************************************************************************************
说明:         初始化记录结构体
参数:
// @precd       记录结构体指针
返回值:  void
******************************************************************************************/
void SRec_InitRecord(SRecRecord* precd, uint8_t* pbuff)
{
    //参数断言
    assert(precd);

    precd->count = 0;
    precd->reclines = 0;
    precd->address = 0;
    precd->pbuff = pbuff;

    precd->maxAddr = 0;
    precd->minAddr = (uint32_t)-1;
}
/******************************************************************************************
说明:         初始化写结构体
参数:
// @pinfo       写操作结构体指针
返回值:  void
******************************************************************************************/
void SRec_InitWriteDesc(SRecWriteDescribe* pdes)
{
    //参数断言
    assert(pdes);

    pdes->index = 0;
}
/******************************************************************************************
说明:         获取下一个记录信息
参数:
// @pinfo       记录信息结构体指针
// @precord     记录结构体指针
// @str         记录行字符
返回值:  SRecResult 结果
******************************************************************************************/
SRecResult SRec_GetNextHeadInfo(SRecRecord* precord, const char * str)
{
    SRecResult result;
    uint32_t end;

    //参数断言
    assert(precord);
    assert(str);

    //转换记录头数据
    if (SREC_RESULT_NOERROR != (result = ParseHead(precord, str)))
        return result;

    //数据记录类型
    if (SREC_TYPE_DATA == precord->type)
    {
        end = precord->address + precord->count - 1;
        //获取地址范围信息
        if (precord->maxAddr < end)
            precord->maxAddr = end;
        if (precord->minAddr > precord->address)
            precord->minAddr = precord->address;
    }
    return SREC_RESULT_NOERROR;
}
/******************************************************************************************
说明:         获取下一个记录数据
参数:
// @precd       记录结构体指针
// @str         记录行字符
返回值:  SRecResult 结果
******************************************************************************************/
SRecResult SRec_GetNextBindata(SRecRecord* precd, const char * str)
{
    SRecResult result;

    //参数断言
    assert(precd);
    assert(str);
    assert(precd->pbuff);

    //转换记录头数据
    if (SREC_RESULT_NOERROR != (result = SRec_GetNextHeadInfo(precd, str)))
        return result;

    //数据记录类型转换BIN数据
    return ParseData(precd, str);
}
/******************************************************************************************
说明:         获取下一个HEX记录字符串
参数:
// @str         字符串缓冲区
// @pdes        写操作描述结构体指针
返回值:  uint32_t 返回发送内存索引
******************************************************************************************/
uint32_t SRec_GetNextlinestr(SRecWriteDescribe* pinfo, char* str)
{
    SRecRecord record;
    uint32_t rlen;

    //参数断言
    assert(str);
    assert(pinfo);
    assert(pinfo->pbuff);
    
    record.pbuff = pinfo->pbuff;        /* 缓冲区 */
    record.address = pinfo->offsetAddr;    /* 记录偏移地址 */

    //判断是否结束
    if (pinfo->index >= pinfo->size)
    {   /* HEX记录结束 */
        record.type = SREC_TYPE_ENDBOOT;
        ConvertHexline(str, &record);
        return 0;
    }

    //计算所需发送数据数量
    rlen = pinfo->size - pinfo->index;
    if (rlen > pinfo->linewidth)
        rlen = pinfo->linewidth;

    //处理数据记录为HEX记录行
    record.type = SREC_TYPE_DATA;       /* 数据记录类型 */
    record.count = (uint8_t)rlen;       /* 数据数量 */
    ConvertHexline(str, &record);

    //处理地址、索引
    pinfo->index += rlen;
    pinfo->pbuff += rlen;
    pinfo->offsetAddr += rlen;

    return pinfo->index;
}


