/******************************************************************************
*                                FILE DESCRIPTOR
 *****************************************************************************/
/*! \file   "hif_usb.h"
 *   \brief
 */


#ifndef _HIF_USB_H
#define _HIF_USB_H
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/
#define HIF_USB_DEBUG  (0)	/* 0:turn off debug msg and assert, 1:turn off debug msg and assert */

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*!
 * \brief A debug print used to print debug messages while compiler flag HIF_SDIO_DEBUG on.
 *
 */
#if HIF_USB_DEBUG
#define DPRINTK(fmt, args...) pr_debug("%s: " fmt, __func__, ## args)
#else
#define DPRINTK(fmt, args...)
#endif

/*!
 * \brief ASSERT function definition.
 *
 */
#if HIF_USB_DEBUG
#define ASSERT(expr) \
	do { \
		if (!(expr)) { \
			pr_err("assertion failed! %s[%d]: %s\n", \
			       __func__, __LINE__, #expr); \
			WARN_ON(!(expr)); \
		} \
	} while (0)
#else
#define ASSERT(expr)    do {} while (0)
#endif

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
#endif /* _HIF_USB_H */
