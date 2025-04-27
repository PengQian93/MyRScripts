#ifndef _LLF_LIB_PUBLIC_H
#define _LLF_LIB_PUBLIC_H


#define ASSERT_LLF(X)     {U8 a=1;while(1){if(X)break;else if(a){printk("assert,%d,%s\r\n",__LINE__,__FILE__);a=0;}}}


U32 llfFdmInit() __attribute__((section(".llf_banka1")));
void BE_llf_handle_UTask();
void llfmpInit(void);
U32 llf_init(void);
U32 llfinitConfigBE(PVENDOR_CMD_RESPONSE pVendorCmdResponse);
void llfFcInitReg();
void llfInitParameters();
#if defined(RL6643_VA)
void llf_thermal_setting();
#endif
void llfInitConfig(void);
void llfAPBECalibrate(U8 ubIFType, U8 ubClkMode);
U32 llfEraseBlock0WritePage0(void);
U32 llfCalibrateDqsOut(void);
void llfAPBuildODBT(void);
void llfAPExecuteLLF(U8 ubIfType, U8 ubClkMode);
U32 llfWriteConfig(U8 ubIFType, U8 ubClkMode);
#if defined(RL6577_VA)
U32 llfCheckOriginalRDTResultExist();
#endif
#if 0 //#ifdef MST_MERGE
U32 llfWriteFWMerge(U16 pSBlkNo[CH_NUM_MAX * CE_NUM_MAX][SYS_BLK]);
#endif
U32 llfEraseSblk();
U32 llfEraseOneBlk(U8 bank_no, U16 block_no);
U32 llffioDefectRead(U32 ubIFType, U8 ubBankNo, U16 block, U16 page, U16 offset);
U32 llfChkDefectBlkForSandisk(U8 bank_no, U16 blk_no);
void llfTodoRDT();
void llfAPEraseAll(U8 ubBypassDefectBlock);
void llfAPEraseNotDefectBlock(U8 ubBypassDefectBlock);
void llfAPBurningTestBankPipeline(void);
void SetDefaultTimingInterface();
U32 TsbForceSetSdrMode();
void llfBECheckStatus();
U32 llfCheckFlashSignature();
U32 llfSetSBlockHWConfig(U32 sblk_addr, U8 ubType, U8 ubMode);
U32 llfSetSBlockFWConfig(U32 sblk_addr);
void llfSettingPerCh(U32 addr, U16 mode, U16 Speed);
void llfLoadSblkPerCh(U32 addr, U16 mode);
U32 llffioBECalibrate(U8 bank, U16 block_no, U8 ubIFType, U8 ubMode);
U32 WriteReadFlashCache(U8 bank, U8 ubClkMode);
U32 WriteReadFlashCache_for_repeate();
U32 WriteReadFlash_for_repeate();
U32 repeated_WriteReadCache(U16 cycle);
U32 llfPollingAllBankCMP(U32 *pCMP, U16 uwExpectTag);
U32 llfWriteCache(U8 ubBankNo);
U32 llfWriteCachePipeline(U8 ubBankNo);
U32 llfWriteCacheRedundant(U8 ubBankNo);
U32 llfWriteCacheRedundantPipeline(U8 ubBankNo);
U32 llfReadCacheAndCompare(U8 ubBankNo);
U32 llfReadCacheRawDataCompare(U8 ubBankNo);
void llfResetAndRecoverFC10M(U8 ubFcMode, U8 ubClockMode);
#if defined(RL6643_VA) || defined(RL6531_VB)
void llfSaveTimingDrivingPerCh(U32 addr);
void llfSetTimingDrivingPerCh(U32 addr);
U32 llfCalibrateTxDelayChain(U8 ubBankNo, U8 ubFcMode, U8 ubClockMode, U16 uwSpeed);
#ifdef RL6643_VA
void llfUpdateVrefToEFUSE(U32 VrefCfg);
#endif
#endif

void llfCopyLdpcTable(U32 d_begin_addr, U32 s_begin_addr, U32 s_end_addr);
U32 llfReadOffsetSetting(U32 addr);
U32 TSBToggleResetFlash();
U32 SandiskResetFlash();
void GetNandODTDiffVrefValue();

void spi_init_llf ( void );
void flash_set_status_llf(int flsh_st);
void flash_tx_cmd_llf(int cmd);
void flash_wait_busy_llf();
void flash_rx_cmd_llf(int cmd);
void spi_flash_wait_busy_llf();
void spi_flash_set_protection_llf(BOOL isProtect);

void llfProgramSeqData(U32 ulAddr, U16 uwLength);
void llfProgramRandomData(U32 ulAddr, U16 uwLength);
void llfProgramAllMixData(U32 ulAddr, U16 uwLength);
U32 llfCompareData(U32 ulSrcAddr, U32 ulDstAddr, U32 ulLength);
U32 llfnandtestCompareData(U32 ulSrcAddr, U32 ulDstAddr, U32 ulLength);
U32 llfCompareHead(U32 ulSrcAddr, U32 ulDstAddr, U32 ulLength);
U32 llfCompareDataPer1KB(U32 ulSrcAddr, U32 ulDstAddr, U32 ulLength);
U16 llfBETagSetting(U8 ubTagType, U8 ubBankNo);

U32 cal_shift(U32 num);
U8 llfCalLog2(U32 input);
void PrintDone();
void llfEndResponce(U32 errCode);
void llfInitErrorMessage();
void llfAddErrorMessage(U8 bank, U16 badBlockNum, U16 errorType);
void llfPrintNandPara();
void llfEndResponceNonPrint(U32 errCode);
U8 llfIsBlockBad(U32 ulTableStartAddr, U8 bank, U16 block);
#ifdef EXTEND_STATIC_DBT
void llfCheckRDTPlaneAndDBTSize(U32 resultAddr);
U8 llfIsSpBlockBadInStaticDBT(U32 ulTableStartAddr, U16 block);
void llfCheckBankRdtResultTag();
#endif
U8 llfIsMpBlockBad(U32 ulTableStartAddr, U8 bank, U16 mpblock);
U8 llfIsSystemBlkDefect(U32 ulTableStartAddr, U8 bank, U16 block);
U8 llfIsBlockBadInRDTDBT(U32 ulTableStartAddr, U16 block);
U32 llfIsDefectBlk(U32 addr, U8 if_type);
U32 llfSearchDBT(U8 bank);
U32 llfSetDBT(U8 bank, U16 blk_no, U32 dbt_addr);
void llfUnSetDBT(U8 bank, U16 Group_no, U32 dbt_addr);
U32 llfSetSpDBT(U8 ubBank, U16 uwSpBlk, U32 ulSpDbtAddr);
void llfUnMarkSystemBlkDefect(U32 ulTableStartAddr, U8 bank, U16 block);
void llfMarkSystemBlkDefect(U32 ulTableStartAddr, U8 bank, U16 block);
void llfMarkUserMPBlkDefect(U32 ulTableStartAddr, U8 bank, U16 group);
void llfUnMarkUserMPBlkDefect(U32 ulTableStartAddr, U8 bank, U16 group);
void CalculateSnapshotArea();
#ifdef HANDLE_BLK_INFO_BEYOND_64K
U32 llfParseBlockInfoToDBT(U32 blockInfo_addr, U16 deltaMpBlockNum, U16 startBlk);
#else
U32 llfParseBlockInfoToDBT(U32 blockInfo_addr);
#endif
BOOL llfBSCheckSinglePlaneBadBlock(U32 blockInfo_addr, U16 BSIndex, U16 BankNo);

void llfFCCmdRead_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page, U32 Data_addr,
                       U32 Data_length, U32 Head_addr, U32 Head_length);
void llfFCCmdHynixRead(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page, U32 Data_addr,
                       U32 Data_length, U32 Head_addr, U32 Head_length);
void llfFCCmdWrite_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page, U32 Data_addr,
                        U32 Data_length, U32 Head_addr, U32 Head_length);
void llfFCCmdMultiWrite_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page,
                             U32 Data_addr, U32 Data_length, U32 Head_addr, U32 Head_length);
void llfFCCmdQuadWrite_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page,
                            U32 Data_addr, U32 Data_length, U32 Head_addr, U32 Head_length);
void llfFCCmdMultiRead_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page, U32 Data_addr,
                            U32 Data_length, U32 Head_addr, U32 Head_length);
void llfFCCmdQuadRead_DRAM(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 block, U16 page, U32 Data_addr,
                           U32 Data_length, U32 Head_addr, U32 Head_length);
void llfFCCmdSlcWrite_Nondata(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 uwBlockNo, U16 uwPageNo);

//void SetLDPCmatrix(U32 matrix_addr);
//void Init_ECC_cfg();
void SetParserSequencerTable(U32 parser_addr, U32 sequencer_addr);
void SetParserSequencerIndex(U32 parser_index_addr, U32 sequencer_index_addr,
                             U32 auto_insert_index_addr);

#ifdef KEEP_ORIG_DBT
U32 llfLoadOriginalDBT(U32 ulAddr);
#endif

#ifdef BLK_REMAP_PRO
U32 LlfInitReMapTable(U32 Address);
void LlfCreatReMappingTable(U8 bank);
U16 LlfGetRealMaxBsNum();
void LlfGetGoodMPBlock(void);
void LlfGetGoodSystemBlock(void);
void LlfUpdateDBT(void);
U32 LlfCheckRemappingTable();
U32 LlfInheritRemappingTable(U32 addr);
void LlfParseBlockRemapTable(U32 addr);
U32 LlfOutPutRemapTable();
U16 LlfBlkRemapProCalc(U16 uwBlock, U8 ubBank);
U32 LlfUpdateRemapBlock();
void LlfGetGoodMPNumInTable();
U16 LlfSearchMpGood(U8 bank_no, U16 per_plane_lenth);
U32 LlfSearchSPNoInRemapTable(U8 bank_no, U16 block_no, U8 plane_no);
#endif

#ifdef REPLACE_IMAGE
void llfReplaceImage();
U32 llfWriteConfigForReplaceImage(U8 ubIfType, U8 ubClkMode);
#endif

void llfSaveInfoToSpi();

#ifdef REPLACE_FW
U32 llfWriteSysGroup(U16 (*pSBlkNo)[SYS_BLK]);
U32 llfSetSblkToDBT(U32 dbt_addr, U16 (*pSBlkNo)[SYS_BLK]);
U32 llfSetSBlockFEConfig(U32 sblk_addr);
U32 llfWriteSBlk(U16 (*pSBlkNo)[SYS_BLK]);
#endif
#if (defined(RL6577_VA) || defined(RTS5771_VA)) && defined(KEEP_RDT_RESULT)
U32 llfSetExtendRdtBlkToDBT(U32 dbt_addr);
#endif


#ifdef AVG_ERASE_COUNT_TEST
U32 llfSearchInheritBSEraseCount();
U32 llfParseBlockInfoToEC(U32 blockinfo_addr, U16 MpBlockNumPerLun);
BOOL CheckInheritEC();
#endif

void llfSetFcPadTxOCD(U8 ubOcd);
void llfSetFcPadRxODT(U8 ubDqsOdt, U8 ubDqOdt);
void llfLoadDefaultTiming();
void llfLoadTimingFromConfig();
void llfAPBECalibrateTxAuto(U8 ubIFType, U8 ubClkMode);
void llfAPBECalibrateRxAuto(U8 ubIFType, U8 ubClkMode);
U32 llfCalibrateTxDelayChain(U8 ubBankNo, U8 ubFcMode, U8 ubClockMode, U16 uwSpeed);
#ifdef RL6643_VA
void Vref_10M_calibrate();
#endif


U32 coarse_rx_delay_calibration(U32 iface, U32 ulMode, U32 bank, U32 thin_rx_delay, U32 fifo_delay);
U32 thin_rx_delay_calibration(U32 iface, U32 ulMode, U32 bank, U32 coarse_rx_delay_min,
                              U32 coarse_range_max,
                              U32 fifo_delay);
U32 fifo_rx_calibration(U32 iface, U32 ulMode, U32 bank, U32 coarse_rx_delay_min,
                        U32 thin_rx_delay);
U32 writeDQ_tx_delay_calibration(U32 iface, U32 ulMode, U32 bank);
void thin_rx_delay_setting();
void llfSetTxDqDelay(U8 ubChNo, U8 ubIndex);

#if defined (FTL_H3DQV5) && (defined(RL6577_VA)||defined(RTS5771_VA))
U32 llfHynixBuildRRT();
U32 llfJudgeFinalHynixRRT(llfHYNIX_RR_Table_QLC *hynixRRT);
#endif
void llfLoadDefaultFromGPIO();

#ifdef AUTO_DETECT_DIE
U32 llfAutoDetectDie();
#endif
void CreateBankConfigByDieMap(void);

#if (defined(RL6577_VA) && defined(FOR_WEIKE)) || defined(RL6643_VA)
void GPIO_init(U32 gpio_num);
void GPIO_toggle(U32 gpio_num);
void GPIO_SetHigh(U32 gpio_num);
void GPIO_SetLow(U32 gpio_num);
#endif

#ifdef SLC_RRT
U32 llfReadRetry(U8 bank, U16 block, U16 page, U32 *cmp,  U32 ulAddr, U32 ulHeadAddr);
U32 llfCompareReadRetrycnt();
#endif

#ifdef SBLK_EXPAND
BOOL llfIsGoodSblk(U8 ubBank, U16 uwBlock);
#endif

#endif
