## BIN2HS库

### 概述
`BIN2HS`是支持Intel Hex、Motorola S格式与bin镜像格式之间进行转换。其支持文件信息快速查看、分段数据处理。

### 镜像文件格式
嵌入式系统中各种系统镜像常用已下几种文件格式来保存:
- hex格式：包含地址信息，由多行纪录条目组成的文本格式文件，每行都由校验和来保证数据完整性。
- srecord格式：为motorola常用的镜像格式，与hex类似，也是由多个记录条目组成的文本格式文件
- bin格式：二进制镜像格式，不包含任何地址信息，也不包含校验和数据。只是数据的二进制集合。

### 文件

- IntelHex.c：包括hex解析及与bin文件相互转换的核心函数
- IntelHex.h：包括hex文件相关的各种类型定义与函数原型
- Srecord.c：包括srecord解析及与bin文件相互转换的核心函数
- Srecord.h：包括srecord文件相关的各种类型定义与函数原型
- binstr_conv.c：数据与字符串的相互转换支持

### hex记录

hex文件的记录行都由`:`字符开始，之后为字节数量、起始地址和数据组成的16进制字符串。
`[:] [字节数:1字节] [起始地址:2字节] [数据] [校验字段:1字节]`
> 示例： `:104000005829002049AE0008DD4F0008DF4F0008A6` 
- **:** 每个记录行都由`:`开头
- **10** 记录行包含的数据字节数(不包括起始地址和校验字段)
- **4000** 当前记录的起始地址
- **00** 记录类型，00-数据记录，01-记录结束，04-扩展线性记录
- **5829-0008** 记录行中包含的0x10长度的数据记录
- **A6** 校验和为`:`之后所有数据字段的代数和的补码(所有数据累加和为0)

每条hex记录有1字节数据用以表示当前记录条目的类型：
- **00** 数据记录，表示当前记录行内包含实际的映像数据
- **01** 记录结束，为hex的最后一个条目，其他字段都为0
- **02** 扩展段地址，数据项包含2字节扩展地址字段，左移4位作为偏移地址
- **03** 起始段地址，保存到8086 CS:IP中的地址
- **04** 扩展线性地址， 数据项包含2字节扩展地址字段，左移16位作为偏移地址
- **05** 起始线性地址，保存到8086 CS:IP中的地址

### srecord记录

srecord记录(已下简称S格式)与hex记录相似。也是由16进制字符组成的记录行构成。每个记录由`S`字符作为开始。
> 示例：`S1137AF00A0A0D0000000000000000000000000061`
- **S** 记录起始标志
- **1** 记录类型，1-16位地址的数据记录
- **13** 当前记录的字节数，位起始地址字段+数据+校验和
- **7AF0** 当前记录起始地址
- **0A0A-0000** 记录中包含的数据字段
- **61** 校验和字段，为字节数、起始地址和数据字段累加和的反码(所有数据累加和为0xff)

### 对象

```C
//HEX记录结构
typedef struct eHexRecord
{
    uint8_t count;          /* 数据字节数 */
    uint16_t rcdAddr;       /* 当前记录数据起始地址 */
    HexRecordType type;     /* 当前记录类型 */
    uint32_t fileAddr;      /* 文件地址=当前记录地址+扩展地址 */
    uint32_t extAddr;       /* 用于保存记录中的扩展地址 */
    uint8_t *pbuff;         /* 转换数据缓存内存指针 */

    uint32_t minAddr;       /* 记录中最小地址 */
    uint32_t maxAddr;       /* 记录中最大地址 */
}IHexRecord;
```

```C
//写HEX描述结构体
typedef struct eHexWirteInfo
{
    uint8_t* pbuff;         /* bin内存区 */
    uint32_t size;          /* bin文件大小 */
    uint32_t startAddr;     /* bin起始偏移地址 */
    uint16_t linewidth;     /* 每行数据字节数量 */

    uint32_t offset;        /* 当前索引 */
}IHexWriteDescribe;
```

### 使用方法

获取hex文件信息：
> 1. 声明`IHexRecord`hex记录对象。
> 2. 调用`IHex_InitRecordData`初始化记录对象
> 3. 对于每个hex记录字符串行，都调用`IHex_GetNextHeadInfo`函数来解析字符串，直到`IHexRecord`中解析的记录类型为`IHEX_TYPE_ENDFILE`为止。
> 4. 由`IHexRecord`结构体可得到hex文件的最小数据地址和最大数据地址

转换hex数据到bin：
> 1. 声明`IHexRecord`，并声明一个数据转换的内存缓冲区，其最大缓冲区大小为`IHEX_MAX_BINDATA_NUM`
> 2. 调用`IHex_InitRecordData`来初始化记录对象
> 3. 对于每个hex记录行，都调用`IHex_GetNextBindata`函数来解析记录行，直到`IHexRecord`中解析的记录类型为`IHEX_TYPE_ENDFILE`为止。
> 4. 当记录类型为`IHEX_TYPE_DATARECORD`时，保存对象内的转换数据

转换bin到hex：
> 1. 声明`IHexWriteDescribe`，并对各个成员进行赋值
> 2. 调用`IHex_InitWriteDesc`来初始化记录对象
> 3. 循环调用`IHex_GetNextHexlinestr`，并将得到的记录字符串保存到需要的位置，知道其返回为0为止


## Srecord

srecerd对象的使用方法与hex类似，请参考hex的使用方法。
