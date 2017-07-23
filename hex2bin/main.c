// hex2bin.cpp : 定义控制台应用程序的入口点。
//

// **************************************************************************
// 头文件
#include "IntelHex.h"
#include "Srecord.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

// **************************************************************************
// 类型定义
typedef struct
{
    FILE* rfile;        // 读取文件句柄
    FILE* wfile;        // 写入文件句柄

    uint8_t* binbuff;   // 二进制缓冲区
    uint8_t* rambuff;   // ram数据缓冲区
    char* strbuff;      // 字符串缓冲区
}ConvertContext;

// **************************************************************************
//  内部数据定义

// 字符串资源定义
static const char* STRING_OPEN_FILEERROR = "打开文件:\"%s\" 时出现错误!\r\n";
static const char* STRING_MALLO_MEMERROR = "分配内存错误!\r\n";
static const char* STRING_READ_ERROREND = "读取文件时发现意外的文件结尾!\r\n";
static const char* STRING_FILE_ERROR = "文件数据错误，错误码为:%d 错误行:%d\r\n";

// **************************************************************************
//  内部函数

// 初始化上下文对象
static void InitContext(ConvertContext* pctx)
{
    if (pctx)
    {
        pctx->rfile = NULL;
        pctx->wfile = NULL;
        pctx->strbuff = NULL;
        pctx->binbuff = NULL;
        pctx->rambuff = NULL;
    }
}
// 检测空行
static bool CheckSpaceLine(const char* linstr)
{
    for (char chr; '\0' != (chr = *linstr); ++linstr)
    {
        if ('\n' == chr || '\r' == chr ||
            ' ' == chr || '\t' == chr)
            continue;
        else
            return false;
    }
    return true;
}
// 退出应用程序
static void ExitApplication(ConvertContext* pctx, const char*fmt, ...)
{
    // 打印信息字符串
    if (fmt)
    {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }

    // 清理使用的资源
    if (pctx)
    {
        if (pctx->rfile) fclose(pctx->rfile);
        if (pctx->wfile) fclose(pctx->wfile);
        if (pctx->binbuff) free(pctx->binbuff);
        if (pctx->strbuff) free(pctx->strbuff);
        if (pctx->rambuff) free(pctx->rambuff);
    }

    getchar();
    exit(0);
}
// 检查路径有效
static void CheckPatchValid(const char* rpath, const char* wpath)
{
    if (NULL == rpath || *rpath == '\0' ||
        NULL == wpath || *wpath == '\0')
        ExitApplication(NULL, "输入文件路径为空!...\r\n");
}

// **************************************************************************
// 数据转换功能函数

//HEX2BIN转换
static void Hex2Bin(const char* rpath, const char* wpath)
{
    IHexResult result;
    IHexRecord record;

    ConvertContext ctx;
    InitContext(&ctx);

    // 检查路径有效
    CheckPatchValid(rpath, wpath);
    // 打开HEX文件
    if (NULL == (ctx.rfile = fopen(rpath, "r")))
        ExitApplication(&ctx, STRING_OPEN_FILEERROR, rpath);

    // 分配相应的缓冲内存空间
    if (NULL == (ctx.strbuff = (char*)malloc(IHEX_STRING_BUFF_SIZE)) ||
        NULL == (ctx.binbuff = (uint8_t*)malloc(IHEX_MAX_BINDATA_NUM)))
        ExitApplication(&ctx, STRING_MALLO_MEMERROR);

    // 读取HEX信息以分配内存空间
    uint32_t startaddress = 0;
    IHex_InitRecord(&record, ctx.binbuff);
    for (int clen = 0;;)
    {
        if (NULL == fgets(ctx.strbuff, IHEX_RECORD_STR_LEN, ctx.rfile))
            ExitApplication(&ctx, STRING_READ_ERROREND);

        ++clen;
        if (CheckSpaceLine(ctx.strbuff))       // 空行
            continue;

        result = IHex_GetNextHeadInfo(&record, ctx.strbuff);
        if (IHEX_RESULT_NOERROR != result)
            ExitApplication(&ctx, STRING_FILE_ERROR, result, clen);
        else if (record.type == IHEX_TYPE_STARTADDRESS)
        {
            startaddress = record.fileAddr;
        }
        else if (record.type == IHEX_TYPE_ENDFILE)
        {
            printf("文件:\"%s\" 读取成功\r\n", rpath);
            printf("文件记录数据范围为:0X%08X - 0X%08X\r\n", record.minAddr, record.maxAddr);
            printf("共: %d 字节数据 %d 行\r\n", record.maxAddr- record.minAddr + 1, clen);
            if (startaddress)
                printf("起始地址为：0X%08X\r\n", startaddress);
            break;
        }
    }

    const uint32_t bytenum = record.maxAddr - record.minAddr + 1;
    //分配内存并填充0xff
    if (NULL == (ctx.rambuff = (uint8_t*)malloc(bytenum)))
        ExitApplication(&ctx, "分配内存错误!\r\n");
    memset(ctx.rambuff, 0xff, bytenum);
    // 打开写入文件
    if (NULL == (ctx.wfile = fopen(wpath, "wb")))
        ExitApplication(&ctx, STRING_OPEN_FILEERROR, wpath);

    // 开始解析hex文件并保存数据
    rewind(ctx.rfile);              // 重定位读取文件
    IHex_InitRecord(&record, ctx.binbuff);
    for (int clen = 0;;)
    {
        if (NULL == fgets(ctx.strbuff, IHEX_RECORD_STR_LEN, ctx.rfile))
            ExitApplication(&ctx, STRING_READ_ERROREND);

        ++clen;
        if (CheckSpaceLine(ctx.strbuff))       // 空行
            continue;

        if (IHEX_RESULT_NOERROR != (result = IHex_GetNextBindata(&record, ctx.strbuff)))
            ExitApplication(&ctx, STRING_FILE_ERROR, result, clen);
        else if (record.type == IHEX_TYPE_ENDFILE)
        {
            fwrite(ctx.rambuff, 1, bytenum, ctx.wfile);
            ExitApplication(&ctx, "转换数据完成\r\n");
        }
        else if (record.type == IHEX_TYPE_DATARECORD)
            memcpy(ctx.rambuff + (record.fileAddr - record.minAddr), ctx.binbuff, record.count);
    }
}
//BIN2HEX转换
static void Bin2Hex(const char* rpath, const char* wpath, uint32_t offset)
{
    uint32_t fsize;
    uint32_t re;

    IHexWriteDescribe winfo;
    ConvertContext ctx;
    InitContext(&ctx);

    // 检查路径有效
    CheckPatchValid(rpath, wpath);

    if (NULL == (ctx.rfile = fopen(rpath, "rb")))
        ExitApplication(&ctx, STRING_OPEN_FILEERROR, rpath);


    // 获取文件大小
    fseek(ctx.rfile, 0, SEEK_END);      // 定位到文件结尾
    fsize = (uint32_t)ftell(ctx.rfile); // 计算文件大小

    // 分配内存并读取数据
    if (NULL == (ctx.rambuff = (uint8_t*)malloc(fsize)) ||
        NULL == (ctx.strbuff = (char*)malloc(IHEX_STRING_BUFF_SIZE)))
        ExitApplication(&ctx, STRING_MALLO_MEMERROR);
    rewind(ctx.rfile);      // 重定位文件
    fread(ctx.rambuff, 1, fsize, ctx.rfile);
    fclose(ctx.rfile);
    ctx.rfile = NULL;

    // 转换并写入文件
    winfo.pbuff = ctx.rambuff;
    winfo.linewidth = 32;
    winfo.startAddr = offset;
    winfo.size = fsize;
    IHex_InitWriteDesc(&winfo);
    if (NULL == (ctx.wfile = fopen(wpath, "w")))
        ExitApplication(&ctx, STRING_OPEN_FILEERROR, wpath);
    for (;;)
    {
        re = IHex_GetNextHexlinestr(&winfo, ctx.strbuff);
        fputs(ctx.strbuff, ctx.wfile);
        if (!re)
            ExitApplication(&ctx, "\"%s\" 转换完成\r\n", rpath);
    }
}
//SREC2BIN转换
static void SRec2Bin(const char* rpath, const char* wpath)
{
    SRecResult result;
    SRecRecord record;
    ConvertContext ctx;
    InitContext(&ctx);

    // 检查路径有效
    CheckPatchValid(rpath, wpath);
    // 打开HEX文件
    if (NULL == (ctx.rfile = fopen(rpath, "r")))
        ExitApplication(&ctx, STRING_OPEN_FILEERROR, rpath);

    // 分配相应的缓冲内存空间
    if (NULL == (ctx.strbuff = (char*)malloc(SREC_RECORD_STR_LEN)) ||
        NULL == (ctx.binbuff = (uint8_t*)malloc(SREC_MAX_BINDATA_NUM)))
        ExitApplication(&ctx, STRING_MALLO_MEMERROR);

    // 读取HEX信息以分配内存空间
    SRec_InitRecord(&record, ctx.binbuff);
    for (int clen = 0;;)
    {
        if (NULL == fgets(ctx.strbuff, IHEX_RECORD_STR_LEN, ctx.rfile))
            ExitApplication(&ctx, STRING_READ_ERROREND);

        ++clen;
        if (CheckSpaceLine(ctx.strbuff))       // 空行
            continue;

        result = SRec_GetNextHeadInfo(&record, ctx.strbuff);
        if (IHEX_RESULT_NOERROR != result)
            ExitApplication(&ctx, STRING_FILE_ERROR, result, clen);
        else if (record.type == SREC_TYPE_ENDBOOT)
        {
            printf("文件:\"%s\" 读取成功\r\n", rpath);
            printf("文件记录数据范围为:0X%08X - 0X%08X\r\n", record.minAddr, record.maxAddr);
            printf("共: %d 字节数据 %d 行\r\n", record.maxAddr-record.minAddr+1, clen);
            break;
        }
    }
    const uint32_t bytenum = record.maxAddr - record.minAddr + 1;
    //分配内存并填充0xff
    if (NULL == (ctx.rambuff = (uint8_t*)malloc(bytenum)))
        ExitApplication(&ctx, "分配内存错误!\r\n");
    memset(ctx.rambuff, 0xff, bytenum);
    // 打开写入文件
    if (NULL == (ctx.wfile = fopen(wpath, "wb")))
        ExitApplication(&ctx, STRING_OPEN_FILEERROR, wpath);

    // 开始解析hex文件并保存数据
    rewind(ctx.rfile);              // 重定位读取文件
    SRec_InitRecord(&record, ctx.binbuff);
    for (int clen = 0;;)
    {
        if (NULL == fgets(ctx.strbuff, IHEX_RECORD_STR_LEN, ctx.rfile))
            ExitApplication(&ctx, STRING_READ_ERROREND);

        ++clen;
        if (CheckSpaceLine(ctx.strbuff))       // 空行
            continue;

        if (IHEX_RESULT_NOERROR != (result = SRec_GetNextBindata(&record, ctx.strbuff)))
            ExitApplication(&ctx, STRING_FILE_ERROR, result, clen);
        else if (record.type == SREC_TYPE_ENDBOOT)
        {
            fwrite(ctx.rambuff, 1, bytenum, ctx.wfile);
            ExitApplication(&ctx, "转换数据完成\r\n");
        }
        else if (record.type == SREC_TYPE_DATA)
            memcpy(ctx.rambuff + (record.address - record.minAddr), ctx.binbuff, record.count);
    }
}
//SREC2BIN转换
static void Bin2SRec(const char* rpath, const char* wpath, uint32_t offset)
{
    uint32_t fsize;
    uint32_t re;

    SRecWriteDescribe winfo;
    ConvertContext ctx;
    InitContext(&ctx);

    // 检查路径有效
    CheckPatchValid(rpath, wpath);

    if (NULL == (ctx.rfile = fopen(rpath, "rb")))
        ExitApplication(&ctx, STRING_OPEN_FILEERROR, rpath);


    // 获取文件大小
    fseek(ctx.rfile, 0, SEEK_END);      // 定位到文件结尾
    fsize = (uint32_t)ftell(ctx.rfile); // 计算文件大小

    // 分配内存并读取数据
    if (NULL == (ctx.rambuff = (uint8_t*)malloc(fsize)) ||
        NULL == (ctx.strbuff = (char*)malloc(SREC_RECORD_STR_LEN)))
        ExitApplication(&ctx, STRING_MALLO_MEMERROR);
    rewind(ctx.rfile);      // 重定位文件
    fread(ctx.rambuff, 1, fsize, ctx.rfile);
    fclose(ctx.rfile);
    ctx.rfile = NULL;

    // 转换并写入文件
    winfo.pbuff = ctx.rambuff;
    winfo.linewidth = 32;
    winfo.offsetAddr = offset;
    winfo.size = fsize;
    SRec_InitWriteDesc(&winfo);
    if (NULL == (ctx.wfile = fopen(wpath, "w")))
        ExitApplication(&ctx, STRING_OPEN_FILEERROR, wpath);
    for (;;)
    {
        re = SRec_GetNextlinestr(&winfo, ctx.strbuff);
        fputs(ctx.strbuff, ctx.wfile);
        if (!re)
            ExitApplication(&ctx, "\"%s\" 转换完成\r\n", rpath);
    }
}

// **************************************************************************
// main主函数

int main(int argc, char *argv[])
{
    uint32_t temp = 0;

    if (argc <= 1)
    {
        printf("%s",
            "\r\nBINCN 操作 输入文件路径 输出文件路径 [偏移地址] \r\n"
            "支持的操作:\r\n"
            "     -hb  -- HEX转换为BIN文件 BINCN -hb in.hex out.bin\r\n"
            "     -bh  -- BIN转换为HEX文件 BINCN -bh in.bin out.hex 0x8004000\r\n"
            "     -sb  -- S转换为BIN文件   BINCN -sb in.s19 out.bin\r\n"
            "     -bs  -- BIN转换为S文件   BINCN -sb in.bin out.s19 0x8004000\r\n"
        );
    }
    else if (!strcmp("-hb", argv[1]))
        Hex2Bin(argv[2], argv[3]);
    else if (!strcmp("-bh", argv[1]))
    {
        if (argc >= 5 && EOF == sscanf(argv[4], "%x", &temp))
        {
            printf("请输入正确的16进制数值参数!\r\n");
            return 0;
        }
        Bin2Hex(argv[2], argv[3], temp);
    }
    else if (!strcmp("-bs", argv[1]))
    {
        if (argc >= 5 && EOF == sscanf(argv[4], "%x", &temp))
        {
            printf("请输入正确的16进制数值参数!\r\n");
            return 0;
        }
        Bin2SRec(argv[2], argv[3], temp);
    }
    else if (!strcmp("-sb", argv[1]))
        SRec2Bin(argv[2], argv[3]);
    else
        printf("错误的命令选项!\r\n");

    
    getchar();
    return 0;
}

