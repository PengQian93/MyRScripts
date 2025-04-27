
#include "setup.h"
#include "system_reg.h"
#include "serial.h"
#include "lib_debug.h"
#include "smp.h"
#include "fc_reg.h"
#include "codebank.h"
#include "spic.h"
#include "memory.h"
#include "scheduler0.h"
#include "fe_misc.h"
#include "platform_pm.h"
#include "ddr.h"
#include "timer.h"

#include "lib_cpu.h"
#include "lib_fc_public.h"
#include "lib_retcode.h"
#include "lib_debug.h"
#include "lib_fio_public.h"
#include "lib_sblk_config.h"
#include "lib_hlc_com.h"
#include "llf_global.h"
#include "llf_public.h"
#include "llf_public_com.h"
#include "llf_lib_public.h"
#include "llfmp.h"
#include "platform_global.h"

U8 llfCalLog2(U32 input)
{
    U8 cnt;
    cnt = 0;
    input = input >> 1;

    while (input)
    {
        input = input >> 1;
        cnt++;
    }

    return cnt;
}

void llfCopyLdpcTable(U32 d_begin_addr, U32 s_begin_addr, U32 s_end_addr)
{
    U32 i;
    for(i = s_begin_addr; i < s_end_addr; i += 4)
    {
        _REG32(d_begin_addr + i - s_begin_addr) = _REG32(i);
    }
}

void llfEndResponceNonPrint(U32 errCode)
{
    PLLF_UNI_INFO pLLFUniInfo;
    PVENDOR_CMD_RESPONSE pLLFRespInfo;
    entry_t entry;
    U32 ulLockFlag;

    pLLFRespInfo = (PVENDOR_CMD_RESPONSE)LLF_RES_BUF_VA_ADDR;
    pLLFUniInfo = (PLLF_UNI_INFO)LLF_UNI_INFO_ADDR;

    pLLFRespInfo->res_progress = 100;
    pLLFRespInfo->res_state = VENDOR_CMD_IDLE;
    pLLFRespInfo->res_err_code = errCode;
    pLLFUniInfo->ulWorkFunc = NONE_FUNC;

    entry = pLLFRespInfo->entry;
    gpHostAdminCmd[entry].message_type = MSG_BE_RESP;
    spin_lock_irqsave(&g_be2fe_admin_lock, &ulLockFlag);
    SllAddToTail(&BE2FEAdminCmdListCtrl, entry);
    spin_unlock_irqrestore(&g_be2fe_admin_lock, &ulLockFlag);
}

void PrintDone()
{
    PVENDOR_CMD_RESPONSE pVendorCmdResponse = (PVENDOR_CMD_RESPONSE)LLF_RES_BUF_VA_ADDR;
    llfDbgPrintk(ALWAYS_MSG, "OK, RET = 0x%x\r\n", pVendorCmdResponse->res_err_code);
}

void llfEndResponce(U32 errCode)
{
    llfEndResponceNonPrint(errCode);
    PrintDone();
}

void llfInitErrorMessage()
{
    PVENDOR_CMD_RESPONSE pLLFRespInfo;
    pLLFRespInfo = (PVENDOR_CMD_RESPONSE)LLF_RES_BUF_VA_ADDR;
    pLLFRespInfo->err_msg_num = 0;
}

void llfAddErrorMessage(U8 bank, U16 badBlockNum, U16 errorType)
{
    PVENDOR_CMD_RESPONSE pLLFRespInfo;
    U8 ubCh, ubCe, errNum;
    pLLFRespInfo = (PVENDOR_CMD_RESPONSE)LLF_RES_BUF_VA_ADDR;
    errNum = pLLFRespInfo->err_msg_num;
    ubCh = (_MEM08(BANK_IMAPPING_TABLE_ADDR + bank) & 0xFF) >> 4;
    ubCe = (_MEM08(BANK_IMAPPING_TABLE_ADDR + bank) & 0xF);

    _MEM32(LLF_RES_ERRMSG_START_VA_ADDR + WORD_BYTE_SIZE * errNum) = (ubCh << 16 | ubCe);
    _MEM32(LLF_RES_ERRMSG_START_VA_ADDR + WORD_BYTE_SIZE * (errNum + 1)) =
        (errorType << 16 | badBlockNum);
    pLLFRespInfo->err_msg_num += 2;
}

U8 llfIsBlockBadInRDTDBT(U32 ulTableStartAddr, U16 block)
{
    U32 ulMarkAddr;

    // Shift to the channel location
    ulMarkAddr = ulTableStartAddr + (block >> _5BIT_SHIFT) * WORD_BYTE_SIZE;

    // Check the defect bit of this block
    if (_REG32(ulMarkAddr) & (1 << (block & _5BIT_MASK)))
    {
        return 1;
    }

    return 0;
}

U32 llfSetDBT(U8 bank, U16 blk_no, U32 dbt_addr)
{
    U8 ubBankNo;
    U16 gwGroupNo;

    ubBankNo = bank;
    gwGroupNo = (blk_no / NandPara.ubPlaneNumPerLun);

    //*(UB *)(dbt_addr + ubBankNo*group_num + gwGroupNo) = 0x7F;
    if(!llfIsBlockBad(dbt_addr, bank, blk_no))//if already mark, don't need to mark twice well
    {
        llfMarkUserMPBlkDefect(dbt_addr, ubBankNo, gwGroupNo);
        llfDbgPrintk(ALWAYS_MSG, "LLF DBT DEFECT( bank:%d, block:%d, mpblock:%d)\r\n", ubBankNo, blk_no,
                     gwGroupNo);
        gubBadBlockNum[bank]++;
        cache_area_dwbinval(ALIGN_32_ROUND(dbt_addr + ubBankNo * DEFECT_USER_MP_BLK_TABLE_SIZE_PER_BANK +
                                           (gwGroupNo >> _5BIT_SHIFT) * WORD_BYTE_SIZE), 32);
        //ALIGN_32_ROUND
    }
    return ERR_OK;
}


void llfUnSetDBT(U8 bank, U16 Group_no, U32 dbt_addr)
{
    U32 i;
    if((Group_no * NandPara.ubPlaneNumPerLun) < SYSTEM_BLOCK_MAX_NUM)
    {
        for(i = 0; i < NandPara.ubPlaneNumPerLun; i++)
        {
            llfUnMarkSystemBlkDefect(SYS_BLK_DBT_ADDR, bank, (Group_no * NandPara.ubPlaneNumPerLun) + i);
        }
        cache_area_dwbinval(ALIGN_32_ROUND(SYS_BLK_DBT_ADDR + ((Group_no * NandPara.ubPlaneNumPerLun)
                                           * SYS_BLK_DBT_BYTE_SIZE_PER_BLOCK) +
                                           (bank >> _5BIT_SHIFT) * WORD_BYTE_SIZE), 32);
    }
    llfUnMarkUserMPBlkDefect(dbt_addr, bank, Group_no);
    llfDbgPrintk(ALWAYS_MSG, "LLF DBT un-DEFECT( bank:%d, block:%d, mpblock:%d)\r\n", bank,
                 Group_no * NandPara.ubPlaneNumPerLun,
                 Group_no);
    cache_area_dwbinval(ALIGN_32_ROUND(dbt_addr + bank * DEFECT_USER_MP_BLK_TABLE_SIZE_PER_BANK +
                                       (Group_no >> _5BIT_SHIFT) * WORD_BYTE_SIZE), 32);
}

#ifdef SPECIAL_SYSTEM_BLOCK
U32 llfSetSpDBT(U8 ubBank, U16 uwSpBlk, U32 ulSpDbtAddr)
{
    U32 ulMarkAddr;



    if (uwSpBlk >= (DEFECT_STATIC_SP_BLK_TABLE_SIZE_PER_BANK * 8))
    {
        printk("[WARN] DBT size not enough Blk:%d Bk:%d\r\n", uwSpBlk, ubBank);
        return ERR_FULL;
    }

    ulMarkAddr = ulSpDbtAddr + ubBank * DEFECT_STATIC_SP_BLK_TABLE_SIZE_PER_BANK;

    if(!llfIsSpBlockBadInStaticDBT(ulMarkAddr, uwSpBlk))
    {
        llfDbgPrintk(ALWAYS_MSG, "LLF SP DBT DEFECT( bank:%d, block:%d)\r\n", ubBank, uwSpBlk);
        ulMarkAddr += ((uwSpBlk >> _5BIT_SHIFT) * WORD_BYTE_SIZE);
        _MEM32(ulMarkAddr) |= (1 << (uwSpBlk & _5BIT_MASK));
        cache_area_dwbinval(ALIGN_32_ROUND(ulMarkAddr), 32);
        cache_dummy_update_read();
    }

    return ERR_OK;
}
#endif

U8 llfIsBlockBad(U32 ulTableStartAddr, U8 bank, U16 block)
{
    U16 group_no;
    U32 ulMarkAddr;

    //return 1 is bad
    group_no = block / NandPara.ubPlaneNumPerLun;
    ulMarkAddr = ulTableStartAddr + (bank * DEFECT_USER_MP_BLK_TABLE_SIZE_PER_BANK)
                 + (group_no >> _5BIT_SHIFT) * WORD_BYTE_SIZE;

    // Check the defect bit of this bs
    if(_REG32(ulMarkAddr) & (1 << (group_no & _5BIT_MASK)))
    {
        return 1;
    }
    return 0;
}
#ifdef EXTEND_STATIC_DBT
void llfCheckRDTPlaneAndDBTSize(U32 resultAddr)
{
    U8 ubRdtPlaneCnt;
    U8 ubStaticDBTperPage;
    U32 ulRdtMpBlockDBTSize;
    ASSERT_LLF(_REG32(resultAddr + RDT_MPBLOCK_PERLUN_OFFSET) != 0);
    ubRdtPlaneCnt = (_REG32(resultAddr + RDT_SPBLOCK_PERLUN_OFFSET) / _REG32(
                         resultAddr + RDT_MPBLOCK_PERLUN_OFFSET));
    ubStaticDBTperPage = ((NandPara.ubSectorNumPerPage << SECTOR_BYTE_SHIFT) /
                          DEFECT_STATIC_SP_BLK_TABLE_SIZE_PER_BANK);
#ifdef IS_8K_PAGE
    ASSERT_LLF(ubStaticDBTperPage == 2);
#else
    ASSERT_LLF(ubStaticDBTperPage == 4);
#endif
    ulRdtMpBlockDBTSize = (_REG32(resultAddr + RDT_MPBLOCK_PERLUN_OFFSET) >> _3BIT_SHIFT);
    llfprintk("StaticDBT Plane %d, RDT Plane %d, DBTSize %d Byte\r\n", NandPara.ubPlaneNumPerLun,
              ubRdtPlaneCnt, ulRdtMpBlockDBTSize);

#if defined(FTL_SSV2) && defined(MULTI_LUN)
//not support remap yet 2022/10/03
#else
    if((NandPara.ubPlaneNumPerLun != ubRdtPlaneCnt)
            || (ulRdtMpBlockDBTSize > RDT_USER_MP_BLK_TABLE_SIZE_PER_BANK))
        gubNeedRebuildRemap = 1;
#endif
}

U8 llfIsSpBlockBadInStaticDBT(U32 ulTableStartAddr, U16 block)
{
    U32 ulMarkAddr;
    ulMarkAddr = ulTableStartAddr + (block >> _5BIT_SHIFT) * WORD_BYTE_SIZE;

    // Check the defect bit of this block
    if(_REG32(ulMarkAddr) & (1 << (block & _5BIT_MASK)))
    {
        return 1;
    }
    return 0;
}

void llfCheckBankRdtResultTag()
{
    U8 bank;

    for (bank = 0; bank < NandPara.ubBankNum; bank++)
    {
        if(bank != NandPara.ubBankNum - 1)
        {
            // check Tick
            if((gulBankRdtResultTag[bank] & 0xFFFF) != (gulBankRdtResultTag[bank + 1] & 0xFFFF))
            {
                llfprintk("StaticDBT bank %d vs %d Tick is diff\r\n", bank, bank + 1);
                gubNeedRebuildRemap = 1;
                break;
            }
        }

        // check RDT bank index
        if(((gulBankRdtResultTag[bank] >> RDT_RESULT_CURBANK_SHIFT) & 0xF) != bank)
        {
            llfprintk("StaticDBT bank %d, but RDT bank is %d\r\n", bank,
                      ((gulBankRdtResultTag[bank] >> 20) & 0xF));
            gubNeedRebuildRemap = 1;
            break;
        }
        // check total bank cnt
        if((((gulBankRdtResultTag[bank] >> RDT_RESULT_TOTALBANK_SHIFT) & 0xF) + 1) != NandPara.ubBankNum)
        {
            llfprintk("StaticDBT BankNum %d, but RDT BankNum is %d\r\n", NandPara.ubBankNum,
                      ((gulBankRdtResultTag[bank] >> RDT_RESULT_TOTALBANK_SHIFT) & 0xF) + 1);
            gubNeedRebuildRemap = 1;
            break;
        }
    }
}
#endif

U8 llfIsMpBlockBad(U32 ulTableStartAddr, U8 bank, U16 mpblock)
{
    U32 ulMarkAddr;
    ulMarkAddr = ulTableStartAddr + (bank * DEFECT_USER_MP_BLK_TABLE_SIZE_PER_BANK)
                 + (mpblock >> _5BIT_SHIFT) * WORD_BYTE_SIZE;

    // Check the defect bit of this bs
    if(_REG32(ulMarkAddr) & (1 << (mpblock & _5BIT_MASK)))
    {
        return 1;
    }
    return 0;
}

void llfPrintNandPara()
{
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_NANDID_OFFSET, gulFlashVendorNum);
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_CHNUM_OFFSET, NandPara.ubChNum);
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_CENUM_OFFSET, NandPara.ubCENumPerCh);
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_INTERFACE_OFFSET,
                 _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_INTERFACE_OFFSET));
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_CLOCK_OFFSET,
                 _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_CLOCK_OFFSET));
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_TOTAL_BLK_OFFSET, NandPara.uwBlockNumPerLun);
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_PAGE_NUM_OFFSET, NandPara.uwPageNumPerBlock);
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_LUN_NUM_OFFSET, NandPara.ubLunNumPerCE);
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_PLANE_NUM_OFFSET, NandPara.ubPlaneNumPerLun);
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_PAGE_SECTOR_SIZE, NandPara.ubSectorNumPerPage);
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_HEADER_SIZE_OFFSET, L2pPara.ubHdr4BLenPerPage);
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_LBN_BUF_SIZE_OFFSET, guwCacheSectorNumPerLbn);
    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", CONFIG_CAPACITY_OFFSET, gulMaxLBAAddr);

    llfDbgPrintk(ALWAYS_MSG, "LLF %x : %x\r\n", 0, gfSelfTestFlag);
}

void llfProgramSeqData(U32 ulAddr, U16 uwLength)
{
    U32 i;
    for (i = 0; i < uwLength; i += 4)
    {
        _REG32(ulAddr + i) = i;
    }
}

void llfProgramAllMixData(U32 ulAddr, U16 uwLength)
{
    int i = 0;
    int j = 0;
    int retry = uwLength / (64 * 4 * 2);
    printk("llfProgramAllMixData: %d\r\n", retry);
    for(j = 0; j < retry; j++) //mix pattern
    {
        //length: 64x4
        _REG32(ulAddr + i) = 0xffff5555;
        i += 4;
        _REG32(ulAddr + i) = 0xaaff5555;
        i += 4;
        _REG32(ulAddr + i) = 0xaaaa0000;
        i += 4;
        _REG32(ulAddr + i) = 0xaaaa5500;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa0000;
        i += 4;
        _REG32(ulAddr + i) = 0xaaaa0055;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa5500;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa0000;
        i += 4;
        _REG32(ulAddr + i) = 0xaaff5555;
        i += 4;
        _REG32(ulAddr + i) = 0xaaaa5500;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa5500;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa0055;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa5555;
        i += 4;
        _REG32(ulAddr + i) = 0xaaaa5500;
        i += 4;
        _REG32(ulAddr + i) = 0xffff5555;
        i += 4;
        _REG32(ulAddr + i) = 0xaaff0055;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa0000;
        i += 4;
        _REG32(ulAddr + i) = 0xaaaa5555;
        i += 4;
        _REG32(ulAddr + i) = 0xaaaa5500;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa0055;
        i += 4;
        _REG32(ulAddr + i) = 0xffff0055;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa5500;
        i += 4;
        _REG32(ulAddr + i) = 0xffff0055;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa5555;
        i += 4;
        _REG32(ulAddr + i) = 0xaaaa0055;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa5555;
        i += 4;
        _REG32(ulAddr + i) = 0xaaff0000;
        i += 4;
        _REG32(ulAddr + i) = 0xaaff5555;
        i += 4;
        _REG32(ulAddr + i) = 0xaaff5555;
        i += 4;
        _REG32(ulAddr + i) = 0xaaff5500;
        i += 4;
        _REG32(ulAddr + i) = 0xffaa5500;
        i += 4;
        _REG32(ulAddr + i) = 0xaaaa5500;
        i += 4;
        //////////////////////////////////////
        _REG32(ulAddr + i) = 0xffffaaaa;
        i += 4;
        _REG32(ulAddr + i) = 0x55ffaaaa;
        i += 4;
        _REG32(ulAddr + i) = 0x55550000;
        i += 4;
        _REG32(ulAddr + i) = 0x5555aa00;
        i += 4;
        _REG32(ulAddr + i) = 0xff550000;
        i += 4;
        _REG32(ulAddr + i) = 0x555500aa;
        i += 4;
        _REG32(ulAddr + i) = 0xff55aa00;
        i += 4;
        _REG32(ulAddr + i) = 0xff550000;
        i += 4;
        _REG32(ulAddr + i) = 0x55ffaaaa;
        i += 4;
        _REG32(ulAddr + i) = 0x5555aa00;
        i += 4;
        _REG32(ulAddr + i) = 0xff55aa00;
        i += 4;
        _REG32(ulAddr + i) = 0xff5500aa;
        i += 4;
        _REG32(ulAddr + i) = 0xff55aaaa;
        i += 4;
        _REG32(ulAddr + i) = 0x5555aa00;
        i += 4;
        _REG32(ulAddr + i) = 0xffffaaaa;
        i += 4;
        _REG32(ulAddr + i) = 0x55ff00aa;
        i += 4;
        _REG32(ulAddr + i) = 0xff550000;
        i += 4;
        _REG32(ulAddr + i) = 0x5555aaaa;
        i += 4;
        _REG32(ulAddr + i) = 0x5555aa00;
        i += 4;
        _REG32(ulAddr + i) = 0xff5500aa;
        i += 4;
        _REG32(ulAddr + i) = 0xffff00aa;
        i += 4;
        _REG32(ulAddr + i) = 0xff55aa00;
        i += 4;
        _REG32(ulAddr + i) = 0xffff00aa;
        i += 4;
        _REG32(ulAddr + i) = 0xff55aaaa;
        i += 4;
        _REG32(ulAddr + i) = 0x555500aa;
        i += 4;
        _REG32(ulAddr + i) = 0xff55aaaa;
        i += 4;
        _REG32(ulAddr + i) = 0x55ff0000;
        i += 4;
        _REG32(ulAddr + i) = 0x55ffaaaa;
        i += 4;
        _REG32(ulAddr + i) = 0x55ffaaaa;
        i += 4;
        _REG32(ulAddr + i) = 0x55ffaa00;
        i += 4;
        _REG32(ulAddr + i) = 0xff55aa00;
        i += 4;
        _REG32(ulAddr + i) = 0x5555aa00;
        i += 4;

        //length: 16x4
        _REG32(ulAddr + i) = 0x01fe01fe;
        i += 4;
        _REG32(ulAddr + i) = 0x01fe01fe;
        i += 4;
        _REG32(ulAddr + i) = 0x02fd02fd;
        i += 4;
        _REG32(ulAddr + i) = 0x02fd02fd;
        i += 4;
        _REG32(ulAddr + i) = 0x04fb04fb;
        i += 4;
        _REG32(ulAddr + i) = 0x04fb04fb;
        i += 4;
        _REG32(ulAddr + i) = 0x08f708f7;
        i += 4;
        _REG32(ulAddr + i) = 0x08f708f7;
        i += 4;
        _REG32(ulAddr + i) = 0x10ef10ef;
        i += 4;
        _REG32(ulAddr + i) = 0x10ef10ef;
        i += 4;
        _REG32(ulAddr + i) = 0x20df20df;
        i += 4;
        _REG32(ulAddr + i) = 0x20df20df;
        i += 4;
        _REG32(ulAddr + i) = 0x40bf40bf;
        i += 4;
        _REG32(ulAddr + i) = 0x40bf40bf;
        i += 4;
        _REG32(ulAddr + i) = 0x807f807f;
        i += 4;
        _REG32(ulAddr + i) = 0x807f807f;
        i += 4;

        //length: 16x4
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;
        _REG32(ulAddr + i) = 0x0C080400;
        i += 4;

        // length: 32x4
        _REG32(ulAddr + i) = 0x4fa753a9;
        i += 4;
        _REG32(ulAddr + i) = 0xfe7f3f9f;
        i += 4;
        _REG32(ulAddr + i) = 0xe0f0f8fc;
        i += 4;
        _REG32(ulAddr + i) = 0x040281c0;
        i += 4;
        _REG32(ulAddr + i) = 0x41201008;
        i += 4;
        _REG32(ulAddr + i) = 0x180c0683;
        i += 4;
        _REG32(ulAddr + i) = 0x85c26130;
        i += 4;
        _REG32(ulAddr + i) = 0x5128140a;
        i += 4;
        _REG32(ulAddr + i) = 0x1e8f47a3;
        i += 4;
        _REG32(ulAddr + i) = 0xe4f2793c;
        i += 4;
        _REG32(ulAddr + i) = 0x452291c8;
        i += 4;
        _REG32(ulAddr + i) = 0x592c168b;
        i += 4;
        _REG32(ulAddr + i) = 0x9dce67b3;
        i += 4;
        _REG32(ulAddr + i) = 0xd4ea753a;
        i += 4;
        _REG32(ulAddr + i) = 0x4fa753a9;
        i += 4;
        _REG32(ulAddr + i) = 0xfa7d3e9f;
        i += 4;
        _REG32(ulAddr + i) = 0xa1d0e8f4;
        i += 4;
        _REG32(ulAddr + i) = 0x1c0e8743;
        i += 4;
        _REG32(ulAddr + i) = 0xc4e27138;
        i += 4;
        _REG32(ulAddr + i) = 0x49241289;
        i += 4;
        _REG32(ulAddr + i) = 0x9b4d2693;
        i += 4;
        _REG32(ulAddr + i) = 0xb5da6d36;
        i += 4;
        _REG32(ulAddr + i) = 0x5badd66b;
        i += 4;
        _REG32(ulAddr + i) = 0xbdde6fb7;
        i += 4;
        _REG32(ulAddr + i) = 0xd8ecf67b;
        i += 4;
        _REG32(ulAddr + i) = 0x8dc663b1;
        i += 4;
        _REG32(ulAddr + i) = 0xd269341a;
        i += 4;
        _REG32(ulAddr + i) = 0x2e974ba5;
        i += 4;
        _REG32(ulAddr + i) = 0xee77bb5d;
        i += 4;
        _REG32(ulAddr + i) = 0xe472b9dc;
        i += 4;
        _REG32(ulAddr + i) = 0x452291c8;
        i += 4;
        _REG32(ulAddr + i) = 0x542a158a;
        i += 4;
    }
}

void llfProgramRandomData(U32 ulAddr, U16 uwLength)
{
    U32 i;

    for (i = 0; i < uwLength; i += 4)
    {
#if defined(RTS5771_FPGA) || defined(RTS5771_VA) || defined(RL6577_FPGA) || defined(RL6577_VA)
        _REG32(ulAddr + i) = (GET_TICK + (i << 16));
#else
        _REG32(ulAddr + i) = (gulTicks + (i << 16));
#endif
    }
}

U16 llfBETagSetting(U8 ubTagType, U8 ubBankNo)
{
    return((ubTagType << 8) | ubBankNo);
}

U32 llfCompareHead(U32 ulSrcAddr, U32 ulDstAddr, U32 ulLength)
{
    U32 i;
    U32 ulSrcData, ulDstData;
    U32 ret = ERR_OK;
    U32 ulCount = 0;
    U8 strBuf[12];

    for (i = 0; i < ulLength; i += 4)
    {
        if( (i & 0xf) == 0xc )
            continue;
        ulSrcData = _REG32(ulSrcAddr + i);
        ulDstData = _REG32(ulDstAddr + i);
        if(ulSrcData != ulDstData)
        {
            strBuf[0] = 'C';
            strBuf[1] = '%';
            strBuf[2] = 'x';
            strBuf[3] = ' ';
            strBuf[4] = '%';
            strBuf[5] = 'x';
            strBuf[6] = ' ';
            strBuf[7] = '%';
            strBuf[8] = 'x';
            strBuf[9] = '\r';
            strBuf[10] = '\n';
            strBuf[11] = 0;
            llfDbgPrintk(ALWAYS_MSG, (const char*)strBuf, i,  ulSrcAddr, ulDstAddr);

            ret = ERR_COMPARE_DATA;
            //ASSERT_LLF(0);
            ulCount++;
            break;
        }
    }
    /*
    strBuf[0]='C';
    strBuf[1]='%';
    strBuf[2]='x';
    strBuf[3]='\r';
    strBuf[4]='\n';
    strBuf[5]=0;
    llfprintk(strBuf, ulCount);
    */
    return ret;
}

U32 llfnandtestCompareData(U32 ulSrcAddr, U32 ulDstAddr, U32 ulLength)
{
    U32 i;
    U32 ulSrcData, ulDstData;
    U32 ret = ERR_OK;
    U32 ulCount = 0;
    U16 ecc_parity_len[6] = {294, 266, 246, 226, 186, 150};
    U32 ecc_len = ecc_parity_len[gubLdpcCodeRate];

    for (i = 0; i < ulLength; i += 4)
    {
        if((i % 2048 == 0) && i > 0)
        {
            ulDstAddr = ulDstAddr + (10 + ecc_len);
        }
        ulSrcData = _REG32(ulSrcAddr + i);
        ulDstData = _REG32(ulDstAddr + i);
        if(ulSrcData != ulDstData)
        {
            llfprintk("compare %d %x  %x\r\n", i,  ulSrcAddr, ulDstAddr);

            ret = ERR_COMPARE_DATA;
            //ASSERT_LLF(0);
            ulCount++;
            break;
        }
    }
    return ret;
}

U32 llfCompareData(U32 ulSrcAddr, U32 ulDstAddr, U32 ulLength)
{
    U8 ulTmpData;
    U32 i, j;
    U32 ulSrcData, ulDstData;
    U32 ret = ERR_OK;
    U32 ulCount = 0;

    for (i = 0; i < ulLength; i += 4)
    {
        ulSrcData = _REG32(ulSrcAddr + i);
        ulDstData = _REG32(ulDstAddr + i);
        if(ulSrcData != ulDstData)
        {
            llfprintk("compare %d %x  %x\r\n", i,  ulSrcAddr, ulDstAddr);

            ret = ERR_COMPARE_DATA;
            //ASSERT_LLF(0);
            ulCount++;
            break;
        }
        if (ulSrcAddr == SBLK_ADDR)
        {
            if ((i == 0) && (ulDstData != SBLOCK_GENERAL_CONFIG_TAG_LOW))
            {
                printk("[SBLK] CONFIG_TAG_LOW missmatch %x\r\n", ulDstData);
                ret = ERR_COMPARE_DATA;
                break;
            }
            else if ((i == 4) && (ulDstData != SBLOCK_GENERAL_CONFIG_TAG_HIGH))
            {
                printk("[SBLK] CONFIG_TAG_HIGH missmatch %x\r\n", ulDstData);
                ret = ERR_COMPARE_DATA;
                break;
            }
            //check model name, serial number, fw version
            else if ((i > 0x0e) && (i < 0x60))
            {
                for (j = 0; j < 4; j++)
                {
                    ulTmpData = _REG08(ulSrcAddr + i + j);
                    if ((ulTmpData < 0x20) || (ulTmpData > 0x7A)) //ASCII
                    {
                        printk("[SBLK] invalid char %x\r\n", ulTmpData);
                        ret = ERR_COMPARE_DATA;
                        break;
                    }
                }
            }
        }
    }
    /*
    strBuf[0]='C';
    strBuf[1]='%';
    strBuf[2]='x';
    strBuf[3]='\r';
    strBuf[4]='\n';
    strBuf[5]=0;
    llfprintk(strBuf, ulCount);
    */
    return ret;
}

U32 llfCompareDataPer1KB(U32 ulSrcAddr, U32 ulDstAddr, U32 ulLength)
{
    U32 i;
    U32 ulSrcData, ulDstData;
    U32 ret = ERR_OK;
    U32 ulCount = 0;

    for (i = 0; i < ulLength; i += 256)
    {
        ulSrcData = _REG32(ulSrcAddr + i);
        ulDstData = _REG32(ulDstAddr + i);
        if(ulSrcData != ulDstData)
        {
            //llfDbgPrintk(ALWAYS_MSG, "[DataERR] %x %x %x\r\n", i,  ulSrcData, ulDstData);
            ret = ERR_COMPARE_DATA;
            ulCount++;
            break;
        }
    }
    return ret;
}

U32 llfIsDefectBlk(U32 addr, U8 if_type)
{
    U32 defect_flag;

    //check out the first byte of user data or defect area in the first page of the checked block
    if (FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON || FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL)
    {
        // Micron detect four bytes, where Toshiba only detect one byte
        if(if_type == 0)//ONFI SDR or Toggle legacy mode
        {
            defect_flag = _REG32(addr) & 0xFFFFFFFF;
            if(defect_flag != 0xFFFFFFFF)
                return ERR_DEFECT_BLOCK;
        }
        else
        {
            defect_flag = _REG32(addr) & 0xFFFFFF;
            if(defect_flag != 0xFFFFFF)
                return ERR_DEFECT_BLOCK;
        }
    }
    else //Tsb or Hynix
    {
        if(if_type == 0)//ONFI SDR or Toggle legacy mode
        {
            defect_flag = _REG32(addr) & 0xFF;
            if(defect_flag != 0xFF)
                return ERR_DEFECT_BLOCK;
        }
        else
        {
            defect_flag = _REG32(addr) & 0xFFFF;
            if(defect_flag != 0xFFFF)
                return ERR_DEFECT_BLOCK;
        }
    }

    return 1;
}

U32 llffioIsBankReady(U8 ubBankNo)
{
    U8 be_no = (ubBankNo & BE_NUM_MASK);

    ASSERT_LLF(ubBankNo < NandPara.ubBankNum);

    if(((gulBankRB[be_no] >> (ubBankNo >> NandPara.ubChNumShift)) & 0x1) == 1)
    {
        return ERR_OK; // ready
    }
    else
    {
        return ERR_BUSY;// busy
    }
}

void llffioMarkBankBusy(U8 ubBankNo)
{
    U8 be_no = (ubBankNo & BE_NUM_MASK);

    ASSERT_LLF(ubBankNo < NandPara.ubBankNum);

    gulBankRB[be_no] &= ~((U32)(1 << (ubBankNo >> NandPara.ubChNumShift)));
}

void llfFCCmdRead_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page, U32 Data_addr,
                       U32 Data_length, U32 Head_addr, U32 Head_length)
{
    if(gubNandFlashType)
        FCSingleReadDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                         Head_length, 1);
    else
        FCSingleReadDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                         Head_length, 0);
}

#if defined(FTL_H3DTV3) || defined(FTL_H3DTV4) || defined(FTL_H3DTV5) || defined(FTL_H3DTV6) || defined(FTL_H3DQV5) || defined(FTL_H3DTV7)
void llfFCCmdHynixRead(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page, U32 Data_addr,
                       U32 Data_length, U32 Head_addr, U32 Head_length)
{
    if(gubNandFlashType)
        FCSingleHynixRead(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                          Head_length, 1);
    else
        FCSingleHynixRead(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                          Head_length, 0);
}
#endif

void llfFCCmdMultiRead_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page, U32 Data_addr,
                            U32 Data_length, U32 Head_addr, U32 Head_length)
{
    if(gubNandFlashType)
        FCMultiReadDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                        Head_length, 1);
    else
        FCMultiReadDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                        Head_length, 0);
}

void llfFCCmdQuadRead_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page, U32 Data_addr,
                           U32 Data_length, U32 Head_addr, U32 Head_length)
{
    if(gubNandFlashType)
        FCQuadReadDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                       Head_length, 1);
    else
        FCQuadReadDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                       Head_length, 0);
}

void llfFCCmdWrite_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page, U32 Data_addr,
                        U32 Data_length, U32 Head_addr, U32 Head_length)
{
    if(gubNandFlashType)
        FCSingleWriteDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                          Head_length, 1);
    else
        FCSingleWriteDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                          Head_length, 0);
}

void llfFCCmdMultiWrite_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page,
                             U32 Data_addr, U32 Data_length, U32 Head_addr, U32 Head_length)
{
    if(gubNandFlashType)
        FCMultiWriteDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                         Head_length, 1);
    else
        FCMultiWriteDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                         Head_length, 0);
}
void llfFCCmdQuadWrite_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page,
                            U32 Data_addr, U32 Data_length, U32 Head_addr, U32 Head_length)
{
    if(gubNandFlashType)
        FCQuadWriteDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                        Head_length, 1);
    else
        FCQuadWriteDRAM(ulMode, ubBankNo, ubLunNo, block, page, Data_addr, Data_length, Head_addr,
                        Head_length, 0);
}

#if defined(RTS5771_FPGA)||defined(RTS5771_VA)
BOOL llfBSCheckSinglePlaneBadBlock(U32 blockInfo_addr, U16 BSIndex, U16 BankNo)
{
    U64 ullMap;

    ASSERT(ALWAYS_MSG, BankNo < NandPara.ubBankNum);

    ullMap = _MEM64(blockInfo_addr + BSIndex * BS_INFO_SIZE + BAD_INFO_OFFSET);
    if (ullMap & ((U64)(1) << BankNo))
    {
        return TRUE;
    }

    ullMap = _MEM64(blockInfo_addr + BSIndex * BS_INFO_SIZE + NEW_BAD_INFO_OFFSET);
    if (ullMap & ((U64)(1) << BankNo))
    {
        return TRUE;
    }

    return FALSE;
}

#else
BOOL llfBSCheckSinglePlaneBadBlock(U32 blockInfo_addr, U16 BSIndex, U16 BankNo)
{
    U32 tempBS;

#ifndef VIRTUAL_BANK
    ASSERT(ALWAYS_MSG, BankNo < NandPara.ubBankNum);
#endif
    tempBS = _MEM32(blockInfo_addr + BSIndex * BS_INFO_SIZE + BAD_INFO_OFFSET);
    if ((tempBS & 0xFFFF) & (1 << BankNo))
        return TRUE;
    tempBS = _MEM32(blockInfo_addr + BSIndex * BS_INFO_SIZE + PF_INFO_OFFSET);
    if (((tempBS >> PF_INFO_BIT_SHIFT) & 1) && ((tempBS & NEW_BAD_INFO_MASK) & (1 << BankNo)))
        return TRUE;
    else
        return FALSE;
}
#endif

void llfMarkSystemBlkDefect(U32 ulTableStartAddr, U8 bank, U16 block)
{
    U32 ulMarkAddr;

    // Shift to the channel location
    ulMarkAddr = ulTableStartAddr + (block * SYS_BLK_DBT_BYTE_SIZE_PER_BLOCK) +
                 (bank >> _5BIT_SHIFT) * WORD_BYTE_SIZE;

    // Mark the right bit of this block
    _REG32(ulMarkAddr) |= (1 << (bank & _5BIT_MASK));
}

void llfUnMarkSystemBlkDefect(U32 ulTableStartAddr, U8 bank, U16 block)
{
    U32 ulMarkAddr;

    // Shift to the channel location
    ulMarkAddr = ulTableStartAddr + (block * SYS_BLK_DBT_BYTE_SIZE_PER_BLOCK) +
                 (bank >> _5BIT_SHIFT) * WORD_BYTE_SIZE;

    // Mark the right bit of this block
    _REG32(ulMarkAddr) &= (~(1 << (bank & _5BIT_MASK)));
}

U8 llfIsSystemBlkDefect(U32 ulTableStartAddr, U8 bank, U16 block)
{
    U32 ulMarkAddr;

    // Shift to the channel location
    ulMarkAddr = ulTableStartAddr + (block * SYS_BLK_DBT_BYTE_SIZE_PER_BLOCK) +
                 (bank >> _5BIT_SHIFT) * WORD_BYTE_SIZE;

    // Check the defect bit of this block
    if(_REG32(ulMarkAddr) & (1 << (bank & _5BIT_MASK)))
        return 1;
    else
        return 0;
}

void llfMarkUserMPBlkDefect(U32 ulTableStartAddr, U8 bank, U16 group)
{
    U32 ulMarkAddr;

    // Shift to the channel location
    ulMarkAddr = ulTableStartAddr + (bank * DEFECT_USER_MP_BLK_TABLE_SIZE_PER_BANK) +
                 (group >> _5BIT_SHIFT) * WORD_BYTE_SIZE;
    // Mark the right bit of this block
    _REG32(ulMarkAddr) |= (1 << (group & _5BIT_MASK));
}

void llfUnMarkUserMPBlkDefect(U32 ulTableStartAddr, U8 bank, U16 group)
{
    U32 ulMarkAddr;

    // Shift to the channel location
    ulMarkAddr = ulTableStartAddr + (bank * DEFECT_USER_MP_BLK_TABLE_SIZE_PER_BANK) +
                 (group >> _5BIT_SHIFT) * WORD_BYTE_SIZE;
    // Mark the right bit of this block
    _REG32(ulMarkAddr) &= (~(1 << (group & _5BIT_MASK)));
}
#ifdef SLC_RRT
#if defined(FTL_H3DTV3) || defined(FTL_H3DTV4) || defined(FTL_H3DTV5) || defined(FTL_H3DTV6) || defined(FTL_H3DTV7) || defined(FTL_SSV2) || defined(FTL_SSV4) || defined(FTL_SSV5) || defined(FTL_SSV6) || defined(FTL_SSV7) || defined(FTL_H3DQV5)
U32 llfInitMemLayout()
{
    U32 offset;
    offset = 0;
    U32 llfRRTSLCBaseAddr;
    // Hynix retry SLC setting
#if defined(FTL_H3DTV3) || defined(FTL_H3DTV4) || defined(FTL_H3DTV5) || defined(FTL_H3DTV6) || defined(FTL_H3DTV7) || defined(FTL_H3DQV5)
    offset = ALIGN_32(offset);
    llfRRTSLCBaseAddr = SLCRRT_BUF_ADDR + offset;
    offset += sizeof(struct _HYNIX_RETRY_MANUAL_SLC);
#elif defined(FTL_SSV2) || defined(FTL_SSV4) || defined(FTL_SSV5) || defined(FTL_SSV6) || defined(FTL_SSV7)
    offset = ALIGN_32(offset);
    llfRRTSLCBaseAddr = SLCRRT_BUF_ADDR + offset;
    offset += sizeof(Samsung_vx_RETRY);
#endif
    gulRRTBaseAddr = llfRRTSLCBaseAddr;
    return llfRRTSLCBaseAddr;
}
#endif
#endif
#if defined (FTL_H3DQV5) && (defined(RL6577_VA)||defined(RTS5771_VA))
U32 llfJudgeFinalHynixRRT(llfHYNIX_RR_Table_QLC *hynixRRT)
{
    U8 retryCount, byteCount;
    U8 votes[4], nominee, voter, elected;
    U8 RRPara_7_0 = 0;

    for(retryCount = 0; retryCount < 8; retryCount++)
    {
        if((hynixRRT->regCount[retryCount] != 15)
                || (hynixRRT->totalCount[retryCount] != 0x32))
        {
            printk("[RRT] FAIL QLC retry reg count not consistent\r\n");
            return ERR_HYNIX_JUDGE_RRT_FAIL;
        }
    }

    retryCount = 7;
    byteCount = 0;

    for(nominee = 0; nominee < 4; nominee++)
    {
        votes[nominee] = 1;
    }
    elected = 0xFF;
    for(nominee = 0; nominee < 4; nominee++)
    {
        for(voter = nominee + 1; voter < 8; voter++)
        {
            if(hynixRRT->sets[voter].ori.ubRRPara[retryCount][byteCount] ==
                    hynixRRT->sets[nominee].ori.ubRRPara[retryCount][byteCount])
            {
                votes[nominee]++;
            }
        }
    }
    for(nominee = 0; nominee < 4; nominee++)
    {
        if(votes[nominee] > 4)
        {
            RRPara_7_0 = hynixRRT->sets[nominee].ori.ubRRPara[retryCount][byteCount];
            elected = nominee;
            break;
        }
    }

    for(nominee = 0; nominee < 4; nominee++)
    {
        votes[nominee] = 1;
    }

    for(nominee = 0; nominee < 4; nominee++)
    {
        for(voter = nominee + 1; voter < 8; voter++)
        {
            if(hynixRRT->sets[voter].inv.ubRRPara[retryCount][byteCount] ==
                    hynixRRT->sets[nominee].inv.ubRRPara[retryCount][byteCount])
            {
                votes[nominee]++;
            }
        }
    }
    for(nominee = 0; nominee < 4; nominee++)
    {
        if(votes[nominee] > 4)
        {
            RRPara_7_0 = (U8)(~hynixRRT->sets[nominee].inv.ubRRPara[retryCount][byteCount]);
            elected = nominee;
            break;
        }
        if((nominee == 3) && (elected == 0xFF))
        {
            return ERR_HYNIX_JUDGE_RRT_FAIL;
        }
    }

    if (RRPara_7_0 == 0)
    {
        printk("[V5Q] RRT Rev. 03 is not supported!!!\r\n");
        return ERR_HYNIX_JUDGE_RRT_FAIL;
    }
    else if (RRPara_7_0 == 0xF7)
    {
        printk("[V5Q] RRT Rev. 02 need HRR\r\n");
        return ERR_HYNIX_JUDGE_RRT_FAIL;
    }
    else
    {
        printk("[V5Q] RRT Rev. 04\r\n");
        return ERR_OK;
    }
    return ERR_OK;
}

U32 llfHynixBuildRRT()
{
    llfHYNIX_RR_Table_QLC *rrt_QLC;
    U8 mode;
    U32 ret = ERR_OK, cmp, addr, phyaddr;
    U16 rrt_size_QLC = 12016;

    mode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);

    addr = TEMP_BUF_ADDR;
    phyaddr = TEMP_BUF_PHY_ADDR;


    FCReadRRT(mode, 0, phyaddr, rrt_size_QLC, 0);
    FcBusyWait1ms(5);

    ret = FCCompletionPolling(&cmp, gul_FW_TAG);
    if(ret != ERR_OK)
    {
        printk("HYNIX get QLC RRT fail ret %x\r\n", ret);
        llfAddErrorMessage(0, 0, ERR_RRT_REV);
        return ERR_HYNIX_JUDGE_RRT_FAIL;
    }
    else
    {
        rrt_QLC = (llfHYNIX_RR_Table_QLC*)addr;
        ret = llfJudgeFinalHynixRRT(rrt_QLC);
        if(ret != ERR_OK)
        {
            printk("Hynix build read retry table fail!\r\n");
            llfAddErrorMessage(0, 0, ERR_RRT_REV);
            return ERR_HYNIX_JUDGE_RRT_FAIL;
        }
    }


    return ret;
}
#endif
#ifdef SLC_RRT
#if defined(FTL_H3DTV3) || defined (FTL_H3DTV6) || defined(FTL_H3DTV4) || defined(FTL_H3DTV5) || defined(FTL_H3DTV7) || defined(FTL_H3DQV5)
U32 llfJudgeFinalHynixRRT(HYNIX_RR_Table_SLC * hynixRRT, U32 retryParaAddress)
{
    struct _HYNIX_RETRY_MANUAL_SLC *retryPara;
    retryPara = (struct _HYNIX_RETRY_MANUAL_SLC *)retryParaAddress;
    U8 retryCount, ubGear;
    U8 votes[4], nominee, voter, elected;
#if defined(FTL_H3DTV3)
    U32 gubLllfSLCMaxRetryCount = 36;
#else
    U32 gubLllfSLCMaxRetryCount = 54;
#endif
#if defined(FTL_H3DTV3)
    U8 ubMaxRetry = 32;
#else
    U8 ubMaxRetry = 50;
#endif
    U8 ubManualGearCount = 4;

    U8 manual_rrt[4] =
    {
        0x04, 0x02, 0xfe, 0xfc,
    };

    for(retryCount = 0; retryCount < 8; retryCount++)
    {
        if((hynixRRT->regCount[retryCount] != 0x1) || (hynixRRT->totalCount[retryCount] != ubMaxRetry))
        {
            printk("[RRT] Fail SLC reg #RETRY %d #BYTE %d\r\n",
                   hynixRRT->totalCount[retryCount], hynixRRT->regCount[retryCount]);
            return ERR_HYNIX_JUDGE_RRT_FAIL;
        }
    }

    for(retryCount = 0; retryCount < gubLllfSLCMaxRetryCount; retryCount++)
    {
        if(retryCount < ubManualGearCount)
        {
            retryPara->ubRRPara[retryCount] = manual_rrt[retryCount];
            continue;
        }
        else
            ubGear = retryCount - ubManualGearCount;

        for(nominee = 0; nominee < 4; nominee++)
        {
            votes[nominee] = 1;
        }
        elected = 0xFF;
        for(nominee = 0; nominee < 4; nominee++)
        {
            for(voter = nominee + 1; voter < 8; voter++)
            {
                if(hynixRRT->sets[voter].ori.ubRRPara[ubGear] ==
                        hynixRRT->sets[nominee].ori.ubRRPara[ubGear])
                {
                    votes[nominee]++;
                }
            }
        }
        for(nominee = 0; nominee < 4; nominee++)
        {
            if(votes[nominee] > 4)
            {
                retryPara->ubRRPara[retryCount] =
                    hynixRRT->sets[nominee].ori.ubRRPara[ubGear];
                elected = nominee;
                break;
            }
        }
        if(elected != 0xFF)
        {
            continue;
        }

        for(nominee = 0; nominee < 4; nominee++)
        {
            votes[nominee] = 1;
        }

        for(nominee = 0; nominee < 4; nominee++)
        {
            for(voter = nominee + 1; voter < 8; voter++)
            {
                if(hynixRRT->sets[voter].inv.ubRRPara[ubGear] ==
                        hynixRRT->sets[nominee].inv.ubRRPara[ubGear])
                {
                    votes[nominee]++;
                }
            }
        }
        for(nominee = 0; nominee < 4; nominee++)
        {
            if(votes[nominee] > 4)
            {
                retryPara->ubRRPara[retryCount] =
                    (U8)(~hynixRRT->sets[nominee].inv.ubRRPara[ubGear]);
                elected = nominee;
                break;
            }
        }
        if(elected == 0xFF)
        {
            return ERR_HYNIX_JUDGE_RRT_FAIL;
            printk("[RRT] FAIL SLC cnt %d\r\n", retryCount);
        }
    }
#if 0
    printk("====== Hynix SLC RRT ====== %d \r\n", gubLllfSLCMaxRetryCount);
    for(retryCount = 0; retryCount < gubLllfSLCMaxRetryCount; retryCount++)
    {
        printk("SLCRR %d: %d\r\n", retryCount, retryPara->ubRRPara[retryCount]);
    }
#endif
    return ERR_OK;
}

U32 llfHynixBuildRRT(U8 ubBank, U32 llfRRTSLCBaseAddr)
{
    HYNIX_RR_Table_SLC *rrt_SLC;
    U8 mode;
    U32 ret = ERR_OK, cmp, addr, phyaddr;
    U16 uwSLCRRTSize = 816;

    mode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);

    addr = TEMP_BUF_ADDR;
    phyaddr = TEMP_BUF_PHY_ADDR;

    memset((void *)llfRRTSLCBaseAddr, 0, sizeof(struct _HYNIX_RETRY_MANUAL_SLC));
    memset((void *)addr, 0, sizeof(struct _HYNIX_RETRY_MANUAL_SLC));
    FCReadRRT(mode, ubBank, phyaddr, uwSLCRRTSize, 1);
    FcBusyWait1ms(5);

    ret = FCCompletionPolling(&cmp, gul_FW_TAG);
    if(ret != ERR_OK)
    {
        printk("HYNIX get SLC RRT fail ret %x\r\n", ret);
        llfAddErrorMessage(0, 0, ERR_RRT_REV);
        return ERR_HYNIX_JUDGE_RRT_FAIL;
    }
    else
    {
        rrt_SLC = (HYNIX_RR_Table_SLC *)addr;
        ret = llfJudgeFinalHynixRRT(rrt_SLC, (U32)llfRRTSLCBaseAddr);
        if(ret != ERR_OK)
        {
            printk("Hynix build read retry table fail!\r\n");
            llfAddErrorMessage(0, 0, ERR_RRT_REV);
            return ERR_HYNIX_JUDGE_RRT_FAIL;
        }
    }


    return ret;
}
#endif
#endif
