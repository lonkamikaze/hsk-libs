//****************************************************************************
// @Module        Project Settings
// @Filename      XC878.H
// @Project        XC878.dav
//----------------------------------------------------------------------------
// @Controller    Infineon XC878CM-16FF
//
// @Compiler      Keil
//
// @Codegenerator 1.1
//
// @Description   This is the include header file for all other modules.
//
//----------------------------------------------------------------------------
// @Date          20.06.2008 16:22:53
//
//****************************************************************************

// USER CODE BEGIN (MAIN_Header,1)

// USER CODE END



#ifndef _MAIN_H_
#define _MAIN_H_

//****************************************************************************
// @Project Includes
//****************************************************************************

// USER CODE BEGIN (MAIN_Header,2)

// USER CODE END


//****************************************************************************
// @Macros
//****************************************************************************

// Please ensure that SCU_PAGE is switched to Page 1 before using these macros
#define MAIN_vUnlockProtecReg() PASSWD = 0x9B 
#define MAIN_vlockProtecReg()   PASSWD = 0xAB


// USER CODE BEGIN (MAIN_Header,3)

// USER CODE END


//****************************************************************************
// @Defines
//****************************************************************************

// USER CODE BEGIN (MAIN_Header,4)

// USER CODE END

#define bool  bit
#define ulong unsigned long
#define uword unsigned int
#define ubyte unsigned char



//****************************************************************************
// @Prototypes Of Global Functions
//****************************************************************************


// USER CODE BEGIN (MAIN_Header,5)

// USER CODE END


//   -------------------------------------------------------------------------
//   Declaration of SFRs
//   -------------------------------------------------------------------------

//   Notes: You can avoid the problem that your compiler does not yet support 
//          the latest derivatives if you use the SFR definitions generated 
//          by DAvE instead of those that come along with your compiler (in 
//          the "Register File").

//          PORT SFRs are defined in file 'IO.H'.

#ifdef SDCC
#define SBIT(name, addr, bit)  __sbit  __at(addr+bit)                  name
#define SFR(name, addr)        __sfr   __at(addr)                      name
#define SFR16(name, addr)      __sfr16 __at(((addr+1U)<<8) | addr)     name
#define data                   __data
#define idata                  __idata
#define xdata                  __xdata
#define bit                    __bit
//#define code                   __code
#define code                   
#define interrupt              __interrupt
#define reentrant              __reentrant
#endif


#ifdef __C51__
#define SBIT(name, addr, bit)  sbit  name = (addr^bit)
#define SFR(name, addr)        sfr   name = addr
#define SFR16(name, addr)      sfr16 name = addr
#endif


//   defines for sbit definitions
#define BIT0    0
#define BIT1    1
#define BIT2    2
#define BIT3    3
#define BIT4    4
#define BIT5    5
#define BIT6    6
#define BIT7    7


//   SFR byte definitions
SFR(ACC       , 0xE0);    
SFR(ADC_CHCTR0, 0xCA);    
SFR(ADC_CHCTR1, 0xCB);    
SFR(ADC_CHCTR2, 0xCC);    
SFR(ADC_CHCTR3, 0xCD);    
SFR(ADC_CHCTR4, 0xCE);    
SFR(ADC_CHCTR5, 0xCF);    
SFR(ADC_CHCTR6, 0xD2);    
SFR(ADC_CHCTR7, 0xD3);    
SFR(ADC_CHINCR, 0xCB);    
SFR(ADC_CHINFR, 0xCA);    
SFR(ADC_CHINPR, 0xCD);    
SFR(ADC_CHINSR, 0xCC);    
SFR(ADC_CRCR1 , 0xCA);    
SFR(ADC_CRMR1 , 0xCC);    
SFR(ADC_CRPR1 , 0xCB);    
SFR(ADC_ETRCR , 0xCF);    
SFR(ADC_EVINCR, 0xCF);    
SFR(ADC_EVINFR, 0xCE);    
SFR(ADC_EVINPR, 0xD3);    
SFR(ADC_EVINSR, 0xD2);    
SFR(ADC_GLOBCTR, 0xCA);    
SFR(ADC_GLOBSTR, 0xCB);    
SFR(ADC_INPCR0, 0xCE);    
SFR(ADC_LCBR  , 0xCD);    
SFR(ADC_PAGE  , 0xD1);    
SFR(ADC_PRAR  , 0xCC);    
SFR(ADC_Q0R0  , 0xCF);    
SFR(ADC_QBUR0 , 0xD2);    
SFR(ADC_QINR0 , 0xD2);    
SFR(ADC_QMR0  , 0xCD);    
SFR(ADC_QSR0  , 0xCE);    
SFR(ADC_RCR0  , 0xCA);    
SFR(ADC_RCR1  , 0xCB);    
SFR(ADC_RCR2  , 0xCC);    
SFR(ADC_RCR3  , 0xCD);    
SFR(ADC_RESR0H, 0xCB);    
SFR(ADC_RESR0L, 0xCA);    
SFR(ADC_RESR1H, 0xCD);    
SFR(ADC_RESR1L, 0xCC);    
SFR(ADC_RESR2H, 0xCF);    
SFR(ADC_RESR2L, 0xCE);    
SFR(ADC_RESR3H, 0xD3);    
SFR(ADC_RESR3L, 0xD2);    
SFR(ADC_RESRA0H, 0xCB);    
SFR(ADC_RESRA0L, 0xCA);    
SFR(ADC_RESRA1H, 0xCD);    
SFR(ADC_RESRA1L, 0xCC);    
SFR(ADC_RESRA2H, 0xCF);    
SFR(ADC_RESRA2L, 0xCE);    
SFR(ADC_RESRA3H, 0xD3);    
SFR(ADC_RESRA3L, 0xD2);    
SFR(ADC_VFCR  , 0xCE);    
SFR(B         , 0xF0);    
SFR(BCON      , 0xBD);    
SFR(BG        , 0xBE);    
SFR(CAN_ADCON , 0xD8);    
SFR(CAN_ADH   , 0xDA);    
SFR(CAN_ADL   , 0xD9);    
SFR(CAN_DATA0 , 0xDB);    
SFR(CAN_DATA1 , 0xDC);    
SFR(CAN_DATA2 , 0xDD);    
SFR(CAN_DATA3 , 0xDE);    
SFR(CCU6_CC60RH, 0xFB);    
SFR(CCU6_CC60RL, 0xFA);    
SFR(CCU6_CC60SRH, 0xFB);    
SFR(CCU6_CC60SRL, 0xFA);    
SFR(CCU6_CC61RH, 0xFD);    
SFR(CCU6_CC61RL, 0xFC);    
SFR(CCU6_CC61SRH, 0xFD);    
SFR(CCU6_CC61SRL, 0xFC);    
SFR(CCU6_CC62RH, 0xFF);    
SFR(CCU6_CC62RL, 0xFE);    
SFR(CCU6_CC62SRH, 0xFF);    
SFR(CCU6_CC62SRL, 0xFE);    
SFR(CCU6_CC63RH, 0x9B);    
SFR(CCU6_CC63RL, 0x9A);    
SFR(CCU6_CC63SRH, 0x9B);    
SFR(CCU6_CC63SRL, 0x9A);    
SFR(CCU6_CMPMODIFH, 0xA7);    
SFR(CCU6_CMPMODIFL, 0xA6);    
SFR(CCU6_CMPSTATH, 0xFF);    
SFR(CCU6_CMPSTATL, 0xFE);    
SFR(CCU6_IENH , 0x9D);    
SFR(CCU6_IENL , 0x9C);    
SFR(CCU6_INPH , 0x9F);    
SFR(CCU6_INPL , 0x9E);    
SFR(CCU6_ISH  , 0x9D);    
SFR(CCU6_ISL  , 0x9C);    
SFR(CCU6_ISRH , 0xA5);    
SFR(CCU6_ISRL , 0xA4);    
SFR(CCU6_ISSH , 0xA5);    
SFR(CCU6_ISSL , 0xA4);    
SFR(CCU6_MCMCTR, 0xA7);    
SFR(CCU6_MCMOUTH, 0x9B);    
SFR(CCU6_MCMOUTL, 0x9A);    
SFR(CCU6_MCMOUTSH, 0x9F);    
SFR(CCU6_MCMOUTSL, 0x9E);    
SFR(CCU6_MODCTRH, 0xFD);    
SFR(CCU6_MODCTRL, 0xFC);    
SFR(CCU6_PAGE , 0xA3);    
SFR(CCU6_PISEL0H, 0x9F);    
SFR(CCU6_PISEL0L, 0x9E);    
SFR(CCU6_PISEL2, 0xA4);    
SFR(CCU6_PSLR , 0xA6);    
SFR(CCU6_T12DTCH, 0xA5);    
SFR(CCU6_T12DTCL, 0xA4);    
SFR(CCU6_T12H , 0xFB);    
SFR(CCU6_T12L , 0xFA);    
SFR(CCU6_T12MSELH, 0x9B);    
SFR(CCU6_T12MSELL, 0x9A);    
SFR(CCU6_T12PRH, 0x9D);    
SFR(CCU6_T12PRL, 0x9C);    
SFR(CCU6_T13H , 0xFD);    
SFR(CCU6_T13L , 0xFC);    
SFR(CCU6_T13PRH, 0x9F);    
SFR(CCU6_T13PRL, 0x9E);    
SFR(CCU6_TCTR0H, 0xA7);    
SFR(CCU6_TCTR0L, 0xA6);    
SFR(CCU6_TCTR2H, 0xFB);    
SFR(CCU6_TCTR2L, 0xFA);    
SFR(CCU6_TCTR4H, 0x9D);    
SFR(CCU6_TCTR4L, 0x9C);    
SFR(CCU6_TRPCTRH, 0xFF);    
SFR(CCU6_TRPCTRL, 0xFE);    
SFR(CD_CON    , 0xA1);    
SFR(CD_CORDXH , 0x9B);    
SFR(CD_CORDXL , 0x9A);    
SFR(CD_CORDYH , 0x9D);    
SFR(CD_CORDYL , 0x9C);    
SFR(CD_CORDZH , 0x9F);    
SFR(CD_CORDZL , 0x9E);    
SFR(CD_STATC  , 0xA0);    
SFR(CMCON     , 0xBA);    
SFR(COCON     , 0xBE);    
SFR(CR_MISC   , 0xEB);    
SFR(DPH       , 0x83);    
SFR(DPL       , 0x82);    
SFR(EO        , 0xA2);    
SFR(EXICON0   , 0xB7);    
SFR(EXICON1   , 0xBA);    
SFR(FDCON     , 0xE9);    
SFR(FDRES     , 0xEB);    
SFR(FDSTEP    , 0xEA);    
SFR(HWBPDR    , 0xF7);    
SFR(HWBPSR    , 0xF6);    
SFR(ID        , 0xB3);    
SFR(IEN0      , 0xA8);    
SFR(IEN1      , 0xE8);    
SFR(IP        , 0xB8);    
SFR(IP1       , 0xF8);    
SFR(IPH       , 0xB9);    
SFR(IPH1      , 0xF9);    
SFR(IRCON0    , 0xB4);    
SFR(IRCON1    , 0xB5);    
SFR(IRCON2    , 0xB6);    
SFR(IRCON3    , 0xB4);    
SFR(IRCON4    , 0xB5);    
SFR(MDU_MD0   , 0xB2);    
SFR(MDU_MD1   , 0xB3);    
SFR(MDU_MD2   , 0xB4);    
SFR(MDU_MD3   , 0xB5);    
SFR(MDU_MD4   , 0xB6);    
SFR(MDU_MD5   , 0xB7);    
SFR(MDU_MDUCON, 0xB1);    
SFR(MDU_MDUSTAT, 0xB0);    
SFR(MDU_MR0   , 0xB2);    
SFR(MDU_MR1   , 0xB3);    
SFR(MDU_MR2   , 0xB4);    
SFR(MDU_MR3   , 0xB5);    
SFR(MDU_MR4   , 0xB6);    
SFR(MDU_MR5   , 0xB7);    
SFR(MEX1      , 0x94);    
SFR(MEX2      , 0x95);    
SFR(MEX3      , 0x96);    
SFR(MEXSP     , 0x97);    
SFR(MEXTCR    , 0xEA);    
SFR(MISC_CON  , 0xE9);    
SFR(MMBPCR    , 0xF3);    
SFR(MMCR      , 0xF1);    
SFR(MMCR2     , 0xE9);    
SFR(MMDR      , 0xF5);    
SFR(MMICR     , 0xF4);    
SFR(MMSR      , 0xF2);    
SFR(MMWR1     , 0xEB);    
SFR(MMWR2     , 0xEC);    
SFR(MODIEN    , 0xB6);    
SFR(MODPISEL  , 0xB3);    
SFR(MODPISEL1 , 0xB7);    
SFR(MODPISEL2 , 0xBA);    
SFR(MODPISEL3 , 0xBE);    
SFR(MODPISEL4 , 0xEA);    
SFR(MODSUSP   , 0xBD);    
SFR(NMICON    , 0xBB);    
SFR(NMISR     , 0xBC);    
SFR(OSC_CON   , 0xB6);    
SFR(P0_ALTSEL0, 0x80);    
SFR(P0_ALTSEL1, 0x86);    
SFR(P0_DATA   , 0x80);    
SFR(P0_DIR    , 0x86);    
SFR(P0_DS     , 0x86);    
SFR(P0_OD     , 0x80);    
SFR(P0_PUDEN  , 0x86);    
SFR(P0_PUDSEL , 0x80);    
SFR(P1_ALTSEL0, 0x90);    
SFR(P1_ALTSEL1, 0x91);    
SFR(P1_DATA   , 0x90);    
SFR(P1_DIR    , 0x91);    
SFR(P1_DS     , 0x91);    
SFR(P1_OD     , 0x90);    
SFR(P1_PUDEN  , 0x91);    
SFR(P1_PUDSEL , 0x90);    
SFR(P3_ALTSEL0, 0xB0);    
SFR(P3_ALTSEL1, 0xB1);    
SFR(P3_DATA   , 0xB0);    
SFR(P3_DIR    , 0xB1);    
SFR(P3_DS     , 0xB1);    
SFR(P3_OD     , 0xB0);    
SFR(P3_PUDEN  , 0xB1);    
SFR(P3_PUDSEL , 0xB0);    
SFR(P4_ALTSEL0, 0xC8);    
SFR(P4_ALTSEL1, 0xC9);    
SFR(P4_DATA   , 0xC8);    
SFR(P4_DIR    , 0xC9);    
SFR(P4_DS     , 0xC9);    
SFR(P4_OD     , 0xC8);    
SFR(P4_PUDEN  , 0xC9);    
SFR(P4_PUDSEL , 0xC8);    
SFR(P5_ALTSEL0, 0x92);    
SFR(P5_ALTSEL1, 0x93);    
SFR(P5_DATA   , 0x92);    
SFR(P5_DIR    , 0x93);    
SFR(P5_DS     , 0x93);    
SFR(P5_OD     , 0x92);    
SFR(P5_PUDEN  , 0x93);    
SFR(P5_PUDSEL , 0x92);    
SFR(PASSWD    , 0xBB);    
SFR(PCON      , 0x87);    
SFR(PLL_CON   , 0xB7);    
SFR(PLL_CON1  , 0xEA);    
SFR(PMCON0    , 0xB4);    
SFR(PMCON1    , 0xB5);    
SFR(PMCON2    , 0xBB);    
SFR(PORT_PAGE , 0xB2);    
SFR(PSW       , 0xD0);    
SFR(SBUF      , 0x99);    
SFR(SCON      , 0x98);    
SFR(SCU_PAGE  , 0xBF);    
SFR(SP        , 0x81);    
SFR(SSC_BRH   , 0xAF);    
SFR(SSC_BRL   , 0xAE);    
SFR(SSC_CONH_O, 0xAB);    
SFR(SSC_CONH_P, 0xAB);    
SFR(SSC_CONL_O, 0xAA);    
SFR(SSC_CONL_P, 0xAA);    
SFR(SSC_RBL   , 0xAD);    
SFR(SSC_TBL   , 0xAC);    
SFR(SYSCON0   , 0x8F);    
SFR(T21_RC2H  , 0xC3);    
SFR(T21_RC2L  , 0xC2);    
SFR(T21_T2CON , 0xC0);    
SFR(T21_T2CON1, 0xC6);    
SFR(T21_T2H   , 0xC5);    
SFR(T21_T2L   , 0xC4);    
SFR(T21_T2MOD , 0xC1);    
SFR(T2CCU_CC0H, 0xC2);    
SFR(T2CCU_CC0L, 0xC1);    
SFR(T2CCU_CC1H, 0xC4);    
SFR(T2CCU_CC1L, 0xC3);    
SFR(T2CCU_CC2H, 0xC6);    
SFR(T2CCU_CC2L, 0xC5);    
SFR(T2CCU_CC3H, 0xC2);    
SFR(T2CCU_CC3L, 0xC1);    
SFR(T2CCU_CC4H, 0xC4);    
SFR(T2CCU_CC4L, 0xC3);    
SFR(T2CCU_CC5H, 0xC6);    
SFR(T2CCU_CC5L, 0xC5);    
SFR(T2CCU_CCEN, 0xC0);    
SFR(T2CCU_CCTBSEL, 0xC1);    
SFR(T2CCU_CCTCON, 0xC6);    
SFR(T2CCU_CCTDTCH, 0xC3);    
SFR(T2CCU_CCTDTCL, 0xC2);    
SFR(T2CCU_CCTH, 0xC5);    
SFR(T2CCU_CCTL, 0xC4);    
SFR(T2CCU_CCTRELH, 0xC3);    
SFR(T2CCU_CCTRELL, 0xC2);    
SFR(T2CCU_COCON, 0xC0);    
SFR(T2CCU_COSHDW, 0xC0);    
SFR(T2_PAGE   , 0xC7);    
SFR(T2_RC2H   , 0xC3);    
SFR(T2_RC2L   , 0xC2);    
SFR(T2_T2CON  , 0xC0);    
SFR(T2_T2CON1 , 0xC6);    
SFR(T2_T2H    , 0xC5);    
SFR(T2_T2L    , 0xC4);    
SFR(T2_T2MOD  , 0xC1);    
SFR(TCON      , 0x88);    
SFR(TH0       , 0x8C);    
SFR(TH1       , 0x8D);    
SFR(TL0       , 0x8A);    
SFR(TL1       , 0x8B);    
SFR(TMOD      , 0x89);    
SFR(UART1_BCON, 0xCA);    
SFR(UART1_BG  , 0xCB);    
SFR(UART1_FDCON, 0xCC);    
SFR(UART1_FDRES, 0xCE);    
SFR(UART1_FDSTEP, 0xCD);    
SFR(UART1_SBUF, 0xC9);    
SFR(UART1_SCON, 0xC8);    
SFR(UART1_SCON1, 0xCF);    
SFR(WDTCON    , 0xBB);    //   located in the mapped SFR area
SFR(WDTH      , 0xBF);    //   located in the mapped SFR area
SFR(WDTL      , 0xBE);    //   located in the mapped SFR area
SFR(WDTREL    , 0xBC);    //   located in the mapped SFR area
SFR(WDTWINB   , 0xBD);    //   located in the mapped SFR area
SFR(XADDRH    , 0xB3);    


//   SFR bit definitions
//   CD_STATC
#define CD_STATC_BASE    0xA0
SBIT(CD_BSY     ,CD_STATC_BASE,BIT0);
SBIT(DMAP       ,CD_STATC_BASE,BIT4);
SBIT(EOC        ,CD_STATC_BASE,BIT2);
SBIT(ERROR      ,CD_STATC_BASE,BIT1);
SBIT(INT_EN     ,CD_STATC_BASE,BIT3);
SBIT(KEEPX      ,CD_STATC_BASE,BIT5);
SBIT(KEEPY      ,CD_STATC_BASE,BIT6);
SBIT(KEEPZ      ,CD_STATC_BASE,BIT7);
#undef CD_STATC_BASE    

//   IEN0
#define IEN0_BASE    0xA8
SBIT(EA         ,IEN0_BASE,BIT7);
SBIT(ES         ,IEN0_BASE,BIT4);
SBIT(ET0        ,IEN0_BASE,BIT1);
SBIT(ET1        ,IEN0_BASE,BIT3);
SBIT(ET2        ,IEN0_BASE,BIT5);
SBIT(EX0        ,IEN0_BASE,BIT0);
SBIT(EX1        ,IEN0_BASE,BIT2);
#undef IEN0_BASE    

//   IEN1
#define IEN1_BASE    0xE8
SBIT(EADC       ,IEN1_BASE,BIT0);
SBIT(ECCIP0     ,IEN1_BASE,BIT4);
SBIT(ECCIP1     ,IEN1_BASE,BIT5);
SBIT(ECCIP2     ,IEN1_BASE,BIT6);
SBIT(ECCIP3     ,IEN1_BASE,BIT7);
SBIT(ESSC       ,IEN1_BASE,BIT1);
SBIT(EX2        ,IEN1_BASE,BIT2);
SBIT(EXM        ,IEN1_BASE,BIT3);
#undef IEN1_BASE    

//   IP1
#define IP1_BASE    0xF8
SBIT(PADC       ,IP1_BASE,BIT0);
SBIT(PCCIP0     ,IP1_BASE,BIT4);
SBIT(PCCIP1     ,IP1_BASE,BIT5);
SBIT(PCCIP2     ,IP1_BASE,BIT6);
SBIT(PCCIP3     ,IP1_BASE,BIT7);
SBIT(PSSC       ,IP1_BASE,BIT1);
SBIT(PX2        ,IP1_BASE,BIT2);
SBIT(PXM        ,IP1_BASE,BIT3);
#undef IP1_BASE    

//   IP
#define IP_BASE    0xB8
SBIT(PS         ,IP_BASE,BIT4);
SBIT(PT0        ,IP_BASE,BIT1);
SBIT(PT1        ,IP_BASE,BIT3);
SBIT(PT2        ,IP_BASE,BIT5);
SBIT(PX0        ,IP_BASE,BIT0);
SBIT(PX1        ,IP_BASE,BIT2);
#undef IP_BASE    

//   MDU_MDUSTAT
#define MDU_MDUSTAT_BASE    0xB0
SBIT(IERR       ,MDU_MDUSTAT_BASE,BIT1);
SBIT(IRDY       ,MDU_MDUSTAT_BASE,BIT0);
SBIT(MDU_BSY    ,MDU_MDUSTAT_BASE,BIT2);
#undef MDU_MDUSTAT_BASE    

//   PSW
#define PSW_BASE    0xD0
SBIT(AC         ,PSW_BASE,BIT6);
SBIT(CY         ,PSW_BASE,BIT7);
SBIT(F0         ,PSW_BASE,BIT5);
SBIT(F1         ,PSW_BASE,BIT1);
SBIT(OV         ,PSW_BASE,BIT2);
SBIT(P          ,PSW_BASE,BIT0);
SBIT(RS0        ,PSW_BASE,BIT3);
SBIT(RS1        ,PSW_BASE,BIT4);
#undef PSW_BASE    

//   SCON
#define SCON_BASE    0x98
SBIT(RB8        ,SCON_BASE,BIT2);
SBIT(REN        ,SCON_BASE,BIT4);
SBIT(RI         ,SCON_BASE,BIT0);
SBIT(SM0        ,SCON_BASE,BIT7);
SBIT(SM1        ,SCON_BASE,BIT6);
SBIT(SM2        ,SCON_BASE,BIT5);
SBIT(TB8        ,SCON_BASE,BIT3);
SBIT(TI         ,SCON_BASE,BIT1);
#undef SCON_BASE    

//   T2CCU_COCON
#define T2CCU_COCON_BASE    0xC0
SBIT(CCM4       ,T2CCU_COCON_BASE,BIT6);
SBIT(CCM5       ,T2CCU_COCON_BASE,BIT7);
SBIT(CM4F       ,T2CCU_COCON_BASE,BIT4);
SBIT(CM5F       ,T2CCU_COCON_BASE,BIT5);
SBIT(COMOD0     ,T2CCU_COCON_BASE,BIT0);
SBIT(COMOD1     ,T2CCU_COCON_BASE,BIT1);
SBIT(POLA       ,T2CCU_COCON_BASE,BIT2);
SBIT(POLB       ,T2CCU_COCON_BASE,BIT3);
#undef T2CCU_COCON_BASE    

//   T2CCU_COSHDW
#define T2CCU_COSHDW_BASE    0xC0
SBIT(COOUT0     ,T2CCU_COSHDW_BASE,BIT0);
SBIT(COOUT1     ,T2CCU_COSHDW_BASE,BIT1);
SBIT(COOUT2     ,T2CCU_COSHDW_BASE,BIT2);
SBIT(COOUT3     ,T2CCU_COSHDW_BASE,BIT3);
SBIT(COOUT4     ,T2CCU_COSHDW_BASE,BIT4);
SBIT(COOUT5     ,T2CCU_COSHDW_BASE,BIT5);
SBIT(ENSHDW     ,T2CCU_COSHDW_BASE,BIT7);
SBIT(TXOV       ,T2CCU_COSHDW_BASE,BIT6);
#undef T2CCU_COSHDW_BASE    

//   T2_T2CON and T21_T2CON
#define T2_T2CON_BASE    0xC0
SBIT(C_T2       ,T2_T2CON_BASE,BIT1);
SBIT(CP_RL2     ,T2_T2CON_BASE,BIT0);
SBIT(EXEN2      ,T2_T2CON_BASE,BIT3);
SBIT(EXF2       ,T2_T2CON_BASE,BIT6);
SBIT(TF2        ,T2_T2CON_BASE,BIT7);
SBIT(TR2        ,T2_T2CON_BASE,BIT2);
#undef T2_T2CON_BASE    

//   TCON
#define TCON_BASE    0x88
SBIT(IE0        ,TCON_BASE,BIT1);
SBIT(IE1        ,TCON_BASE,BIT3);
SBIT(IT0        ,TCON_BASE,BIT0);
SBIT(IT1        ,TCON_BASE,BIT2);
SBIT(TF0        ,TCON_BASE,BIT5);
SBIT(TF1        ,TCON_BASE,BIT7);
SBIT(TR0        ,TCON_BASE,BIT4);
SBIT(TR1        ,TCON_BASE,BIT6);
#undef TCON_BASE    

//   UART1_SCON
#define UART1_SCON_BASE    0xC8
SBIT(RB8_1      ,UART1_SCON_BASE,BIT2);
SBIT(REN_1      ,UART1_SCON_BASE,BIT4);
SBIT(RI_1       ,UART1_SCON_BASE,BIT0);
SBIT(SM0_1      ,UART1_SCON_BASE,BIT7);
SBIT(SM1_1      ,UART1_SCON_BASE,BIT6);
SBIT(SM2_1      ,UART1_SCON_BASE,BIT5);
SBIT(TB8_1      ,UART1_SCON_BASE,BIT3);
SBIT(TI_1       ,UART1_SCON_BASE,BIT1);
#undef UART1_SCON_BASE    


//   Definition of the 16-bit SFR
//   sfr16 data type to access two 8-bit SFRs as a single 16-bit SFR.

SFR16( ADC_RESR0LH, 0xCA);       // 16-bit Address
SFR16( ADC_RESR1LH, 0xCC);       // 16-bit Address
SFR16( ADC_RESR2LH, 0xCE);       // 16-bit Address
SFR16( ADC_RESR3LH, 0xD2);       // 16-bit Address
SFR16( ADC_RESRA0LH, 0xCA);      // 16-bit Address
SFR16( ADC_RESRA1LH, 0xCC);      // 16-bit Address
SFR16( ADC_RESRA2LH, 0xCE);      // 16-bit Address
SFR16( ADC_RESRA3LH, 0xD2);      // 16-bit Address
SFR16( CAN_ADLH, 0xD9);          // 16-bit Address
SFR16( CAN_DATA01, 0xDB);        // 16-bit Address
SFR16( CAN_DATA23, 0xDD);        // 16-bit Address
SFR16( CCU6_CC60RLH, 0xFA);      // 16-bit Address
SFR16( CCU6_CC60SRLH, 0xFA);     // 16-bit Address
SFR16( CCU6_CC61RLH, 0xFC);      // 16-bit Address
SFR16( CCU6_CC61SRLH, 0xFC);     // 16-bit Address
SFR16( CCU6_CC62RLH, 0xFE);      // 16-bit Address
SFR16( CCU6_CC62SRLH, 0xFE);     // 16-bit Address
SFR16( CCU6_CC63RLH, 0x9A);      // 16-bit Address
SFR16( CCU6_CC63SRLH, 0x9A);     // 16-bit Address
SFR16( CCU6_T12LH, 0xFA);        // 16-bit Address
SFR16( CCU6_T12PRLH, 0x9C);      // 16-bit Address
SFR16( CCU6_T13LH, 0xFC);        // 16-bit Address
SFR16( CCU6_T13PRLH, 0x9E);      // 16-bit Address
SFR16( CD_CORDXLH, 0x9A);        // 16-bit Address
SFR16( CD_CORDYLH, 0x9C);        // 16-bit Address
SFR16( CD_CORDZLH, 0x9E);        // 16-bit Address
SFR16( MDU_MD01, 0xB2);          // 16-bit Address
SFR16( MDU_MD23, 0xB4);          // 16-bit Address
SFR16( MDU_MD45, 0xB6);          // 16-bit Address
SFR16( MDU_MR01, 0xB2);          // 16-bit Address
SFR16( MDU_MR23, 0xB4);          // 16-bit Address
SFR16( MDU_MR45, 0xB6);          // 16-bit Address
SFR16( T21_RC2LH, 0xC2);         // 16-bit Address
SFR16( T21_T2LH, 0xC4);          // 16-bit Address
SFR16( T2CCU_CC0LH, 0xC1);       // 16-bit Address
SFR16( T2CCU_CC1LH, 0xC3);       // 16-bit Address
SFR16( T2CCU_CC2LH, 0xC5);       // 16-bit Address
SFR16( T2CCU_CC3LH, 0xC1);       // 16-bit Address
SFR16( T2CCU_CC4LH, 0xC3);       // 16-bit Address
SFR16( T2CCU_CC5LH, 0xC5);       // 16-bit Address
SFR16( T2CCU_CCTDTCLH, 0xC2);    // 16-bit Address
SFR16( T2CCU_CCTLH, 0xC4);       // 16-bit Address
SFR16( T2CCU_CCTRELLH, 0xC2);    // 16-bit Address
SFR16( T2_RC2LH, 0xC2);          // 16-bit Address
SFR16( T2_T2LH, 0xC4);           // 16-bit Address


//   Definition of the PAGE SFR

//   PORT_PAGE
#define _pp0 PORT_PAGE=0 // PORT_PAGE postfix
#define _pp1 PORT_PAGE=1 // PORT_PAGE postfix
#define _pp2 PORT_PAGE=2 // PORT_PAGE postfix
#define _pp3 PORT_PAGE=3 // PORT_PAGE postfix

//   ADC_PAGE
#define _ad0 ADC_PAGE=0 // ADC_PAGE postfix
#define _ad1 ADC_PAGE=1 // ADC_PAGE postfix
#define _ad2 ADC_PAGE=2 // ADC_PAGE postfix
#define _ad3 ADC_PAGE=3 // ADC_PAGE postfix
#define _ad4 ADC_PAGE=4 // ADC_PAGE postfix
#define _ad5 ADC_PAGE=5 // ADC_PAGE postfix
#define _ad6 ADC_PAGE=6 // ADC_PAGE postfix

//   SCU_PAGE
#define _su0 SCU_PAGE=0 // SCU_PAGE postfix
#define _su1 SCU_PAGE=1 // SCU_PAGE postfix
#define _su2 SCU_PAGE=2 // SCU_PAGE postfix
#define _su3 SCU_PAGE=3 // SCU_PAGE postfix

//   CCU6_PAGE
#define _cc0 CCU6_PAGE=0 // CCU6_PAGE postfix
#define _cc1 CCU6_PAGE=1 // CCU6_PAGE postfix
#define _cc2 CCU6_PAGE=2 // CCU6_PAGE postfix
#define _cc3 CCU6_PAGE=3 // CCU6_PAGE postfix

//   T2_PAGE
#define _t2_0 T2_PAGE=0 // T2_PAGE postfix
#define _t2_1 T2_PAGE=1 // T2_PAGE postfix
#define _t2_2 T2_PAGE=2 // T2_PAGE postfix
#define _t2_3 T2_PAGE=3 // T2_PAGE postfix
#define _t2_4 T2_PAGE=4 // T2_PAGE postfix

#define SST0  0x80        // Save SFR page to ST0
#define RST0  0xC0        // Restore SFR page from ST0
#define SST1  0x90        // Save SFR page to ST1
#define RST1  0xD0        // Restore SFR page from ST1
#define SST2  0xA0        // Save SFR page to ST2
#define RST2  0xE0        // Restore SFR page from ST2
#define SST3  0xB0        // Save SFR page to ST3
#define RST3  0xF0        // Restore SFR page from ST3
#define noSST 0x00        // Switch page without saving

#define SFR_PAGE(pg,op) pg+op

//   SYSCON0_RMAP
//   The access to the mapped SFR area is enabled.
#define SET_RMAP() SYSCON0 |= 0x01

//   The access to the standard SFR area is enabled.
#define RESET_RMAP() SYSCON0 &= ~0x01


#define _su  SCU_PAGE // SCU_PAGE

#ifdef __C51__
#define STR_PAGE(pg,op)  { _push_(op); \
   pg ; }

#define RST_PAGE(op)  _pop_(op)
#endif  // __C51__

//****************************************************************************
// @Typedefs
//****************************************************************************

// USER CODE BEGIN (MAIN_Header,6)

// USER CODE END


//****************************************************************************
// @Imported Global Variables
//****************************************************************************

// USER CODE BEGIN (MAIN_Header,7)

// USER CODE END


//****************************************************************************
// @Global Variables
//****************************************************************************

// USER CODE BEGIN (MAIN_Header,8)

// USER CODE END


//****************************************************************************
// @Prototypes Of Global Functions
//****************************************************************************

// USER CODE BEGIN (MAIN_Header,9)

// USER CODE END


//****************************************************************************
// @Interrupt Vectors
//****************************************************************************

// USER CODE BEGIN (MAIN_Header,10)

// USER CODE END


//****************************************************************************
// @Project Includes
//****************************************************************************



#ifdef __C51__
#include <intrins.h>
#endif







// USER CODE BEGIN (MAIN_Header,11)

// USER CODE END


#endif  // ifndef _MAIN_H_
