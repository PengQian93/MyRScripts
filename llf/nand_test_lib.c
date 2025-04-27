
#include "lib_basetype.h"
#include "timer.h"
#include "system_reg.h"
#include "platform.h"
#include "memory.h"
#include "spic.h"
#include "codebank.h"
#include "lib_phy_public.h"
#include "lib_fc_public.h"
#include "fc_reg.h"
#include "fe_be_public.h"
#include "scheduler0.h"
#include "scheduler1.h"
#include "serial.h"
#include "smp.h"
#include "llf_public.h"
#include "llfmp.h"
#include "llf_lib_public.h"
#include "llf_global.h"
#include "nand_test_public.h"
#include "nand_test.h"
#include "platform_global.h"


#include "lib_retcode.h"
#include "lib_fc_public.h"
#include "lib_retcode.h"
#include "lib_debug.h"
#include "lib_cpu.h"
#include "lib_fio_public.h"
#include "lib_sblk_config.h"
#include "lib_hlc_com.h"

#ifdef LLF_ALONE
#if defined(RTS5771_FPGA)|| defined(RTS5771_VA)
#else
#ifdef FTL_SSV4
void llfFCCmdWriteNoData(U8 ubMode, U8 ubBank, U16 uwBlock, U8 ubPlaneCnt)
{
    hlc_Samsung_write_nondata sWriteNonData;
    U8 ubHlcSize, ubDCnt = 0;
    U8 ubPlane;

    sWriteNonData.control_bit.AsU32 = 0;
    sWriteNonData.control_bit.bits.cmd_valid = 1;

    if(ubPlaneCnt == 1)
    {
        // TODO: single plane write  handle block index - row addr
        sWriteNonData.control_bit.bits.cmd_index = g_parser.index.WriteNoData;
    }
    else
    {
        sWriteNonData.control_bit.bits.cmd_index = g_parser.index.MultiWriteNoData;
    }
    sWriteNonData.control_bit.bits.scramble_bypass = 1;
    sWriteNonData.control_bit.bits.ecc_bypass = 1;
    sWriteNonData.control_bit.bits.aes_bypass = 1;

    sWriteNonData.head2.AsU32 = 0;
    sWriteNonData.head2.bytes.FW_tag = gul_FW_TAG;
    sWriteNonData.head2.bytes.IF_sel = ONFI_DDR2_TOGGLE;//ubMode;


    sWriteNonData.head3.AsU32 = 0;

    for(ubPlane = 0; ubPlane < ubPlaneCnt; ubPlane++)
    {
        sWriteNonData.parameter[ubDCnt++] = 0;
        sWriteNonData.parameter[ubDCnt++] = 0;
        sWriteNonData.parameter[ubDCnt++] = 0;
        sWriteNonData.parameter[ubDCnt++] = 0;
        sWriteNonData.parameter[ubDCnt++] = 0;
#if 0
        if(ubPlane == ubPlaneCnt - 1)
        {

            if(ubPlaneCnt == 1)
            {
                sWriteNonData.parameter[ubDCnt++] = (((uwBlock & 0x1) + 1) << 4) | 0x1;
            }
            else
            {
                sWriteNonData.parameter[ubDCnt++] = 0x31;
            }
        }
#endif
        if (ubPlane == ubPlaneCnt - 1)
        {
            sWriteNonData.parameter[ubDCnt++] = (0x1) | ((ubPlaneCnt + (uwBlock & 0x1) + 1 ) << 4);
        }

        sWriteNonData.parameter[ubDCnt++] = 0;
        sWriteNonData.parameter[ubDCnt++] = 0;
        sWriteNonData.parameter[ubDCnt++] = 0;
    }

    while((ubDCnt & 0x3) != 0)
    {
        sWriteNonData.parameter[ubDCnt++] = 0;
    }

    ubHlcSize = (ubDCnt >> 2) + 3;
    sWriteNonData.control_bit.bits.hlcmd_size = ubHlcSize - 1;

    PushFcCmdFifo(ubBank, &sWriteNonData.control_bit.AsU32, ubHlcSize);
}

/*U16 GetHSPAddress(U16 uwPage)
{
    ASSERT(ALWAYS_MSG, uwPage <= 0x2FF)

    if(uwPage >= 0x2F8)
    {
        return ((uwPage - 0x2F8) + 0x100);
    }
    else if(uwPage < 8)
    {
        return (uwPage >> 1);
    }
    else if(uwPage >= 0x2F0)
    {
        return (((uwPage - 0x2F0) >> 1) + 0xFC);
    }
    else
    {
        return (((uwPage - 8) / 3) + 4);
    }
}*/

void llfFcCmdWriteConfirm(U8 ubMode, U8 ubBank, U16 uwBlock, U16 uwPage, U8 ubPlaneCnt)
{
    hlc_Samsung_multiwrite_confirm sWriteConfirm;
    U8 ubHlcSize, ubDCnt = 0;
    U8 ubPlane;
    //U16 HSPAddr GetHSPAddress(uwPage);//TBD
    U32 ulRowAddr[2];

    sWriteConfirm.control_bit.AsU32 = 0;
    sWriteConfirm.control_bit.bits.cmd_valid = 1;

    if(ubPlaneCnt == 1)
    {
        sWriteConfirm.control_bit.bits.cmd_index = g_parser.index.WriteConfirm;
    }
    else
    {
        sWriteConfirm.control_bit.bits.cmd_index = g_parser.index.MultiWriteConfirm;
    }
    sWriteConfirm.control_bit.bits.scramble_bypass = 1;
    sWriteConfirm.control_bit.bits.ecc_bypass = 1;
    sWriteConfirm.control_bit.bits.aes_bypass = 1;

    sWriteConfirm.head2.AsU32 = 0;
    sWriteConfirm.head2.bytes.FW_tag = gul_FW_TAG;
    sWriteConfirm.head2.bytes.IF_sel = ONFI_DDR2_TOGGLE;//ubMode;

    sWriteConfirm.head3.AsU32 = 0;

    for(ubPlane = 0; ubPlane < NandPara.ubPlaneNumPerLun; ubPlane++)
    {
        //ulRowAddr[ubPlane] = ((CalLunBlockRowValue(ubBank, uwBlock + ubPlane)
        //                     << NandPara.ubSLCPageNumPerBlockShift) | HSPAddr);
        ASSERT(ALWAYS_MSG, 0);//TBD
    }

    for(ubPlane = 0; ubPlane < ubPlaneCnt; ubPlane++)
    {
        sWriteConfirm.parameter[ubDCnt++] = 0;
        sWriteConfirm.parameter[ubDCnt++] = 0;
        sWriteConfirm.parameter[ubDCnt++] = (U8)(ulRowAddr[ubPlane] & 0xFF);
        sWriteConfirm.parameter[ubDCnt++] = (U8)((ulRowAddr[ubPlane] >> 8) & 0xFF);
        sWriteConfirm.parameter[ubDCnt++] = (U8)((ulRowAddr[ubPlane] >> 16) & 0xFF);

        sWriteConfirm.parameter[ubDCnt++] = (U8)(ulRowAddr[ubPlane] & 0xFF);
        sWriteConfirm.parameter[ubDCnt++] = (U8)((ulRowAddr[ubPlane] >> 8) & 0xFF);
        sWriteConfirm.parameter[ubDCnt++] = (U8)((ulRowAddr[ubPlane] >> 16) & 0xFF);
    }

    while((ubDCnt & 0x3) != 0)
    {
        sWriteConfirm.parameter[ubDCnt++] = 0;
    }

    ubHlcSize = (ubDCnt >> 2) + hlc_size_statusPolling;//3;
    sWriteConfirm.control_bit.bits.hlcmd_size = ubHlcSize - 1;

    PushFcCmdFifo(ubBank, &sWriteConfirm.control_bit.AsU32, ubHlcSize);
}

#endif
#endif


void NandTestRead_Redundant(U32 ulMode, U8 ubBank, U16 uwBlockNo, U16 uwPageNo,
                            U32 uwReadAddress, U32 uwReadDataLength)
{
#if 0
    U32 row_addr;
    U32 mul_num;
    U32 data_length;
    U32 dcnt = 0;
    U8 hlc_size;
#if 0
    if(gubIsSerialMultiLUN)
    {
        llfMultiLunSetRowAddr(uwBlockNo, uwPageNo, &row_addr);
    }
    else
#endif
    {
        row_addr = ((uwBlockNo << NandPara.ubSLCPageNumPerBlockShift) | uwPageNo);
    }

    // Consider RedundantRead has min unit 16 bytes so make sure datalength over then data 16 bytes
    mul_num = ((uwReadDataLength % 16) != 0) ? ((uwReadDataLength >> 4) + 1) : (uwReadDataLength >> 4);
    uwReadDataLength = mul_num * 16;
    data_length = (ulMode != 0) ? (uwReadDataLength >> 1) : (uwReadDataLength);

    gsHlcReadRedundant.control_bit.bits.cmd_valid = 1;
    gsHlcReadRedundant.control_bit.bits.hlcmd_size = 5;
    gsHlcReadRedundant.control_bit.bits.cmd_index = g_parser.index.ReadRedundant;
    gsHlcReadRedundant.control_bit.bits.with_dram_address = 1;
    gsHlcReadRedundant.control_bit.bits.with_cache_list_index = 0;
    gsHlcReadRedundant.control_bit.bits.data_list_mask_en = 0;
    gsHlcReadRedundant.control_bit.bits.data_en = 1;
    gsHlcReadRedundant.control_bit.bits.head_en = 0;
    gsHlcReadRedundant.control_bit.bits.head_chk_unc_en = 0;
    gsHlcReadRedundant.control_bit.bits.head_chk_lbn_en = 0;
    gsHlcReadRedundant.control_bit.bits.cmd_stop = 0;
    gsHlcReadRedundant.control_bit.bits.scramble_bypass = 1;
    gsHlcReadRedundant.control_bit.bits.ecc_bypass = 1;
    gsHlcReadRedundant.control_bit.bits.aes_bypass = 1;
    gsHlcReadRedundant.control_bit.bits.abort = 0;
    gsHlcReadRedundant.control_bit.bits.drop = 1;

    gsHlcReadRedundant.FW_tag = gul_FW_TAG;
    gsHlcReadRedundant.IF_sel = ulMode;
    gsHlcReadRedundant.xor_data_dir = 0;
    gsHlcReadRedundant.xor_buff_idx = 0;

    gsHlcReadRedundant.mat_idx = 0;
    gsHlcReadRedundant.soft_decision_num = 0;
    gsHlcReadRedundant.soft_decision_enable = 0;
    gsHlcReadRedundant.row_address_auto_gen_enable = 0;
    gsHlcReadRedundant.column_address_auto_gen_enable = 0;
    gsHlcReadRedundant.command_power_credit = 0;
    gsHlcReadRedundant.soft_dec_last = 0;
    gsHlcReadRedundant.rsvd = 0;
    gsHlcReadRedundant.loop_count_0 = 0;
    gsHlcReadRedundant.loop_count_1 = 0;
    gsHlcReadRedundant.loop_count_2 = 0;
    gsHlcReadRedundant.loop_count_3 = 0;


    //dram or sram address
    gsHlcReadRedundant.data_addr = uwReadAddress;
    gsHlcReadRedundant.data_len = uwReadDataLength; //make sure datalenght over than data 16 bytes

    //FIXME
    //if( g_PreCmdEn == 1 )
    gsHlcReadRedundant.parameter[dcnt++] = 0xDF;  // mlc/tlc cmd

    // Row address
    gsHlcReadRedundant.parameter[dcnt++] = uwReadAddress & 0xff;
    gsHlcReadRedundant.parameter[dcnt++] = (uwReadAddress & 0xff00) >> 8;
    gsHlcReadRedundant.parameter[dcnt++] = (row_addr & 0xff);
    gsHlcReadRedundant.parameter[dcnt++] = (row_addr & 0xff00) >> 8;
    gsHlcReadRedundant.parameter[dcnt++] = (row_addr & 0xff0000) >> 16;
    if(FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_B16)
    {
        gsHlcReadRedundant.parameter[dcnt++] = 0x30;
    }
    gsHlcReadRedundant.parameter[dcnt++] = 0x0;
    gsHlcReadRedundant.parameter[dcnt++] = (data_length - 1) & 0xff;
    gsHlcReadRedundant.parameter[dcnt++] = ((data_length - 1) & 0xff00) >> 8;


    hlc_size = (((dcnt & 0x3) == 0) ? (dcnt / 4) : ((dcnt / 4) + 1));
    hlc_size += 5;

    gsHlcReadRedundant.control_bit.bits.hlcmd_size = (hlc_size - 1);
    llfPushFcCmdFifo(ubBank, &gsHlcReadRedundant.control_bit.AsU32, hlc_size);
#endif
}

void NandTest_ReadCache(U8 ulMode, U8 bank)
{
    U32 pAddr;
    //U32 pAddr_phy;
    //U32 head_pAddr;
    U32 cmp;
    U32 i;
    U32 ret = ERR_OK;
    U16 ecc_parity_len[6] = {294, 266, 246, 226, 186, 150}; //bytes,ecc length + 6
    U32 ecc_len = ecc_parity_len[gubLdpcCodeRate];

    for (i = 0; i < (HEADER_DMA_MAX_LEN); i++)
        _REG32(TEMP_HBUF_ADDR + (i * 4)) = ((i << 24) | (i << 16) | (i << 8) | i);

    // DWB that cache line
    cache_area_dwbinval(TEMP_HBUF_ADDR, HEADER_MAX_LEN);
    cache_dummy_update_read();



    for(i = 0; i < 1; i++)
    {
        //---------read----------
        _REG32(TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)) = 0xBadBad;
        _REG32(TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun) + 32) = 0xBadBad;
        cache_area_dwbinval((TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                            NandPara.ubSectorNumPerPage * 512); // 16k Data
        cache_area_dwbinval((TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)),
                            L2pPara.ubHdr4BLenPerPage * 4);
        cache_dummy_update_read();

        // set header DMA
        //head_pAddr = TEMP_HBUF_PHY_ADDR + HEADER_MAX_LEN;

        // set data DMA
        pAddr = (TEMP_BUF_PHY_ADDR + NandPara.ubSectorNumPerPage * 512);

        //ecc_len = FC_TOP_REG(FR_ECC_CFG) & 0xffff;

        gul_FW_TAG = llfBETagSetting(TAG_READ_CACHE, bank);
        FCReadCacheRedundant(ulMode, bank, pAddr,
                             ((NandPara.ubSectorNumPerPage * 512) + 8 * (10 + ecc_len)));
        FcBusyWait1ms(1);
        ret = FCCompletionPolling(&cmp, (gul_FW_TAG ));
        if(ret == ERR_OK)
        {
            if((cmp & BE_COMPLETION_ERROR_MASK) != 0)
            {
                DbgPrintk(ALWAYS_MSG, "Read cache error %x\r\n", cmp);
                //return ERR_WRITE_SBLK;
                ASSERT_LLF(0);
            }
        }
        else
        {
            printk("read cache error %x\r\n", ret);
            ASSERT_LLF(0);
        }
        // Clean read memory cache
        cache_area_dinval((TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                          NandPara.ubSectorNumPerPage * 512); // 16k Data
        cache_area_dinval((TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)),
                          L2pPara.ubHdr4BLenPerPage * 4);
        cache_dummy_update_read();


        // Compare header and data
        /*ret |= llfCompareHead(TEMP_HBUF_ADDR,
                              (TEMP_HBUF_ADDR + (HEADER_MAX_LEN)),
                              L2pPara.ubHdr4BLenPerPage * 4);*/
        ret |= llfnandtestCompareData(TEMP_BUF_ADDR, (TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                                      (NandPara.ubSectorNumPerPage * 512));
        if (ret != ERR_OK)
        {
            ret |= ERR_COMPARE_DATA;
            DbgPrintk(ALWAYS_MSG, "Bank %d\r\n", bank);
        }
    }

}

void NandTest_WriteCache(U8 ulMode, U8 bank)
{
    U32 pAddr;
    //U32 pAddr_phy;
    U32 head_pAddr;
    U32 cmp;
    U32 i;
    U32 ret = ERR_OK;
    U16 ecc_parity_len[6] = {294, 266, 246, 226, 186, 150}; //bytes,ecc length + 6
    U32 ecc_len = ecc_parity_len[gubLdpcCodeRate];
#if defined(RL6577_VA) || defined(RL6643_VA)
    FR_G_CFG_REG32(FR_ECC_BP_MODE_CH) = 0;
    FC_TOP_REG(FR_ECC_BP_MODE) = 0x0;
#endif
    for (i = 0; i < (HEADER_DMA_MAX_LEN); i++)
        _REG32(TEMP_HBUF_ADDR + (i * 4)) = ((i << 24) | (i << 16) | (i << 8) | i);

    // DWB that cache line
    cache_area_dwbinval(TEMP_HBUF_ADDR, HEADER_MAX_LEN);
    cache_dummy_update_read();


    for(i = 0; i < 1; i++)
    {
        head_pAddr = TEMP_HBUF_PHY_ADDR; //TEMP_HBUF_PHY_ADDR;

        //set data DMA
        pAddr = TEMP_BUF_PHY_ADDR;
        ulMode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);
        gul_FW_TAG = llfBETagSetting(TAG_WRITE_CACHE, bank);
        FCWriteCacheRedundant(ulMode, bank, 0, pAddr, NandPara.ubSectorNumPerPage * 512,
                              head_pAddr, 8 * (10 + ecc_len));
        FcBusyWait1ms(1);
        ret = FCCompletionPolling(&cmp, (gul_FW_TAG));
        if(ret == ERR_OK)
        {
            // printk("w,b=%d,cmp=%x\r\n", bank, cmp);
            if((cmp & BE_COMPLETION_ERROR_MASK) != 0)
            {
                DbgPrintk(ALWAYS_MSG, "write cache error: %d\r\n", bank);
                //return ERR_WRITE_SBLK;
                ASSERT_LLF(0);
            }
        }
        else
        {
            printk("write cache error %x\r\n", ret);
            ASSERT_LLF(0);
        }
    }

}

void NandTest_LunDetect(void)
{
    U32 cmp;
    U32 ulmode;
    U32 ret = ERR_OK;

    //Config BE
    _REG32(TEMP_HBUF_ADDR) = 0x0;
    cache_dwbinval(TEMP_HBUF_ADDR);
    cache_dummy_update_read();

    llfInitErrorMessage();
    ulmode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);

    U8 Lun_no, bank_no;
    U8 lun_exit_num = 0;
    U8 ch_no, ce_no;

    for(ch_no = 0 ; ch_no < CH_NUM_MAX; ch_no ++)
    {
        gulCHDieMap[ch_no][0] = 0;
        gulCHDieMap[ch_no][1] = 0;
    }
    // reset all ce  & polling
    for(Lun_no = 0; Lun_no < 8; Lun_no ++)
    {
        for(ch_no = 0 ; ch_no < CH_NUM_MAX; ch_no ++)
        {
            for(ce_no = 0; ce_no < CE_NUM_MAX; ce_no ++)
            {
                bank_no = _MEM08(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_FC_DIE_MAPPING_CE + ch_no * 8 + ce_no);
                //printk("--> bank = %d\r\n", bank_no);
                //reset
                gul_FW_TAG = llfBETagSetting(TAG_RESET, bank_no);
                FCReset(ulmode, bank_no);
                ret = FCCompletionPolling(&cmp, (gul_FW_TAG));
                FcBusyWait1ms(5);

                //polling satus
                gul_FW_TAG = llfBETagSetting(TAG_POLLING_STATUS, bank_no);
                FCStatusPollingLun(ulmode, bank_no, Lun_no);
                ret = FCCompletionPolling(&cmp, (gul_FW_TAG));
                if(ret == ERR_OK)
                {
                    printk("CH%d CE%d LUN %d\r\n", ch_no, ce_no, Lun_no);
                    if(ce_no < 4)
                    {
                        gulCHDieMap[ch_no][0] = gulCHDieMap[ch_no][0] | (1 << (ce_no * 8 + Lun_no));
                    }
                    else
                    {
                        gulCHDieMap[ch_no][1] = gulCHDieMap[ch_no][1] | (1 << ((ce_no - 4) * 8 + Lun_no));
                    }
                }
                else
                {
                    //printk("RESET FAIL,CH%d CE%d LUN %d cmp is %x\r\n", ch_no, ce_no, Lun_no, cmp);
                    llfResetAndRecoverFC(ulmode, FC_PLL_CLK_10M, 10);
                    ConvertBankToChCe(CONFIG_BASE_VA_ADDR, BANK_IMAPPING_TABLE_ADDR);
                }

            }
        }
        lun_exit_num++;
        if(lun_exit_num == NandPara.ubLunNumPerCE)
        {
            break;
        }
    }

    for(ch_no = 0 ; ch_no < CH_NUM_MAX; ch_no ++)
    {
        printk("CHDieMapping[CH%d] = CH%x_CE%x\r\n", ch_no, gulCHDieMap[ch_no][1], gulCHDieMap[ch_no][0]);
    }
    CreateBankConfigByDieMap();
    llfResetAndRecoverFC(ulmode, FC_PLL_CLK_10M, 10);
    //printk("Total_Bank_Num is %d\r\n", NandPara.ubBankNum);
    ConvertBankToChCe(CONFIG_BASE_VA_ADDR, BANK_IMAPPING_TABLE_ADDR);

    // return ret;
}

#endif
