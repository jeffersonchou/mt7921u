/******************************************************************************
*                                FILE DESCRIPTOR
 *****************************************************************************/
/*
 * Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/nic/cmd_buf.c#1
 */

/*! \file   "cmd_buf.c"
 *    \brief  This file contain the management function of internal Command
 *	    Buffer for CMD_INFO_T.
 *
 *	We'll convert the OID into Command Packet and then send to FW.
 *  Thus we need to copy the OID information to Command Buffer
 *  for following reasons.
 *    1. The data structure of OID information may not equal to the data
 *       structure of Command, we cannot use the OID buffer directly.
 *    2. If the Command was not generated by driver we also need a place
 *       to store the information.
 *    3. Because the CMD is NOT FIFO when doing memory allocation (CMD will be
 *       generated from OID or interrupt handler), thus we'll use the Block
 *       style of Memory Allocation here.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
u_int8_t fgCmdDumpIsDone = FALSE;
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to initial the MGMT memory pool for CMD Packet.
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cmdBufInitialize(IN struct ADAPTER *prAdapter)
{
	struct CMD_INFO *prCmdInfo;
	uint32_t i;

	ASSERT(prAdapter);

	QUEUE_INITIALIZE(&prAdapter->rFreeCmdList);

	for (i = 0; i < CFG_TX_MAX_CMD_PKT_NUM; i++) {
		prCmdInfo = &prAdapter->arHifCmdDesc[i];
		kalMemZero(prCmdInfo, sizeof(struct CMD_INFO));
		QUEUE_INSERT_TAIL(&prAdapter->rFreeCmdList,
				  &prCmdInfo->rQueEntry);
	}

#if (CFG_TX_DYN_CMD_SUPPORT == 1)
	QUEUE_INITIALIZE(&prAdapter->rDynCmdQ);
#endif
}				/* end of cmdBufInitialize() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to uninit the MGMT memory pool for CMD Packet.
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t cmdBufUninit(IN struct ADAPTER *prAdapter)
{
#if (CFG_TX_DYN_CMD_SUPPORT == 1)
	cmdDynBufUninit(prAdapter);
#endif

	return WLAN_STATUS_SUCCESS;
}				/* end of cmdBufUninit() */

#if (CFG_TX_DYN_CMD_SUPPORT == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to alloc dynamic CmdInfo.
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return Allocated CMD_INFO buffer
 */
/*----------------------------------------------------------------------------*/
struct CMD_INFO * cmdDynBufAlloc(IN struct ADAPTER
				  *prAdapter)
{
	struct CMD_INFO *prCmdInfo = NULL;

	KAL_SPIN_LOCK_DECLARATION();

	/* Try to use allocated buf first */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_DYN_CMD);
	QUEUE_REMOVE_HEAD(&prAdapter->rDynCmdQ, prCmdInfo,
				  struct CMD_INFO *);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_DYN_CMD);

	/* Allocat new one if there is nothing available */
	if (!prCmdInfo) {
		prCmdInfo = cnmMemAlloc(prAdapter,
				RAM_TYPE_BUF, sizeof(struct CMD_INFO));
		if (prCmdInfo) {
			GLUE_INC_REF_CNT(prAdapter->u4DynCmdAllocCnt);
			DBGLOG(MEM, ERROR,
			       "Alloc DynCmd @ %p Cnt:%d\n",
			       prCmdInfo,
			       prAdapter->u4DynCmdAllocCnt);
		}
		else
			DBGLOG(MEM, ERROR,
			       "DynCmd alloc fail. Cnt:%d\n",
			       prAdapter->u4DynCmdAllocCnt);
	}

	/* Prepare addtional info */
	if (prCmdInfo) {
		kalMemZero(prCmdInfo, sizeof(struct CMD_INFO));
		prCmdInfo->fgDynCmd = TRUE;
	}

	return prCmdInfo;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to free all dynamic CmdInfo in rDynCmdQ.
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @return status
 */
/*----------------------------------------------------------------------------*/
uint32_t cmdDynBufUninit(IN struct ADAPTER
				  *prAdapter)
{
	uint32_t u4Ret = WLAN_STATUS_SUCCESS;
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	QUEUE_INITIALIZE(prTempCmdQue);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_DYN_CMD);
	QUEUE_MOVE_ALL(prTempCmdQue, &prAdapter->rDynCmdQ);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_DYN_CMD);

	/* DBG : Memleak check */
	if (prAdapter->u4DynCmdAllocCnt != prTempCmdQue->u4NumElem)
		DBGLOG(INIT, ERROR, "Memleak! Alloc:%d Free:%d\n",
			GLUE_GET_REF_CNT(prAdapter->u4DynCmdAllocCnt),
			prAdapter->rDynCmdQ.u4NumElem);

	/* Free all items in queue */
	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);

		cnmMemFree(prAdapter , prCmdInfo);
	}

	return u4Ret;
}
#endif /* CFG_TX_DYN_CMD_SUPPORT */


/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to enqueue the returned CmdInfo.
 *
 * @param prAdapter  Pointer to the Adapter structure.
 *
 * @param prCmdInfo  Pointer to the CMD_INFO structure.
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cmdFreeBufEnQ(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo)
{
	KAL_SPIN_LOCK_DECLARATION();

#if (CFG_TX_DYN_CMD_SUPPORT == 1)
	if (prCmdInfo->fgDynCmd) {
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_DYN_CMD);
		QUEUE_INSERT_TAIL(&prAdapter->rDynCmdQ,
			&prCmdInfo->rQueEntry);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_DYN_CMD);
	} else
#endif
	{
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_RESOURCE);
		QUEUE_INSERT_TAIL(&prAdapter->rFreeCmdList,
				  &prCmdInfo->rQueEntry);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_RESOURCE);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief dump CMD queue and print to trace, for debug use only
 * @param[in] prQueue	Pointer to the command Queue to be dumped
 * @param[in] quename	Name of the queue
 */
/*----------------------------------------------------------------------------*/
void cmdBufDumpCmdQueue(struct QUE *prQueue,
			int8_t *queName)
{
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *)
				     QUEUE_GET_HEAD(prQueue);

	DBGLOG(NIC, INFO, "Dump CMD info for %s, Elem number:%u\n",
	       queName, prQueue->u4NumElem);
	while (prCmdInfo) {
		struct CMD_INFO *prCmdInfo1, *prCmdInfo2, *prCmdInfo3;

		prCmdInfo1 = (struct CMD_INFO *)QUEUE_GET_NEXT_ENTRY((
					struct QUE_ENTRY *)prCmdInfo);
		if (!prCmdInfo1) {
			DBGLOG(NIC, INFO, "CID:%d SEQ:%d\n", prCmdInfo->ucCID,
			       prCmdInfo->ucCmdSeqNum);
			break;
		}
		prCmdInfo2 = (struct CMD_INFO *)QUEUE_GET_NEXT_ENTRY((
					struct QUE_ENTRY *)prCmdInfo1);
		if (!prCmdInfo2) {
			DBGLOG(NIC, INFO, "CID:%d, SEQ:%d; CID:%d, SEQ:%d\n",
			       prCmdInfo->ucCID,
			       prCmdInfo->ucCmdSeqNum, prCmdInfo1->ucCID,
			       prCmdInfo1->ucCmdSeqNum);
			break;
		}
		prCmdInfo3 = (struct CMD_INFO *)QUEUE_GET_NEXT_ENTRY((
					struct QUE_ENTRY *)prCmdInfo2);
		if (!prCmdInfo3) {
			DBGLOG(NIC, INFO,
			       "CID:%d, SEQ:%d; CID:%d, SEQ:%d; CID:%d, SEQ:%d\n",
			       prCmdInfo->ucCID,
			       prCmdInfo->ucCmdSeqNum, prCmdInfo1->ucCID,
			       prCmdInfo1->ucCmdSeqNum,
			       prCmdInfo2->ucCID, prCmdInfo2->ucCmdSeqNum);
			break;
		}
		DBGLOG(NIC, INFO,
		       "CID:%d, SEQ:%d; CID:%d, SEQ:%d; CID:%d, SEQ:%d; CID:%d, SEQ:%d\n",
		       prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum,
		       prCmdInfo1->ucCID, prCmdInfo1->ucCmdSeqNum,
		       prCmdInfo2->ucCID, prCmdInfo2->ucCmdSeqNum,
		       prCmdInfo3->ucCID, prCmdInfo3->ucCmdSeqNum);
		prCmdInfo = (struct CMD_INFO *)QUEUE_GET_NEXT_ENTRY((
					struct QUE_ENTRY *)prCmdInfo3);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Allocate CMD_INFO_T from a free list and MGMT memory pool.
 *
 * @param[in] prAdapter      Pointer to the Adapter structure.
 * @param[in] u4Length       Length of the frame buffer to allocate.
 *
 * @retval NULL      Pointer to the valid CMD Packet handler
 * @retval !NULL     Fail to allocat CMD Packet
 */
/*----------------------------------------------------------------------------*/
#if CFG_DBG_MGT_BUF
struct CMD_INFO *cmdBufAllocateCmdInfoX(IN struct ADAPTER
					   *prAdapter, IN uint32_t u4Length,
					   uint8_t *fileAndLine)
#else
struct CMD_INFO *cmdBufAllocateCmdInfo(IN struct ADAPTER
				       *prAdapter, IN uint32_t u4Length)
#endif
{
	struct CMD_INFO *prCmdInfo;

	KAL_SPIN_LOCK_DECLARATION();

	DEBUGFUNC("cmdBufAllocateCmdInfo");

	ASSERT(prAdapter);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_RESOURCE);
	QUEUE_REMOVE_HEAD(&prAdapter->rFreeCmdList, prCmdInfo,
			  struct CMD_INFO *);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_RESOURCE);

#if (CFG_TX_DYN_CMD_SUPPORT == 1)
	/* If nothing available in FreeCmdList, try to take DynCmd instead */
	if (!prCmdInfo)
		prCmdInfo = cmdDynBufAlloc(prAdapter);
#endif

	if (prCmdInfo) {
		/* Setup initial value in CMD_INFO_T */
		prCmdInfo->u2InfoBufLen = 0;
		prCmdInfo->fgIsOid = FALSE;
		prCmdInfo->fgNeedResp = FALSE;

		if (u4Length) {
			/* Start address of allocated memory */
			u4Length = TFCB_FRAME_PAD_TO_DW(u4Length);

#if CFG_DBG_MGT_BUF
			prCmdInfo->pucInfoBuffer = cnmMemAllocX(prAdapter,
				RAM_TYPE_BUF, u4Length, fileAndLine);
#else
			prCmdInfo->pucInfoBuffer = cnmMemAlloc(prAdapter,
				RAM_TYPE_BUF, u4Length);
#endif

			if (prCmdInfo->pucInfoBuffer == NULL) {
				cmdFreeBufEnQ(prAdapter, prCmdInfo);

				prCmdInfo = NULL;
			} else {
				kalMemZero(prCmdInfo->pucInfoBuffer, u4Length);
			}
		} else {
			prCmdInfo->pucInfoBuffer = NULL;
		}
		fgCmdDumpIsDone = FALSE;
	} else if (!fgCmdDumpIsDone) {
		struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
		struct QUE *prCmdQue = &prGlueInfo->rCmdQueue;
		struct QUE *prPendingCmdQue = &prAdapter->rPendingCmdQueue;
		struct TX_TCQ_STATUS *prTc = &prAdapter->rTxCtrl.rTc;

		fgCmdDumpIsDone = TRUE;
		cmdBufDumpCmdQueue(prCmdQue, "waiting Tx CMD queue");
		cmdBufDumpCmdQueue(prPendingCmdQue,
				   "waiting response CMD queue");
		DBGLOG(NIC, INFO, "Tc4 number:%d\n",
		       prTc->au4FreeBufferCount[TC4_INDEX]);
	}

	if (prCmdInfo) {
		DBGLOG(MEM, LOUD,
		       "CMD[0x%p] allocated! LEN[%04u], Rest[%u]\n",
		       prCmdInfo, u4Length, prAdapter->rFreeCmdList.u4NumElem);

		prAdapter->fgIsCmdAllocFail = FALSE;
	} else {
		DBGLOG(MEM, ERROR,
		       "CMD allocation failed! LEN[%04u], Rest[%u]\n",
		       u4Length, prAdapter->rFreeCmdList.u4NumElem);

		if (!prAdapter->fgIsCmdAllocFail) {
			prAdapter->fgIsCmdAllocFail = TRUE;
			prAdapter->u4CmdAllocStartFailTime = kalGetTimeTick();
		} else
			if (CHECK_FOR_TIMEOUT(kalGetTimeTick(),
					     prAdapter->u4CmdAllocStartFailTime,
					      CFG_CMD_ALLOC_FAIL_TIMEOUT_MS))
				GL_DEFAULT_RESET_TRIGGER(prAdapter,
							 RST_CMD_EVT_FAIL);
	}

	return prCmdInfo;

}				/* end of cmdBufAllocateCmdInfo() */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is used to free the CMD Packet to the MGMT memory pool.
 *
 * @param prAdapter  Pointer to the Adapter structure.
 * @param prCmdInfo  CMD Packet handler
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void cmdBufFreeCmdInfo(IN struct ADAPTER *prAdapter,
		       IN struct CMD_INFO *prCmdInfo)
{
	DEBUGFUNC("cmdBufFreeCmdInfo");

	ASSERT(prAdapter);

	if (prCmdInfo) {
		if (prCmdInfo->pucInfoBuffer) {
			cnmMemFree(prAdapter, prCmdInfo->pucInfoBuffer);
			prCmdInfo->pucInfoBuffer = NULL;
		}

		cmdFreeBufEnQ(prAdapter, prCmdInfo);
	}

	if (prCmdInfo)
		DBGLOG(MEM, LOUD, "CMD[0x%x] SEQ[%d] freed! Rest[%u]\n",
			prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum,
			prAdapter->rFreeCmdList.u4NumElem);

	return;

}				/* end of cmdBufFreeCmdPacket() */
