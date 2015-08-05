/*
    gs2_vfd.c
    Copyright (C) 2007, 2008 Stephen Wille Padnos, Thoth Systems, Inc.

    Based on a work (test-modbus program, part of libmodbus) which is
    Copyright (C) 2001-2005 Stéphane Raimbault <stephane.raimbault@free.fr>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation, version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA.


    This is a userspace HAL program, which may be loaded using the halcmd "loadusr" command:
        loadusr gs2_vfd
    There are several command-line options.  Options that have a set list of possible values may
        be set by using any number of characters that are unique.  For example, --rate 5 will use
        a baud rate of 57600, since no other available baud rates start with "5"
    -b or --bits <n> (default 8)
        Set number of data bits to <n>, where n must be from 5 to 8 inclusive
    -d or --device <path> (default /dev/ttyS0)
        Set the name of the serial device node to use
    -v or --verbose or -g or --debug
        Turn on debugging messages.  Debug mode will cause all modbus
        messages to be printed in hex on the terminal.
    -n or --name <string> (default gs2_vfd)
        Set the name of the HAL module.  The HAL comp name will be set to <string>, and all pin
        and parameter names will begin with <string>.
    -p or --parity {even,odd,none} (defalt odd)
        Set serial parity to even, odd, or none.
    -r or --rate <n> (default 38400)
        Set baud rate to <n>.  It is an error if the rate is not one of the following:
        110, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
    -s or --stopbits {1,2} (default 1)
        Set serial stop bits to 1 or 2
    -t or --target <n> (default 1)
        Set MODBUS target (slave) number.  This must match the device number you set on the GS2.

    Add is-stopped pin John Thornton
*/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include "rtapi.h"
#include "hal.h"
#include <modbus.h>

/* Read Registers:
	0x2100 = status word 1
	0x2101 = status word 2
	0x2102 = frequency command
	0x2103 = actual frequency
	0x2104 = output current
	0x2105 = DC bus voltage
	0x2106 = actual output voltage
	0x2107 = actual RPM
	0x2108 + 0x2109 = scale freq (not sure what this actually is - it's the same as 0x2103)
	0x210A = power factor.  Not sure of the units (1/10 or 1/100)
	0x210B = load percentage
	0x210C = Firmware revision (never saw anything other than 0 here)
	total of 13 registers		*/
#define START_REGISTER_R	0x2100
#define NUM_REGISTERS_R		13
/* write registers:
	0x91A = Speed reference, in 1/10Hz increments
	0x91B = RUN command, 0=stop, 1=run
	0x91C = direction, 0=forward, 1=reverse
	0x91D = serial fault, 0=no fault, 1=fault (maybe can stop with this?)
	0x91E = serial fault reset, 0=no reset, 1 = reset fault
	total of 5 registers */
#define START_REGISTER_W	0x091A
#define NUM_REGISTERS_W		5

/* modbus slave data struct */
typedef struct {
	int slave;		/* slave address */
	int read_reg_start;	/* starting read register number */
	int read_reg_count;	/* number of registers to read */
	int write_reg_start;	/* starting write register number */
	int write_reg_count;	/* number of registers to write */
} slavedata_t;

/* HAL data struct */
typedef struct {
  hal_s32_t	*stat1;		// status words from the VFD.  Maybe split these out sometime
  hal_s32_t	*stat2;
  hal_float_t	*freq_cmd;	// frequency command
  hal_float_t	*freq_out;	// actual output frequency
  hal_float_t	*curr_out;	// output current
  hal_float_t	*DCBusV;	// 
  hal_float_t	*outV;
  hal_float_t	*RPM;
  hal_float_t	*scale_freq;
  hal_float_t	*power_factor;
  hal_float_t	*load_pct;
  hal_s32_t	*FW_Rev;
  hal_s32_t	errorcount;
  hal_float_t	looptime;
  hal_float_t	speed_tolerance;
  hal_s32_t	retval;
  hal_bit_t		*at_speed;		// when drive freq_cmd == freq_out and running
  hal_bit_t		*is_stopped;	// when drive freq out is 0
  hal_float_t	*speed_command;		// speed command input
  hal_float_t	motor_hz;		// speeds are scaled in Hz, not RPM
  hal_float_t	motor_RPM;		// nameplate RPM at default Hz
  hal_bit_t	*spindle_on;		// spindle 1=on, 0=off
  hal_bit_t	*spindle_fwd;		// direction, 0=fwd, 1=rev
  hal_bit_t *spindle_rev;		// on when in rev and running
  hal_bit_t	*err_reset;		// reset errors when 1
  hal_s32_t ack_delay;		// number of read/writes before checking at-speed

  hal_bit_t	old_run;		// so we can detect changes in the run state
  hal_bit_t	old_dir;
  hal_bit_t	old_err_reset;
} haldata_t;

static int done;
char *modname = "gs2_vfd";

static struct option long_options[] = {
    {"bits", 1, 0, 'b'},
    {"device", 1, 0, 'd'},
    {"debug", 0, 0, 'g'},
    {"help", 0, 0, 'h'},
    {"name", 1, 0, 'n'},
    {"parity", 1, 0, 'p'},
    {"rate", 1, 0, 'r'},
    {"stopbits", 1, 0, 's'},
    {"target", 1, 0, 't'},
    {"verbose", 0, 0, 'v'},
    {0,0,0,0}
};

static char *option_string = "b:d:hn:p:r:s:t:v";

static char *bitstrings[] = {"5", "6", "7", "8", NULL};

// The old libmodbus (v2?) used strings to indicate parity, the new one
// (v3.0.1) uses chars.  The gs2_vfd driver gets the string indicating the
// parity to use from the command line, and I don't want to change the
// command-line usage.  The command-line argument string must match an
// entry in paritystrings, and the index of the matching string is used as
// the index to the parity character for the new libmodbus.
static char *paritystrings[] = {"even", "odd", "none", NULL};
static char paritychars[] = {'E', 'O', 'N'};

static char *ratestrings[] = {"110", "300", "600", "1200", "2400", "4800", "9600",
    "19200", "38400", "57600", "115200", NULL};
static char *stopstrings[] = {"1", "2", NULL};

static void quit(int sig) {
    done = 1;
}

static int comm_delay = 0; // JET delay counter for at-speed

int match_string(char *string, char **matches) {
    int len, which, match;
    which=0;
    match=-1;
    if ((matches==NULL) || (string==NULL)) return -1;
    len = strlen(string);
    while (matches[which] != NULL) {
        if ((!strncmp(string, matches[which], len)) && (len <= strlen(matches[which]))) {
            if (match>=0) return -1;        // multiple matches
            match=which;
        }
        ++which;
    }
    return match;
}

int write_data(modbus_t *mb_ctx, slavedata_t *slavedata, haldata_t *haldata) {
//  int write_data[MAX_WRITE_REGS];
    int retval;
    hal_float_t hzcalc;
        
    if (haldata->motor_hz<10)
        haldata->motor_hz = 60;
    if ((haldata->motor_RPM < 600) || (haldata->motor_RPM > 5000))
        haldata->motor_RPM = 1800;
    hzcalc = haldata->motor_hz/haldata->motor_RPM;

    retval = modbus_write_register(
        mb_ctx,
        slavedata->write_reg_start,
        abs((int)(*(haldata->speed_command)*hzcalc*10))
    );

    if (*(haldata->spindle_on) != haldata->old_run) {
        if (*haldata->spindle_on){
            modbus_write_register(mb_ctx, slavedata->write_reg_start+1, 1);
            comm_delay=0;
        }    
        else
            modbus_write_register(mb_ctx, slavedata->write_reg_start+1, 0);
        haldata->old_run = *(haldata->spindle_on);
    }
    if (*(haldata->spindle_fwd) != haldata->old_dir) {
        if (*haldata->spindle_fwd)
            modbus_write_register(mb_ctx, slavedata->write_reg_start+2, 0);
        else
            modbus_write_register(mb_ctx, slavedata->write_reg_start+2, 1);
        haldata->old_dir = *(haldata->spindle_fwd);
    }
    if (*(haldata->spindle_fwd) || !(*(haldata->spindle_on)))  // JET turn on and off rev based on the status of fwd
    	*(haldata->spindle_rev) = 0;
    if (!(*haldata->spindle_fwd) && *(haldata->spindle_on))
    	*(haldata->spindle_rev) = 1;	
    if (*(haldata->err_reset) != haldata->old_err_reset) {
        if (*(haldata->err_reset))
            modbus_write_register(mb_ctx, slavedata->write_reg_start+4, 1);
        else
            modbus_write_register(mb_ctx, slavedata->write_reg_start+4, 0);
        haldata->old_err_reset = *(haldata->err_reset);
    }
    if (comm_delay < haldata->ack_delay){ // JET allow time for communications between drive and EMC
    	comm_delay++;
    }
    if ((*haldata->spindle_on) && comm_delay == haldata->ack_delay){ // JET test for up to speed
    	if ((*(haldata->freq_cmd))==(*(haldata->freq_out)))
    		*(haldata->at_speed) = 1;
    } 
    if (*(haldata->spindle_on)==0){ // JET reset at-speed
    	*(haldata->at_speed) = 0;
    }
    haldata->retval = retval;
    return retval;
}

void usage(int argc, char **argv) {
    printf("Usage:  %s [options]\n", argv[0]);
    printf(
    "This is a userspace HAL program, typically loaded using the halcmd \"loadusr\" command:\n"
    "    loadusr gs2_vfd\n"
    "There are several command-line options.  Options that have a set list of possible values may\n"
    "    be set by using any number of characters that are unique.  For example, --rate 5 will use\n"
    "    a baud rate of 57600, since no other available baud rates start with \"5\"\n"
    "-b or --bits <n> (default 8)\n"
    "    Set number of data bits to <n>, where n must be from 5 to 8 inclusive\n"
    "-d or --device <path> (default /dev/ttyS0)\n"
    "    Set the name of the serial device node to use\n"
    "-v or --verbose or -g or --debug\n"
    "    Turn on verbose mode.  This will cause all modbus messages to be\n"
    "    printed in hex on the terminal.\n"
    "-n or --name <string> (default gs2_vfd)\n"
    "    Set the name of the HAL module.  The HAL comp name will be set to <string>, and all pin\n"
    "    and parameter names will begin with <string>.\n"
    "-p or --parity {even,odd,none} (defalt odd)\n"
    "    Set serial parity to even, odd, or none.\n"
    "-r or --rate <n> (default 38400)\n"
    "    Set baud rate to <n>.  It is an error if the rate is not one of the following:\n"
    "    110, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200\n"
    "-s or --stopbits {1,2} (default 1)\n"
    "    Set serial stop bits to 1 or 2\n"
    "-t or --target <n> (default 1)\n"
    "    Set MODBUS target (slave) number.  This must match the device number you set on the GS2.\n"
    );
}
int read_data(modbus_t *mb_ctx, slavedata_t *slavedata, haldata_t *hal_data_block) {
    uint16_t receive_data[MODBUS_MAX_READ_REGISTERS];	/* a little padding in there */
    int retval;

    /* can't do anything with a null HAL data block */
    if (hal_data_block == NULL)
        return -1;
    /* but we can signal an error if the other params are null */
    if ((mb_ctx==NULL) || (slavedata == NULL)) {
        hal_data_block->errorcount++;
        return -1;
    }
    retval = modbus_read_registers(mb_ctx, slavedata->read_reg_start,
                                slavedata->read_reg_count, receive_data);
    if (retval==slavedata->read_reg_count) {
        retval = 0;
        hal_data_block->retval = retval;
        if (retval==0) {
        *(hal_data_block->stat1) = receive_data[0];
        *(hal_data_block->stat2) = receive_data[1];
        *(hal_data_block->freq_cmd) = receive_data[2] * 0.1;
        *(hal_data_block->freq_out) = receive_data[3] * 0.1;
        if (receive_data[3]==0){	// JET if freq out is 0 then the drive is stopped
        *(hal_data_block->is_stopped) = 1;	
        } else {	
        *(hal_data_block->is_stopped) = 0; 
        }	
        *(hal_data_block->curr_out) = receive_data[4] * 0.1;
        *(hal_data_block->DCBusV) = receive_data[5] * 0.1;
        *(hal_data_block->outV) = receive_data[6] * 0.1;
        *(hal_data_block->RPM) = receive_data[7];
        *(hal_data_block->scale_freq) = (receive_data[8] | (receive_data[9] << 16)) * 0.1;
        *(hal_data_block->power_factor) = receive_data[10];
        *(hal_data_block->load_pct) = receive_data[11] * 0.1;
        *(hal_data_block->FW_Rev) = receive_data[12];
        retval = 0;
        }
    } else {
        hal_data_block->retval = retval;
        hal_data_block->errorcount++;
        retval = -1;
    }
    return retval;
}

int main(int argc, char **argv)
{
    int retval = 0;
    modbus_t *mb_ctx;
    haldata_t *haldata;
    slavedata_t slavedata;
    int slave;
    int hal_comp_id;
    struct timespec loop_timespec, remaining;
    int baud, bits, stopbits, verbose;
    char *device, *endarg;
    char parity;
    int opt;
    int argindex, argvalue;
    done = 0;

    // assume that nothing is specified on the command line
    baud = 38400;
    bits = 8;
    stopbits = 1;
    verbose = 0;
    device = "/dev/ttyS0";
    parity = 'O';

    /* slave / register info */
    slave = 1;
    slavedata.read_reg_start = START_REGISTER_R;
    slavedata.read_reg_count = NUM_REGISTERS_R;
    slavedata.write_reg_start = START_REGISTER_W;
    slavedata.write_reg_count = NUM_REGISTERS_R;

    // process command line options
    while ((opt=getopt_long(argc, argv, option_string, long_options, NULL)) != -1) {
        switch(opt) {
            case 'b':   // serial data bits, probably should be 8 (and defaults to 8)
                argindex=match_string(optarg, bitstrings);
                if (argindex<0) {
                    printf("gs2_vfd: ERROR: invalid number of bits: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                bits = atoi(bitstrings[argindex]);
                break;
            case 'd':   // device name, default /dev/ttyS0
                // could check the device name here, but we'll leave it to the library open
                if (strlen(optarg) > FILENAME_MAX) {
                    printf("gs2_vfd: ERROR: device node name is too long: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                device = strdup(optarg);
                break;
            case 'g':
            case 'v':
                verbose = 1;
                break;
            case 'n':   // module base name
                if (strlen(optarg) > HAL_NAME_LEN-20) {
                    printf("gs2_vfd: ERROR: HAL module name too long: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                modname = strdup(optarg);
                break;
            case 'p':   // parity, should be a string like "even", "odd", or "none"
                argindex=match_string(optarg, paritystrings);
                if (argindex<0) {
                    printf("gs2_vfd: ERROR: invalid parity: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                parity = paritychars[argindex];
                break;
            case 'r':   // Baud rate, 38400 default
                argindex=match_string(optarg, ratestrings);
                if (argindex<0) {
                    printf("gs2_vfd: ERROR: invalid baud rate: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                baud = atoi(ratestrings[argindex]);
                break;
            case 's':   // stop bits, defaults to 1
                argindex=match_string(optarg, stopstrings);
                if (argindex<0) {
                    printf("gs2_vfd: ERROR: invalid number of stop bits: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                stopbits = atoi(stopstrings[argindex]);
                break;
            case 't':   // target number (MODBUS ID), default 1
                argvalue = strtol(optarg, &endarg, 10);
                if ((*endarg != '\0') || (argvalue < 1) || (argvalue > 254)) {
                    printf("gs2_vfd: ERROR: invalid slave number: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                slave = argvalue;
                break;
            case 'h':
            default:
                usage(argc, argv);
                exit(0);
                break;
        }
    }

    printf("%s: device='%s', baud=%d, parity='%c', bits=%d, stopbits=%d, address=%d, verbose=%d\n",
           modname, device, baud, parity, bits, stopbits, slave, verbose);
    /* point TERM and INT signals at our quit function */
    /* if a signal is received between here and the main loop, it should prevent
            some initialization from happening */
    signal(SIGINT, quit);
    signal(SIGTERM, quit);

    /* Assume 38.4k O-8-1 serial settings, device 1 */
    mb_ctx = modbus_new_rtu(device, baud, parity, bits, stopbits);
    if (mb_ctx == NULL) {
        printf("%s: ERROR: couldn't open modbus serial device: %s\n", modname, modbus_strerror(errno));
        goto out_noclose;
    }

    /* the open has got to work, or we're out of business */
    if (((retval = modbus_connect(mb_ctx))!=0) || done) {
        printf("%s: ERROR: couldn't open serial device: %s\n", modname, modbus_strerror(errno));
        goto out_noclose;
    }

    modbus_set_debug(mb_ctx, verbose);

    modbus_set_slave(mb_ctx, slave);

    /* create HAL component */
    hal_comp_id = hal_init(modname);
    if ((hal_comp_id < 0) || done) {
        printf("%s: ERROR: hal_init failed\n", modname);
        retval = hal_comp_id;
        goto out_close;
    }

    /* grab some shmem to store the HAL data in */
    haldata = (haldata_t *)hal_malloc(sizeof(haldata_t));
    if ((haldata == 0) || done) {
        printf("%s: ERROR: unable to allocate shared memory\n", modname);
        retval = -1;
        goto out_close;
    }

    retval = hal_pin_s32_newf(HAL_OUT, &(haldata->stat1), hal_comp_id, "%s.status-1", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_s32_newf(HAL_OUT, &(haldata->stat2), hal_comp_id, "%s.status-2", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->freq_cmd), hal_comp_id, "%s.frequency-command", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->freq_out), hal_comp_id, "%s.frequency-out", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->curr_out), hal_comp_id, "%s.output-current", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->DCBusV), hal_comp_id, "%s.DC-bus-volts", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->outV), hal_comp_id, "%s.output-voltage", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->RPM), hal_comp_id, "%s.motor-RPM", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->scale_freq), hal_comp_id, "%s.scale-frequency", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->power_factor), hal_comp_id, "%s.power-factor", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->load_pct), hal_comp_id, "%s.load-percentage", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_s32_newf(HAL_OUT, &(haldata->FW_Rev), hal_comp_id, "%s.firmware-revision", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_param_s32_newf(HAL_RW, &(haldata->errorcount), hal_comp_id, "%s.error-count", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_param_float_newf(HAL_RW, &(haldata->looptime), hal_comp_id, "%s.loop-time", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_param_s32_newf(HAL_RW, &(haldata->retval), hal_comp_id, "%s.retval", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_bit_newf(HAL_OUT, &(haldata->at_speed), hal_comp_id, "%s.at-speed", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_bit_newf(HAL_OUT, &(haldata->is_stopped), hal_comp_id, "%s.is-stopped", modname); // JET
    if (retval!=0) goto out_closeHAL; 
    retval = hal_pin_float_newf(HAL_IN, &(haldata->speed_command), hal_comp_id, "%s.speed-command", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_bit_newf(HAL_IN, &(haldata->spindle_on), hal_comp_id, "%s.spindle-on", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_bit_newf(HAL_IN, &(haldata->spindle_fwd), hal_comp_id, "%s.spindle-fwd", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_bit_newf(HAL_IN, &(haldata->spindle_rev), hal_comp_id, "%s.spindle-rev", modname); //JET
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_bit_newf(HAL_IN, &(haldata->err_reset), hal_comp_id, "%s.err-reset", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_param_float_newf(HAL_RW, &(haldata->speed_tolerance), hal_comp_id, "%s.tolerance", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_param_float_newf(HAL_RW, &(haldata->motor_hz), hal_comp_id, "%s.nameplate-HZ", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_param_float_newf(HAL_RW, &(haldata->motor_RPM), hal_comp_id, "%s.nameplate-RPM", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_param_s32_newf(HAL_RW, &(haldata->ack_delay), hal_comp_id, "%s.ack-delay", modname);
    if (retval!=0) goto out_closeHAL;

    /* make default data match what we expect to use */
    *(haldata->stat1) = 0;
    *(haldata->stat2) = 0;
    *(haldata->freq_cmd) = 0;
    *(haldata->freq_out) = 0;
    *(haldata->curr_out) = 0;
    *(haldata->DCBusV) = 0;
    *(haldata->outV) = 0;
    *(haldata->RPM) = 0;
    *(haldata->scale_freq) = 0;
    *(haldata->power_factor) = 0;
    *(haldata->load_pct) = 0;
    *(haldata->FW_Rev) = 0;
    haldata->errorcount = 0;
    haldata->looptime = 0.1;
    haldata->motor_RPM = 1730;
    haldata->motor_hz = 60;
    haldata->speed_tolerance = 0.01;
    haldata->ack_delay = 2;
    *(haldata->err_reset) = 0;
    *(haldata->spindle_on) = 0;
    *(haldata->spindle_fwd) = 1;
    *(haldata->spindle_rev) = 0;
    haldata->old_run = -1;		// make sure the initial value gets output
    haldata->old_dir = -1;
    haldata->old_err_reset = -1;
    hal_ready(hal_comp_id);
    
    /* here's the meat of the program.  loop until done (which may be never) */
    while (done==0) {
        read_data(mb_ctx, &slavedata, haldata);
        write_data(mb_ctx, &slavedata, haldata);
        /* don't want to scan too fast, and shouldn't delay more than a few seconds */
        if (haldata->looptime < 0.001) haldata->looptime = 0.001;
        if (haldata->looptime > 2.0) haldata->looptime = 2.0;
        loop_timespec.tv_sec = (time_t)(haldata->looptime);
        loop_timespec.tv_nsec = (long)((haldata->looptime - loop_timespec.tv_sec) * 1000000000l);
        nanosleep(&loop_timespec, &remaining);
    }
    
    retval = 0;	/* if we get here, then everything is fine, so just clean up and exit */
out_closeHAL:
    hal_exit(hal_comp_id);
out_close:
    modbus_close(mb_ctx);
    modbus_free(mb_ctx);
out_noclose:
    return retval;
}
