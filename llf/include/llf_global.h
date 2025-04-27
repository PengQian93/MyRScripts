#ifndef _LLF_GLOBAL_H
#define _LLF_GLOBAL_H

#include "llf_public_com.h"
#include "lib_fc_public.h"
#include "lib_structure.h"
#include "fe_be_public.h"
#include "task_table.h"
#include "lib_sblk_config.h"

#if defined(RL6643_VA)
#define PBA_TOTAL_USABLE_BITS 27
#define SUPER_PBA_BITS        27
#elif defined(RL6577_VA) || defined(RTS5771_FPGA) || defined(RTS5771_VA)
#define PBA_TOTAL_USABLE_BITS 26
#define PBA_NUM_IN_FW_23BIT   23
#define PBA_NUM_IN_FW_24BIT   24
#define PBA_NUM_IN_FW_25BIT   25
#define PBA_NUM_IN_FW_26BIT   26
#else
#define PBA_NUM_IN_FW_TSB_BICS4 25
#define PBA_NUM_IN_FW         23
#define PBA_NUM_IN_FW_HYV5Q   25
#define PBA_NUM_IN_FW_B16A    25
#define PBA_NUM_IN_FW_B17A    26
#define PBA_NUM_IN_FW_B27A    26
#define PBA_NUM_IN_FW_N18A    26
#define PBA_NUM_IN_FW_B37R    26
#define PBA_NUM_IN_FW_B47R    26
#define PBA_NUM_IN_FW_YX2T    25
#define PBA_NUM_IN_FW_YX2Q    26
#define PBA_NUM_IN_FW_N48R    26
#endif

//for Dump LLF Log
#define LLF_UART_MEM_MAX_LEN 123
#define LLF_ETH_ALEN  6

#ifdef LLF_GLOBAL
#define _LLF_GLOBAL_DECL_ __attribute__((section(".ram_global")))
#if defined(RL6643_VA) || defined(RL6531_VB)
#define _LLF_GLOBAL_NEW_DECL_ __attribute__((section(".ram_global_new")))
#else
#define _LLF_GLOBAL_NEW_DECL_ _LLF_GLOBAL_DECL_
#endif

#else

#define _LLF_GLOBAL_DECL_ extern __attribute__((section(".ram_global")))
#ifdef RL6643_VA
#define _LLF_GLOBAL_NEW_DECL_ extern __attribute__((section(".ram_global_new")))
#else
#define _LLF_GLOBAL_NEW_DECL_ _LLF_GLOBAL_DECL_
#endif

#endif

// support wsRDT NAND log
typedef struct _RDTLOG_SPBLOCK
{
    U8 ubBank;
    U16 uwPage;
    U8 ubBlock;
    U8 ubGetBlock;
    U8 ufFoundBlock;
} RDTLOG_SPBLOCK;

#ifdef LLF_AUTO_RMA
//support ws Header struct
typedef union _HeaderFor4k
{
    struct
    {
        U32 LBN;
        union
        {
            struct
            {
                U8 isGC: 4;
                U8 isSLC: 2;
                U8 isSLCBuff: 2;
            } u8_1;
            U8 AsU8_1;
        };
        U32 UNC: 8;
        U32 WCount_part: 16;
        U32 PrePage_orgPBA;
        U32 rsvd0;
    } bits;
    U32 AsU32[4];
} HeaderFor4k;

typedef struct _head_content
{
    HeaderFor4k head_4k[4];  // for 1-plane
} head_content;

//support AutoRMA Nand Log
typedef struct _NandLogRecIndex
{
    U64 Wc;
    U16 Block;
    U16 Page;
    U8 Bank: 4; //max:16 bank
    U8 Offset: 2; //max: 4 offset
    U8 isSLC: 1;
    U8 Rsvd: 1;
} NandLogRecIndex; //13 byte

_LLF_GLOBAL_NEW_DECL_ U16 guwMpBlkL2PStart;
_LLF_GLOBAL_NEW_DECL_ U16 guwSpBlkUserStart;
_LLF_GLOBAL_NEW_DECL_ U16 guwDynamicSblockStart;
_LLF_GLOBAL_NEW_DECL_ U16 guwDynamicSblockEnd;
_LLF_GLOBAL_NEW_DECL_ U16 guwMpBlkSSStart;
_LLF_GLOBAL_NEW_DECL_ U16 guwMpBlkSSEnd;
_LLF_GLOBAL_NEW_DECL_ U8 gubSSDataPageSize;
_LLF_GLOBAL_NEW_DECL_ U8 gubSTableEntrySizeShift;
_LLF_GLOBAL_NEW_DECL_ NandLogRecIndex gNLRecIndex[NAND_LOG_REC_MAX]; //256*13= 3.25KB
_LLF_GLOBAL_NEW_DECL_ U8 gubNLHashTable[NAND_LOG_REC_MAX]; //256B
_LLF_GLOBAL_NEW_DECL_ U16 guwCurNLRecIndex;
_LLF_GLOBAL_NEW_DECL_ U8 gubCurTxPart;
_LLF_GLOBAL_NEW_DECL_ U32 gulCurTxLength;
_LLF_GLOBAL_NEW_DECL_ U8 gubConfigTag;
_LLF_GLOBAL_NEW_DECL_ U8 gubRemapTag;
_LLF_GLOBAL_NEW_DECL_ U8 gubIsSLC;
_LLF_GLOBAL_NEW_DECL_ U32 gulCurTxPAGE0Length;
_LLF_GLOBAL_NEW_DECL_ U32 gulCurTxBLOCKLength;
_LLF_GLOBAL_NEW_DECL_ U32 gulCurTxURFileLength;
#endif

//Add LLF global from here (need fix)
_LLF_GLOBAL_NEW_DECL_ hlc_copyback gsHlc_copyback;
#ifdef COPYBACK_WITH_MTL
_LLF_GLOBAL_NEW_DECL_ U16 gubRDTCopyBackSourceBlock[4];//max support 4 lun
#else
_LLF_GLOBAL_NEW_DECL_ U16 gubRDTCopyBackSourceBlock;
#endif
_LLF_GLOBAL_NEW_DECL_ U8 hlc_size_write_6p_cache_DRAM;
_LLF_GLOBAL_NEW_DECL_ U8 hlc_size_read_6p_cache_DRAM;
_LLF_GLOBAL_NEW_DECL_ hlc_Hexaread_cache_DRAM gsHlcReadCacheHexaDRAM;
_LLF_GLOBAL_NEW_DECL_ hlc_Hexawrite_cache_DRAM gsHlcWriteCacheHexaDRAM;
_LLF_GLOBAL_NEW_DECL_ U16 guwSBlkNo[CH_NUM_MAX * CE_NUM_MAX][SYS_BLK];
_LLF_GLOBAL_NEW_DECL_ LLF_STEP_INFO gubLLFSubStep;
_LLF_GLOBAL_NEW_DECL_ U8   gubZQKEn;
_LLF_GLOBAL_NEW_DECL_ U32 gulVrefCfg;
_LLF_GLOBAL_NEW_DECL_ U8 gubxor_ctrl;
#ifdef SBLK_EXPAND
_LLF_GLOBAL_NEW_DECL_ U8 gubSblkBankCnt;
_LLF_GLOBAL_NEW_DECL_ U8 gubSblkStart;
_LLF_GLOBAL_NEW_DECL_ U8 gubSblkBankStart;
_LLF_GLOBAL_NEW_DECL_ U32 gulSblkCHCEMap[CH_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U32 gulSblkBankBitMap;
_LLF_GLOBAL_NEW_DECL_ U8 gubTSblk;
#endif
#ifndef AUTO_DETECT_DIE
_LLF_GLOBAL_NEW_DECL_ U32 gulCHDieMap[CH_NUM_MAX][2];
_LLF_GLOBAL_NEW_DECL_ U32 gulBankMapPro[2];
_LLF_GLOBAL_NEW_DECL_ U8  gubTrueBankNum;
#endif
#ifdef LLF_CHECK_SYSTEMBLK_ERASE_FAIL
_LLF_GLOBAL_NEW_DECL_ U32 gulEraseFailSysblk;
#endif
_LLF_GLOBAL_NEW_DECL_ U8 gfPureSLC;
_LLF_GLOBAL_NEW_DECL_ U32 gulAutoCapacity[8];
_LLF_GLOBAL_NEW_DECL_ U8 gubSmartInfo;
_LLF_GLOBAL_NEW_DECL_ U8 gubFWFeatureSetting;
_LLF_GLOBAL_NEW_DECL_ RDTLOG_SPBLOCK gsRDTLog;
_LLF_GLOBAL_NEW_DECL_ U8  gubEraseCmpNum[BANK_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U8  gubErasetimeout;
_LLF_GLOBAL_NEW_DECL_ U32 gulFcOcd0;
_LLF_GLOBAL_NEW_DECL_ U32 gulFcOcd1;
_LLF_GLOBAL_NEW_DECL_ U32 gulFcOcd2;
_LLF_GLOBAL_NEW_DECL_ U32 gulFcOcd3;
_LLF_GLOBAL_NEW_DECL_ U32 gulMultiWRBlockIndex[BANK_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U32 gulMultiWRPageIndex;
_LLF_GLOBAL_NEW_DECL_ U8  gubMultiWRCacheBankIndex;
_LLF_GLOBAL_NEW_DECL_ U32 gulMultiWRCacheBlockIndex;
_LLF_GLOBAL_NEW_DECL_ U8  gubNvmeThinDelayMin[CH_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U8  gubNvmeThinDelayMax[CH_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U8  gubNvmeThinMax;
_LLF_GLOBAL_NEW_DECL_ U8  gubNvmeRxDelayMinTemp;
_LLF_GLOBAL_NEW_DECL_ U8  gubNvmeRxDelayMaxTemp;
_LLF_GLOBAL_NEW_DECL_ U8  gubNvmeTxDelayMin[CH_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U8  gubNvmeTxDelayMax[CH_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U8  gubOnfiTxPi[CH_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U8  gubNvmeThinGap;
_LLF_GLOBAL_NEW_DECL_ U8 gubSlcCacheReadEn;
_LLF_GLOBAL_NEW_DECL_ U8 gubSlcCacheProgramEn;
_LLF_GLOBAL_NEW_DECL_ U8  gubFcClkOffset;
_LLF_GLOBAL_NEW_DECL_ U8  gubFcDqOdtIndex;
_LLF_GLOBAL_NEW_DECL_ U8 gubSysBankNo;
_LLF_GLOBAL_NEW_DECL_ U8 gubRdtReadWriteFlowMode;

_LLF_GLOBAL_NEW_DECL_ U32 gulCmdStart[BANK_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U32 gulTxDone[BANK_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U32 gulCmpDone[BANK_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U32 gulCmp[BANK_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U8 gubDisablePrintTag;

_LLF_GLOBAL_NEW_DECL_ hlc_read_RRT gsHlcReadRRT;
_LLF_GLOBAL_NEW_DECL_ hlc_oneCmd gsHlcOneCmd;
_LLF_GLOBAL_NEW_DECL_ hlc_cacheread_MPpage_dummy gsHlcCacheReadMPDummy;
_LLF_GLOBAL_NEW_DECL_ hlc_nandtestread_cache_DRAM gsHlcnandtestReadCacheDRAM;
_LLF_GLOBAL_NEW_DECL_ hlc_nandtestwrite_cache_DRAM gsHlcnandtestWriteCacheDRAM;
_LLF_GLOBAL_NEW_DECL_ hlc_setParams gsHlcSetParams;
_LLF_GLOBAL_NEW_DECL_ hlc_xor_read_1page_DRAM gsHlcXorRead1PDRAM;
_LLF_GLOBAL_NEW_DECL_ hlc_read_1page_DRAM gsHlcRead1PDRAM;
#if defined(FTL_N38B) || defined(FTL_Q5171A)
_LLF_GLOBAL_NEW_DECL_ hlc_Preset_Mode gshlcPresetMode;
_LLF_GLOBAL_NEW_DECL_ hlc_Preset_Mode_MP gshlcPresetModeMP;
#endif
_LLF_GLOBAL_NEW_DECL_ parser_table_index g_parser;

_LLF_GLOBAL_NEW_DECL_ U16   gubBadBlockNum[BANK_NUM_MAX];

_LLF_GLOBAL_NEW_DECL_ U8 gubLlfMSTMergeEnable;
_LLF_GLOBAL_NEW_DECL_ U8 gubRdtSpecialTestFail;

#ifdef BLK_REMAP_PRO
_LLF_GLOBAL_NEW_DECL_ U32 guwRealMPBlkNum;
_LLF_GLOBAL_NEW_DECL_ U8  gubBlkRemapProFlag;
_LLF_GLOBAL_NEW_DECL_ U8  gubBlkIheritRemapFlag;
_LLF_GLOBAL_NEW_DECL_ U8  gubSpBlkCntPerBkPlnLun[BANK_NUM_MAX][PLANE_NUM_MAX][LUN_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U16   guwReMapTableSizePerBank;
_LLF_GLOBAL_NEW_DECL_ U16	guwRemapFlag[BANK_NUM_MAX][PLANE_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U16   guwRemapTempBlock[BANK_NUM_MAX][PLANE_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U16	guwMpDefectBlock[BANK_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U16	guwRealMarkBlockNum[BANK_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U16   gubBlockNoBank[BANK_NUM_MAX];
#endif

#ifdef NEW_BLK_REMAP
//_LLF_GLOBAL_NEW_DECL_ U16	guwRemapLastBlock[BANK_NUM_MAX][PLANE_NUM_MAX];
_LLF_GLOBAL_NEW_DECL_ U8    gubCurRemapIndex[BANK_NUM_MAX];
#endif

#ifdef KEEP_ORIG_DBT
_LLF_GLOBAL_NEW_DECL_ U8 gubSetOrigDBT;
#endif

#ifdef COM_BANKING
_LLF_GLOBAL_NEW_DECL_ U32 gulFailedImageBitMap;
#endif

#ifdef AVG_ERASE_COUNT_TEST
_LLF_GLOBAL_NEW_DECL_   U32 gulDataBsAvgEraseCnt;
_LLF_GLOBAL_NEW_DECL_   U32 gulL2PBsAvgEraseCnt;
_LLF_GLOBAL_NEW_DECL_   U16 guwL2PGroupBegin;
_LLF_GLOBAL_NEW_DECL_   U16 guwL2PGroupEnd;
#endif

_LLF_GLOBAL_NEW_DECL_ U16 guwHostClaimCapacity;
_LLF_GLOBAL_NEW_DECL_ U8 gubRealCapacity;

//for Dump LLF Log
_LLF_GLOBAL_NEW_DECL_ U8  gubLLFMemMsgFlag;
_LLF_GLOBAL_NEW_DECL_ U32 gulLLFMemMsgFreeRoom;

#ifdef EXTEND_STATIC_DBT
_LLF_GLOBAL_NEW_DECL_ U8 gubNeedRebuildRemap;
_LLF_GLOBAL_NEW_DECL_ U32 gulBankRdtResultTag[BANK_NUM_MAX];
#endif

#if defined(RL6643_VA)
_LLF_GLOBAL_NEW_DECL_ U8 gubRaidPagePerRaid;
_LLF_GLOBAL_NEW_DECL_ U8 gubTLCRaidPagePerRaid;
#endif
_LLF_GLOBAL_NEW_DECL_ U32 gulRetryIndexShift;
_LLF_GLOBAL_NEW_DECL_ U8  gubRdtIsArcEnable;
_LLF_GLOBAL_NEW_DECL_ U8 gubRdtRetryInheritEnable;

#ifdef SLC_RRT
_LLF_GLOBAL_NEW_DECL_ U8  gubmaxslcretry;
#endif

/*DDR related global variable, only called by imem code*/
_LLF_GLOBAL_DECL_ U8 gubdram_row_bit;
_LLF_GLOBAL_DECL_ U8 gubdram_colum_bit;
_LLF_GLOBAL_DECL_ U8 gubdram_dq_width;
_LLF_GLOBAL_DECL_ U8 gubDramCurSpeed;
_LLF_GLOBAL_DECL_ U8 gubThreePointKSetting;

_LLF_GLOBAL_DECL_ U32 gulMcIntf;
_LLF_GLOBAL_DECL_ U32 gulMcTestVal;
_LLF_GLOBAL_DECL_ U32 gulDDRDummy;

_LLF_GLOBAL_DECL_ U64 gullckpimap[2];
_LLF_GLOBAL_DECL_ U64 gullwrlvlmap[5];
_LLF_GLOBAL_DECL_ U64 gullwrlvlmap_r[5];
_LLF_GLOBAL_DECL_ U64 gulltm_dqs_dly_map[5];
_LLF_GLOBAL_DECL_ U64 gullrxfifomap;
_LLF_GLOBAL_DECL_ U32 gulrxdelaymap[2][32];
_LLF_GLOBAL_DECL_ U16 guwtxdelaymap[32];

_LLF_GLOBAL_DECL_ FCParameter FCPara;
_LLF_GLOBAL_DECL_ NAND_PARA NandPara;
_LLF_GLOBAL_DECL_ L2pParameter L2pPara;
//_LLF_GLOBAL_DECL_ DEBUGPARA DebugPara;
_LLF_GLOBAL_DECL_ FC_Cmd_Bypass FcCmdBypass;
_LLF_GLOBAL_DECL_ DRAMPARA* PDramPara;
_LLF_GLOBAL_DECL_ DRAMPARA DramPara_default[3];

_LLF_GLOBAL_DECL_ U8 gubIsSerialMultiLUN;
_LLF_GLOBAL_DECL_ U8 gubRealLunNum;
_LLF_GLOBAL_DECL_ U8 gubRowAddrLunShift;
_LLF_GLOBAL_DECL_ U16 guwRealBlockNum;

_LLF_GLOBAL_DECL_ U8 gubCacheSectorNumPerLbnShift;
_LLF_GLOBAL_DECL_ U8 gubLbnNumPerPlane;
_LLF_GLOBAL_DECL_ U8 gubMpModeSelect;
// The acceptable number of free mpblock before GC start for each bank
_LLF_GLOBAL_DECL_ U8 gubGCFreeLimit;
_LLF_GLOBAL_DECL_ U8 gubMaxRetryCount;


//Below are some global counter
_LLF_GLOBAL_DECL_   U32          gulBankRB[CH_NUM_MAX];
_LLF_GLOBAL_DECL_   U32          gulBankFail[CH_NUM_MAX];


_LLF_GLOBAL_DECL_ U32 gulRRTBaseAddr;
_LLF_GLOBAL_DECL_ U32 gulTsbARCBaseAddr;

_LLF_GLOBAL_DECL_   U32  gulFlashVendorNum;
_LLF_GLOBAL_DECL_   U32  gulSysDataLBAStartAddr;
_LLF_GLOBAL_DECL_   U8   gubFCDiffEnable;
_LLF_GLOBAL_DECL_   U8   gubNANDODTEnable;
_LLF_GLOBAL_DECL_   U8   gubNANDODTCfg;
_LLF_GLOBAL_DECL_   U8   gubNANDVrefEn;
_LLF_GLOBAL_DECL_   U8   gubNandDriv;
_LLF_GLOBAL_DECL_   U32  gulNandODTDiffVrefValue;
_LLF_GLOBAL_DECL_   U32  gulFc_ocd;
_LLF_GLOBAL_DECL_   U32  gulFc_dqs_odt;
_LLF_GLOBAL_DECL_   U32  gulFc_dq_re_odt;
_LLF_GLOBAL_DECL_   U32  gulFc_dqs_odt_en;
_LLF_GLOBAL_DECL_   U32  gulFc_dq_re_odt_en;
_LLF_GLOBAL_DECL_   U8   gubCalibrateConfig;
_LLF_GLOBAL_DECL_   U8   gubDqsFix90;
_LLF_GLOBAL_DECL_   U8   gubFcOcdIndex;
_LLF_GLOBAL_DECL_   U8   gubFcDqsOdtIndex;


_LLF_GLOBAL_DECL_   U16  gub_Total_len_per_page;
_LLF_GLOBAL_DECL_   U16  guwCacheSectorNumPerLbn;

_LLF_GLOBAL_DECL_ U8 volatile gfSelfTestFlag;
_LLF_GLOBAL_DECL_ U8 volatile gfTestFlag;
_LLF_GLOBAL_DECL_ U8 gub_column_address_low[MAX_LBN_NUM_PER_PAGE];
_LLF_GLOBAL_DECL_ U8 gub_column_address_high[MAX_LBN_NUM_PER_PAGE];
_LLF_GLOBAL_DECL_ U8 gub_Header_len_per_2K;
_LLF_GLOBAL_DECL_ U8 gub_num_of_2K_per_LBN;
_LLF_GLOBAL_DECL_ U8 gub_num_of_lbn_per_page;
_LLF_GLOBAL_DECL_ U8 gub_num_of_lbn_per_page_Mask;
_LLF_GLOBAL_DECL_ U8 gubCacheErrbitLimit;
#ifndef RL6447_VA
_LLF_GLOBAL_DECL_ U8 gubStartCH;
_LLF_GLOBAL_DECL_ U8 gubCHNum;
#endif

/*
High level command module
*/
_LLF_GLOBAL_DECL_ hlc_reset gsHlcReset;
_LLF_GLOBAL_DECL_ hlc_resetLun gsHlcResetLun;
_LLF_GLOBAL_DECL_ hlc_statusPolling gsHlcStatusPolling;
_LLF_GLOBAL_DECL_ hlc_statusPollingLun gsHlcStatusPollingLun;
_LLF_GLOBAL_DECL_ hlc_readID gsHlcReadID;
_LLF_GLOBAL_DECL_ hlc_readParams gsHlcReadParams;
_LLF_GLOBAL_DECL_ hlc_setFeature gsHlcSetFeature;
_LLF_GLOBAL_DECL_ hlc_ZQCal gsHlcZqCal;
_LLF_GLOBAL_DECL_ hlc_getFeature gsHlcGetFeature;
_LLF_GLOBAL_DECL_ hlc_setFeatureLun gsHlcSetFeatureLun;
_LLF_GLOBAL_DECL_ hlc_getFeatureLun gsHlcGetFeatureLun;
_LLF_GLOBAL_DECL_ U32 gsHlcSetParamsDummy[5];
_LLF_GLOBAL_DECL_ hlc_getParams gsHlcGetParams;
_LLF_GLOBAL_DECL_ hlc_setVCCQ gsHlcSetVCCQ;
_LLF_GLOBAL_DECL_ hlc_read_1page_cache_list gsHlcRead1PCacheList;
_LLF_GLOBAL_DECL_ hlc_read_MPpage_cache_list gsHlcReadMPCacheList;
_LLF_GLOBAL_DECL_ hlc_read_withoutLBNcheck_1page_cache_list gsHlcReadNoLbnCheck1PCacheList;
_LLF_GLOBAL_DECL_ hlc_read_withoutLBNcheck_MPpage_cache_list gsHlcReadNoLbnCheckMPCacheList;
_LLF_GLOBAL_DECL_ U32 gsHlcRead1PDRAMDummy[26];
_LLF_GLOBAL_DECL_ hlc_read_MPpage_DRAM gsHlcReadMPDRAM;
_LLF_GLOBAL_DECL_ hlc_autoBMread_1page gsHlcAutoBMRead1PCacheList;
_LLF_GLOBAL_DECL_ hlc_autoBMread_MPpage gsHlcAutoBMReadMPCacheList;
_LLF_GLOBAL_DECL_ hlc_autoBMread_dummy gsHlcAutoBMReadDummy;
_LLF_GLOBAL_DECL_ hlc_mask_read gsHlcMaskRead;
_LLF_GLOBAL_DECL_ hlc_write_1page_cache_list gsHlcWrite1PCacheList;
_LLF_GLOBAL_DECL_ hlc_write_MPpage_cache_list gsHlcWriteMPCacheList;
_LLF_GLOBAL_DECL_ hlc_write_1page_DRAM gsHlcWrite1PDRAM;
_LLF_GLOBAL_DECL_ hlc_write_MPpage_DRAM gsHlcWriteMPDRAM;
_LLF_GLOBAL_DECL_ hlc_erase gsHlcErase;
_LLF_GLOBAL_DECL_ hlc_erase_MP gsHlcEraseMP;
_LLF_GLOBAL_DECL_ hlc_read_cache_DRAM gsHlcReadCacheDRAM;
_LLF_GLOBAL_DECL_ hlc_Mpread_cache_DRAM gsHlcReadCacheMPDRAM;
_LLF_GLOBAL_DECL_ hlc_write_cache_cache_list gsHlcWriteCacheCacheList;
_LLF_GLOBAL_DECL_ hlc_mpwrite_cache_cache_list gsHlcWriteCacheMPCacheList;
_LLF_GLOBAL_DECL_ hlc_write_cache_DRAM gsHlcWriteCacheDRAM;
_LLF_GLOBAL_DECL_ hlc_Mpwrite_cache_DRAM gsHlcWriteCacheMPDRAM;
_LLF_GLOBAL_DECL_ hlc_read_redundant gsHlcReadRedundant;
_LLF_GLOBAL_DECL_ hlc_DCCTraining gsHlcDCCTraining;
_LLF_GLOBAL_DECL_ hlc_write_redundant gsHlcWriteRedundant;
_LLF_GLOBAL_DECL_ hlc_write_NonData gsHlcslcWriteNondata;
_LLF_GLOBAL_DECL_ hlc_softread_DRAM gsHlcSoftReadDRAM;
_LLF_GLOBAL_DECL_ hlc_write_NonData gsHlcWriteNonData;
_LLF_GLOBAL_DECL_ hlc_Multiwrite_NonData gsHlcMultiWriteNonData;
_LLF_GLOBAL_DECL_ hlc_write_Confirm gsHlcWriteConfirm;
_LLF_GLOBAL_DECL_ hlc_Multiwrite_Confirm gsHlcMultiWriteConfirm;
//_LLF_GLOBAL_DECL_ parser_table_index g_parser;
_LLF_GLOBAL_DECL_ U16 g_parser_dummy[86]; //need fix

//for llf
_LLF_GLOBAL_DECL_ U32 gufLastEraseFlag;
_LLF_GLOBAL_DECL_ U16 guwLLFSnapshotGroupEnd;
//_LLF_GLOBAL_DECL_ U16 guwTemperatureLimit;
_LLF_GLOBAL_DECL_ U16 guwCacll2PGroupEnd;
_LLF_GLOBAL_DECL_ U16 guwCaclDynSBlockBegin;
_LLF_GLOBAL_DECL_ U16 guwCaclDynSBlockEnd;
_LLF_GLOBAL_DECL_ U16 guwCaclSSGroupBegin;
_LLF_GLOBAL_DECL_ U16 guwCaclSSGroupEnd;
_LLF_GLOBAL_DECL_ U16 guwLdpcParityLen;
_LLF_GLOBAL_DECL_ U16 guwLdpclength;
#if defined(RL6577_VA)||defined(RTS5771_VA)
_LLF_GLOBAL_DECL_ U8  gubExtendRdtResultBlk;
#endif

_LLF_GLOBAL_NEW_DECL_ U16 guwVirtualBsNum;
_LLF_GLOBAL_NEW_DECL_ U16 guwVirtualBsStart;
_LLF_GLOBAL_NEW_DECL_ U8 gubVirtualBankNum;

_LLF_GLOBAL_DECL_ U8 gubLLFALLStep;
_LLF_GLOBAL_DECL_ U8 gubVthEnNum;
_LLF_GLOBAL_DECL_ U8 hlc_size_write_mp_cache_DRAM;
_LLF_GLOBAL_DECL_ U8 hlc_size_read_mp_cache_DRAM;
_LLF_GLOBAL_DECL_ U8 ubRDTLLFNormalEn;
_LLF_GLOBAL_DECL_ U8 ubRDTLLFCheckPerDieEn;
_LLF_GLOBAL_DECL_ U8 gubDefectRatioPerBank;

_LLF_GLOBAL_DECL_ U8 gubTemperatureOffset;
_LLF_GLOBAL_DECL_ U8 gubTemperatureMax;
_LLF_GLOBAL_DECL_ U8 gubTemperatureMin;
_LLF_GLOBAL_DECL_ U8 gubFixTemperature;
_LLF_GLOBAL_DECL_ U8 gubLLFMode;
_LLF_GLOBAL_DECL_ U8 gfDBTInitDone;
_LLF_GLOBAL_DECL_ U8 gubLdpcCodeRate;
_LLF_GLOBAL_DECL_ U8 gubECC_CFG;

_LLF_GLOBAL_DECL_ U8 gubEraseNum;
_LLF_GLOBAL_DECL_ U32 gulPlaneBad[PLANE_NUM_MAX];

_LLF_GLOBAL_DECL_ U32 gulLLFTypeRecord;
_LLF_GLOBAL_DECL_ U32 gulWriteMBRecord[2];

// This is for Block remap
_LLF_GLOBAL_DECL_ U32 gulBlkRemapProFlag;
_LLF_GLOBAL_DECL_ U16 guwRemapBlkStart;
_LLF_GLOBAL_DECL_ U32 guwRemapTableCheck;
_LLF_GLOBAL_DECL_ U16 gul_FW_TAG;
#if defined(RL6577_FPGA) || defined(RTS5771_FPGA) || defined(RL6447_FPGA) || defined(RL6447_VA)

_LLF_GLOBAL_DECL_ U32 gulMaxLBAAddr;

#elif defined(RL6643_FPGA) || defined(RL6643_VA) ||defined(RL6531_VB) || defined(RL6577_VA) ||defined(RTS5771_VA)

#ifdef EXTEND_LBA
_LLF_GLOBAL_DECL_ U64 gulMaxLBAAddr;
#else
_LLF_GLOBAL_DECL_ U32 gulMaxLBAAddr;
#endif

#endif
_LLF_GLOBAL_DECL_ U8 vdd_1v8_en; //GPIO
_LLF_GLOBAL_DECL_ U8 gubNandFlashVendor;//CONFIG
_LLF_GLOBAL_DECL_ U8 gubFcAddressCycleNum;//GPIO
_LLF_GLOBAL_DECL_ U8 gubNandDefaultMode;//0 for SDR,1 for DDR2
_LLF_GLOBAL_DECL_ U8 gubHeaderlen;
_LLF_GLOBAL_DECL_ U8 gubRdtImg;
_LLF_GLOBAL_DECL_ U8 gubCmpSel; //³õÊ¼»¯Îª0
_LLF_GLOBAL_DECL_ U32 gulSysblk;
_LLF_GLOBAL_DECL_ U32 g_be2fe_admin_lock;
_LLF_GLOBAL_DECL_ U8 gubNandFlashType;
_LLF_GLOBAL_DECL_ U8 gubBank0CH;
_LLF_GLOBAL_DECL_ U8 gubBank0CE;
_LLF_GLOBAL_DECL_ U8 gubBank1CH;
_LLF_GLOBAL_DECL_ U8 gubBank1CE;
_LLF_GLOBAL_DECL_ U16 gub_Total_len_per_2K;
_LLF_GLOBAL_DECL_ U16 INDEX_ParserTable1[PARSER_MAX_LEN];
_LLF_GLOBAL_DECL_ U8 gub_fe_mode; //0:NVMe, 1:SATA, 2:AHCI
_LLF_GLOBAL_DECL_ U16 remap_ofst[PAGE_NUM];
_LLF_GLOBAL_DECL_ U8 gubECC_CFG0;
_LLF_GLOBAL_DECL_ U8 INDEX_SequencerTable1[3][16];
_LLF_GLOBAL_DECL_ U16 gub_ECC_parity_len_per_2K;
#if defined(RL6643_VA) || defined(RL6577_VA)
#ifdef SBLK_EXPAND
_LLF_GLOBAL_DECL_ U8 gubSblkBadDetect;
_LLF_GLOBAL_DECL_ U8 gubDefaultSblkBad;
#endif
#endif

_LLF_GLOBAL_DECL_ U8 curCopybackLoopCntforMtLUN;
_LLF_GLOBAL_DECL_ U8 curProgramLoopCntforMtLUN;


#if defined(RL6447_FPGA) || defined(RL6447_VA) || defined(RL6531_VB)
_LLF_GLOBAL_DECL_ U32 gulLdpcTableAddr[8];

#endif


#if defined(RL6447_FPGA) || defined(RL6447_VA)

#define SPI_DL_SHIFT_SETTING_WHEN_EA		0xA003FFF0

#endif

#ifdef _COMMEM_GLOBAL
#define _COMMEM_GLOBAL_DECL_ __attribute__((section(".rom_ram_global")))
#else
#define _COMMEM_GLOBAL_DECL_ extern __attribute__((section(".rom_ram_global")))
#endif



_COMMEM_GLOBAL_DECL_ SLLCTRL BE2FEAdminCmdListCtrl;// for admin command
_COMMEM_GLOBAL_DECL_ SLLCTRL BE2FEStatus1MsgListCtrl;// for status1 msg

_COMMEM_GLOBAL_DECL_ HostNonAdminCommand gpHostNonAdminCmd[HOST_NON_ADMIN_CMD_NUM_MAX];
_COMMEM_GLOBAL_DECL_ HostAdminCommand gpHostAdminCmd[HOST_ADMIN_CMD_NUM_MAX];
_COMMEM_GLOBAL_DECL_ U32 gulHostNonAdminCmdBufStart;// for HostNonAdminCmd
_COMMEM_GLOBAL_DECL_ U32 gulHostAdminCmdBufStart;// for HostAdminCmd
_COMMEM_GLOBAL_DECL_ taskTable_t taskTable_BE_Cpu;
_COMMEM_GLOBAL_DECL_ taskTable_t taskTable_FE_Cpu;

#endif
