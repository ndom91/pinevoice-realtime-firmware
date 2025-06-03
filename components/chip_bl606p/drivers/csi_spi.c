/*
 *  Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/*******************************************************
 * @file    dw_spi.c
 * @brief   source file for spi csi driver
 * @version V1.0
 * @date    23. Sep 2020
 * ******************************************************/

#include <csi_config.h>
#include <drv/spi.h>
#include <drv/irq.h>
#include <bl606p_spi.h>
#include <bl606p_glb.h>
#include <bl606p_dma.h>
#include <utils_list.h>
#include <drv/tick.h>

#define SPI_CLK_TOP_FREQ    (160*1000*1000)
#define SPI_FIFO_LEN 32

#define SPI_DMA_ID_USE          1       /* DMA id, 0:DMA0, 1:DMA1 */
#define SPI_DMA_CHAN_TX_USE     DMA_CH1
#define SPI_DMA_CHAN_RX_USE     DMA_CH0
#define SPI_DMA_MAX_COUNT       4032
#define SPI_DMA_MAX_LLI_NUM     256

#if (SPI_DMA_ID_USE == 0)
  #define SPI_DMA_IRQn          DMA0_ALL_IRQn
  #define SPI_DMA_BASE          DMA0_BASE
  #define SPI_DMA_CLOCK         GLB_AHB_CLOCK_DMA_0
  #define GLB_PERI_DMA_CN_SEL   GLB_PERI_DMA_CN_SEL_DMA0
#elif (SPI_DMA_ID_USE == 1)
  #define SPI_DMA_IRQn          DMA1_ALL_IRQn
  #define SPI_DMA_BASE          DMA1_BASE
  #define SPI_DMA_CLOCK         GLB_AHB_CLOCK_DMA_1
  #define GLB_PERI_DMA_CN_SEL   GLB_PERI_DMA_CN_SEL_DMA1
#endif

static DMA_LLI_Ctrl_Type spi_tx_dma_lli[SPI_DMA_MAX_LLI_NUM] __attribute__((aligned(32)));
static DMA_LLI_Ctrl_Type spi_rx_dma_lli[SPI_DMA_MAX_LLI_NUM] __attribute__((aligned(32)));

/* dma init config */
static DMA_LLI_Cfg_Type spi_tx_dma_lli_cfg = {
    .dir = DMA_TRNS_M2P,
    .srcPeriph = DMA_REQ_NONE,
    .dstPeriph = DMA_REQ_SPI0_TX,
};

static DMA_LLI_Cfg_Type spi_rx_dma_lli_cfg = {
    .dir = DMA_TRNS_P2M,
    .srcPeriph = DMA_REQ_SPI0_RX,
    .dstPeriph = DMA_REQ_NONE,
};

/**
  \brief       Spi interrupt handling function
  \param[in]   arg    Callback function member variables
  \return      None
*/
void spi_irq_handler(void *arg)
{
    csi_dev_t *pdev = (csi_dev_t *)arg;
    csi_spi_t *spi;
    uint32_t tmp_val;

    spi = (csi_spi_t*)utils_container_of(pdev, csi_spi_t, dev);

    if (spi->callback == NULL) {
        SPI_ClrIntStatus(spi->dev.idx, SPI_INT_ALL);
        return;
    }

    /* Slave mode tx underrun error interrupt,trigged when tx is not ready during transfer */
    tmp_val = BL_RD_REG(spi->dev.reg_base, SPI_INT_STS);
    if (BL_IS_REG_BIT_SET(tmp_val, SPI_TXU_INT) && !BL_IS_REG_BIT_SET(tmp_val, SPI_CR_SPI_TXU_MASK)) {
        BL_WR_REG(spi->dev.reg_base, SPI_INT_STS, BL_SET_REG_BIT(tmp_val, SPI_CR_SPI_TXU_CLR));
        if(spi->callback){
            spi->callback(spi, SPI_EVENT_ERROR_UNDERFLOW, spi->arg);
        }
    }

    /* TX/RX fifo overflow/underflow interrupt */
    tmp_val = BL_RD_REG(spi->dev.reg_base, SPI_INT_STS);
    if (BL_IS_REG_BIT_SET(tmp_val, SPI_FER_INT) && !BL_IS_REG_BIT_SET(tmp_val, SPI_CR_SPI_FER_MASK)) {
        if(SPI_GetFifoStatus(spi->dev.idx, SPI_FIFO_TX_UNDERFLOW)){
            if(spi->callback){
                spi->callback(spi, SPI_EVENT_ERROR_UNDERFLOW, spi->arg);
            }
        }

        if(SPI_GetFifoStatus(spi->dev.idx, SPI_FIFO_RX_OVERFLOW)){
            if(spi->callback){
                spi->callback(spi, SPI_EVENT_ERROR_OVERFLOW, spi->arg);
            }
        }
    }

    return;
}

void spi_dma_irq_handler(void *arg)
{
    csi_dev_t *pdev = (csi_dev_t *)arg;
    csi_spi_t *spi;
    uint32_t tmp_val;
    uint32_t int_clr;

    spi = (csi_spi_t*)utils_container_of(pdev, csi_spi_t, dev);

    /* spi rx dma int */
    tmp_val = BL_RD_REG(SPI_DMA_BASE, DMA_INTTCSTATUS);
    if ((BL_GET_REG_BITS_VAL(tmp_val, DMA_INTTCSTATUS) & (1 << SPI_DMA_CHAN_RX_USE))) {
        /* Clear interrupt */
        tmp_val = BL_RD_REG(SPI_DMA_BASE, DMA_INTTCCLEAR);
        int_clr = BL_GET_REG_BITS_VAL(tmp_val, DMA_INTTCCLEAR);
        int_clr |= (1 << SPI_DMA_CHAN_RX_USE);
        tmp_val = BL_SET_REG_BITS_VAL(tmp_val, DMA_INTTCCLEAR, int_clr);
        BL_WR_REG(SPI_DMA_BASE, DMA_INTTCCLEAR, tmp_val);

        DMA_Channel_Disable(SPI_DMA_ID_USE, SPI_DMA_CHAN_TX_USE);
        DMA_Channel_Disable(SPI_DMA_ID_USE, SPI_DMA_CHAN_RX_USE);

        tmp_val = BL_RD_REG(spi->dev.reg_base, SPI_FIFO_CONFIG_0);
        tmp_val = BL_CLR_REG_BIT(tmp_val, SPI_DMA_TX_EN);
        tmp_val = BL_CLR_REG_BIT(tmp_val, SPI_DMA_RX_EN);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_TX_FIFO_CLR);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_RX_FIFO_CLR);
        BL_WR_REG(spi->dev.reg_base, SPI_FIFO_CONFIG_0, tmp_val);

        if(spi->callback){
            spi->callback(spi, (uint32_t)(uintptr_t)(spi->priv), spi->arg);
            spi->priv = NULL;
        }
    }

#if 0
    /* spi tx dma int, will not be used */
    tmp_val = BL_RD_REG(SPI_DMA_BASE, DMA_INTTCSTATUS);
    if ((BL_GET_REG_BITS_VAL(tmp_val, DMA_INTTCSTATUS) & (1 << SPI_DMA_CHAN_TX_USE))) {
        /* Clear interrupt */
        tmp_val = BL_RD_REG(SPI_DMA_BASE, DMA_INTTCCLEAR);
        int_clr = BL_GET_REG_BITS_VAL(tmp_val, DMA_INTTCCLEAR);
        int_clr |= (1 << SPI_DMA_CHAN_TX_USE);
        tmp_val = BL_SET_REG_BITS_VAL(tmp_val, DMA_INTTCCLEAR, int_clr);
        BL_WR_REG(SPI_DMA_BASE, DMA_INTTCCLEAR, tmp_val);

        /* Waiting for sending to end */
        while (SPI_GetBusyStatus(spi->dev.idx) == SET || SPI_GetTxFifoCount(spi->dev.idx) < SPI_FIFO_LEN) {
            __NOP();
            __NOP();
        }

        DMA_Channel_Disable(SPI_DMA_ID_USE, SPI_DMA_CHAN_TX_USE);
        DMA_Channel_Disable(SPI_DMA_ID_USE, SPI_DMA_CHAN_RX_USE);

        tmp_val = BL_RD_REG(spi->dev.reg_base, SPI_FIFO_CONFIG_0);
        tmp_val = BL_CLR_REG_BIT(tmp_val, SPI_DMA_TX_EN);
        tmp_val = BL_CLR_REG_BIT(tmp_val, SPI_DMA_RX_EN);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_TX_FIFO_CLR);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_RX_FIFO_CLR);
        BL_WR_REG(spi->dev.reg_base, SPI_FIFO_CONFIG_0, tmp_val);

        if(spi->callback){
            spi->callback(spi, (uint32_t)(uintptr_t)(spi->priv), spi->arg);
            spi->priv = NULL;
        }
    }
#endif
}

/**
  \brief       Initialize SPI Interface
               Initialize the resources needed for the SPI instance
  \param[in]   spi    SPI handle
  \param[in]   idx    SPI instance index
  \return      Error code
*/
csi_error_t csi_spi_init(csi_spi_t *spi, uint32_t idx)
{
    CSI_PARAM_CHK(spi, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    SPI_FifoCfg_Type spi_fifo_cfg = {
        .txFifoThreshold = 0,
        .rxFifoThreshold = 0,
        .txFifoDmaEnable = DISABLE,
        .rxFifoDmaEnable = DISABLE,
    };

    if(idx > 0){
        return CSI_ERROR;
    }

    spi->dev.idx = idx;
    spi->dev.reg_base = SPI0_BASE;
    spi->dev.irq_num = SPI0_IRQn;

    GLB_PER_Clock_UnGate(SPI_DMA_CLOCK);
    GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_SPI);
    GLB_Set_SPI_CLK(ENABLE, GLB_SPI_CLK_MCU_MUXPLL_160M, 0);
    GLB_Set_MCU_SPI_0_ACT_MOD_Sel(GLB_SPI_PAD_ACT_AS_MASTER);
    SPI_FifoConfig(spi->dev.idx, &spi_fifo_cfg);
    SPI_RxIgnoreDisable(spi->dev.idx);

    GLB_Set_Peripheral_DMA_CN(GLB_PERI_DMA_SPI_TX, GLB_PERI_DMA_CN_SEL);
    GLB_Set_Peripheral_DMA_CN(GLB_PERI_DMA_SPI_RX, GLB_PERI_DMA_CN_SEL);

    return ret;
}

/**
  \brief       De-initialize SPI Interface
               stops Operation and releases the software resources used by the spi instance
  \param[in]   spi    Handle
  \return      None
*/
void csi_spi_uninit(csi_spi_t *spi)
{
    CSI_PARAM_CHK_NORETVAL(spi);

    SPI_DeInit(spi->dev.idx);
    GLB_PER_Clock_Gate(GLB_AHB_CLOCK_SPI);
}

/**
  \brief       Attach the callback handler to SPI
  \param[in]   spi    Operate handle
  \param[in]   callback    Callback function
  \param[in]   arg         User can define it by himself as callback's param
  \return      Error code
*/
csi_error_t csi_spi_attach_callback(csi_spi_t *spi, void *callback, void *arg)
{
    CSI_PARAM_CHK(spi, CSI_ERROR);

    if(spi->dev.idx > 0){
        return CSI_UNSUPPORTED;
    }

    spi->callback = callback;
    spi->arg = arg;

    csi_irq_attach(spi->dev.irq_num, &spi_irq_handler, &spi->dev);
    csi_irq_enable(spi->dev.irq_num);

    SPI_IntMask(spi->dev.idx, SPI_INT_ALL, MASK);
    SPI_IntMask(spi->dev.idx, SPI_INT_FIFO_ERROR, UNMASK);
    SPI_IntMask(spi->dev.idx, SPI_INT_SLAVE_UNDERRUN, UNMASK);

    return CSI_OK;
}

/**
  \brief       Detach the callback handler
  \param[in]   spi    Operate handle
  \return      None
*/
void csi_spi_detach_callback(csi_spi_t *spi)
{
    csi_irq_detach(spi->dev.irq_num);
    csi_irq_disable(spi->dev.irq_num);

    SPI_IntMask(spi->dev.idx, SPI_INT_ALL, MASK);
}

/**
  \brief       Config spi mode (master or slave)
  \param[in]   spi     SPI handle
  \param[in]   mode    The mode of spi (master or slave)
  \return      Error code
*/
csi_error_t csi_spi_mode(csi_spi_t *spi, csi_spi_mode_t mode)
{
    uint32_t tmp_val;
    tmp_val = BL_RD_REG(spi->dev.reg_base, SPI_CONFIG);

    if(mode == SPI_MASTER){
        GLB_Set_MCU_SPI_0_ACT_MOD_Sel(GLB_SPI_PAD_ACT_AS_MASTER);
        tmp_val = BL_CLR_REG_BIT(tmp_val, SPI_CR_SPI_S_EN);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_CR_SPI_M_EN);
    } else if(mode == SPI_SLAVE){
        GLB_Set_MCU_SPI_0_ACT_MOD_Sel(GLB_SPI_PAD_ACT_AS_SLAVE);
        tmp_val = BL_CLR_REG_BIT(tmp_val, SPI_CR_SPI_M_EN);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_CR_SPI_S_EN);
    }
    BL_WR_REG(spi->dev.reg_base, SPI_CONFIG, tmp_val);

    return CSI_OK;
}

/**
  \brief       Config spi cp format
  \param[in]   spi       SPI handle
  \param[in]   format    SPI cp format
  \return      Error code
*/
csi_error_t csi_spi_cp_format(csi_spi_t *spi, csi_spi_cp_format_t format)
{
    uint32_t tmp_val;
    tmp_val = BL_RD_REG(spi->dev.reg_base, SPI_CONFIG);

    if(format == SPI_FORMAT_CPOL0_CPHA0 || format == SPI_FORMAT_CPOL0_CPHA1){
        tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_SCLK_POL, 0);
    }else{
        tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_SCLK_POL, 1);
    }

    if(format == SPI_FORMAT_CPOL0_CPHA0 || format == SPI_FORMAT_CPOL1_CPHA0){
        tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_SCLK_PH, 1);
    }else{
        tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_SCLK_PH, 0);
    }

    BL_WR_REG(spi->dev.reg_base, SPI_CONFIG, tmp_val);

    return CSI_OK;
}

/**
  \brief       Config spi frame len
  \param[in]   spi       SPI handle
  \param[in]   length    SPI frame len
  \return      Error code
*/
csi_error_t csi_spi_frame_len(csi_spi_t *spi, csi_spi_frame_len_t length)
{
    uint32_t tmp_val;
    SPI_FrameSize_Type frame_size;
    SPI_BYTE_INVERSE_Type byte_sequence;

    switch (length)
    {
    case SPI_FRAME_LEN_8:
        frame_size = SPI_FRAME_SIZE_8;
        byte_sequence = SPI_BYTE_INVERSE_BYTE0_FIRST;
        break;

    case SPI_FRAME_LEN_16:
        frame_size = SPI_FRAME_SIZE_16;
        byte_sequence = SPI_BYTE_INVERSE_BYTE3_FIRST;
        break;

    default:
        return CSI_UNSUPPORTED;
    }

    tmp_val = BL_RD_REG(spi->dev.reg_base, SPI_FIFO_CONFIG_0);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_TX_FIFO_CLR);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_RX_FIFO_CLR);
    BL_WR_REG(spi->dev.reg_base, SPI_FIFO_CONFIG_0, tmp_val);

    tmp_val = BL_RD_REG(spi->dev.reg_base, SPI_CONFIG);
    tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_FRAME_SIZE, frame_size);
    tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_BYTE_INV, byte_sequence);
    tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_BIT_INV, SPI_BIT_INVERSE_MSB_FIRST);
    tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_M_CONT_EN, ENABLE);
    BL_WR_REG(spi->dev.reg_base, SPI_CONFIG, tmp_val);

    return CSI_OK;
}

/**
  \brief       Config spi work frequence
  \param[in]   spi     SPI handle
  \param[in]   baud    SPI work baud
  \return      the actual config frequency
*/
uint32_t csi_spi_baud(csi_spi_t *spi, uint32_t baud)
{
    uint32_t tmp_val;
    uint32_t spi_div;
    uint32_t spi_freq;

    spi_div = SPI_CLK_TOP_FREQ / baud / 2;

    spi_div = (spi_div > 0) ? (spi_div - 1) : 0;
    spi_div = (spi_div > 0xFF) ? (0xFF) : spi_div;

    spi_freq = SPI_CLK_TOP_FREQ / (spi_div + 1) / 2;

    /* Configure length of data phase1/0 and start/stop condition */
    tmp_val = BL_RD_REG(spi->dev.reg_base, SPI_PRD_0);
    tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_PRD_S, spi_div);
    tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_PRD_P, spi_div);
    tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_PRD_D_PH_0, spi_div);
    tmp_val = BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_PRD_D_PH_1, spi_div);
    BL_WR_REG(spi->dev.reg_base, SPI_PRD_0, tmp_val);

    tmp_val = BL_RD_REG(spi->dev.reg_base, SPI_PRD_1);
    BL_WR_REG(spi->dev.reg_base, SPI_PRD_1, BL_SET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_PRD_I, spi_div));

    return spi_freq;
}

static csi_error_t spi_dma_lli_init(csi_spi_t *spi, const void *data_out, void *data_in, uint32_t size)
{
    uint32_t tmp_val;
    uint32_t data_offset, data_cnt;
    uint16_t lli_cnt;
    uint8_t frame_size, dma_dw;
    /* Invalid data used by DMA when read-only or write only */
    static uint32_t __attribute__((aligned(32))) spi_dma_tx_invalid_data = 0xFFFFFFFF;
    static uint32_t __attribute__((aligned(32))) spi_dma_rx_invalid_data;

    CSI_PARAM_CHK(spi, CSI_ERROR);

    if(data_out == NULL && data_in == NULL){
        return CSI_ERROR;
    }

    /* Get fifo valid width */
    tmp_val = BL_RD_REG(spi->dev.reg_base, SPI_CONFIG);
    frame_size = BL_GET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_FRAME_SIZE) + 1;
    if (frame_size == 3) {
        frame_size = 4;
    }

    /* Too much data */
    if(size > SPI_DMA_MAX_COUNT * SPI_DMA_MAX_LLI_NUM *frame_size){
        return CSI_ERROR;
    }

    switch (frame_size)
    {
    case 1:
        dma_dw = DMA_TRNS_WIDTH_8BITS;
        break;
    case 2:
        dma_dw = DMA_TRNS_WIDTH_16BITS;
        break;
    case 3:
    case 4:
        dma_dw = DMA_TRNS_WIDTH_32BITS;
        break;
    default:
        break;
    }

    /* tx dma lli init */
    spi_tx_dma_lli[0].dmaCtrl.SBSize = DMA_BURST_SIZE_1;
    spi_tx_dma_lli[0].dmaCtrl.dst_min_mode = DISABLE;
    spi_tx_dma_lli[0].dmaCtrl.DBSize = DMA_BURST_SIZE_1;
    spi_tx_dma_lli[0].dmaCtrl.dst_add_mode = DISABLE;
    spi_tx_dma_lli[0].dmaCtrl.SWidth = dma_dw;
    spi_tx_dma_lli[0].dmaCtrl.DWidth = dma_dw;
    spi_tx_dma_lli[0].dmaCtrl.fix_cnt = 0;
    if(data_out){
        spi_tx_dma_lli[0].dmaCtrl.SI = DMA_MINC_ENABLE;
    }else{
        spi_tx_dma_lli[0].dmaCtrl.SI = DMA_MINC_DISABLE;
    }
    spi_tx_dma_lli[0].dmaCtrl.DI = DMA_MINC_DISABLE;
    spi_tx_dma_lli[0].dmaCtrl.I = 0;
    spi_tx_dma_lli[0].destDmaAddr = spi->dev.reg_base + SPI_FIFO_WDATA_OFFSET;
    spi_tx_dma_lli[0].srcDmaAddr = (uint32_t)(uintptr_t)(&spi_dma_tx_invalid_data);
    spi_tx_dma_lli[0].dmaCtrl.TransferSize = SPI_DMA_MAX_COUNT;
    spi_tx_dma_lli[0].nextLLI = (uint32_t)(uintptr_t)(NULL);

    /* rx dma lli init */
    spi_rx_dma_lli[0].dmaCtrl.SBSize = DMA_BURST_SIZE_1;
    spi_rx_dma_lli[0].dmaCtrl.dst_min_mode = DISABLE;
    spi_rx_dma_lli[0].dmaCtrl.DBSize = DMA_BURST_SIZE_1;
    spi_rx_dma_lli[0].dmaCtrl.dst_add_mode = DISABLE;
    spi_rx_dma_lli[0].dmaCtrl.SWidth = dma_dw;
    spi_rx_dma_lli[0].dmaCtrl.DWidth = dma_dw;
    spi_rx_dma_lli[0].dmaCtrl.fix_cnt = 0;
    spi_rx_dma_lli[0].dmaCtrl.SI = DMA_MINC_DISABLE;
    if(data_in){
        spi_rx_dma_lli[0].dmaCtrl.DI = DMA_MINC_ENABLE;
    }else{
        spi_rx_dma_lli[0].dmaCtrl.DI = DMA_MINC_DISABLE;
    }
    spi_rx_dma_lli[0].dmaCtrl.I = 0;
    spi_rx_dma_lli[0].destDmaAddr = (uint32_t)(uintptr_t)(&spi_dma_rx_invalid_data);
    spi_rx_dma_lli[0].srcDmaAddr = spi->dev.reg_base + SPI_FIFO_RDATA_OFFSET;
    spi_rx_dma_lli[0].dmaCtrl.TransferSize = SPI_DMA_MAX_COUNT;
    spi_rx_dma_lli[0].nextLLI = (uint32_t)(uintptr_t)(NULL);

    for(lli_cnt = 0, data_cnt = (size / frame_size); ; lli_cnt++){
        data_offset = frame_size * SPI_DMA_MAX_COUNT * lli_cnt;

        spi_tx_dma_lli[lli_cnt] = spi_tx_dma_lli[0];
        if(data_out){
            spi_tx_dma_lli[lli_cnt].srcDmaAddr = (uint32_t)(uintptr_t)(data_out) + data_offset;
        }

        spi_rx_dma_lli[lli_cnt] = spi_rx_dma_lli[0];
        if(data_in){
            spi_rx_dma_lli[lli_cnt].destDmaAddr = (uint32_t)(uintptr_t)(data_in) + data_offset;
        }

        if(data_cnt > SPI_DMA_MAX_COUNT){
            spi_tx_dma_lli[lli_cnt].nextLLI = (uint32_t)(uintptr_t)(&spi_tx_dma_lli[lli_cnt + 1]);
            spi_tx_dma_lli[lli_cnt].dmaCtrl.I = 0;

            spi_rx_dma_lli[lli_cnt].nextLLI = (uint32_t)(uintptr_t)(&spi_rx_dma_lli[lli_cnt + 1]);
            spi_rx_dma_lli[lli_cnt].dmaCtrl.I = 0;
            data_cnt -= SPI_DMA_MAX_COUNT;
        }else{
            spi_tx_dma_lli[lli_cnt].dmaCtrl.TransferSize = data_cnt;
            spi_tx_dma_lli[lli_cnt].nextLLI = (uint32_t)(uintptr_t)(NULL);
            spi_tx_dma_lli[lli_cnt].dmaCtrl.I = 1;

            spi_rx_dma_lli[lli_cnt].dmaCtrl.TransferSize = data_cnt;
            spi_rx_dma_lli[lli_cnt].nextLLI = (uint32_t)(uintptr_t)(NULL);
            spi_rx_dma_lli[lli_cnt].dmaCtrl.I = 1;

            lli_cnt++;
            break;
        }
    }
    /* clean dma-lli dcache */
    csi_dcache_clean_range((uintptr_t *)spi_tx_dma_lli, sizeof(spi_tx_dma_lli[0]) * lli_cnt);
    csi_dcache_clean_range((uintptr_t *)spi_rx_dma_lli, sizeof(spi_rx_dma_lli[0]) * lli_cnt);
    /* clean date_out dcache */
    if(data_out != NULL){
        csi_dcache_clean_range((uintptr_t *)data_out, size);
    }
    /* invalid date_in dcache */
    if(data_in != NULL){
        csi_dcache_invalid_range((uintptr_t *)data_in, size);
        csi_dcache_clean_range((uintptr_t *)&spi_dma_tx_invalid_data, sizeof(spi_dma_tx_invalid_data));
    }

    DMA_Enable(SPI_DMA_ID_USE);

    DMA_Channel_Disable(SPI_DMA_ID_USE, SPI_DMA_CHAN_TX_USE);
    DMA_LLI_Init(SPI_DMA_ID_USE, SPI_DMA_CHAN_TX_USE, &spi_tx_dma_lli_cfg);
    DMA_IntMask(SPI_DMA_ID_USE, SPI_DMA_CHAN_TX_USE, DMA_INT_ALL, MASK);
    // DMA_IntMask(SPI_DMA_ID_USE, SPI_DMA_CHAN_TX_USE, DMA_INT_TCOMPLETED, UNMASK);
    DMA_LLI_Update(SPI_DMA_ID_USE, SPI_DMA_CHAN_TX_USE, (uint32_t)(uintptr_t)(spi_tx_dma_lli));

    DMA_Channel_Disable(SPI_DMA_ID_USE, SPI_DMA_CHAN_RX_USE);
    DMA_LLI_Init(SPI_DMA_ID_USE, SPI_DMA_CHAN_RX_USE, &spi_rx_dma_lli_cfg);
    DMA_IntMask(SPI_DMA_ID_USE, SPI_DMA_CHAN_RX_USE, DMA_INT_ALL, MASK);
    DMA_IntMask(SPI_DMA_ID_USE, SPI_DMA_CHAN_RX_USE, DMA_INT_TCOMPLETED, UNMASK);
    DMA_LLI_Update(SPI_DMA_ID_USE, SPI_DMA_CHAN_RX_USE, (uint32_t)(uintptr_t)(spi_rx_dma_lli));

    return CSI_OK;
}

/**
  \brief       Sending data to SPI transmitter,(received data is ignored)
               blocking mode ,return unti all data has been sent or err happened
  \param[in]   spi        Handle to operate
  \param[in]   data       Pointer to buffer with data to send to SPI transmitter
  \param[in]   size       Number of data to send(byte)
  \param[in]   timeout    Unit in mini-second
  \return      If send successful, this function shall return the num of data witch is sent successful
               otherwise, the function shall return Error code
*/
int32_t csi_spi_send(csi_spi_t *spi, const void *data, uint32_t size, uint32_t timeout)
{
    uint32_t tmp_val, tx_data;
    uint32_t spi_base = spi->dev.reg_base;
    uint32_t start_time ;
    uint32_t send_cnt = 0;
    uint8_t fifo_cnt, frame_size;

    /* dma status */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    if(BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_TX_EN) || BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_RX_EN)){
        return CSI_BUSY;
    }

    /* Get fifo valid width */
    tmp_val = BL_RD_REG(spi_base, SPI_CONFIG);
    frame_size = BL_GET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_FRAME_SIZE) + 1;
    if (frame_size == 3) {
        frame_size = 4;
    }

    /* size to conut */
    size /= frame_size;

    /* ready to clear rx fifo */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_RX_FIFO_CLR);

    start_time = csi_tick_get_ms();

    for (; size > 0;) {
        /* get tx fifo cnt */
        fifo_cnt = SPI_GetTxFifoCount(spi->dev.idx) / frame_size;

        if (fifo_cnt > 1) {
            fifo_cnt -= 1;
            fifo_cnt = fifo_cnt > size ? size : fifo_cnt;
            size -= fifo_cnt;
        } else {
            if (timeout) {
                if (csi_tick_get_ms() - start_time > timeout) {
                    return CSI_TIMEOUT;
                }
            }
            continue;
        }
        /* clear rx fifo */
        BL_WR_REG(spi_base, SPI_FIFO_CONFIG_0, tmp_val);
        /* write to tx fifo */
        for (; fifo_cnt > 0; fifo_cnt--) {
            switch (frame_size) {
                case 1:
                    tx_data = *(uint8_t *)data;
                    BL_WR_REG(spi_base, SPI_FIFO_WDATA, tx_data);
                    data++;
                    break;
                case 2:
                    tx_data = *(uint16_t *)data;
                    BL_WR_REG(spi_base, SPI_FIFO_WDATA, tx_data);
                    data += 2;
                    break;
                case 3:
                case 4:
                    tx_data = *(uint32_t *)data;
                    BL_WR_REG(spi_base, SPI_FIFO_WDATA, tx_data);
                    data += 4;
                    break;
                default:
                    return INVALID;
                    break;
            }
            send_cnt++;
        }
    }

    /* Waiting for sending to end */
    while (SPI_GetBusyStatus(spi->dev.idx) == SET || SPI_GetTxFifoCount(spi->dev.idx) < SPI_FIFO_LEN) {
        if (timeout) {
            if (csi_tick_get_ms() - start_time > timeout) {
                return CSI_TIMEOUT;
            }
        }
    }

    /* clear rx fifo */
    BL_WR_REG(spi_base, SPI_FIFO_CONFIG_0, tmp_val);

    return send_cnt * frame_size;
}

/**
  \brief       Sending data to SPI transmitter,(received data is ignored)
               non-blocking mode,transfer done event will be signaled by driver
  \param[in]   spi     Handle to operate
  \param[in]   data    Pointer to buffer with data to send to SPI transmitter
  \param[in]   size    Number of data items to send(byte)
  \return      Error code
*/
csi_error_t csi_spi_send_async(csi_spi_t *spi, const void *data, uint32_t size)\
{
    uint32_t tmp_val;
    uint32_t spi_base = spi->dev.reg_base;
    SPI_WORK_MODE_Type spi_mode;

    CSI_PARAM_CHK(spi, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);

    /* dma status */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    if(BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_TX_EN) || BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_RX_EN)){
        return CSI_BUSY;
    }

    spi_mode = BL_IS_REG_BIT_SET(tmp_val, SPI_CR_SPI_S_EN) ? SPI_WORK_MODE_SLAVE : SPI_WORK_MODE_MASTER;

    if(spi_mode == SPI_WORK_MODE_SLAVE){
        /* spi slave : suspend spi */
        tmp_val = BL_RD_REG(spi_base, SPI_CONFIG);
        tmp_val = BL_CLR_REG_BIT(tmp_val, SPI_CR_SPI_S_EN);
        BL_WR_REG(spi_base, SPI_CONFIG, tmp_val);
    }

    /* clear tx/rx fifo, Otherwise, the sending and receiving data are not synchronized */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_TX_FIFO_CLR);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_RX_FIFO_CLR);
    BL_WR_REG(spi_base, SPI_FIFO_CONFIG_0, tmp_val);
    /* fifo dma mode */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_DMA_TX_EN);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_DMA_RX_EN);
    BL_WR_REG(spi_base, SPI_FIFO_CONFIG_0, tmp_val);

    spi->priv = (void *)(uintptr_t)SPI_EVENT_SEND_COMPLETE;

    csi_irq_attach(SPI_DMA_IRQn, &spi_dma_irq_handler, &spi->dev);
    csi_irq_enable(SPI_DMA_IRQn);

    /* dma lli init */
    if(spi_dma_lli_init(spi, data, NULL, size) < 0){
        return CSI_ERROR;
    }

    DMA_Channel_Enable(SPI_DMA_ID_USE, SPI_DMA_CHAN_RX_USE);
    DMA_Channel_Enable(SPI_DMA_ID_USE, SPI_DMA_CHAN_TX_USE);

    if(spi_mode == SPI_WORK_MODE_SLAVE){
        /* spi slave : resume spi */
        tmp_val = BL_RD_REG(spi_base, SPI_CONFIG);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_CR_SPI_S_EN);
        BL_WR_REG(spi_base, SPI_CONFIG, tmp_val);
    }

    return CSI_OK;
}

/**
  \brief       Receiving data from SPI receiver
               blocking mode, return untill curtain data items are readed
  \param[in]   spi        Handle to operate
  \param[out]  data       Pointer to buffer for data to receive from SPI receiver
  \param[in]   size       Number of data items to receive(byte)
  \param[in]   timeout    Unit in mini-second
  \return      If receive successful, this function shall return the num of data witch is  received successful
               otherwise, the function shall return Error code
*/
int32_t csi_spi_receive(csi_spi_t *spi, void *data, uint32_t size, uint32_t timeout)
{
    uint32_t tmp_val;
    uint32_t spi_base = spi->dev.reg_base;
    uint32_t start_time;
    uint32_t tx_cnt,send_cnt = 0;
    uint8_t fifo_cnt, frame_size;
    SPI_WORK_MODE_Type spi_mode;

    /* dma status */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    if(BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_TX_EN) || BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_RX_EN)){
        return CSI_BUSY;
    }

    /* Get fifo valid width and spi mode */
    tmp_val = BL_RD_REG(spi_base, SPI_CONFIG);
    frame_size = BL_GET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_FRAME_SIZE) + 1;
    if (frame_size == 3) {
        frame_size = 4;
    }
    spi_mode = BL_IS_REG_BIT_SET(tmp_val, SPI_CR_SPI_S_EN) ? SPI_WORK_MODE_SLAVE : SPI_WORK_MODE_MASTER;

    /* size to conut */
    size /= frame_size;
    tx_cnt = size;

    if (spi_mode == SPI_WORK_MODE_MASTER) {
        /* clear tx/rx fifo when master mode */
        tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_TX_FIFO_CLR);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_RX_FIFO_CLR);
        BL_WR_REG(spi_base, SPI_FIFO_CONFIG_0, tmp_val);

        /* first fill tx fifo when master mode */
        fifo_cnt = SPI_GetTxFifoCount(spi->dev.idx) / frame_size;
        fifo_cnt = fifo_cnt > size ? size : fifo_cnt;
        tx_cnt -= fifo_cnt;
        for (; fifo_cnt > 0; fifo_cnt--) {
            BL_WR_REG(spi_base, SPI_FIFO_WDATA, 0xFFFFFFFF);
        }
    }

    start_time = csi_tick_get_ms();

    for (; size > 0;) {
        /* get rx fifo cnt */
        fifo_cnt = SPI_GetRxFifoCount(spi->dev.idx) / frame_size;

        if (fifo_cnt) {
            fifo_cnt = (fifo_cnt > size) ? size : fifo_cnt;
            size -= fifo_cnt;
        } else {
            if (timeout) {
                if (csi_tick_get_ms() - start_time > timeout) {
                    return CSI_TIMEOUT;
                }
            }
            continue;
        }

        /* read  data */
        for (; fifo_cnt > 0; fifo_cnt--) {
            switch (frame_size) {
                case 1:
                    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_RDATA);
                    *(uint8_t *)data = (uint8_t)tmp_val;
                    data++;
                    break;
                case 2:
                    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_RDATA);
                    *(uint16_t *)data = (uint16_t)tmp_val;
                    data += 2;
                    break;
                case 3:
                case 4:
                    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_RDATA);
                    *(uint32_t *)data = (uint32_t)tmp_val;
                    data += 4;
                    break;
                default:
                    return INVALID;
                    break;
            }
            /* master: write idle data */
            if (spi_mode == SPI_WORK_MODE_MASTER && tx_cnt) {
                BL_WR_REG(spi_base, SPI_FIFO_WDATA, 0xFFFFFFFF);
                tx_cnt--;
            }
            send_cnt ++;
        }
    }

    return send_cnt * frame_size;
}

/**
  \brief       Receiving data from SPI receiver
               not-blocking mode, event will be signaled when receive done or err happend
  \param[in]   spi     Handle to operate
  \param[out]  data    Pointer to buffer for data to receive from SPI receiver
  \param[in]   size    Number of data items to receive(byte)
  \return      Error code
*/
csi_error_t csi_spi_receive_async(csi_spi_t *spi, void *data, uint32_t size)
{
    uint32_t tmp_val;
    uint32_t spi_base = spi->dev.reg_base;
    SPI_WORK_MODE_Type spi_mode;

    CSI_PARAM_CHK(spi, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);

    /* dma status */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    if(BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_TX_EN) || BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_RX_EN)){
        return CSI_BUSY;
    }

    spi_mode = BL_IS_REG_BIT_SET(tmp_val, SPI_CR_SPI_S_EN) ? SPI_WORK_MODE_SLAVE : SPI_WORK_MODE_MASTER;

    if(spi_mode == SPI_WORK_MODE_SLAVE){
        /* spi slave : suspend spi */
        tmp_val = BL_RD_REG(spi_base, SPI_CONFIG);
        tmp_val = BL_CLR_REG_BIT(tmp_val, SPI_CR_SPI_S_EN);
        BL_WR_REG(spi_base, SPI_CONFIG, tmp_val);
    }

    /* clear tx/rx fifo, Otherwise, the sending and receiving data are not synchronized */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_TX_FIFO_CLR);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_RX_FIFO_CLR);
    BL_WR_REG(spi_base, SPI_FIFO_CONFIG_0, tmp_val);
    /* fifo dma mode */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_DMA_TX_EN);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_DMA_RX_EN);
    BL_WR_REG(spi_base, SPI_FIFO_CONFIG_0, tmp_val);

    spi->priv = (void *)(uintptr_t)SPI_EVENT_RECEIVE_COMPLETE;

    csi_irq_attach(SPI_DMA_IRQn, &spi_dma_irq_handler, &spi->dev);
    csi_irq_enable(SPI_DMA_IRQn);

    /* dma lli init */
    if(spi_dma_lli_init(spi, NULL, data, size) < 0){
        return CSI_ERROR;
    }

    DMA_Channel_Enable(SPI_DMA_ID_USE, SPI_DMA_CHAN_RX_USE);
    DMA_Channel_Enable(SPI_DMA_ID_USE, SPI_DMA_CHAN_TX_USE);

    if(spi_mode == SPI_WORK_MODE_SLAVE){
        /* spi slave : resume spi */
        tmp_val = BL_RD_REG(spi_base, SPI_CONFIG);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_CR_SPI_S_EN);
        BL_WR_REG(spi_base, SPI_CONFIG, tmp_val);
    }

    return CSI_OK;
}

/**
  \brief       Dulplex,sending and receiving data at the same time
               \ref csi_spi_event_t is signaled when operation completes or error happens
               \ref csi_spi_get_state can get operation status
               blocking mode, this function returns after operation completes or error happens
  \param[in]   spi         SPI handle to operate
  \param[in]   data_out    Pointer to buffer with data to send to SPI transmitter
  \param[out]  data_in     Pointer to buffer for data to receive from SPI receiver
  \param[in]   size        Data size(byte)
  \return      If transfer successful, this function shall return the num of data witch is transfer successful,
               otherwise, the function shall return Error code
*/
int32_t csi_spi_send_receive(csi_spi_t *spi, const void *data_out, void *data_in, uint32_t size, uint32_t timeout)
{
    uint32_t tmp_val;
    uint32_t spi_base = spi->dev.reg_base;
    uint32_t start_time;
    uint32_t tx_cnt,send_cnt = 0;
    uint8_t fifo_cnt, frame_size;
    SPI_WORK_MODE_Type spi_mode;

    CSI_PARAM_CHK(spi, CSI_ERROR);
    CSI_PARAM_CHK(data_in, CSI_ERROR);
    CSI_PARAM_CHK(data_out, CSI_ERROR);

    /* dma status */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    if(BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_TX_EN) || BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_RX_EN)){
        return CSI_BUSY;
    }

    /* Get fifo valid width */
    tmp_val = BL_RD_REG(spi_base, SPI_CONFIG);
    frame_size = BL_GET_REG_BITS_VAL(tmp_val, SPI_CR_SPI_FRAME_SIZE) + 1;
    if (frame_size == 3) {
        frame_size = 4;
    }
    spi_mode = BL_IS_REG_BIT_SET(tmp_val, SPI_CR_SPI_S_EN) ? SPI_WORK_MODE_SLAVE : SPI_WORK_MODE_MASTER;

    /* size to conut */
    size /= frame_size;
    tx_cnt = size;

    if(spi_mode == SPI_WORK_MODE_SLAVE){
        /* spi slave : suspend spi */
        tmp_val = BL_RD_REG(spi_base, SPI_CONFIG);
        tmp_val = BL_CLR_REG_BIT(tmp_val, SPI_CR_SPI_S_EN);
        BL_WR_REG(spi_base, SPI_CONFIG, tmp_val);
    }

    /* clear tx/rx fifo, Otherwise, the sending and receiving data are not synchronized */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_TX_FIFO_CLR);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_RX_FIFO_CLR);
    BL_WR_REG(spi_base, SPI_FIFO_CONFIG_0, tmp_val);

    /* First fill tx fifo */
    fifo_cnt = SPI_GetTxFifoCount(spi->dev.idx) / frame_size;
    fifo_cnt = fifo_cnt > size ? size : fifo_cnt;
    tx_cnt -= fifo_cnt;
    for (; fifo_cnt > 0; fifo_cnt--) {
        switch (frame_size) {
            case 1:
                tmp_val = *(uint8_t *)data_out;
                BL_WR_REG(spi_base, SPI_FIFO_WDATA, tmp_val);
                data_out++;
                break;
            case 2:
                tmp_val = *(uint16_t *)data_out;
                BL_WR_REG(spi_base, SPI_FIFO_WDATA, tmp_val);
                data_out += 2;
                break;
            case 3:
            case 4:
                tmp_val = *(uint32_t *)data_out;
                BL_WR_REG(spi_base, SPI_FIFO_WDATA, tmp_val);
                data_out += 4;
                break;
            default:
                return INVALID;
                break;
        }
    }

    if(spi_mode == SPI_WORK_MODE_SLAVE){
        /* spi slave : resume spi */
        tmp_val = BL_RD_REG(spi_base, SPI_CONFIG);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_CR_SPI_S_EN);
        BL_WR_REG(spi_base, SPI_CONFIG, tmp_val);
    }

    start_time = csi_tick_get_ms();

    /* read and write rest of the data */
    for (; size > 0;) {
        /* get rx fifo cnt */
        fifo_cnt = SPI_GetRxFifoCount(spi->dev.idx) / frame_size;

        if (fifo_cnt) {
            fifo_cnt = fifo_cnt > size ? size : fifo_cnt;
            size -= fifo_cnt;
        } else {
            if (timeout) {
                if (csi_tick_get_ms() - start_time > timeout) {
                    return CSI_TIMEOUT;
                }
            }
            continue;
        }

        /* read and write data */
        for (; fifo_cnt > 0; fifo_cnt--) {
            switch (frame_size) {
                case 1:
                    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_RDATA);
                    *((uint8_t *)data_in) = (uint8_t)tmp_val;
                    data_in++;
                    if (tx_cnt) {
                        tmp_val = *(uint8_t *)data_out;
                        BL_WR_REG(spi_base, SPI_FIFO_WDATA, tmp_val);
                        data_out++;
                        tx_cnt--;
                    }
                    break;
                case 2:
                    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_RDATA);
                    *((uint16_t *)data_in) = (uint16_t)tmp_val;
                    data_in += 2;
                    if (tx_cnt) {
                        tmp_val = *(uint16_t *)data_out;
                        BL_WR_REG(spi_base, SPI_FIFO_WDATA, tmp_val);
                        data_out += 2;
                        tx_cnt--;
                    }
                    break;
                case 3:
                case 4:
                    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_RDATA);
                    *((uint32_t *)data_in) = (uint32_t)tmp_val;
                    data_in += 4;
                    if (tx_cnt) {
                        tmp_val = *(uint32_t *)data_out;
                        BL_WR_REG(spi_base, SPI_FIFO_WDATA, tmp_val);
                        data_out += 4;
                        tx_cnt--;
                    }
                    break;
                default:
                    return INVALID;
                    break;
            }
            send_cnt ++;
        }
    }

    return send_cnt * frame_size;
}

/**
  \brief       Transmit first then receive ,receive will begin after transmit is done
               if non-blocking mode, this function only starts the transfer,
               \ref csi_spi_event_t is signaled when operation completes or error happens
               \ref csi_spi_get_state can get operation status
  \param[in]   spi         SPI handle to operate
  \param[in]   data_out    Pointer to buffer with data to send to SPI transmitter
  \param[out]  data_in     Pointer to buffer for data to receive from SPI receiver
  \param[in]   size        Data size(byte)
  \return      Error code
*/
csi_error_t csi_spi_send_receive_async(csi_spi_t *spi, const void *data_out, void *data_in, uint32_t size)
{
    uint32_t tmp_val;
    uint32_t spi_base = spi->dev.reg_base;
    SPI_WORK_MODE_Type spi_mode;

    CSI_PARAM_CHK(spi, CSI_ERROR);
    CSI_PARAM_CHK(data_in, CSI_ERROR);
    CSI_PARAM_CHK(data_out, CSI_ERROR);

    /* dma status */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    if(BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_TX_EN) || BL_IS_REG_BIT_SET(tmp_val, SPI_DMA_RX_EN)){
        return CSI_BUSY;
    }

    spi_mode = BL_IS_REG_BIT_SET(tmp_val, SPI_CR_SPI_S_EN) ? SPI_WORK_MODE_SLAVE : SPI_WORK_MODE_MASTER;

    if(spi_mode == SPI_WORK_MODE_SLAVE){
        /* spi slave : suspend spi */
        tmp_val = BL_RD_REG(spi_base, SPI_CONFIG);
        tmp_val = BL_CLR_REG_BIT(tmp_val, SPI_CR_SPI_S_EN);
        BL_WR_REG(spi_base, SPI_CONFIG, tmp_val);
    }

    /* clear tx/rx fifo, Otherwise, the sending and receiving data are not synchronized */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_TX_FIFO_CLR);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_RX_FIFO_CLR);
    BL_WR_REG(spi_base, SPI_FIFO_CONFIG_0, tmp_val);
    /* fifo dma mode */
    tmp_val = BL_RD_REG(spi_base, SPI_FIFO_CONFIG_0);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_DMA_TX_EN);
    tmp_val = BL_SET_REG_BIT(tmp_val, SPI_DMA_RX_EN);
    BL_WR_REG(spi_base, SPI_FIFO_CONFIG_0, tmp_val);

    spi->priv = (void *)(uintptr_t)SPI_EVENT_SEND_RECEIVE_COMPLETE;

    csi_irq_attach(SPI_DMA_IRQn, &spi_dma_irq_handler, &spi->dev);
    csi_irq_enable(SPI_DMA_IRQn);

    /* dma lli init */
    if(spi_dma_lli_init(spi, data_out, data_in, size) < 0){
        return CSI_ERROR;
    }

    DMA_Channel_Enable(SPI_DMA_ID_USE, SPI_DMA_CHAN_RX_USE);
    DMA_Channel_Enable(SPI_DMA_ID_USE, SPI_DMA_CHAN_TX_USE);

    if(spi_mode == SPI_WORK_MODE_SLAVE){
        /* spi slave : resume spi */
        tmp_val = BL_RD_REG(spi_base, SPI_CONFIG);
        tmp_val = BL_SET_REG_BIT(tmp_val, SPI_CR_SPI_S_EN);
        BL_WR_REG(spi_base, SPI_CONFIG, tmp_val);
    }

    return CSI_OK;
}

/*
  \brief       Set slave select num. Only valid for master
  \param[in]   handle       SPI handle to operate
  \param[in]   slave_num    SPI slave num
  \return      None
 */
void csi_spi_select_slave(csi_spi_t *spi, uint32_t slave_num)
{
    return;
}

/**
  \brief       Link DMA channel to spi device
  \param[in]   spi       SPI handle to operate
  \param[in]   tx_dma    The DMA channel handle for send, when it is NULL means to unlink the channel
  \param[in]   rx_dma    The DMA channel handle for receive, when it is NULL means to unlink the channel
  \return      Error code
*/
csi_error_t csi_spi_link_dma(csi_spi_t *spi, csi_dma_ch_t *tx_dma, csi_dma_ch_t *rx_dma)
{
    return CSI_UNSUPPORTED;
}

/**
  \brief       Get the state of spi device
  \param[in]   spi      SPI handle to operate
  \param[out]  state    The state of spi device
  \return      Error code
*/
csi_error_t csi_spi_get_state(csi_spi_t *spi, csi_state_t *state)
{
    return CSI_UNSUPPORTED;
}


