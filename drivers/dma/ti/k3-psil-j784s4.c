// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2021 Texas Instruments Incorporated - https://www.ti.com
 */

#include <linux/kernel.h>

#include "k3-psil-priv.h"

#define PSIL_PDMA_XY_TR(x)				\
	{						\
		.thread_id = x,				\
		.ep_config = {				\
			.ep_type = PSIL_EP_PDMA_XY,	\
		},					\
	}

#define PSIL_PDMA_XY_PKT(x)				\
	{						\
		.thread_id = x,				\
		.ep_config = {				\
			.ep_type = PSIL_EP_PDMA_XY,	\
			.pkt_mode = 1,			\
		},					\
	}

#define PSIL_PDMA_MCASP(x)				\
	{						\
		.thread_id = x,				\
		.ep_config = {				\
			.ep_type = PSIL_EP_PDMA_XY,	\
			.pdma_acc32 = 1,		\
			.pdma_burst = 1,		\
		},					\
	}

#define PSIL_ETHERNET(x)				\
	{						\
		.thread_id = x,				\
		.ep_config = {				\
			.ep_type = PSIL_EP_NATIVE,	\
			.pkt_mode = 1,			\
			.needs_epib = 1,		\
			.psd_size = 16,			\
		},					\
	}

#define PSIL_SA2UL(x, tx)				\
	{						\
		.thread_id = x,				\
		.ep_config = {				\
			.ep_type = PSIL_EP_NATIVE,	\
			.pkt_mode = 1,			\
			.needs_epib = 1,		\
			.psd_size = 64,			\
			.notdpkt = tx,			\
		},					\
	}

#define PSIL_CSI2RX(x)					\
	{						\
		.thread_id = x,				\
		.ep_config = {				\
			.ep_type = PSIL_EP_NATIVE,	\
		},					\
	}

/* PSI-L source thread IDs, used for RX (DMA_DEV_TO_MEM) */
static struct psil_ep j784s4_src_ep_map[] = {
	/* PDMA_MCASP - McASP0-4 */
	PSIL_PDMA_MCASP(0x4400),
	PSIL_PDMA_MCASP(0x4401),
	PSIL_PDMA_MCASP(0x4402),
	PSIL_PDMA_MCASP(0x4403),
	PSIL_PDMA_MCASP(0x4404),
	/* PDMA_SPI_G0 - SPI0-3 */
	PSIL_PDMA_XY_PKT(0x4600),
	PSIL_PDMA_XY_PKT(0x4601),
	PSIL_PDMA_XY_PKT(0x4602),
	PSIL_PDMA_XY_PKT(0x4603),
	PSIL_PDMA_XY_PKT(0x4604),
	PSIL_PDMA_XY_PKT(0x4605),
	PSIL_PDMA_XY_PKT(0x4606),
	PSIL_PDMA_XY_PKT(0x4607),
	PSIL_PDMA_XY_PKT(0x4608),
	PSIL_PDMA_XY_PKT(0x4609),
	PSIL_PDMA_XY_PKT(0x460a),
	PSIL_PDMA_XY_PKT(0x460b),
	PSIL_PDMA_XY_PKT(0x460c),
	PSIL_PDMA_XY_PKT(0x460d),
	PSIL_PDMA_XY_PKT(0x460e),
	PSIL_PDMA_XY_PKT(0x460f),
	/* PDMA_SPI_G1 - SPI4-7 */
	PSIL_PDMA_XY_PKT(0x4610),
	PSIL_PDMA_XY_PKT(0x4611),
	PSIL_PDMA_XY_PKT(0x4612),
	PSIL_PDMA_XY_PKT(0x4613),
	PSIL_PDMA_XY_PKT(0x4614),
	PSIL_PDMA_XY_PKT(0x4615),
	PSIL_PDMA_XY_PKT(0x4616),
	PSIL_PDMA_XY_PKT(0x4617),
	PSIL_PDMA_XY_PKT(0x4618),
	PSIL_PDMA_XY_PKT(0x4619),
	PSIL_PDMA_XY_PKT(0x461a),
	PSIL_PDMA_XY_PKT(0x461b),
	PSIL_PDMA_XY_PKT(0x461c),
	PSIL_PDMA_XY_PKT(0x461d),
	PSIL_PDMA_XY_PKT(0x461e),
	PSIL_PDMA_XY_PKT(0x461f),
	/* MAIN_CPSW2G */
	PSIL_ETHERNET(0x4640),
	/* PDMA_USART_G0 - UART0-1 */
	PSIL_PDMA_XY_PKT(0x4700),
	PSIL_PDMA_XY_PKT(0x4701),
	/* PDMA_USART_G1 - UART2-3 */
	PSIL_PDMA_XY_PKT(0x4702),
	PSIL_PDMA_XY_PKT(0x4703),
	/* PDMA_USART_G2 - UART4-9 */
	PSIL_PDMA_XY_PKT(0x4704),
	PSIL_PDMA_XY_PKT(0x4705),
	PSIL_PDMA_XY_PKT(0x4706),
	PSIL_PDMA_XY_PKT(0x4707),
	PSIL_PDMA_XY_PKT(0x4708),
	PSIL_PDMA_XY_PKT(0x4709),
	/* CSI2RX */
	PSIL_CSI2RX(0x4900),
	PSIL_CSI2RX(0x4901),
	PSIL_CSI2RX(0x4902),
	PSIL_CSI2RX(0x4903),
	PSIL_CSI2RX(0x4940),
	PSIL_CSI2RX(0x4941),
	PSIL_CSI2RX(0x4942),
	PSIL_CSI2RX(0x4943),
	PSIL_CSI2RX(0x4944),
	PSIL_CSI2RX(0x4945),
	PSIL_CSI2RX(0x4946),
	PSIL_CSI2RX(0x4947),
	PSIL_CSI2RX(0x4948),
	PSIL_CSI2RX(0x4949),
	PSIL_CSI2RX(0x494a),
	PSIL_CSI2RX(0x494b),
	PSIL_CSI2RX(0x494c),
	PSIL_CSI2RX(0x494d),
	PSIL_CSI2RX(0x494e),
	PSIL_CSI2RX(0x494f),
	PSIL_CSI2RX(0x4950),
	PSIL_CSI2RX(0x4951),
	PSIL_CSI2RX(0x4952),
	PSIL_CSI2RX(0x4953),
	PSIL_CSI2RX(0x4954),
	PSIL_CSI2RX(0x4955),
	PSIL_CSI2RX(0x4956),
	PSIL_CSI2RX(0x4957),
	PSIL_CSI2RX(0x4958),
	PSIL_CSI2RX(0x4959),
	PSIL_CSI2RX(0x495a),
	PSIL_CSI2RX(0x495b),
	PSIL_CSI2RX(0x495c),
	PSIL_CSI2RX(0x495d),
	PSIL_CSI2RX(0x495e),
	PSIL_CSI2RX(0x495f),
	PSIL_CSI2RX(0x4960),
	PSIL_CSI2RX(0x4961),
	PSIL_CSI2RX(0x4962),
	PSIL_CSI2RX(0x4963),
	PSIL_CSI2RX(0x4964),
	PSIL_CSI2RX(0x4965),
	PSIL_CSI2RX(0x4966),
	PSIL_CSI2RX(0x4967),
	PSIL_CSI2RX(0x4968),
	PSIL_CSI2RX(0x4969),
	PSIL_CSI2RX(0x496a),
	PSIL_CSI2RX(0x496b),
	PSIL_CSI2RX(0x496c),
	PSIL_CSI2RX(0x496d),
	PSIL_CSI2RX(0x496e),
	PSIL_CSI2RX(0x496f),
	PSIL_CSI2RX(0x4970),
	PSIL_CSI2RX(0x4971),
	PSIL_CSI2RX(0x4972),
	PSIL_CSI2RX(0x4973),
	PSIL_CSI2RX(0x4974),
	PSIL_CSI2RX(0x4975),
	PSIL_CSI2RX(0x4976),
	PSIL_CSI2RX(0x4977),
	PSIL_CSI2RX(0x4978),
	PSIL_CSI2RX(0x4979),
	PSIL_CSI2RX(0x497a),
	PSIL_CSI2RX(0x497b),
	PSIL_CSI2RX(0x497c),
	PSIL_CSI2RX(0x497d),
	PSIL_CSI2RX(0x497e),
	PSIL_CSI2RX(0x497f),
	/* MAIN_CPSW9G */
	PSIL_ETHERNET(0x4a00),
	/* MCU_CPSW0 */
	PSIL_ETHERNET(0x7000),
	/* MCU_PDMA0 (MCU_PDMA_MISC_G0) - SPI0 */
	PSIL_PDMA_XY_PKT(0x7100),
	PSIL_PDMA_XY_PKT(0x7101),
	PSIL_PDMA_XY_PKT(0x7102),
	PSIL_PDMA_XY_PKT(0x7103),
	/* MCU_PDMA1 (MCU_PDMA_MISC_G1) - SPI1-2 */
	PSIL_PDMA_XY_PKT(0x7200),
	PSIL_PDMA_XY_PKT(0x7201),
	PSIL_PDMA_XY_PKT(0x7202),
	PSIL_PDMA_XY_PKT(0x7203),
	PSIL_PDMA_XY_PKT(0x7204),
	PSIL_PDMA_XY_PKT(0x7205),
	PSIL_PDMA_XY_PKT(0x7206),
	PSIL_PDMA_XY_PKT(0x7207),
	/* MCU_PDMA2 (MCU_PDMA_MISC_G2) - UART0 */
	PSIL_PDMA_XY_PKT(0x7300),
	/* MCU_PDMA_ADC - ADC0-1 */
	PSIL_PDMA_XY_TR(0x7400),
	PSIL_PDMA_XY_TR(0x7401),
	PSIL_PDMA_XY_TR(0x7402),
	PSIL_PDMA_XY_TR(0x7403),
	/* SA2UL */
	PSIL_SA2UL(0x7500, 0),
	PSIL_SA2UL(0x7501, 0),
	PSIL_SA2UL(0x7502, 0),
	PSIL_SA2UL(0x7503, 0),
};

/* PSI-L destination thread IDs, used for TX (DMA_MEM_TO_DEV) */
static struct psil_ep j784s4_dst_ep_map[] = {
	/* MCU_CPSW0 */
	PSIL_ETHERNET(0xf000),
	PSIL_ETHERNET(0xf001),
	PSIL_ETHERNET(0xf002),
	PSIL_ETHERNET(0xf003),
	PSIL_ETHERNET(0xf004),
	PSIL_ETHERNET(0xf005),
	PSIL_ETHERNET(0xf006),
	PSIL_ETHERNET(0xf007),
	/* MAIN_CPSW2G */
	PSIL_ETHERNET(0xc640),
	PSIL_ETHERNET(0xc641),
	PSIL_ETHERNET(0xc642),
	PSIL_ETHERNET(0xc643),
	PSIL_ETHERNET(0xc644),
	PSIL_ETHERNET(0xc645),
	PSIL_ETHERNET(0xc646),
	PSIL_ETHERNET(0xc647),
	/* MAIN_CPSW9G */
	PSIL_ETHERNET(0xca00),
	PSIL_ETHERNET(0xca01),
	PSIL_ETHERNET(0xca02),
	PSIL_ETHERNET(0xca03),
	PSIL_ETHERNET(0xca04),
	PSIL_ETHERNET(0xca05),
	PSIL_ETHERNET(0xca06),
	PSIL_ETHERNET(0xca07),
	/* SA2UL */
	PSIL_SA2UL(0xf500, 1),
	PSIL_SA2UL(0xf501, 1),
};

struct psil_ep_map j784s4_ep_map = {
	.name = "j784s4",
	.src = j784s4_src_ep_map,
	.src_count = ARRAY_SIZE(j784s4_src_ep_map),
	.dst = j784s4_dst_ep_map,
	.dst_count = ARRAY_SIZE(j784s4_dst_ep_map),
};
