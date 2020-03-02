/*
 * country.h
 *
 * COUNTRY definitions for active country,
 * usable for:
 * - settings in NVRAM
 * - settings in os_header of ROM
 * - AES language
 * - _AKP cookie
 */

#define COUNTRY_US	 0	/* en_US USA */
#define COUNTRY_DE	 1	/* de_DE Germany */
#define COUNTRY_FR	 2	/* fr_FR France */
#define COUNTRY_UK	 3	/* en_GB United Kingdom */
#define COUNTRY_ES	 4	/* es_ES Spain */
#define COUNTRY_IT	 5	/* it_IT Italy */
#define COUNTRY_SE	 6	/* sv_SE Sweden */
#define COUNTRY_SF	 7	/* fr_CH Switzerland (french) */
#define COUNTRY_SG	 8	/* de_CH Switzerland (german) */
#define COUNTRY_TR	 9	/* tr_TR Turkey */
#define COUNTRY_FI	10	/* fi_FI Finland */
#define COUNTRY_NO	11	/* no_NO Norway */
#define COUNTRY_DK	12	/* da_DK Denmark */
#define COUNTRY_SA	13	/* ar_SA Saudi Arabia */
#define COUNTRY_NL	14	/* nl_NL Netherlands */
#define COUNTRY_CZ	15	/* cs_CZ Czech Republic */
#define COUNTRY_HU	16	/* hu_HU Hungary */
#define COUNTRY_PL	17	/* pl_PL Poland */

#define COUNTRY_LT	18	/* lt_LT Lithuania */
#define COUNTRY_RU	19	/* ru_RU Russia */
#define COUNTRY_EE	20	/* et_EE Estonia */
#define COUNTRY_BY	21	/* be_BY Belarus */
#define COUNTRY_UA	22	/* uk_UA Ukraine */
#define COUNTRY_SK	23	/* sk_SK Slovak Republic */
#define COUNTRY_RO	24	/* ro_RO Romania */
#define COUNTRY_BG	25	/* bg_BG Bulgaria */
#define COUNTRY_SI	26	/* sl_SI Slovenia */
#define COUNTRY_HR	27	/* hr_HR Croatia */
#define COUNTRY_RS	28	/* sr_RS Serbia */
#define COUNTRY_ME	29	/* sr_ME Montenegro */
#define COUNTRY_MK	30	/* mk_MK Macedonia */
#define COUNTRY_GR	31	/* el_GR Greece */
#define COUNTRY_LV	32	/* lv_LV Latvia */
#define COUNTRY_IL	33	/* he_IL Israel */
#define COUNTRY_ZA	34	/* af_ZA South Africa */
#define COUNTRY_PT	35	/* pt_PT Portugal */
#define COUNTRY_BE	36	/* nl_BE Belgium */
#define COUNTRY_JP	37	/* ja_JP Japan */
#define COUNTRY_CN	38	/* zh_CN China */
#define COUNTRY_KR	39	/* ko_KR Korea */
#define COUNTRY_VN	40	/* vi_VN Vietnam */
#define COUNTRY_IN	41	/* hi_IN India */
#define COUNTRY_IR	42	/* fa_IR Iran */
#define COUNTRY_MN	43	/* mn_MN Mongolia */
#define COUNTRY_NP	44	/* ne_NP Nepal */
#define COUNTRY_LA	45	/* lo_LA Lao People's Democratic Republic */
#define COUNTRY_KH	46	/* km_KH Cambodia */
#define COUNTRY_ID	47	/* id_ID Indonesia */
#define COUNTRY_BD	48	/* bn_BD Bangladesh */
#define COUNTRY_MX	99	/* es_MX Mexico (Spanish) */


#ifndef COUNTRY
#define COUNTRY COUNTRY_US
#endif
