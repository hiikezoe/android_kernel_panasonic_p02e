/*
 * drivers/fgc/pfgc_info.c
 *
 * Copyright(C) 2010 Panasonic Mobile Communications Co., Ltd.
 * Author: Naoto Suzuki <suzuki.naoto@kk.jp.panasonic.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*----------------------------------------------------------------------------*/
/* include                                                                    */
/*----------------------------------------------------------------------------*/
#include "inc/pfgc_local.h"                      /*         DD                */

/* Jessie add start */
#if 1 /*RUPY*/
#define GPIO_CHG_STS (72)
#else
#define GPIO_CHG_STS (107)
#endif /*RUPY*/
#define WIRELESS_ON  0
#define WIRELESS_OFF 1
/* Jessie add end */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1201XX*/
extern unsigned char degra_batt_select_val; /*                 [%] *//*<Conan>1201XX*/
#endif /*<Conan>1201XX*/

int pfgc_fgic_reset_flg = 0;   /* Quad add <npdc300085229> */
int pfgc_fgic_is_reseting = 0; /* Quad add <npdc300085229> */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

/******************************************************************************/
/*                pfgc_info_init                                              */
/*                                                                            */
/*                2008.10.17                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_OK                                                     */
/*                DFGC_NG                                                     */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
unsigned char pfgc_info_init( void )
{
    unsigned char ret;
    int mod_ret;
    T_FGC_WAIT_INFO pfgc_info;

    MFGC_RDPROC_PATH( DFGC_INFO_INIT | 0x0001 );

    ret = DFGC_OK;

    spin_lock_init( &gfgc_spinlock_if );         /*  V                        */
    spin_lock_init( &gfgc_spinlock_info );       /*  V                        */
    spin_lock_init( &gfgc_spinlock_log );        /*  V                        */
    spin_lock_init( &gfgc_spinlock_fgc );        /*<1905359>  V               */

    gfgc_int_flag = 0;                           /*<3210076>                  */

    pfgc_init_timer();
    pfgc_info.pfunc = pfgc_fgc_nvm_get;
    pfgc_info.pnext = NULL;
    mod_ret = pfgc_exec_retry(&pfgc_info);
    if( mod_ret < 0 )
    {
        FGC_ERR("pfgc_exec_retry(pfgc_fgc_nvm_get) failed. ret=%d\n", mod_ret);
    }

    pfgc_log_ocv_init();                         /*  V OCV                    */

    if( gfgc_ctl.remap_ocv->fgc_ocv_data[ 0 ].ocv
                           != DFGC_OFF )
    {
        pfgc_info.pfunc = pfgc_log_ocv;
        pfgc_info.pnext = NULL;
        mod_ret = pfgc_exec_retry(&pfgc_info);
        if( mod_ret < 0 )
        {
            FGC_ERR("pfgc_exec_retry(pfgc_log_ocv) failed. ret=%d\n", mod_ret);
        }
    }

    pfgc_log_dump_init();                        /*  V FG-IC                  */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>#if DFGC_SW_DELETED_FUNC*/
    pfgc_info_health_init();                     /*  V                        */
/*<Conan>#endif*/ /* DFGC_SW_DELETED_FUNC */
#else /*<Conan>1111XX*/
#if DFGC_SW_DELETED_FUNC
    pfgc_info_health_init();                     /*  V                        */
#endif /* DFGC_SW_DELETED_FUNC */
#endif /*<Conan>1111XX*/
    gfgc_ctl.mac_state     = DCTL_STATE_POWOFF;  /*                           */
    gfgc_ctl.chg_sts       = DCTL_CHG_NO_CHARGE; /*                           */
    gfgc_ctl.dev1_chgsts   = BAT_NO_CHARGE;      /*                           */
    gfgc_ctl.dev1_volt     = DFGC_DEV1_VOLT_INIT;/*                           */
    gfgc_ctl.dev1_class    = 0;                  /* RF                        */
    gfgc_ctl.capacity_old  = DFGC_CAPACITY_MAX;  /*           100%            */
/*<3131029>pfgc_info_health_init()          */
/*<3131029>    gfgc_ctl.health_old    = DFGC_HEALTH_MEASURE;*//*                           */
    gfgc_ctl.flag          = DFGC_OFF;           /*                           */
    gfgc_ctl.chg_start     = DFGC_OFF;           /*                           */

    gfgc_batt_info.capacity                      /*           100%            */
                    = DFGC_CAPACITY_MAX;
/*<3131029>pfgc_info_health_init()          */
/*<3131029>    gfgc_batt_info.health                        *//*                           */
/*<3131029>                    = DFGC_HEALTH_MEASURE;*/
    gfgc_batt_info.level                         /*                           */
                    = DCTL_BLVL_LVL3;
    gfgc_batt_info.batt_status                   /*                           */
                    = BAT_ST_INT;
    gfgc_batt_info.rest_batt                     /*           100%            */
                    = DCTL_BAT_PAR_FULL;
    gfgc_batt_info.rest_time                     /*               1000min     */
                    = DCTL_BAT_MIN_FULL;
    gfgc_batt_info.temp                          /*       24.55               */
                    = DFGC_SOH_TEMP_INIT;
    gfgc_batt_info.volt                          /*           4127mV          */
                    = DFGC_SOH_VOLT_INIT;
    gfgc_batt_info.nac                           /*           800mA           */
                    = DFGC_SOH_CAPACITY_INIT;
    gfgc_batt_info.fac                           /*           800mA           */
                    = DFGC_SOH_CAPACITY_INIT;
    gfgc_batt_info.rm                            /*           800mA           */
                    = DFGC_SOH_CAPACITY_INIT;
    gfgc_batt_info.fcc                           /*           800mA           */
                    = DFGC_SOH_CAPACITY_INIT;
    gfgc_batt_info.ai                            /* 1s            -51mA       */
                    = DFGC_AI_INIT;
    gfgc_batt_info.tte                           /*             65535min      */
                    = DFGC_SOH_TTE_INIT;
    gfgc_batt_info.ttf                           /*             65535min      */
                    = DFGC_SOH_TTF_INIT;
    gfgc_batt_info.soh                           /*           100%            */
                    = DFGC_SOH_LEVEL_INIT;
    gfgc_batt_info.cc                            /*                           */
                    = 0;
    gfgc_batt_info.soc                           /*           100%            */
                    = DCTL_BAT_PAR_FULL;
//Jessie add start
    gfgc_chgsts_thread = DCTL_CHG_NO_CHARGE;
//Jessie add end

    MFGC_RDPROC_PATH( DFGC_INFO_INIT | 0xFFFF );
    return ret;
}

/******************************************************************************/
/*                pfgc_info_update                                            */
/*                                                                            */
/*                2008.10.20                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DFGC_ON                                                     */
/*                DFGC_OFF                                                    */
/*                unsigned char type                                          */
/*                                                                            */
/******************************************************************************/
unsigned long pfgc_info_update( unsigned char type )
{
    TCOM_RW_DATA     com_acc_data;
    t_dctl_batinfo   dctl_batinfo;
    T_FGC_REGDUMP    regdump;
    int              ret;
    unsigned long    wakeup;
    T_FGC_SYSERR_DATA syserr_info;               /* SYS                       */
    unsigned long    regget;                     /*                           */
    unsigned long    flags;                      /*<3210076>                  */
/* IDPower #12   volatile void __iomem *reg_addr;             *//*                           */
    volatile unsigned int  gpio_reg;             /* GPIO                      */
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
    T_FGC_SOH      *t_soh;                       /*                           *//*<Conan>*/
#endif /*<Conan>1111XX*/
//Jessie add start
    int wireless_charge = WIRELESS_OFF;
//Jessie add end

//    DEBUG_FGIC(("pfgc_info_update start\n"));

     MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0000 );

    wakeup = DFGC_OFF;                           /*                           */
    regget = DFGC_OFF;                           /*                           */

                                                 /* FGC                       */
    if( gfgc_ctl.update_enable == DFGC_OFF )
    {
        MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0001 );
        return wakeup;
    }

    down( &gfgc_sem_update );                    /*<1905359>  V               */

/*<3210076>    if( type != D_FGC_MSTATE_IND )               *//* D_FGC_MSTATE_IND          */
/*<1905359>    if(( type == D_FGC_FG_INIT_IND ) ||          *//*<3210076>                  */
/*<1905359>       ( type == D_FGC_FG_INT_IND ))             *//*<3210076> FGIC             */
    if(( ( type & D_FGC_FG_INIT_IND )            /*<1905359>                  */
         == D_FGC_FG_INIT_IND ) ||               /*<1905359> FGIC             */
       ( ( type & D_FGC_FG_INT_IND )             /*<1905359>                  */
         == D_FGC_FG_INT_IND ))                  /*<1905359>                  */
    {
	int i;
	for(i = 0; i < 3; i++ )
	{
		MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0003 );
		memset( &regdump, 0x00, sizeof(T_FGC_REGDUMP));

		com_acc_data.offset = ( unsigned long * )gfgc_fgic_offset;
		com_acc_data.data   = ( void * )&regdump;
		com_acc_data.number = DFGC_REGGET_MAX;
		com_acc_data.size   = 1;
		com_acc_data.option = NULL;

		ret = pfgc_fgc_regget( type, &com_acc_data ); /*  V FG-IC             */
		if( ret != 0 )
		{
			pfgc_log_i2c_error(PFGC_INFO_UPDATE + 0x01, 0);
			MFGC_RDPROC_ERROR( DFGC_INFO_UPDATE | 0x0001 );
			FGC_ERR("pfgc_fgc_regget() failed. ret=%d\n", ret);
		}
		else
		{
			break;
		}
	}
/*npdc30003452 add comment *//*if ret != 0 retun this func (after dec gfgc_int_flag)*/

        spin_lock_irqsave(                       /*<3210076>                  */
            &gfgc_spinlock_info, flags );        /*<3210076>                  */
                                                 /*<3210076>                  */
#if 1 /* IDPower #12 */
    	gpio_reg = 0;
#else
        reg_addr = pfgc_get_reg_addr(OMAP4_GPIO_IRQSTATUS0);
        gpio_reg = *(( volatile unsigned int * ) reg_addr );
#endif
                                                 /*<3210076>                  */
/*<1905359>        if( type == D_FGC_FG_INT_IND )           *//*<3210076>                  */
        if( ( type & D_FGC_FG_INT_IND )          /*<1905359>                  */
            == D_FGC_FG_INT_IND)                 /*<1905359> (            )   */
        {                                        /*<3210076>                  */
            gfgc_int_flag--;                     /*<3210076>                  */
                                                 /*<3210076>                  */
#if 0 /* IDPower */
            /* FGIC                            *//*<3210076>                  */
            /* (                               *//*<3210076>                  */
            /*                          )      *//*<3210076>                  */
            reg_addr = pfgc_get_reg_addr(OMAP4_GPIO_IRQSTATUSSET0);
            *(( volatile unsigned int * ) reg_addr )
                = DFGC_MASK_GPIO_FGC_PORT;
#endif
        }                                        /*<3210076>                  */
                                                 /*<3210076>                  */
        spin_unlock_irqrestore(                  /*<3210076>                  */
            &gfgc_spinlock_info, flags );        /*<3210076>                  */
                                                 /*<3210076>                  */

/*npdc30003452 add start*//*fail safe*//*          */
    	if(ret != 0)
    	{
	    up( &gfgc_sem_update );
	    FGC_ERR("REGGET() failed x times.(ret=%d) so return this func\n", ret);
	    MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0xFFF0 );
	    return wakeup;
    	}
/*npdc300035452 add end *//*fail safe*/

        if((( gpio_reg & DFGC_MASK_GPIO_FGC_PORT ) /*                         */
               == DFGC_MASK_GPIO_FGC_PORT ) &&     /*                         */
            (( type & D_FGC_FG_INT_IND )           /*                         */
               == D_FGC_FG_INT_IND ))
        {                                        /*<3210076>                  */
            MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0012 );         /*<3210076>*/
            pfgc_log_dump( ( type | 0x80 ),        /*  V FG-IC                */
                           &regdump );             /* (                      )*/
            regget = DFGC_OFF;                   /*<3210076> regget           */
        	DEBUG_FGIC(("regget = DFGC_OFF\n"));
        }                                        /*<3210076>                  */
        else                                     /*<3210076>                  */
        {                                        /*<3210076>                  */
            MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0013 );         /*<3210076>*/

        /* <npdc300054685> start */
        if(( regdump.cntl_l & DFGC_INIT_COMPLETE ) != DFGC_INIT_COMPLETE )
        {
            up( &gfgc_sem_update );
            MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0xFFFF );
            return wakeup;
        }
        /* <npdc300054685> end */

        gfgc_batt_info.temp                      /*                           */
            = ( unsigned short )((( unsigned short )regdump.temp_h << 8 )
                | regdump.temp_l );
        gfgc_batt_info.volt                      /*                           */
            = ( unsigned short )((( unsigned short )regdump.volt_h << 8 )
                | regdump.volt_l );
        gfgc_batt_info.nac                       /*                           */
            = ( unsigned short )((( unsigned short )regdump.nac_h << 8 )
                | regdump.nac_l );
        gfgc_batt_info.fac                       /*                           */
            = ( unsigned short )((( unsigned short )regdump.fac_h << 8 )
                | regdump.fac_l );
        gfgc_batt_info.rm                        /*                           */
            = ( unsigned short )((( unsigned short )regdump.rm_h << 8 )
                | regdump.rm_l );
        gfgc_batt_info.fcc                       /*                           */
            = ( unsigned short )((( unsigned short )regdump.fcc_h << 8 )
                | regdump.fcc_l );
        gfgc_batt_info.ai                        /*                           */
            = ( unsigned short )((( unsigned short )regdump.ai_h << 8 )
                | regdump.ai_l );
        gfgc_batt_info.tte                       /*                           */
            = ( unsigned short )((( unsigned short )regdump.tte_h << 8 )
                | regdump.tte_l );
        gfgc_batt_info.ttf                       /*                           */
            = ( unsigned short )((( unsigned short )regdump.ttf_h << 8 )
                | regdump.ttf_l );
        gfgc_batt_info.soh                       /*             %             */
            = ( unsigned short )regdump.soh;
        gfgc_batt_info.cc                        /*                           */
/*<QAIO028>            = ( unsigned short )((( unsigned short )regdump.cc_h << 8 )*/
/*<QAIO028>                | regdump.cc_l );                                  */
            = regdump.mstate;                    /*<QAIO028>                  */
        gfgc_batt_info.soc                       /*           %               */
            = ( unsigned short )((( unsigned short )regdump.soc_h << 8 )
                | regdump.soc_l );

/*<npdc300029319>        gfgc_batt_info.capacity = gfgc_batt_info.soc; *//*<npdc300020757>*/

        regget = DFGC_ON;                        /*                           */
        DEBUG_FGIC(("regget = DFGC_ON, gfgc_batt_info.soc = 0x%x\n", gfgc_batt_info.soc));

#if 0 /* IDPower */
        if(( type & D_FGC_FG_INT_IND )           /* interrupt only            */
            == D_FGC_FG_INT_IND)
        {
            MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0016 );
            FGC_CHG("set_battery_capacity(%d)\n", gfgc_batt_info.soc);
            ret = set_battery_capacity( (int)gfgc_batt_info.soc );
            if(ret != 0)
            {
                MFGC_RDPROC_ERROR( DFGC_INFO_UPDATE | 0x0002 );
                FGC_ERR("set_battery_capacity() failed. ret=%d\n", ret);
            }
        }
#endif /* IDPower */

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>#if DFGC_SW_DELETED_FUNC*/
        t_soh = gfgc_ctl.remap_soh;              /*                           *//*<Conan>*/

                                                 /*                           *//*<Conan>*/
        pfgc_log_time_ex( &t_soh->soh_system_timestamp,                         /*<Conan>*/
                          &t_soh->soh_monotonic_timestamp);                     /*<Conan>*/
        
        gfgc_batt_info.health
            = pfgc_info_health_check( &regdump );/*  V                        */

/*<Conan>        pfgc_log_health( gfgc_batt_info.health );*//*  V                        */
        if( gfgc_batt_info.health !=             /*                           *//*<Conan>*/
                DFGC_HEALTH_MEASURE )                                           /*<Conan>*/
        {                                                                       /*<Conan>*/
            pfgc_log_health(DFGC_OFF);           /*  V                        *//*<Conan>*/
        }                                                                       /*<Conan>*/
/*<Conan>#endif*/ /* DFGC_SW_DELETED_FUNC */
#else
#if DFGC_SW_DELETED_FUNC
        gfgc_batt_info.health
            = pfgc_info_health_check( &regdump );/*  V                        */

        pfgc_log_health( gfgc_batt_info.health );/*  V                        */
#endif /* DFGC_SW_DELETED_FUNC */
#endif
        }                                        /*<3210076>                  */

    }

    dctl_batinfo.level =                         /*                           */
        pfgc_info_batt_level(( unsigned char ) gfgc_batt_info.soc );/*  V     */

    dctl_batinfo.det1 = DCTL_OFF;                /* DET1                      */

    if( regget == DFGC_ON )                      /*                           */
    {
                                                 /* DET1                      */
        if(( gfgc_ctl.mac_state                  /*     OFF                   */
                    == DCTL_STATE_POWOFF )
        && ( regdump.soc_l == DFGC_CAPACITY_MIN )/*                           */
        && (( regdump.flags_l & DFGC_IC_FLAGS_SYSDOWN ) /* SysDownSetTreshold */
                    == DFGC_IC_FLAGS_SYSDOWN ))  /*                           */
        {
            MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0002 );
            dctl_batinfo.det1 = DCTL_ON;         /* DET1                      */
        }
    }                                            /*<3490001>                  */

                                                 /*<3490001>                  */
                                                 /*<3490001>                  */
/*<3490001>        dctl_batinfo.capacity                    *//*                           */
/*<3490001>            = ( unsigned char )gfgc_batt_info.soc;*/
    if(( gfgc_ctl.capacity_fix                   /*<3490001>                  */
             == DFGC_CAPACITY_FIX_ENA ) &&       /*<3490001>                  */
       ( dctl_batinfo.level                      /*<3490001>                  */
             != DCTL_BLVL_ALM ))                 /*<3490001> LVA              */
    {                                            /*<3490001>                  */
        MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0010 ); /*<3490001>            */
        dctl_batinfo.capacity                    /*<3490001>                  */
            = DFGC_CAPACITY_MAX;                 /*<3490001>                  */
        dctl_batinfo.level = DCTL_BLVL_LVL3;     /*<3490001>                  */
    }                                            /*<3490001>                  */
    else                                         /*<3490001>                  */
    {                                            /*<3490001>                  */
        MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0011 ); /*<3490001>            */
        dctl_batinfo.capacity                    /*<3490001>                  */
            = ( unsigned char )gfgc_batt_info.soc; /*<3490001>                */
    }                                            /*<3490001>                  */

        ret = pdctl_mstate_batget( &dctl_batinfo );/*  C         DD            */
        if( ret != DCTL_OK )                     /*     NG                    */
        {
            MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0004 );
            gfgc_ctl.dev1_volt                   /*                           */
                        = DFGC_DEV1_VOLT_INIT;
            gfgc_batt_info.level                 /*                           */
                        = DCTL_BLVL_LVL3;
            gfgc_batt_info.rest_batt             /* MTF                       */
                        = DCTL_BAT_PAR_FULL;
            gfgc_batt_info.rest_time             /* MTF                       */
                        = DCTL_BAT_MIN_FULL;
            syserr_info.kind                     /* SYSERR                    */
                        = CSYSERR_ALM_RANK_B;
            syserr_info.func_no
                        = DFGC_SYSERR_BATGET_ERR;
            pfgc_log_syserr( &syserr_info );     /*  V                        */
        }
        else                                     /*     OK                    */
        {
            MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0005 );
            gfgc_ctl.dev1_volt                   /*                           */
                        = dctl_batinfo.adcnv_var;
            gfgc_batt_info.level                 /*                           */
                        = dctl_batinfo.level;
            gfgc_batt_info.rest_batt             /* MTF                       */
                        = dctl_batinfo.rest_batt;
            gfgc_batt_info.rest_time             /* MTF                       */
                        = dctl_batinfo.rest_time;
        }

/*<3210076>    if( regget == DFGC_ON )         *//*<3490001>                  */
    if(( regget == DFGC_ON ) ||                  /*<3210076>regget     or     */
/*<1905359>       ( type   == D_FGC_CHG_INT_IND ) ||        *//*<3210076>             or   */
/*<1905359>       ( type   == D_FGC_RF_CLASS_IND ))         *//*<3210076>RF Class          */
       (( type & D_FGC_CHG_INT_IND )             /*<1905359>             or   */
        == D_FGC_CHG_INT_IND ) ||                /*<1905359>                  */
       (( type & D_FGC_RF_CLASS_IND )            /*<1905359>RF Class          */
        == D_FGC_RF_CLASS_IND ))                 /*<1905359>                  */
    {                                            /*<3490001>                  */
        MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0015 );             /*<3210076>*/
                                                 /* UI                        */
        if( gfgc_ctl.capacity_fix                /*                           */
                 == DFGC_CAPACITY_FIX_ENA )      /*                           */
        {
            MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0006 );
            gfgc_batt_info.capacity              /*                           */
                        = DFGC_CAPACITY_MAX;
        }
        else                                     /*                           */
        {
            wireless_charge = gpio_get_value(GPIO_CHG_STS);                                        //Jessie add 

//Jessie del            if( gfgc_ctl.chg_sts                 /*                           */
//Jessie del                        == DCTL_CHG_CHARGE )
            if( (gfgc_chgsts_thread               /*                           *///Jessie add
                        == DCTL_CHG_CHARGE) ||                                                     //Jessie add
                (gfgc_chgsts_thread                                                                //Jessie add
                        == DCTL_CHG_WIRELESS_CHARGE) ||                                            //Jessie add
                ( (gfgc_chgsts_thread == DCTL_CHG_NO_CHARGE) &&                                    //Jessie add
                  (wireless_charge    == WIRELESS_ON       ) ) )                                   //Jessie add
            {
                                                 /*                           */
                                                 /*           100%            */
/*<3210076>                if( regdump.soc_l >= DFGC_CAPACITY_MAX )           */
                if( gfgc_batt_info.soc >= DFGC_CAPACITY_MAX )      /*<3210076>*/
                {
                                                 /*         100%              */
                                                 /* 100%                      */
                                                 /*           99%             */
                                                 /*                           */
                    /* IDPower:npdc300029319 start */
                    if( gfgc_ctl.capacity_old != DFGC_CAPACITY_MAX )
                    {
                        MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x000B );
                        gfgc_batt_info.capacity  /*           99%             */
                            = DFGC_CAPACITY_MAX - 1;
                    }
                    /* IDPower:npdc300029319 end */
                }
                else
                {
                    MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x000C );
                    gfgc_batt_info.capacity      /*                     /     */
/*<3210076>                            = regdump.soc_l;   *//*                           */
                        = gfgc_batt_info.soc;    /*<3210076>                  */
                }
            }
//Jessie del            else if( gfgc_ctl.chg_sts            /*                           */
//Jessie del                         == DCTL_CHG_CHARGE_FIN )
            else if( (gfgc_chgsts_thread          /*                           *///Jessie add
                         == DCTL_CHG_CHARGE_FIN) ||                                                  //Jessie add
                     (gfgc_chgsts_thread                                                             //Jessie add
                         == DCTL_CHG_WIRELESS_CHG_FIN) )                                             //Jessie add
            {
                MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0007 );
                gfgc_batt_info.capacity          /*                     /     */
/*<3210076>                            = regdump.soc_l;   *//*                           */
                    = gfgc_batt_info.soc;        /*<3210076>                  */
            }
            else                                 /*                           */
            {
                MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0008 );
/*<3210076>                if( regdump.soc_l             *//*                            */
/* Quad add start <npdc300085229> */
                /*                            */
                if(pfgc_fgic_is_reseting == 0){
/* Quad add end   <npdc300085229> */
                    if( gfgc_batt_info.soc           /*<3210076>                  */
                            < gfgc_ctl.capacity_old )
                    {
                        MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x0009 );
                        gfgc_batt_info.capacity      /*                           */
/*<3210076>                            = regdump.soc_l;                       */
                            = gfgc_batt_info.soc;    /*<3210076>                  */
                    }
/* Quad add start <npdc300085229> */
                    /*  */
                    else if(pfgc_fgic_reset_flg == 1){
                        /*  */
                        gfgc_batt_info.capacity = gfgc_batt_info.soc;
                    }
                }
/* Quad add end   <npdc300085229> */
            }
        }
/* Quad add start <npdc300085229> */
        if((pfgc_fgic_reset_flg == 1) && (pfgc_fgic_is_reseting == 0)){
            pfgc_fgic_reset_flg = 0;
        }
/* Quad add end   <npdc300085229> */

        pfgc_log_dump( type, &regdump );         /*  V FG-IC                  */

        pfgc_log_charge( type, &regdump );       /*  V                        */

        pfgc_log_capacity( type );               /*  V                        */
    }

    if(( gfgc_ctl.capacity_old                   /*                           */
                != gfgc_batt_info.capacity )
        || ( gfgc_ctl.health_old                 /*                           */
                != gfgc_batt_info.health ))
    {
        MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0x000A );
        wakeup = DFGC_ON;
    }

#if 1 /* IDPower:npdc300020757,npdc300029449,npdc300029319 */
/*<npdc300030059>	if (gfgc_ctl.capacity_old != gfgc_batt_info.capacity) { */
	if (((type & D_FGC_FG_INIT_IND) == D_FGC_FG_INIT_IND)    ||
		(((type & D_FGC_FG_INT_IND) == D_FGC_FG_INT_IND) &&
		 (gfgc_ctl.capacity_old != gfgc_batt_info.capacity)) ||
		(gfgc_batt_info.soc == 100)) {
		/* IDPower:npdc300034186 start */
		if((gfgc_ctl.no_lva == 1) && (gfgc_batt_info.soc <= DFGC_CAPACITY_LVA)){
			gfgc_batt_info.capacity = gfgc_ctl.get_capacity();
			pr_info("gfgc_batt_info.soc = 0x%x, gfgc_batt_info.capacity = 0x%x\n", gfgc_batt_info.soc, gfgc_batt_info.capacity);
		}
		/* IDPower:npdc300034186 end */
/* IDPower:npdc300021082		ret = pm8921_set_battery_capacity( gfgc_batt_info.capacity ); */
		ret = pm8921_set_battery_capacity( (int)gfgc_batt_info.soc, gfgc_batt_info.capacity ); /* IDPower:npdc300021082 */
		if(ret != 0) {
			MFGC_RDPROC_ERROR( DFGC_INFO_UPDATE | 0x0002 );
			FGC_ERR("set_battery_capacity() failed. ret=%d\n", ret);
		}
		gfgc_ctl.capacity_old = gfgc_batt_info.capacity;
	}
#endif /* IDPower:npdc300020757,npdc300029449,npdc300029319 */

    if (((type & D_FGC_FG_INIT_IND) == D_FGC_FG_INIT_IND)
//kawai        || ((type & D_FGC_FG_INT_IND) == D_FGC_FG_INT_IND))
        || (((type & D_FGC_FG_INT_IND) == D_FGC_FG_INT_IND) 
#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
        ))                                                                      /*<Conan>*/
/*<Conan>		&& (gfgc_ctl.capacity_old != gfgc_batt_info.capacity))) *///kawai
#else /*<Conan>1111XX*/
		&& (gfgc_ctl.capacity_old != gfgc_batt_info.capacity))) //kawai
#endif /*<Conan>1111XX*/
    {
	//kawai start

	if(gfgc_int_flag == 0) {
#if 0 /* IDPower:npdc300020757,npdc300029449 */
		pfgc_uevent_send(); //send an event to the framework
		ret = set_battery_capacity( (int)gfgc_batt_info.soc );
		if(ret != 0) {
			MFGC_RDPROC_ERROR( DFGC_INFO_UPDATE | 0x0002 );
			FGC_ERR("set_battery_capacity() failed. ret=%d\n", ret);
		}
#endif /* IDPower:npdc300020757,npdc300029449 */
	} else { //we read registers again because multiple interrupts occured.
/*<npdc100388275>		spin_lock_irqsave(&gfgc_spinlock_info, flags); */
		spin_lock_irqsave(&gfgc_spinlock_if, flags);      /*<npdc100388275>*/
		gfgc_kthread_flag |= D_FGC_FG_INT_IND;
/*<npdc100388275>		spin_unlock_irqrestore(&gfgc_spinlock_info, flags); */// SlimID Add
		spin_unlock_irqrestore(&gfgc_spinlock_if, flags); /*<npdc100388275>*/
		wake_up(&gfgc_task_wq);
// SlimID Del		spin_unlock_irqrestore(&gfgc_spinlock_info, flags);
	}
	//kawai end
    }

    up( &gfgc_sem_update );                      /*<1905359>  V               */

    MFGC_RDPROC_PATH( DFGC_INFO_UPDATE | 0xFFFF );
    
//    DEBUG_FGIC(("pfgc_info_update end\n"));

    return wakeup;
}

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/*<Conan>#if DFGC_SW_DELETED_FUNC*/
/******************************************************************************/
/*                pfgc_info_health_init                                       */
/*                                                                            */
/*                2008.10.17                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void pfgc_info_health_init( void )
{
    T_FGC_SYSERR_DATA syserr_info;               /* SYS                       */

    MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_INIT | 0x0000 );

                                                 /*                 WBB       */
    gfgc_ctl.remap_soh = ioremap_cache( DFGC_SDRAM_SOH, /*  C                 */
                                        sizeof( T_FGC_SOH ),
                                        CACHE_WBB );

    if( gfgc_ctl.remap_soh == NULL )
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_INIT | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_INFO_HEALTH_INIT | 0x0001 );
        MFGC_SYSERR_DATA_SET2(
                       CSYSERR_ALM_RANK_A,
                       DFGC_SYSERR_IOREMAP_ERR,
                       ( DFGC_INFO_HEALTH_INIT | 0x0001 ),
                       DFGC_SDRAM_SOH,
                       sizeof( T_FGC_SOH ),
                       syserr_info );
        pfgc_log_syserr( &syserr_info );         /*  V                        */
    }

    if( gfgc_ctl.remap_soh->soh_wp               /* soh_wp                    */
                    >= DFGC_SOH_DATA_MAX )
    {
        gfgc_ctl.remap_soh->soh_wp = 0;          /* soh_wp                    */
    }
                                                 /* OCV                       */
/*<3131029>    if( gfgc_ctl.remap_ocv->fgc_ocv_data[ 0 ].ocv != 0 )*/
    if(( gfgc_ctl.remap_ocv->fgc_ocv_data[ 0 ].ocv != 0 ) || /*<3131029>      */
       ( gfgc_ctl.ocv_enable != DFGC_ON ))       /*<3131029>OCV               */
    {
                                                 /*                           */
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_INIT | 0x0002 );
//        gfgc_ctl.remap_soh->soh_wp = 0; /*<Conan>1112XX*/
//        memset( ( void * )gfgc_ctl.remap_soh->soh, /*  V   D                  *//*<Conan>1112XX*/
//                gfgc_ctl.soh_init, /*<Conan>1112XX*/
//                DFGC_SOH_DATA_MAX ); /*<Conan>1112XX*/
//        gfgc_ctl.remap_soh->soh_old = DFGC_HEALTH_MEASURE; /*<Conan>1112XX*/
//        gfgc_ctl.remap_soh->soh_sum /*<Conan>1112XX*/
//                = ( unsigned short )( gfgc_ctl.soh_init * gfgc_ctl.soh_n ); /*<Conan>1112XX*/
//        gfgc_ctl.remap_soh->soh_ave = gfgc_ctl.soh_init; /*<Conan>1112XX*/
//        gfgc_ctl.remap_soh->soh_cnt = 0; /*<Conan>1112XX*/
//                                                 /*                           *//*<Conan>1112XX*/
//        gfgc_ctl.health_old = DFGC_HEALTH_MEASURE; /*<3131029>                *//*<Conan>1112XX*/
//        gfgc_batt_info.health = DFGC_HEALTH_MEASURE; /*<Conan>1112XX*/
    }
    else
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_INIT | 0x0003 );
        gfgc_ctl.health_old = gfgc_ctl.remap_soh->soh_old;
        gfgc_batt_info.health = gfgc_ctl.remap_soh->soh_old; /*<3131029>      */
    }

    { /* if            *//*<Conan>1112XX*/
        gfgc_ctl.remap_soh->soh_wp = 0; /*<Conan>1112XX*/
        memset( ( void * )gfgc_ctl.remap_soh->soh, DFGC_SOH_INIT_INIT, DFGC_SOH_DATA_MAX ); /*  V   D                  *//*<Conan>1112XX*/
        gfgc_ctl.remap_soh->soh_old = DFGC_HEALTH_MEASURE; /*<Conan>1112XX*/
        gfgc_ctl.remap_soh->soh_sum = ( unsigned short )( DFGC_SOH_INIT_INIT * DFGC_SOH_N_INIT ); /*<Conan>1112XX*/
        gfgc_ctl.remap_soh->soh_ave = DFGC_SOH_INIT_INIT; /*<Conan>1112XX*/
        gfgc_ctl.remap_soh->soh_cnt = 0; /*<Conan>1112XX*/
        gfgc_ctl.health_old = DFGC_HEALTH_MEASURE; /*<Conan>1112XX*/
        gfgc_batt_info.health = DFGC_HEALTH_MEASURE; /*<Conan>1112XX*/
    } /*<Conan>1112XX*/

    /*                                             :               */       /*<Conan>*/
    gfgc_ctl.save_soh_thres_timestamp = DFGC_SOH_VAL_INIT;                  /*<Conan>*/
    /*                                                   */                 /*<Conan>*/
    gfgc_ctl.remap_soh->degra_batt_measure_val                              /*<Conan>*/
        = DFGC_SOH_VAL_INIT;                                                /*<Conan>*/
    /*                                                   */                 /*<Conan>*/
    gfgc_ctl.remap_soh->soh_health                                          /*<Conan>*/
        = DFGC_SOH_VAL_INIT;                                                /*<Conan>*/
    /*                                                   */                 /*<Conan>*/
    gfgc_ctl.remap_soh->degra_batt_estimate_val                             /*<Conan>*/
        = DFGC_FGC_CNV_PERCENT * DFGC_FGC_10_TO_7_POWER;                    /*<Conan>*/
    /*                                                  */                  /*<Conan>*/
    gfgc_ctl.remap_soh->cycle_degra_batt_capa                               /*<Conan>*/
        = DFGC_FGC_CNV_PERCENT * DFGC_FGC_10_TO_7_POWER;                    /*<Conan>*/
    /*                                                  */                  /*<Conan>*/
    gfgc_ctl.remap_soh->soh_degra_batt_capa                                 /*<Conan>*/
        = DFGC_FGC_CNV_PERCENT * DFGC_FGC_10_TO_7_POWER;                    /*<Conan>*/
    /*                                                 */                   /*<Conan>*/
    gfgc_ctl.remap_soh->cycle_cnt                                           /*<Conan>*/
        = DFGC_SOH_VAL_INIT;                                                /*<Conan>*/
    /*                                                    */                /*<Conan>*/
    gfgc_ctl.remap_soh->soh_system_timestamp                                /*<Conan>*/
        = DFGC_SOH_VAL_INIT;                                                /*<Conan>*/
    /*                                                      */              /*<Conan>*/
    gfgc_ctl.remap_soh->soh_degra_save_timestamp                            /*<Conan>*/
        = DFGC_SOH_VAL_INIT;                                                /*<Conan>*/
    /*                                                    */                /*<Conan>*/
    gfgc_ctl.remap_soh->soh_thres_timestamp                                 /*<Conan>*/
        = DFGC_SOH_VAL_INIT;                                                /*<Conan>*/
    /*                                                    */                /*<Conan>*/
    gfgc_ctl.remap_soh->soh_monotonic_timestamp                             /*<Conan>*/
        = DFGC_SOH_VAL_INIT;                                                /*<Conan>*/
    /*                                                    */                /*<Conan>*/
    gfgc_ctl.remap_soh->soh_degra_save_monotonic_timestamp                  /*<Conan>*/
        = DFGC_SOH_VAL_INIT;                                                /*<Conan>*/
    /*                                                    */                /*<Conan>*/
    gfgc_ctl.remap_soh->soh_degra_batt_total_capa                           /*<Conan>*/
        = DFGC_SOH_VAL_INIT;                                                /*<Conan>*/
    /*       SOC                                          */                /*<Conan>*/
    gfgc_ctl.remap_soh->soc_old                                             /*<Conan>*/
        = DFGC_FGC_SOC_MAX_PERCENT;                                         /*<Conan>*/

    MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_INIT | 0xFFFF );
}

/******************************************************************************/
/*                pfgc_info_health_check                                      */
/*                                                                            */
/*                2008.10.20                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                unsigned char health                                        */
/*                T_FGC_REGDUMP *regdump  FG-IC                               */
/*                                                                            */
/******************************************************************************/
unsigned char pfgc_info_health_check( T_FGC_REGDUMP *regdump )
{
/*<3200086>    unsigned short temp;                                           *//*                           */
    unsigned short temp;                         /*                           *//*<Conan>*/
    T_FGC_SOH      *t_soh;                       /*                           */
    unsigned char  soh_new;                      /* N                  SOH    */
    unsigned char  soh_old;                      /* N                  SOH    */
    unsigned char  soh_wp;                       /* SOH                       */
    unsigned char  soh_wp_old;                   /*                 SOH  WP   */
    unsigned char  soh_n;                        /*                           */
    unsigned short soh_sum;                      /* SOH                        */
    unsigned char  soh_ave;                      /* SOH                       */
    unsigned long  soh_cnt;                      /* SOH                       */
    unsigned char  health;                       /*                           */
    unsigned long  flags;
    unsigned long long cycle_degra_capa;         /*                           *//*<Conan>*/
    unsigned long long degra_capa;               /*                           *//*<Conan>*/
    unsigned long long degra_capa2;              /*                           *//*<Conan>*/
    unsigned long long capa_design_tmp_10t7p;    /*                 (10^7)    *//*<Conan>*/
    unsigned long  capa_design_tmp;              /*                           *//*<Conan>*/
    unsigned long  soh_allowable_error;          /*                           *//*<Conan>*/
    unsigned long long cycle_degra;              /*                           *//*<Conan>*/
    unsigned long long degra_val;                /*                           *//*<Conan>*/
    unsigned long long degra_total;              /*                           *//*<Conan>*/
    unsigned long long degra_calc;               /*                           *//*<Conan>*/
    unsigned char measure_val;                   /*         (%)               *//*<Conan>*/
    unsigned short soc_new;                      /* SOC                       *//*<Conan>*/
    unsigned long  cycle_cnt_tmp;                /* SOC                       *//*<Conan>*/
    unsigned long  cycle_cnt_total;              /* SOC                       *//*<Conan>*/
    int            ret;                          /*                           *//*<Conan>*/

    MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0000 );

    health = DFGC_HEALTH_MEASURE;                /*                           */

    t_soh = gfgc_ctl.remap_soh;                  /*                           *//*<Conan>*/

    
    if( gfgc_ctl.batt_capa_design ==             /*                           *//*<Conan>*/
        DFGC_SOH_VAL_INIT)                                                      /*<Conan>*/
    {                                                                           /*<Conan>*/
        return health;                           /*                           *//*<Conan>*/
    }                                                                           /*<Conan>*/

    spin_lock_irqsave( &gfgc_spinlock_info, flags ); /*  V                    *//*<Conan>*/

    /*----------------------------------------*/                                /*<Conan>*/
    /*                                        */                                /*<Conan>*/
    /*----------------------------------------*/                                /*<Conan>*/
    /*                                        */                                /*<Conan>*/
    soc_new                                      /*           %               *//*<Conan>*/
        = ( unsigned short )((( unsigned short )regdump->soc_h << 8 )           /*<Conan>*/
            | regdump->soc_l );                                                 /*<Conan>*/

    cycle_cnt_tmp = 0;                           /* SOC                       *//*<Conan>1112XX*/
    if(( soc_new < t_soh->soc_old ) &&           /* SOC                       *//*<Conan>*/
       ( t_soh->soc_old != DFGC_FGC_SOC_MAX_PERCENT)) /* SOC                  *//*<Conan>*/
    {
        t_soh->soc_total += ( t_soh->soc_old - soc_new );/*                   *//*<Conan>*/
                
        if( t_soh->soc_total >= gfgc_ctl.soc_cycle_val) /* SOC                     *//*<Conan>*/
        {
            cycle_cnt_tmp =                     /*                            *//*<Conan>*/
                (unsigned long)(t_soh->soc_total / gfgc_ctl.soc_cycle_val);
            
            cycle_cnt_total =                   /*                            *//*<Conan>*/
                cycle_cnt_tmp + t_soh->cycle_cnt;
            
            if( cycle_cnt_total <= DFGC_SOC_MAX_CYCLE )
            {
                t_soh->cycle_cnt =              /*                            *//*<Conan>*/
                    (unsigned short)cycle_cnt_total;
            }
            else
            {
                t_soh->cycle_cnt =              /*                            *//*<Conan>*/
                    DFGC_SOC_MAX_CYCLE;
            }
            
            t_soh->soc_total = t_soh->soc_total -/*                           *//*<Conan>*/
                (cycle_cnt_tmp * gfgc_ctl.soc_cycle_val);
            
        }
    }
    
    t_soh->soc_old = soc_new;                   /*       SOC          SOC        *//*<Conan>*/

    capa_design_tmp_10t7p = (unsigned long long)gfgc_ctl.batt_capa_design *     /*<Conan>*/
                            DFGC_FGC_10_TO_7_POWER;                             /*<Conan>*/

    cycle_degra = ( unsigned long long )gfgc_ctl.soh_cycle_val * DFGC_FGC_10_TO_3_POWER * /*<Conan>1112XX 1201XX*/
                     ( unsigned long long )t_soh->cycle_cnt;                              /*<Conan>1201XX*/

    if( cycle_degra > capa_design_tmp_10t7p)     /*                           *//*<Conan>*/
    {                                                                           /*<Conan>*/
        cycle_degra = capa_design_tmp_10t7p;                                    /*<Conan>*/
    }                                                                           /*<Conan>*/
    
    cycle_degra_capa =                                                          /*<Conan>*/
        capa_design_tmp_10t7p - cycle_degra;     /*                           *//*<Conan>*/

    cycle_degra_capa = cycle_degra_capa *        /* %                         *//*<Conan>*/
        DFGC_FGC_CNV_PERCENT;
    
    capa_design_tmp = gfgc_ctl.batt_capa_design; /*                           *//*<Conan>*/
    
    if(cycle_degra_capa > 0)                     /*                 0          *//*<Conan>*/
    {                                                                           /*<Conan>*/
        do_div(cycle_degra_capa, capa_design_tmp);/*                          *//*<Conan>*/
    }                                                                           /*<Conan>*/
    else                                                                        /*<Conan>*/
    {                                                                           /*<Conan>*/
        cycle_degra_capa = 0;                    /*                           *//*<Conan>*/
                                                 /* 0%                        *//*<Conan>*/
    }                                                                           /*<Conan>*/

    t_soh->cycle_degra_batt_capa =               /*                           *//*<Conan>*/
        (unsigned long)cycle_degra_capa;         /*      ((%) 10^7  )         *//*<Conan>*/

    /*                                        */                                /*<Conan>*/
    ret =                                        /*                           *//*<Conan>*/
        pfgc_info_soh_degra_check( regdump , &degra_val);                       /*<Conan>*/

    if((t_soh->soh_degra_batt_total_capa == DFGC_SOH_VAL_INIT) &&
       (gfgc_ctl.save_soh_degra_batt_capa != (DFGC_FGC_CNV_PERCENT * DFGC_FGC_10_TO_7_POWER)))
    {
        /*                                          */                          /*<Conan>*/
        degra_calc = (unsigned long long)gfgc_ctl.batt_capa_design * 
                     gfgc_ctl.save_soh_degra_batt_capa;
        
        do_div( degra_calc, DFGC_FGC_CNV_PERCENT );
        
        t_soh->soh_degra_batt_total_capa = capa_design_tmp_10t7p - degra_calc;
    }

    degra_val = degra_val +                    /*                             *//*<Conan>*/
                t_soh->soh_degra_batt_total_capa;                               /*<Conan>*/
    
    t_soh->soh_degra_batt_total_capa = degra_val; /*                          *//*<Conan>*/

    if( degra_val > capa_design_tmp_10t7p)     /*                             *//*<Conan>*/
    {                                                                           /*<Conan>*/
        degra_val = capa_design_tmp_10t7p;     /*                             *//*<Conan>*/
    }                                                                           /*<Conan>*/
    
                                               /*                   mAh 10^1  *//*<Conan>*/
    degra_capa = capa_design_tmp_10t7p - degra_val;                             /*<Conan>*/

    degra_capa2 = degra_capa *                 /* %                           *//*<Conan>*/
        DFGC_FGC_CNV_PERCENT;                                                   /*<Conan>*/

    do_div( degra_capa2, capa_design_tmp );    /*                             *//*<Conan>*/
    
    t_soh->soh_degra_batt_capa = degra_capa2;  /*                             *//*<Conan>*/

    if( cycle_degra > degra_capa )             /*                             *//*<Conan>*/
    {                                                                           /*<Conan>*/
        cycle_degra = degra_capa;              /*                             *//*<Conan>*/
    }                                                                           /*<Conan>*/
    
    degra_total = degra_capa - cycle_degra;      /*                           *//*<Conan>*/

    degra_total = degra_total *                  /* %                         *//*<Conan>*/
        DFGC_FGC_CNV_PERCENT;                                                   /*<Conan>*/

    /*                 ((%)10^7  )            */                                /*<Conan>*/
    do_div( degra_total ,capa_design_tmp);       /*                           *//*<Conan>*/

    t_soh->degra_batt_estimate_val =             /*                           *//*<Conan>*/
        (unsigned long)degra_total;                                             /*<Conan>*/

                                                 /*           SOH             */
//    if(( regdump->soh_stt > DFGC_SOH_LEVEL_MAX ) /*                           *//*<Conan>1201XX*/
//        || ( regdump->soh_stt < gfgc_ctl.soh_level ))/*                       *//*<Conan>1201XX*/
//    {                                                                           /*<Conan>1201XX*/
//        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0001 );                    /*<Conan>1201XX*/
//        spin_unlock_irqrestore( &gfgc_spinlock_info, /*  V                    *//*<Conan>*//*<Conan>1201XX*/
//                                flags );             /*                       *//*<Conan>*//*<Conan>1201XX*/
//        return health;                           /*                           *//*<Conan>1201XX*/
//    }                                                                           /*<Conan>1201XX*/

                                                 /*                           */
                                                 /*                           */
    temp = ( regdump->temp_h << 8 )                                             /*<Conan>*/
                | ( regdump->temp_l );                                          /*<Conan>*/
//    if(( temp < DFGC_HELTE_TEMP_LOW  )           /*                           *//*<Conan>*//*<Conan>1201XX*/
//        || ( DFGC_HELTE_TEMP_HIGH < temp ))                                     /*<Conan>*//*<Conan>1201XX*/
//    {                                                                           /*<Conan>*//*<Conan>1201XX*/
//        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0002 );                    /*<Conan>*//*<Conan>1201XX*/
//        spin_unlock_irqrestore( &gfgc_spinlock_info, /*  V                    *//*<Conan>*//*<Conan>1201XX*/
//                                flags );             /*                       *//*<Conan>*//*<Conan>1201XX*/
//        return health;                           /*                           *//*<Conan>*//*<Conan>1201XX*/
//    }                                                                           /*<Conan>*//*<Conan>1201XX*/

/*<3200086>    temp = ( regdump->temp_h << 8 )                                */
/*<3200086>                | ( regdump->temp_l );                             */
/*<3200086>    if(( temp <= DFGC_HELTE_TEMP_LOW  )                            *//*                           */
/*<3200086>        || ( DFGC_HELTE_TEMP_HIGH <= temp ))                       */
/*<3200086>    {                                                              */
/*<3200086>        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0002 );       */
/*<3200086>        return health;                                             *//*                           */
/*<3200086>    }                                                              */

/*<Conan>    spin_lock_irqsave( &gfgc_spinlock_info, flags );*/ /*  V                    */

/*<Conan>    t_soh = gfgc_ctl.remap_soh;*/                  /*                           */

/*<3131029>                                                 *//*       SOH                 */
    soh_wp               = t_soh->soh_wp;        /*                           */
/*<3131029>    soh_new              = regdump->soh;         *//*       SOH                 */
/*<3131029>    t_soh->soh[ soh_wp ] = soh_new;              *//* SDRAM                     */

                                                 /*       SOH                 */
    soh_n = gfgc_ctl.soh_n;                      /*                           */
/*<3131029>    if( soh_wp > ( unsigned long )soh_n )*/
    if( soh_wp >= soh_n )                        /*<3131029>      WP          */
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0003 );
        soh_wp_old                               /*                           */
            = ( unsigned char )( soh_wp - soh_n );
    }
    else
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0004 );
        soh_wp_old                               /*                           */
            = ( unsigned char )( DFGC_SOH_DATA_MAX - ( soh_n - soh_wp ));
    }

    soh_old = t_soh->soh[ soh_wp_old ];          /*       SOH                 */
                                                 /*<3131029>      SOH         */
                                                 /*<3131029>n=255      old    */
    soh_new              = regdump->soh;         /*<3131029>      SOH         */

    if ( (( regdump->soh_stt > DFGC_SOH_LEVEL_MAX) || (regdump->soh_stt < gfgc_ctl.soh_level )) || /*                        *//*<Conan>1201XX*/
        (( temp < DFGC_HELTE_TEMP_LOW) || (DFGC_HELTE_TEMP_HIGH < temp )) ) { /*                          *//*<Conan>1201XX*/
        t_soh->soh_sum = DFGC_SOH_INIT_INIT * soh_n; /*<Conan>1201XX*/
        t_soh->soh_ave = DFGC_SOH_INIT_INIT; /*<Conan>1201XX*/
        t_soh->degra_batt_measure_val = DFGC_SOH_INIT_INIT; /*<Conan>1201XX*/
        soh_ave = DFGC_SOH_INIT_INIT; /*<Conan>1201XX*/
        soh_n = 0; /*                                  *//*<Conan>1201XX*/
    }
    else { /* SOH               *//*<Conan>1201XX*/

        t_soh->soh[ soh_wp ] = soh_new;              /*<3131029>SDRAM              */

                                                   /* SOH_SUM,SOH_AVE           */
                                                   /* SDRAM                     */
        soh_sum = ( unsigned short )( t_soh->soh_sum - soh_old + soh_new );
        soh_ave = ( unsigned char )( soh_sum / soh_n );/*                         */
        t_soh->soh_sum = soh_sum;
        t_soh->soh_ave = soh_ave;
    
        t_soh->degra_batt_measure_val = soh_ave;     /* SOH                       *//*<Conan>*/

        if( soh_wp >= DFGC_SOH_DATA_MAX - 1 )        /*                           */
        {
            MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0005 );
            t_soh->soh_wp = 0;                       /*                           */
        }
        else
        {
            MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0006 );
                                                     /*     SOH_WP                */
            t_soh->soh_wp                            /* SDRAM                     */
                = ( unsigned char )( soh_wp + 1 );
        }

                                                     /*         SOH_CNT           */
        soh_cnt = t_soh->soh_cnt;                    /*                           */
        if( soh_cnt < DFGC_SOH_CNT_MAX )             /*                           */
        {
            MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0007 );
            t_soh->soh_cnt = soh_cnt + 1;            /*               SDRAM       */
        }
        if ( t_soh->soh_cnt >= ( unsigned long )soh_n ) ; /*                          *//*<Conan>1201XX*/
        else { soh_n = 0; } /*                                        *//*<Conan1>201XX*/

     } /*<Conan>1201XX*/

/*<3131029>    spin_unlock_irqrestore( &gfgc_spinlock_info, *//*  V                        */
/*<3131029>                            flags );*/

                                                 /*                           */
    if( ret == DFGC_NG )                         /*             NG            *//*<Conan>*/
    {                                                                           /*<Conan>*/
        spin_unlock_irqrestore( &gfgc_spinlock_info, /*  V                    *//*<Conan>*/
                                flags );             /*                       *//*<Conan>*/
        return health;                           /*                           *//*<Conan>*/
    }                                                                           /*<Conan>*/
    
/*<3131029>    if(( unsigned char)soh_cnt >= soh_n )        *//*                           */
/*<Conan>    if( soh_cnt >= ( unsigned long )soh_n )      *//*<3131029>                  */
    if( t_soh->soh_cnt >= ( unsigned long )soh_n ) /*<Conan>                  */
    {                                            /*                           */
        
        measure_val = ( unsigned char )(t_soh->degra_batt_estimate_val / /* %           *//*<Conan>*/
                          DFGC_FGC_10_TO_7_POWER);                             /*<Conan>*/
                                                 /*                           *//*<Conan>*/
        if ( soh_n == 0 ) { soh_allowable_error = 0; } /*                                        *//*<Conan>1201XX*/
        else { /*                                        *//*<Conan>1201XX*/
          soh_allowable_error =                    /*                 (10%)     *//*<Conan>*/
            ( unsigned char )( measure_val / DFGC_FGC_ALLOWABLE_ERROR );        /*<Conan>*/
        } /*<Conan>1201XX*/
        
        if(( soh_ave > (measure_val + soh_allowable_error)) ||                  /*<Conan>*/
           ((measure_val - soh_allowable_error) > soh_ave ))                    /*<Conan>*/
        {                                                                       /*<Conan>*/
                                                 /*                           *//*<Conan>*/
            soh_ave = measure_val;               /*                           *//*<Conan>*/
        }                                                                       /*<Conan>*/
        degra_batt_select_val = soh_ave;         /*                           *//*<Conan>1201XX*/
        if( soh_ave >= gfgc_ctl.soh_down )       /* SOH                       */
        {                                        /*                           */
            MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0008 );
            health = DFGC_HEALTH_GOOD;           /*                           */
            t_soh->soh_health = 0x0;                                            /*<Conan>*/
        }
        else if( soh_ave >= gfgc_ctl.soh_bad )   /* SOH                       */
        {                                        /*                           */
            MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0009 );
            health = DFGC_HEALTH_DOWN;           /*                           */
            t_soh->soh_health = 0x1;                                            /*<Conan>*/
        }
        else
        {
            MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x000A );
            health = DFGC_HEALTH_BAD;            /*                           */
            t_soh->soh_health = 0x2;                                            /*<Conan>*/
        }
    }

    t_soh->soh_old = health;                     /*<3131029>                  */

    spin_unlock_irqrestore( &gfgc_spinlock_info, /*<3131029>  V               */
                            flags );             /*<3131029>                  */

    MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0xFFFF );

    return health;
}
/*<Conan>#endif*/ /* DFGC_SW_DELETED_FUNC */
#else /*<Conan>1111XX*/
#if DFGC_SW_DELETED_FUNC
/******************************************************************************/
/*                pfgc_info_health_init                                       */
/*                                                                            */
/*                2008.10.17                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void pfgc_info_health_init( void )
{
    T_FGC_SYSERR_DATA syserr_info;               /* SYS                       */

    MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_INIT | 0x0000 );

                                                 /*                 WBB       */
    gfgc_ctl.remap_soh = ioremap_cache( DFGC_SDRAM_SOH, /*  C                 */
                                        sizeof( T_FGC_SOH ),
                                        CACHE_WBB );

    if( gfgc_ctl.remap_soh == NULL )
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_INIT | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_INFO_HEALTH_INIT | 0x0001 );
        MFGC_SYSERR_DATA_SET2(
                       CSYSERR_ALM_RANK_A,
                       DFGC_SYSERR_IOREMAP_ERR,
                       ( DFGC_INFO_HEALTH_INIT | 0x0001 ),
                       DFGC_SDRAM_SOH,
                       sizeof( T_FGC_SOH ),
                       syserr_info );
        pfgc_log_syserr( &syserr_info );         /*  V                        */
    }

    if( gfgc_ctl.remap_soh->soh_wp               /* soh_wp                    */
                    >= DFGC_SOH_DATA_MAX )
    {
        gfgc_ctl.remap_soh->soh_wp = 0;          /* soh_wp                    */
    }
                                                 /* OCV                       */
/*<3131029>    if( gfgc_ctl.remap_ocv->fgc_ocv_data[ 0 ].ocv != 0 )*/
    if(( gfgc_ctl.remap_ocv->fgc_ocv_data[ 0 ].ocv != 0 ) || /*<3131029>      */
       ( gfgc_ctl.ocv_enable != DFGC_ON ))       /*<3131029>OCV               */
    {
                                                 /*                           */
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_INIT | 0x0002 );
        gfgc_ctl.remap_soh->soh_wp = 0;
        memset( ( void * )gfgc_ctl.remap_soh->soh, /*  V   D                  */
                gfgc_ctl.soh_init,
                DFGC_SOH_DATA_MAX );
        gfgc_ctl.remap_soh->soh_old = DFGC_HEALTH_MEASURE;
        gfgc_ctl.remap_soh->soh_sum
                = ( unsigned short )( gfgc_ctl.soh_init * gfgc_ctl.soh_n );
        gfgc_ctl.remap_soh->soh_ave = gfgc_ctl.soh_init;
        gfgc_ctl.remap_soh->soh_cnt = 0;
                                                 /*                           */
        gfgc_ctl.health_old = DFGC_HEALTH_MEASURE; /*<3131029>                */
        gfgc_batt_info.health = DFGC_HEALTH_MEASURE;
    }
    else
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_INIT | 0x0003 );
        gfgc_ctl.health_old = gfgc_ctl.remap_soh->soh_old;
        gfgc_batt_info.health = gfgc_ctl.remap_soh->soh_old; /*<3131029>      */
    }

    MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_INIT | 0xFFFF );
}

/******************************************************************************/
/*                pfgc_info_health_check                                      */
/*                                                                            */
/*                2008.10.20                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                unsigned char health                                        */
/*                T_FGC_REGDUMP *regdump  FG-IC                               */
/*                                                                            */
/******************************************************************************/
unsigned char pfgc_info_health_check( T_FGC_REGDUMP *regdump )
{
/*<3200086>    unsigned short temp;                                           *//*                           */
    T_FGC_SOH      *t_soh;                       /*                           */
    unsigned char  soh_new;                      /* N                  SOH    */
    unsigned char  soh_old;                      /* N                  SOH    */
    unsigned char  soh_wp;                       /* SOH                       */
    unsigned char  soh_wp_old;                   /*                 SOH  WP   */
    unsigned char  soh_n;                        /*                           */
    unsigned short soh_sum;                      /* SOH                        */
    unsigned char  soh_ave;                      /* SOH                       */
    unsigned long  soh_cnt;                      /* SOH                       */
    unsigned char  health;                       /*                           */
    unsigned long  flags;

    MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0000 );

    health = DFGC_HEALTH_MEASURE;                /*                           */

                                                 /*           SOH             */
    if(( regdump->soh_stt > DFGC_SOH_LEVEL_MAX ) /*                           */
        || ( regdump->soh_stt < gfgc_ctl.soh_level ))/*                       */
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0001 );
        return health;                           /*                           */
    }

                                                 /*                           */
                                                 /*                           */
/*<3200086>    temp = ( regdump->temp_h << 8 )                                */
/*<3200086>                | ( regdump->temp_l );                             */
/*<3200086>    if(( temp <= DFGC_HELTE_TEMP_LOW  )                            *//*                           */
/*<3200086>        || ( DFGC_HELTE_TEMP_HIGH <= temp ))                       */
/*<3200086>    {                                                              */
/*<3200086>        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0002 );       */
/*<3200086>        return health;                                             *//*                           */
/*<3200086>    }                                                              */

    spin_lock_irqsave( &gfgc_spinlock_info, flags ); /*  V                    */

    t_soh = gfgc_ctl.remap_soh;                  /*                           */

/*<3131029>                                                 *//*       SOH                 */
    soh_wp               = t_soh->soh_wp;        /*                           */
/*<3131029>    soh_new              = regdump->soh;         *//*       SOH                 */
/*<3131029>    t_soh->soh[ soh_wp ] = soh_new;              *//* SDRAM                     */

                                                 /*       SOH                 */
    soh_n = gfgc_ctl.soh_n;                      /*                           */
/*<3131029>    if( soh_wp > ( unsigned long )soh_n )*/
    if( soh_wp >= soh_n )                        /*<3131029>      WP          */
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0003 );
        soh_wp_old                               /*                           */
            = ( unsigned char )( soh_wp - soh_n );
    }
    else
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0004 );
        soh_wp_old                               /*                           */
            = ( unsigned char )( DFGC_SOH_DATA_MAX - ( soh_n - soh_wp ));
    }

    soh_old = t_soh->soh[ soh_wp_old ];          /*       SOH                 */
                                                 /*<3131029>      SOH         */
                                                 /*<3131029>n=255      old    */
    soh_new              = regdump->soh;         /*<3131029>      SOH         */
    t_soh->soh[ soh_wp ] = soh_new;              /*<3131029>SDRAM              */

                                                 /* SOH_SUM,SOH_AVE           */
                                                 /* SDRAM                     */
    soh_sum = ( unsigned short )( t_soh->soh_sum - soh_old + soh_new );
    soh_ave = ( unsigned char )( soh_sum / soh_n );/*                         */
    t_soh->soh_sum = soh_sum;
    t_soh->soh_ave = soh_ave;

    if( soh_wp >= DFGC_SOH_DATA_MAX - 1 )        /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0005 );
        t_soh->soh_wp = 0;                       /*                           */
    }
    else
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0006 );
                                                 /*     SOH_WP                */
        t_soh->soh_wp                            /* SDRAM                     */
            = ( unsigned char )( soh_wp + 1 );
    }

                                                 /*         SOH_CNT           */
    soh_cnt = t_soh->soh_cnt;                    /*                           */
    if( soh_cnt < DFGC_SOH_CNT_MAX )             /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0007 );
        t_soh->soh_cnt = soh_cnt + 1;            /*               SDRAM       */
    }

/*<3131029>    spin_unlock_irqrestore( &gfgc_spinlock_info, *//*  V                        */
/*<3131029>                            flags );*/

                                                 /*                           */
/*<3131029>    if(( unsigned char)soh_cnt >= soh_n )        *//*                           */
    if( soh_cnt >= ( unsigned long )soh_n )      /*<3131029>                  */
    {                                            /*                           */
        if( soh_ave >= gfgc_ctl.soh_down )       /* SOH                       */
        {                                        /*                           */
            MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0008 );
            health = DFGC_HEALTH_GOOD;           /*                           */
        }
        else if( soh_ave >= gfgc_ctl.soh_bad )   /* SOH                       */
        {                                        /*                           */
            MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x0009 );
            health = DFGC_HEALTH_DOWN;           /*                           */
        }
        else
        {
            MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0x000A );
            health = DFGC_HEALTH_BAD;            /*                           */
        }
    }

    t_soh->soh_old = health;                     /*<3131029>                  */

    spin_unlock_irqrestore( &gfgc_spinlock_info, /*<3131029>  V               */
                            flags );             /*<3131029>                  */

    MFGC_RDPROC_PATH( DFGC_INFO_HEALTH_CHECK | 0xFFFF );

    return health;
}
#endif /* DFGC_SW_DELETED_FUNC */
#endif /*<Conan>1111XX*/

#ifdef CONFIG_BATTERY_DEGRADATION_JUDGEMENT /*<Conan>1111XX*/
/******************************************************************************//*<Conan>*/
/*                pfgc_info_soh_degra_check                                   *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                                                                            *//*<Conan>*/
/*                DFGC_OK                                                     *//*<Conan>*/
/*                DFGC_NG                                                     *//*<Conan>*/
/*                T_FGC_REGDUMP *regdump  FG-IC                               *//*<Conan>*/
/*                unsigned long* degra_val                                    *//*<Conan>*/
/******************************************************************************//*<Conan>*/
unsigned char pfgc_info_soh_degra_check( T_FGC_REGDUMP *regdump ,unsigned long long * degra_val )
{
    unsigned short     degra_tbl_val;            /*                           */
    unsigned short     temp;                     /*                           */
    unsigned short     volt;                     /*                           */
    unsigned char      level_v;                  /*                           */
    unsigned char      level_temp;               /*                           */
    unsigned long      tmp_usec;                 /*                           */
    unsigned long      result;                   /*                           */
    unsigned long      result2;                  /*                           */
    unsigned long long tmp_degra;                /*                           */
    unsigned char      ret;                      /*                           */
    T_FGC_SOH          *t_soh;                   /*                           */
    T_FGC_TIME_DIFF_INFO    time_diff;           /*                           */


    result = DFGC_SOH_VAL_INIT;                  /*                           */
    t_soh = gfgc_ctl.remap_soh;                  /*                           */
    ret = DFGC_OK;                               /*                           */


    time_diff =                                  /*             (     -     ) */
        pfgc_log_time_diff( t_soh->soh_degra_save_timestamp,
                            t_soh->soh_monotonic_timestamp );

    t_soh->soh_degra_save_timestamp =            /*                           */
        t_soh->soh_monotonic_timestamp;          /*                           */


    /*                */
    volt = ( unsigned short )                    /*                           */
           ((( unsigned short )regdump->volt_h << 8 )
                | ( regdump->volt_l ));
    
    if( volt < gfgc_ctl.soh_degra_volt[0] )      /*         A                 */
    {
        level_v = DFGC_HELTE_TBL_LEVEL_0;
    }
    else if( volt < gfgc_ctl.soh_degra_volt[1] ) /*         A              B     */
    {
        level_v = DFGC_HELTE_TBL_LEVEL_1;
    }
    else if( volt < gfgc_ctl.soh_degra_volt[2] ) /*         B              C     */
    {
        level_v = DFGC_HELTE_TBL_LEVEL_2;
    }
    else                                         /*         C                 */
    {
        *degra_val = result;
        ret = DFGC_NG;
        return ret;
    }

    
    /*                */
    temp = ( unsigned short )                    /*                           */
           ((( unsigned short )regdump->temp_h << 8 )
                | ( regdump->temp_l ));
    
    if( temp <= DFGC_HELTE_TEMP_LEVEL_0 )
    {
        level_temp = DFGC_HELTE_TBL_LEVEL_0;
    }
    else if( temp <= DFGC_HELTE_TEMP_LEVEL_1 )
    {
        level_temp = DFGC_HELTE_TBL_LEVEL_1;
    }
    else if( temp <= DFGC_HELTE_TEMP_LEVEL_2 )
    {
        level_temp = DFGC_HELTE_TBL_LEVEL_2;
    }
    else                                         /*                           */
    {
        level_temp = DFGC_HELTE_TBL_LEVEL_3;
    }

    /*                              */
    degra_tbl_val = gfgc_ctl.soh_degra_val_tbl[level_temp][level_v];
    
    /*                              */
                                                 /*                           */
    result  = time_diff.min * degra_tbl_val;

                                                 /*                                */
    tmp_usec = ( time_diff.sec * DFGC_FGC_10_TO_6_POWER ) +
               time_diff.usec;
                                                 /*                           */
    tmp_degra = (unsigned long long)
        ((unsigned long long)tmp_usec * degra_tbl_val * DFGC_FGC_10_TO_1_POWER);

    do_div( tmp_degra, ( 60 * DFGC_FGC_10_TO_6_POWER ));
    
    result2 = (unsigned long)(((unsigned long)tmp_degra + 5 ) /
                DFGC_FGC_10_TO_1_POWER);
                

    result += result2;                          /*                            */

    *degra_val = result;                        /*                            */

    return ret;
}
#endif /*<Conan>1111XX*/
/******************************************************************************/
/*                pfgc_info_batt_level                                        */
/*                                                                            */
/*                2008.10.20                                                  */
/*                NTTDMSE                                                     */
/*                                                                            */
/*                DCTL_BLVL_LVL3                    3                         */
/*                DCTL_BLVL_LVL2                    2                         */
/*                DCTL_BLVL_ALM                                               */
/*                unsigned char soc                                           */
/*                                                                            */
/******************************************************************************/
unsigned char pfgc_info_batt_level( unsigned char soc )
{
    unsigned char batt_level;
    T_FGC_SYSERR_DATA syserr_info;               /* SYS                       */

    MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0000 );

    batt_level = DCTL_BLVL_MIN;                  /*           1               */

    if( soc > DFGC_CAPACITY_MAX )                /*                           */
    {
        MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0001 );
        MFGC_RDPROC_ERROR( DFGC_INFO_BATT_LEVEL | 0x0001 );
        MFGC_SYSERR_DATA_SET(
                CSYSERR_ALM_RANK_B,
                DFGC_SYSERR_BATTLEVEL_ERR,
                ( DFGC_INFO_BATT_LEVEL | 0x0001 ),
                soc,
                syserr_info );
        pfgc_log_syserr( &syserr_info );         /*  V SYSER                  */
        batt_level = DCTL_BLVL_LVL3;             /*           3               */
    }
    else if( soc >= gfgc_ctl.lvl_2 )             /*           3               */
    {
        MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0002 );
        batt_level = DCTL_BLVL_LVL3;             /*           3               */
    }
    else if( soc >= gfgc_ctl.lvl_1 )             /*           2               */
    {
        MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0003 );
        batt_level = DCTL_BLVL_LVL2;             /*           2               */
    }
    else                                         /* LVA                       */
    {
                                                 /*<3490001>      0x00        */
                                                 /*<3490001>                  */
                                                 /*<3490001>        LVA        */
/*<3210049>        if(( gfgc_ctl.mac_state                  *//* 2G                        */
/*<3210049>                    == DCTL_STATE_INTMIT_2G )    */
/*<3210049>            || ( gfgc_ctl.mac_state              *//* 2G                        */
/*<3210049>                    == DCTL_STATE_CALL_2G )      */
/*<3210049>            || ( gfgc_ctl.mac_state              *//*                           */
/*<3210049>                    == DCTL_STATE_CALL_IRAT ))   */
/*<3210049>        {                                        */
/*<3210049>            MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0004 );*/
/*<3210049>            if( soc < gfgc_ctl.lva_2g )          *//* 2G LVA                    */
/*<3210049>            {                                    */
/*<3210049>                MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0005 );*/
/*<3210049>                batt_level = DCTL_BLVL_ALM;      *//*                           */
/*<3210049>            }                                    */
/*<3210049>        }                                        */
/*<3210049>        else                                     *//*           3G              */
/*<3210049>        {                                        */
/*<3210049>            MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0006 );*/
/*<3210049>            if( soc < gfgc_ctl.lva_3g )          *//* 3G LVA                    */
/*<3210049>            {                                    */
/*<3210049>                MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0007 );*/
/*<3210049>                batt_level = DCTL_BLVL_ALM;      *//*                           */
/*<3210049>            }                                    */
/*<3210049>        }                                        */
        switch( gfgc_ctl.mac_state )             /*<3210049>                  */
        {                                        /*<3210049>                  */
            case DCTL_STATE_CALL_2G:             /*<3210049>2G                */
            case DCTL_STATE_CALL_IRAT:           /*<3210049>2G  3GHandover    */
            case DCTL_STATE_TEST2_2G:            /*<3210049>2G TEST mode      */
                MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0004 ); /*<3210049>*/
                if( soc < gfgc_ctl.lva_2g )      /*<3210049>2G LVA            */
                {                                /*<3210049>                  */
                    MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0005 ); /*<3210049>*/
                    batt_level = DCTL_BLVL_ALM;  /*<3210049>                  */
                }                                /*<3210049>                  */
                break;                           /*<3210049>                  */
                                                 /*<3210049>                  */
            case DCTL_STATE_CALL:                /*<3210049>3G                */
            case DCTL_STATE_TV_CALL:             /*<3210049>TV                */
            case DCTL_STATE_TEST2:               /*<3210049>3G TEST mode      */
                MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0006 ); /*<3210049>*/
                if( soc < gfgc_ctl.lva_3g )      /*<3210049>3G LVA            */
                {                                /*<3210049>                  */
                    MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0007 ); /*<3210049>*/
                    batt_level = DCTL_BLVL_ALM;  /*<3210049>                  */
                }                                /*<3210049>                  */
                break;                           /*<3210049>                  */
                                                 /*<3210049>                  */
            case DCTL_STATE_INTMIT_2G:           /*<3210049>2G                */
            case DCTL_STATE_TEST_2G:             /*<3210049>2G TEST mode      */
                MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0008 ); /*<3210049>*/
/*<3210072_2>                if( soc < gfgc_ctl.alarm_2g )    *//*<3210049>2G alarm          */
                if( soc < gfgc_ctl.lva_2g )      /*<3210072_2>                */
                {                                /*<3210049>                  */
                    MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x0009 ); /*<3210049>*/
                    batt_level = DCTL_BLVL_ALM;  /*<3210049>                  */
                }                                /*<3210049>                  */
                break;                           /*<3210049>                  */
                                                 /*<3210049>                  */
            default:                             /*<3210049>                  */
                MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x000A ); /*<3210049>*/
/*<3210072_2>                if( soc < gfgc_ctl.alarm_3g )    *//*<3210049>3G alarm          */
                if( soc < gfgc_ctl.lva_3g )      /*<3210072_2>                */
                {                                /*<3210049>                  */
                    MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0x000B ); /*<3210049>*/
                    batt_level = DCTL_BLVL_ALM;  /*<3210049>                  */
                }                                /*<3210049>                  */
                break;                           /*<3210049>                  */
        }                                        /*<3210049>                  */
    }

    MFGC_RDPROC_PATH( DFGC_INFO_BATT_LEVEL | 0xFFFF );

    return batt_level;
}

