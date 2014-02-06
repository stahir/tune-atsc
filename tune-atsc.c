/* tune-atsc -- simple zapping tool for the Linux DVB API
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

#include "tune-atsc.h"

#if DVB_API_VERSION < 5
#error tune-atsc requires Linux DVB driver API version 5.0 or newer!
#endif

int name2value(char *name, struct options *table)
{
	while (table->name) {
		if (!strcmp(table->name, name))
			return table->value;
		table++;
	}
}

char * value2name(int value, struct options *table)
{
	while (table->name) {
		if (table->value == value)
			return table->name;
		table++;
	}
}

int check_frontend (int frontend_fd)
{
	fe_status_t status;
	uint16_t snr, signal;
	uint32_t ber, uncorrected_blocks;

	if (ioctl(frontend_fd, FE_READ_STATUS, &status) == -1)
		perror("FE_READ_STATUS failed \n");

	/* some frontends might not support all these ioctls, thus we
	 * avoid printing errors
	 */
	if (ioctl(frontend_fd, FE_READ_SIGNAL_STRENGTH, &signal) == -1)
		signal = -2;
	if (ioctl(frontend_fd, FE_READ_SNR, &snr) == -1)
		snr = -2;
	if (ioctl(frontend_fd, FE_READ_BER, &ber) == -1)
		ber = -2;
	if (ioctl(frontend_fd, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks) == -1)
		uncorrected_blocks = -2;
	printf ("status %02x | signal %d | snr %d.%d dB | ber %d | unc %d | ",
	status, signal , snr/10 , snr%10, ber, uncorrected_blocks);
	if (status & FE_HAS_LOCK)
		printf("FE_HAS_LOCK \n");
	printf("\n");

	return 0;
}

int tune(int frontend_fd, struct tune_p *t)
{
	struct dtv_property p_clear[] = { 
		{ .cmd = DTV_CLEAR }
	};

	struct dtv_properties cmdseq_clear = {
		.num = 1,
		.props = p_clear
	};

	if ((ioctl(frontend_fd, FE_SET_PROPERTY, &cmdseq_clear)) == -1) {
		perror("FE_SET_PROPERTY DTV_CLEAR failed \n");
		return -1;
	}
	usleep (20000);

	// discard stale QPSK events
	struct dvb_frontend_event ev;
	while (1) {
		if (ioctl(frontend_fd, FE_GET_EVENT, &ev) == -1)
			break;
	}

	printf("t->freq = %d \n", t->freq);
	struct dtv_property p_tune[] = {
        { .cmd = DTV_DELIVERY_SYSTEM,	.u.data = t->system },
		{ .cmd = DTV_FREQUENCY,			.u.data = t->freq * 1000 },
		{ .cmd = DTV_MODULATION,		.u.data = t->modulation },
		{ .cmd = DTV_INVERSION,			.u.data = t->inversion },
		{ .cmd = DTV_TUNE },
	};
	struct dtv_properties cmdseq_tune = {
        .num = 5,
		.props = p_tune
	};

	printf("\nTuning specs: \n");
    printf("System:     %s \n", value2name(p_tune[0].u.data, dvb_system));
    printf("Frequency:  %d \n", p_tune[1].u.data/1000);
    printf("Modulation: %s \n", value2name(p_tune[2].u.data, dvb_modulation));

	if (ioctl(frontend_fd, FE_SET_PROPERTY, &cmdseq_tune) == -1) {
		perror("FE_SET_PROPERTY TUNE failed");
		return;
	}
    sleep(1);

	// wait for zero status indicating start of tunning
	do {
		ioctl(frontend_fd, FE_GET_EVENT, &ev);
	}
	while(ev.status != 0);

	if (ioctl(frontend_fd, FE_GET_EVENT, &ev) == -1) {
		ev.status = 0;
	}

	int i;
	fe_status_t status;
	for ( i = 0; i < 5; i++)
	{
		if (ioctl(frontend_fd, FE_READ_STATUS, &status) == -1) {
			perror("FE_READ_STATUS failed");
			return;
		}
	
        if (status & FE_HAS_LOCK)
            break;
		else
			sleep(1);
	}

	struct dtv_property p[] = {
		{ .cmd = DTV_DELIVERY_SYSTEM },
		{ .cmd = DTV_FREQUENCY },
		{ .cmd = DTV_MODULATION },
		{ .cmd = DTV_INVERSION }
	};

	struct dtv_properties p_status = {
		.num = 4,
		.props = p
	};

	// get the actual parameters from the driver for that channel
	if ((ioctl(frontend_fd, FE_GET_PROPERTY, &p_status)) == -1) {
		perror("FE_GET_PROPERTY failed\n");
		return -1;
	}

	printf("Tuned specs: \n");
	printf("System:     %s %d \n", value2name(p_status.props[0].u.data, dvb_system), p_status.props[0].u.data);
	printf("Frequency:  %d \n", p_status.props[1].u.data/1000);
	printf("Modulation: %s %d \n", value2name(p_status.props[2].u.data, dvb_modulation), p_status.props[2].u.data);
	printf("Inversion:  %s %d \n", value2name(p_status.props[3].u.data, dvb_inversion), p_status.props[3].u.data);

	char c;
	do
	{
		check_frontend(frontend_fd);
		c = getch();
	} while(c != 'q');
	return 0;
}

char *usage =
    "\nusage: tune-atsc 639000 [options]\n"
	"	-adapter N     : use given adapter (default 0)\n"
	"	-frontend N    : use given frontend (default 0)\n"
        "       -system        : System ATSC or DVBC\n"
        "       -modulation    : modulation VSB_8 VSB_16 QAM_64 QAM_256 QAM_AUTO\n"
	"	-inversion N   : spectral inversion (OFF / ON / AUTO [default])\n"
	"	-help          : help\n";

int main(int argc, char *argv[])
{
	if (!argv[1] || strcmp(argv[1], "-help") == 0)
	{
		printf("%s", usage);
		return -1;
	}

	char frontend_devname[80];
	int adapter = 0, frontend = 0;

	struct tune_p t;
    t.system        = SYS_ATSC;
    t.modulation	= VSB_8;
    t.inversion     = INVERSION_AUTO;
    t.freq          = strtoul(argv[1], NULL, 0);

	int a;
    for( a = 2; a < argc; a++ )
	{
		if ( !strcmp(argv[a], "-adapter") )
			adapter = strtoul(argv[a+1], NULL, 0);
		if ( !strcmp(argv[a], "-frontend") )
			frontend = strtoul(argv[a+1], NULL, 0);
		if ( !strcmp(argv[a], "-system") )
			t.system = name2value(argv[a+1], dvb_system);
		if ( !strcmp(argv[a], "-modulation") )
			t.modulation = name2value(argv[a+1], dvb_modulation);
		if ( !strcmp(argv[a], "-inversion") )
			t.inversion = name2value(argv[a+1], dvb_inversion);
		if ( !strcmp(argv[a], "-help") )
		{
			printf("%s", usage);
			return -1;
		}
	}

	snprintf(frontend_devname, 80, "/dev/dvb/adapter%d/frontend%d", adapter, frontend);
	printf("opening: %s\n", frontend_devname);
	int frontend_fd;
	if ((frontend_fd = open(frontend_devname, O_RDWR | O_NONBLOCK)) < 0)
	{
		printf("failed to open '%s': %d %m\n", frontend_devname, errno);
		return -1;
	}

	tune(frontend_fd, &t);

	printf("Closing frontend ... \n");
	close (frontend_fd);
	return 0;
}
