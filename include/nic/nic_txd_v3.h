/******************************************************************************
*                                FILE DESCRIPTOR
 *****************************************************************************/
/*
 ** Id: //Department/DaVinci/BRANCHES/
 *			MT6620_WIFI_DRIVER_v3_3/include/nic/nic_tx.h#1
 */

/*! \file   nic_tx.h
 *    \brief  Functions that provide TX operation in NIC's point of view.
 *
 *    This file provides TX functions which are responsible for both Hardware
 *    and Software Resource Management and keep their Synchronization.
 *
 */


#ifndef _NIC_TXD_v3_H
#define _NIC_TXD_v3_H

#if (CFG_SUPPORT_CONNAC3X == 1)
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

uint8_t nic_txd_v3_long_format_op(
	void *prTxDesc,
	uint8_t fgSet);
uint8_t nic_txd_v3_tid_op(
	void *prTxDesc,
	uint8_t ucTid,
	uint8_t fgSet);
uint8_t nic_txd_v3_queue_idx_op(
	void *prTxDesc,
	uint8_t ucQueIdx,
	uint8_t fgSet);
#if (CFG_TCP_IP_CHKSUM_OFFLOAD == 1)
void nic_txd_v3_chksum_op(
	void *prTxDesc,
	uint8_t ucChksumFlag);
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD == 1 */
void nic_txd_v3_header_format_op(
	void *prTxDesc,
	struct MSDU_INFO *prMsduInfo);

void nic_txd_v3_fill_by_pkt_option(
	struct MSDU_INFO *prMsduInfo,
	void *prTxD);

void nic_txd_v3_compose(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	u_int32_t u4TxDescLength,
	u_int8_t fgIsTemplate,
	u_int8_t *prTxDescBuffer);
void nic_txd_v3_compose_security_frame(
	struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo,
	uint8_t *prTxDescBuffer,
	uint8_t *pucTxDescLength);
void nic_txd_v3_set_pkt_fixed_rate_option_full(
	struct MSDU_INFO *prMsduInfo,
	uint16_t u2RateCode,
	uint8_t ucBandwidth,
	u_int8_t fgShortGI,
	u_int8_t fgLDPC,
	u_int8_t fgDynamicBwRts,
	u_int8_t fgBeamforming,
	uint8_t ucAntennaIndex);
void nic_txd_v3_set_pkt_fixed_rate_option(
	struct MSDU_INFO *prMsduInfo,
	uint16_t u2RateCode,
	uint8_t ucBandwidth,
	u_int8_t fgShortGI,
	u_int8_t fgDynamicBwRts);
void nic_txd_v3_set_hw_amsdu_template(
	struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucTid,
	u_int8_t fgSet);
#endif /* CFG_SUPPORT_CONNAC3X == 1 */
#endif /* _NIC_TXD_v3_H */
