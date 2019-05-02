/**
 * Derived from https://github.com/pingumacpenguin/STM32-O-Scope/blob/master/STM32-O-Scope.ino
 */
/*.
(c) Andrew Hull - 2015
STM32-O-Scope - aka "The Pig Scope" or pigScope released under the GNU GENERAL PUBLIC LICENSE Version 2, June 1991
https://github.com/pingumacpenguin/STM32-O-Scope
Adafruit Libraries released under their specific licenses Copyright (c) 2013 Adafruit Industries.  All rights reserved.
*/
/**
 We use PA0 as input pin
 * DMA1, channel 0
 
 * Vref is using PWM mode for Timer4/Channel 3
 * 
 */
#include "dso_adc_const.h"

static voidFuncPtr adcIrqHandler=NULL;
HardwareTimer pwmtimer(4); // Vref PWM is Timer4 Channel3
/**
 */
uint32_t DSOADC::getVCCmv()
{
    return vcc;
}

/**
 * 
 * @return 
 */
bool DSOADC::readCalibrationValue()
{
    #define NB_SAMPLE 16

   adc_Register = ADC1->regs;
   adc_Register->CR2 |= ADC_CR2_TSVREFE;    // enable VREFINT and temp sensor
   adc_Register->SMPR1 =  ADC_SMPR1_SMP17;  // sample ra

    
   float fvcc=0;
   for(int i=0;i<NB_SAMPLE;i++)
   {
       delay(10);   
       fvcc+=  adc_read(ADC1, 17); 
   }
    fvcc=(1200. * 4096.*NB_SAMPLE) /fvcc;   
    fvcc=3380;
    vcc=(int)fvcc;
    
    
    // 1b fill up the conversion table
    for(int i=0;i<11;i++)
    {
        vSettings[i].offset=calibrationDC[i+1];
        vSettings[i].multiplier=inputScale[i+1]*fvcc/4096000.;
    }
    return true;
}

/**
 * 
 */
void DSOADC::setADCs ()
{
 // 1 - Read VCC
   adc_Register = ADC1->regs;
   adc_Register->CR2 |= ADC_CR2_TSVREFE;    // enable VREFINT and temp sensor
   adc_Register->SMPR1 =  ADC_SMPR1_SMP17;  // sample ra

 // 2 - Setup ADC
    
  int pinMapADCin = PIN_MAP[analogInPin].adc_channel;
  adc_set_sample_rate(ADC1, ADC_SMPR_1_5); //=0,58uS/sample.  ADC_SMPR_13_5 = 1.08uS - use this one if Rin>10Kohm,
  adc_set_sample_rate(ADC2, ADC_SMPR_1_5);    // if not may get some sporadic noise. see datasheet.
  adc_set_prescaler(ADC_PRE_PCLK2_DIV_2);
   
  adc_set_reg_seqlen(ADC1, 1);
  
  adc_Register->SQR3 = pinMapADCin;
  adc_Register->CR2 |= ADC_CR2_CONT; // | ADC_CR2_DMA; // Set continuous mode and DMA
  adc_Register->CR1 |= ADC_CR1_FASTINT; // Interleaved mode
  //adc_Register->CR2 |= ADC_CR2_SWSTART;
  
  ADC2->regs->CR2 |= ADC_CR2_CONT; // ADC 2 continuos
  ADC2->regs->SQR3 = pinMapADCin;
  
  pinMode(triggerPin,INPUT);
    
  pwmtimer.setPeriod(2000); // 5Khz pwm
  pinMode(vRefPin,PWM);
  
  pwmWrite(vRefPin,3000);
  setTriggerMode(DSOADC::Trigger_Both);
  
  
  enableDisableIrqSource(false,ADC_AWD);
  enableDisableIrqSource(false,ADC_EOC);
  enableDisableIrqSource(false,ADC_JEOC);
  enableDisableIrq(false);
  attachWatchdogInterrupt(NULL);
}
/**
 * 
 * @param mode
 * @return 
 */
 bool DSOADC::setTriggerMode(TriggerMode mode)
 {
    ExtIntTriggerMode m;
    _triggerMode=mode;
#if 0    
    switch(_triggerMode)
    {
        case DSOADC::Trigger_Falling: m=FALLING;break;
        case DSOADC::Trigger_Rising:  m=RISING;break;
        case DSOADC::Trigger_Both:    m=CHANGE;break;
        default: xAssert(0);break;
    }
    // Hook trigger interrupt  
    attachInterrupt(triggerPin,TriggerInterrupt,m );
#endif    
 }
 /**
  * 
  * @param ratio
  * @return 
  */
 bool DSOADC::setVrefPWM(int ratio)
 {
     //ratio=(ratio*16384)/0xffff;
     pwmWrite(vRefPin,(uint16)ratio);
 }
 
 /**
  * 
  * @return 
  */
 bool     DSOADC::getTriggerState()
 {
     return !!digitalRead(triggerPin);
 }
/**
 * 
 * @param timeScaleUs
 * @return 
 */
 bool    DSOADC::setTimeScale(adc_smp_rate one, adc_prescaler two)
 {
    adc_set_sample_rate(ADC1, one); //=0,58uS/sample.  ADC_SMPR_13_5 = 1.08uS - use this one if Rin>10Kohm,
    adc_set_prescaler(two);
    return true;
 }
 /**
  * 
  * @param count
  * @return 
  */
bool    DSOADC::prepareDMASampling (adc_smp_rate rate,adc_prescaler scale)
{    

    setTimeScale(rate,scale);
    return true;
}
/**
 * 
 * @param count
 * @return 
 */
bool DSOADC::getSamples(FullSampleSet &fullSet)
{
    if(!dmaSemaphore->take(10000))
        return false;
    fullSet=_captured;
    return true;
}
 

/**
* @brief Enable DMA requests
* @param dev ADC device on which to enable DMA requests
*/

void DSOADC::adc_dma_enable(const adc_dev * dev) 
{
  bb_peri_set_bit(&dev->regs->CR2, ADC_CR2_DMA_BIT, 1);  
}


/**
* @brief Disable DMA requests
* @param dev ADC device on which to disable DMA requests
*/

void DSOADC::adc_dma_disable(const adc_dev * dev) 
{
  bb_peri_set_bit(&dev->regs->CR2, ADC_CR2_DMA_BIT, 0);
  dma_attach_interrupt(DMA1, DMA_CH1, SPURIOUS_INTERRUPT);
}


/**
 * 
 */
void Oopps()
{
    while(1)
    {
        
    };
}
/**
 * 
 * @param count
 * @param buffer
 * @param handler
 */
void DSOADC::setupAdcDmaTransfer(   int count,uint32_t *buffer, void (*handler)(void) )
{
  dma_init(DMA1);
  dma_attach_interrupt(DMA1, DMA_CH1, handler); 
  dma_setup_transfer(DMA1, DMA_CH1, &ADC1->regs->DR, DMA_SIZE_32BITS, buffer, DMA_SIZE_32BITS, (DMA_MINC_MODE | DMA_TRNS_CMPLT));// Receive buffer DMA
  dma_set_num_transfers(DMA1, DMA_CH1, count );
  adc_dma_enable(ADC1);
  dma_enable(DMA1, DMA_CH1); // Enable the channel and start the transfer.

}

void DSOADC::nextAdcDmaTransfer( int count,uint32_t *buffer)
{
    dma_setup_transfer(DMA1, DMA_CH1, &ADC1->regs->DR, DMA_SIZE_32BITS, buffer, DMA_SIZE_32BITS, (DMA_MINC_MODE | DMA_TRNS_CMPLT));// Receive buffer DMA
    dma_set_num_transfers(DMA1, DMA_CH1, count );
    dma_enable(DMA1, DMA_CH1); // Enable the channel and start the transfer.
}

/**
 * 
 * @param interruptMask
 */
void DSOADC::enableDisableIrqSource(bool onoff, int interrupt)
{
    if(onoff)
    {
        if(interrupt==ADC_AWD)
        {                      
            int channel=0;
             ADC1->regs->CR1 |= (channel & ADC_CR1_AWDCH) | 0*ADC_CR1_AWDSGL ;
             ADC1->regs->CR1 |= ADC_CR1_AWDEN  | ADC_CR1_AWDIE;
        }
        ADC1->regs->CR1 |= (1U<<((interrupt) +ADC_CR1_EOCIE_BIT));
    }else
    {
        ADC1->regs->CR1 &= ~(1U<<((interrupt) +ADC_CR1_EOCIE_BIT));
        if(interrupt==ADC_AWD)
             ADC1->regs->CR1 &= ~ADC_CR1_AWDEN  | ~ ADC_CR1_AWDIE;
        
    }
}
/**
 * 
 * @param onoff
 */
void DSOADC::enableDisableIrq(bool onoff)
{
    if(onoff)
    {
        nvic_irq_enable(ADC1->irq_num);
    }
    else
        nvic_irq_disable(ADC1->irq_num);
}
/**
 * 
 */
extern void watchDog();
void DSOADC::defaultAdcIrqHandler()
{    
    if(adcIrqHandler)
        adcIrqHandler();
}
        
/**
 * 
 * @param handler
 */
void DSOADC::attachWatchdogInterrupt(voidFuncPtr handler)
{
    adcIrqHandler=handler;
    adc_attach_interrupt(ADC1,ADC_AWD,defaultAdcIrqHandler);
}

/**
 * 
 * @param high
 * @param low
 */
void DSOADC::setWatchdogTriggerValue(uint32_t high, uint32_t low)
{
    ADC1->regs->HTR=high;
    ADC1->regs->LTR=low;
}
//
