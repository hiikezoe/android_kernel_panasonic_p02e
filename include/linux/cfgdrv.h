/******************************************************************************/
/*                  :  CFG DRV                                                */
/*                  :  cfgdrv.h                                               */
/*                                                                            */
/*                  :  2010.05.31                                             */
/*                  :                                                         */
/*                  :                                                         */
/*                                                                            */
/*         Unpublished Copyright(c)  2010                                     */
/******************************************************************************/
/*****                *********************************************************/
/*      No.                                                                   */
/*                                                                            */
/* ---------                                               10.05.31 MSE       */
/******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
#ifndef ___CFGDRV_H___
#define ___CFGDRV_H___

#include <linux/hcm_eep.h>

/******************************************************************************/
/*  #include                                                                  */
/******************************************************************************/
/******************************************************************************/
/* define                                                                     */
/******************************************************************************/
#ifndef MMC_PATCH     /* <motsuka> start */
#define MMC_PATCH     /* <motsuka> add */
#endif /* MMC_PATCH *//* <motsuka> end */

#define D_HCM_PARA_NG            -1         /*                                */
#define D_HCM_NOT_INITIALIZE     -2        /*                                */

/******************************************************************************/
/*               ( extern )                                                   */
/******************************************************************************/
extern int cfgdrv_read( unsigned short id, unsigned short size, unsigned char *data );
extern int cfgdrv_write( unsigned short id, unsigned short size, unsigned char *data );

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
struct cfgdata_info
{
    unsigned short id;   /*     ID */
    unsigned short size;
};

/*                                */
struct blk_cfg_page
{
    unsigned short page_no;                    /*            */
    unsigned short size;                       /*                      */
    unsigned long seq_no;                      /*                    */
    unsigned long s_addr;                      /*             (      ) */
#ifndef MMC_PATCH     /* <motsuka> start */
    unsigned char data[4080]; /*              */
#else
    unsigned char data[496]; /*              *//* <motsuka> add */
#endif /* MMC_PATCH *//* <motsuka> end */
    unsigned long magic;                       /*                */
};

/* ACPU                 */
struct map_tbl
{
    unsigned short data_size;                    /*                           */
    unsigned short offset;                       /*                           */
    unsigned char  sum_size;                     /*                               (      ) */
    unsigned char  kind;                         /*                 (      )  */
    unsigned short page_no;                      /*                           */
};

#ifdef __KERNEL__
/*                                  */
struct cfg_operations
{
    int (*read)(unsigned short, unsigned short, unsigned char *);
    int (*write)(unsigned short, unsigned short, unsigned char *);
};

extern struct map_tbl *cfg_map_tbl;             /* ACPU                 */
extern unsigned short cfg_map_id_min;           /* ACPU  ID       */
extern unsigned short cfg_map_id_max;           /* ACPU  ID       */
extern unsigned long  *cfg_map;
extern struct blk_cfg_page *cfg_mngtbl;     /*              */
extern struct cfg_operations Hcm_cfg_ops;  /*                                          */
#endif /* __KERNEL__ */

#endif /* ___CFGDRV_H___ */
/******************************************************************************/
/*         Unpublished Copyright(c)  2009                                     */
/******************************************************************************/
