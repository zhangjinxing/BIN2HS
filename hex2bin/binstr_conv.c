/*----------------------------------------头文件----------------------------------------------*/
#include <stdint.h>
#include <assert.h>

/*---------------------------------------接口函数---------------------------------------------*/

/******************************************************************************************
说明:         将两个16进制字符转换为一个字节数据
参数:
// @pchr        字符数组
// @len         转换的字符数量
// @presult     转换结果指针 0 失败 1成功
返回值:  void    0为因非法字符而失败 否则为成功
******************************************************************************************/
uint32_t Bcv_Hexstring2Uint32(const char* pchr, uint8_t len, uint8_t* presult)
{
    register char ch;
    uint32_t temp = 0;

    for (register uint8_t i = len; i ; --i)
    {
        ch = *pchr++;

        if (ch <= '9') ch -= '0';           /* 数字 */
        else if (ch >= 'a') ch -= 'a' - 10; /* 小写字符 */
        else if (ch >= 'A') ch -= 'A' - 10; /* 大写字符 */
        else ch = 0xff;

        //是否为合法的16进制字符
        if ((uint8_t)ch >= 16)
        {
            *presult = 0;
            return 0;
        }
        //组合数据
        temp = temp << 4 | (uint8_t)ch;
    }
    *presult = 1;
    return temp;
}

/******************************************************************************************
说明:         将一个字节数据转换为2个HEX字符
参数:
// @pdes        字符数组
// @adata       待转换数据
返回值:  uint8_t 结算结果
******************************************************************************************/
void Bcv_Byte2Hexstring(char pdes[], uint8_t adata)
{
    static const char ByteHexCodeTable[] = "0123456789ABCDEF";

    pdes[0] = ByteHexCodeTable[adata >> 4];
    pdes[1] = ByteHexCodeTable[adata & 0x0f];
}

/******************************************************************************************
说明:         拷贝字符串到指定的内存区域
参数:
// @pdes        目标存储区
// @source      源字符串
返回值:  char* 返回目标内存区字符串结束位置
******************************************************************************************/
char* Bcv_strcpy(char* dest, const char* source)
{
    //参数断言
    assert(dest);
    assert(source);

    if (dest != source)
    {
        for (; '\0' != (*dest = *source); ++dest, ++source);
    }
    return dest;
}
/******************************************************************************************
说明:         将指定的bin数据转换为hex字符串并计算校验和
参数:
// @strdeststr  目标字符串数组
// @binbuff     bin数据指针
// @count       数据数量
// @sum         校验和变量指针
返回值:  char* 返回目标内存区字符串后第一个空闲位置
******************************************************************************************/
char* Bcv_ConvertHexAndSum(char* deststr, void* binbuff, uint8_t count, uint8_t* sum)
{
    uint8_t* bytebuff = (uint8_t*)binbuff;

    for (uint8_t i = count; i; --i, deststr+=2, ++bytebuff)
    {
        Bcv_Byte2Hexstring(deststr, *bytebuff);
        *sum += *bytebuff;
    }
    return deststr;
}

