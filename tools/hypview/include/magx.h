
/* Sconfig(2) -> */
#ifndef _DOSVARS
#define _DOSVARS
typedef struct
{
   char      *in_dos;                 /* Address of DOS flags       */
   int       *dos_time;               /* Address of DOS time        */
   int       *dos_date;               /* Address of DOS date        */
   long      res1;                    /*                            */
   long      res2;                    /*                            */
   long      res3;                    /* Is 0L                      */
   void      *act_pd;                 /* Running program            */
   long      res4;                    /*                            */
   int       res5;                    /*                            */
   void      *res6;                   /*                            */
   void      *res7;                   /* Internal DOS-memory list   */
   void      (*resv_intmem)();        /* Extend DOS memory          */
   long      (*etv_critic)();         /* etv_critic of GEMDOS       */
   char *    ((*err_to_str)(char e)); /* Conversion code->plaintext */
   long      res8;                    /*                            */
   long      res9;                    /*                            */
   long      res10;                   /*                            */
} DOSVARS;
#endif

/* os_magic -> */
#ifndef _AESVARS
#define _AESVARS
typedef struct
{
   long magic;                   /* Must be $87654321               */
   void *membot;                 /* End of the AES-variables        */
   void *aes_start;              /* Start address                   */
   long magic2;                  /* Is 'MAGX'                       */
   long date;                    /* Creation date ddmmyyyy          */
   void (*chgres)(int res, int txt);  /* Change resolution          */
   long (**shel_vector)(void);   /* Resident desktop                */
   char *aes_bootdrv;            /* Booting took place from here    */
   int  *vdi_device;             /* VDI-driver used by AES          */
   void *reservd1;
   void *reservd2;
   void *reservd3;
   int  version;                 /* e.g. $0201 is V2.1              */
   int  release;                 /* 0=alpha..3=release              */
} AESVARS;
#endif

/* Cookie MagX --> */
#ifndef _MAGX_COOKIE
#define _MAGX_COOKIE
typedef struct
{
   long    config_status;
   DOSVARS *dosvars;
   AESVARS *aesvars;
   void *res1;
   void *hddrv_functions;
   long status_bits;             /* MagiC 3 from 24.5.95 on      */
} MAGX_COOKIE;
#endif
