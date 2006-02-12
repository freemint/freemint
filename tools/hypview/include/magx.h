
/* Sconfig(2) -> */
typedef struct
{
    char    *in_dos;                    /* Adresse der DOS- Semaphore */
    short   *dos_time;                  /* Adresse der DOS- Zeit      */
    short   *dos_date;                  /* Adresse des DOS- Datums    */
    long    res1;                       /*                            */
    long    res2;                       /*                            */
    long    res3;                       /* ist 0L                     */
    void    *act_pd;                    /* Laufendes Programm         */
    long    res4;                       /*                            */
    short   res5;                       /*                            */
    void    *res6;                      /*                            */
    void    *res7;                      /* interne DOS- Speicherliste */
    void    (*resv_intmem)();           /* DOS- Speicher erweitern    */
    long    (*etv_critic)();            /* etv_critic des GEMDOS      */
    char    *((*err_to_str)(char e));   /* Umrechnung Code->Klartext  */
    long    res8;                       /*                            */
    long    res9;                       /*                            */
    long    res10;                      /*                            */
} DOSVARS;

/* os_magic -> */
typedef struct
{
    long    magic;                   /* mu $87654321 sein              */
    void    *membot;                 /* Ende der AES- Variablen         */
    void    *aes_start;              /* Startadresse                    */
    long    magic2;                  /* ist 'MAGX'                      */
    long    date;                    /* Erstelldatum ttmmjjjj           */
    void    (*chgres)(short res, short txt);  /* Auflsung ndern           */
    long    (**shel_vector)(void);   /* residentes Desktop              */
    char    *aes_bootdrv;            /* von hieraus wurde gebootet      */
    short   *vdi_device;             /* vom AES benutzter VDI-Treiber   */
    void    *reservd1;
    void    *reservd2;
    void    *reservd3;
    short   version;                 /* z.B. $0201 ist V2.1             */
    short   release;                 /* 0=alpha..3=release              */
} AESVARS;

/* Cookie MagX --> */
typedef struct
{
    long    config_status;
    DOSVARS *dosvars;
    AESVARS *aesvars;
    void    *res1;
    void    *hddrv_functions;
    long    status_bits;             /* MagiC 3 ab 24.5.95         */
} MAGX_COOKIE;
