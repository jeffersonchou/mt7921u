/******************************************************************************
*                                FILE DESCRIPTOR
 *****************************************************************************/

#ifndef _AP_SELECTION_H
#define _AP_SELECTION_H

/* Support AP Selection */
struct BSS_DESC *scanSearchBssDescByScoreForAis(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
void scanGetCurrentEssChnlList(struct ADAPTER *prAdapter, uint8_t ucBssIndex);
uint8_t scanCheckNeedDriverRoaming(
	struct ADAPTER *prAdapter, uint8_t ucBssIndex);
uint8_t scanBeaconTimeoutFilterPolicyForAis(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
u_int8_t scanApOverload(uint16_t status, uint16_t reason);

#endif
