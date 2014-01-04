/* tune-s2 -- simple zapping tool for the Linux DVB S2 API
 *
 * UDL (updatelee@gmail.com)
 * Derived from work by:
 * 	Igor M. Liplianin (liplianin@me.by)
 * 	Alex Betis <alex.betis@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/param.h>
#include <sys/time.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>
#include <termios.h>

#include "kb.h"

struct tune_p
{
	int freq;
	int sr;
	int fec;
	int system;
	int modulation;
	int inversion;
	int rolloff;
	int pilot;
	int voltage;
	int tone;
	int LO;
};

struct options
{
	char *name;
	int value;
};

struct options dvb_system[] =
{
	{ "UNDEFINED", SYS_UNDEFINED },
	{ "DVB-C_ANNEX_AC", SYS_DVBC_ANNEX_AC },
	{ "DVB-C_ANNEX_B", SYS_DVBC_ANNEX_B },
	{ "DVB-T", SYS_DVBT },
	{ "DSS", SYS_DSS },
	{ "DVB-S", SYS_DVBS },
	{ "DVB-S2", SYS_DVBS2 },
	{ "DVB-H", SYS_DVBH },
	{ "ISDBT", SYS_ISDBT },
	{ "ISDBS", SYS_ISDBS },
	{ "ISDBC", SYS_ISDBC },
	{ "ATSC", SYS_ATSC },
	{ "ATSCMH", SYS_ATSCMH },
	{ "DMBTH", SYS_DMBTH },
	{ "CMMB", SYS_CMMB },
	{ "DAB", SYS_DAB },
    { "ATSC", SYS_ATSC },
//	{ "DCII_C_QPSK", DCII_C_QPSK },
//	{ "DCII_I_QPSK", DCII_I_QPSK },
//	{ "DCII_Q_QPSK", DCII_Q_QPSK },
//	{ "DCII_C_OQPSK", DCII_C_OQPSK },
	{ NULL, 0 }
};

struct options dvb_modulation[] =
{
	{ "QPSK", QPSK },
	{ "QAM_16", QAM_16 },
	{ "QAM_32", QAM_32 },
	{ "QAM_64", QAM_64 },
	{ "QAM_128", QAM_128 },
	{ "QAM_256", QAM_256 },
	{ "QAM_AUTO", QAM_AUTO },
	{ "VSB_8", VSB_8 },
	{ "VSB_16", VSB_16 },
	{ "8PSK", PSK_8 },
	{ "APSK_16", APSK_16 },
	{ "APSK_32", APSK_32 },
	{ "DQPSK", DQPSK },
	{ NULL, 0 }
};

struct options dvb_inversion[] =
{
	{ "OFF", INVERSION_OFF },
	{ "ON", INVERSION_ON },
	{ "AUTO", INVERSION_AUTO },
	{ NULL, 0 }
};

int name2value(char *name, struct options *table);
char * value2name(int value, struct options *table);
