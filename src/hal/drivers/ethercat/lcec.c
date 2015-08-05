//
//    Copyright (C) 2012 Sascha Ittner <sascha.ittner@modusoft.de>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#include "lcec.h"
#include "lcec_generic.h"
#include "lcec_ek1100.h"
#include "lcec_el1xxx.h"
#include "lcec_el2xxx.h"
#include "lcec_el31x2.h"
#include "lcec_el40x2.h"
#include "lcec_el41x2.h"
#include "lcec_el5101.h"
#include "lcec_el5151.h"
#include "lcec_el5152.h"
#include "lcec_el2521.h"
#include "lcec_el7342.h"
#include "lcec_el95xx.h"
#include "lcec_stmds5k.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sascha Ittner <sascha.ittner@modusoft.de>");
MODULE_DESCRIPTION("Driver for EtherCAT devices");

typedef struct lcec_typelist {
  LCEC_SLAVE_TYPE_T type;
  uint32_t vid;
  uint32_t pid;
  int pdo_entry_count;
  lcec_slave_init_t proc_init;
} lcec_typelist_t;

static const lcec_typelist_t types[] = {
  // bus coupler
  { lcecSlaveTypeEK1100, LCEC_EK1100_VID, LCEC_EK1100_PID, LCEC_EK1100_PDOS, NULL},

  // digital in
  { lcecSlaveTypeEL1002, LCEC_EL1xxx_VID, LCEC_EL1002_PID, LCEC_EL1002_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1004, LCEC_EL1xxx_VID, LCEC_EL1004_PID, LCEC_EL1004_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1008, LCEC_EL1xxx_VID, LCEC_EL1008_PID, LCEC_EL1008_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1012, LCEC_EL1xxx_VID, LCEC_EL1012_PID, LCEC_EL1012_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1014, LCEC_EL1xxx_VID, LCEC_EL1014_PID, LCEC_EL1014_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1018, LCEC_EL1xxx_VID, LCEC_EL1018_PID, LCEC_EL1018_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1024, LCEC_EL1xxx_VID, LCEC_EL1024_PID, LCEC_EL1024_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1034, LCEC_EL1xxx_VID, LCEC_EL1034_PID, LCEC_EL1034_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1084, LCEC_EL1xxx_VID, LCEC_EL1084_PID, LCEC_EL1084_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1088, LCEC_EL1xxx_VID, LCEC_EL1088_PID, LCEC_EL1088_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1094, LCEC_EL1xxx_VID, LCEC_EL1094_PID, LCEC_EL1094_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1098, LCEC_EL1xxx_VID, LCEC_EL1098_PID, LCEC_EL1098_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1104, LCEC_EL1xxx_VID, LCEC_EL1104_PID, LCEC_EL1104_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1114, LCEC_EL1xxx_VID, LCEC_EL1114_PID, LCEC_EL1114_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1124, LCEC_EL1xxx_VID, LCEC_EL1124_PID, LCEC_EL1124_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1134, LCEC_EL1xxx_VID, LCEC_EL1134_PID, LCEC_EL1134_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1144, LCEC_EL1xxx_VID, LCEC_EL1144_PID, LCEC_EL1144_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1808, LCEC_EL1xxx_VID, LCEC_EL1808_PID, LCEC_EL1808_PDOS, lcec_el1xxx_init},
  { lcecSlaveTypeEL1809, LCEC_EL1xxx_VID, LCEC_EL1809_PID, LCEC_EL1809_PDOS, lcec_el1xxx_init},

  // digital out
  { lcecSlaveTypeEL2002, LCEC_EL2xxx_VID, LCEC_EL2002_PID, LCEC_EL2002_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2004, LCEC_EL2xxx_VID, LCEC_EL2004_PID, LCEC_EL2004_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2008, LCEC_EL2xxx_VID, LCEC_EL2008_PID, LCEC_EL2008_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2022, LCEC_EL2xxx_VID, LCEC_EL2022_PID, LCEC_EL2022_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2024, LCEC_EL2xxx_VID, LCEC_EL2024_PID, LCEC_EL2024_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2032, LCEC_EL2xxx_VID, LCEC_EL2032_PID, LCEC_EL2032_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2034, LCEC_EL2xxx_VID, LCEC_EL2034_PID, LCEC_EL2034_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2042, LCEC_EL2xxx_VID, LCEC_EL2042_PID, LCEC_EL2042_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2084, LCEC_EL2xxx_VID, LCEC_EL2084_PID, LCEC_EL2084_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2088, LCEC_EL2xxx_VID, LCEC_EL2088_PID, LCEC_EL2088_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2124, LCEC_EL2xxx_VID, LCEC_EL2124_PID, LCEC_EL2124_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2808, LCEC_EL2xxx_VID, LCEC_EL2808_PID, LCEC_EL2808_PDOS, lcec_el2xxx_init},
  { lcecSlaveTypeEL2809, LCEC_EL2xxx_VID, LCEC_EL2809_PID, LCEC_EL2809_PDOS, lcec_el2xxx_init},

  // analog in, 2ch, 16 bits
  { lcecSlaveTypeEL3102, LCEC_EL31x2_VID, LCEC_EL3102_PID, LCEC_EL31x2_PDOS, lcec_el31x2_init},
  { lcecSlaveTypeEL3112, LCEC_EL31x2_VID, LCEC_EL3112_PID, LCEC_EL31x2_PDOS, lcec_el31x2_init},
  { lcecSlaveTypeEL3122, LCEC_EL31x2_VID, LCEC_EL3122_PID, LCEC_EL31x2_PDOS, lcec_el31x2_init},
  { lcecSlaveTypeEL3142, LCEC_EL31x2_VID, LCEC_EL3142_PID, LCEC_EL31x2_PDOS, lcec_el31x2_init},
  { lcecSlaveTypeEL3152, LCEC_EL31x2_VID, LCEC_EL3152_PID, LCEC_EL31x2_PDOS, lcec_el31x2_init},
  { lcecSlaveTypeEL3162, LCEC_EL31x2_VID, LCEC_EL3162_PID, LCEC_EL31x2_PDOS, lcec_el31x2_init},

  // analog out, 2ch, 12 bits
  { lcecSlaveTypeEL4002, LCEC_EL40x2_VID, LCEC_EL4002_PID, LCEC_EL40x2_PDOS, lcec_el40x2_init},
  { lcecSlaveTypeEL4012, LCEC_EL40x2_VID, LCEC_EL4012_PID, LCEC_EL40x2_PDOS, lcec_el40x2_init},
  { lcecSlaveTypeEL4022, LCEC_EL40x2_VID, LCEC_EL4022_PID, LCEC_EL40x2_PDOS, lcec_el40x2_init},
  { lcecSlaveTypeEL4032, LCEC_EL40x2_VID, LCEC_EL4032_PID, LCEC_EL40x2_PDOS, lcec_el40x2_init},

  // analog out, 2ch, 16 bits
  { lcecSlaveTypeEL4102, LCEC_EL41x2_VID, LCEC_EL4102_PID, LCEC_EL41x2_PDOS, lcec_el41x2_init},
  { lcecSlaveTypeEL4112, LCEC_EL41x2_VID, LCEC_EL4112_PID, LCEC_EL41x2_PDOS, lcec_el41x2_init},
  { lcecSlaveTypeEL4122, LCEC_EL41x2_VID, LCEC_EL4122_PID, LCEC_EL41x2_PDOS, lcec_el41x2_init},
  { lcecSlaveTypeEL4132, LCEC_EL41x2_VID, LCEC_EL4132_PID, LCEC_EL41x2_PDOS, lcec_el41x2_init},

  // encoder inputs
  { lcecSlaveTypeEL5101, LCEC_EL5101_VID, LCEC_EL5101_PID, LCEC_EL5101_PDOS, lcec_el5101_init},
  { lcecSlaveTypeEL5151, LCEC_EL5151_VID, LCEC_EL5151_PID, LCEC_EL5151_PDOS, lcec_el5151_init},
  { lcecSlaveTypeEL5152, LCEC_EL5152_VID, LCEC_EL5152_PID, LCEC_EL5152_PDOS, lcec_el5152_init},

  // pulse train (stepper) output
  { lcecSlaveTypeEL2521, LCEC_EL2521_VID, LCEC_EL2521_PID, LCEC_EL2521_PDOS, lcec_el2521_init},

  // dc servo
  { lcecSlaveTypeEL7342, LCEC_EL7342_VID, LCEC_EL7342_PID, LCEC_EL7342_PDOS, lcec_el7342_init},

  // power supply
  { lcecSlaveTypeEL9505, LCEC_EL95xx_VID, LCEC_EL9505_PID, LCEC_EL95xx_PDOS, lcec_el95xx_init},
  { lcecSlaveTypeEL9508, LCEC_EL95xx_VID, LCEC_EL9508_PID, LCEC_EL95xx_PDOS, lcec_el95xx_init},
  { lcecSlaveTypeEL9510, LCEC_EL95xx_VID, LCEC_EL9510_PID, LCEC_EL95xx_PDOS, lcec_el95xx_init},
  { lcecSlaveTypeEL9512, LCEC_EL95xx_VID, LCEC_EL9512_PID, LCEC_EL95xx_PDOS, lcec_el95xx_init},
  { lcecSlaveTypeEL9515, LCEC_EL95xx_VID, LCEC_EL9515_PID, LCEC_EL95xx_PDOS, lcec_el95xx_init},

  // stoeber MDS5000 series
  { lcecSlaveTypeStMDS5k, LCEC_STMDS5K_VID, LCEC_STMDS5K_PID, LCEC_STMDS5K_PDOS, lcec_stmds5k_init},

  { lcecSlaveTypeInvalid }
};

static lcec_master_t *first_master = NULL;
static lcec_master_t *last_master = NULL;
static int comp_id = -1;

static lcec_master_data_t *global_hal_data;
static ec_master_state_t global_ms;

int lcec_parse_config(void);
void lcec_clear_config(void);

void lcec_request_lock(void *data);
void lcec_release_lock(void *data);

lcec_master_data_t *lcec_init_master_hal(const char *pfx);
lcec_slave_state_t *lcec_init_slave_state_hal(char *master_name, char *slave_name);
void lcec_update_master_hal(lcec_master_data_t *hal_data, ec_master_state_t *ms);
void lcec_update_slave_state_hal(lcec_slave_state_t *hal_data, ec_slave_config_state_t *ss);

void lcec_read_all(void *arg, long period);
void lcec_write_all(void *arg, long period);
void lcec_read_master(void *arg, long period);
void lcec_write_master(void *arg, long period);

int rtapi_app_main(void) {
  int slave_count;
  lcec_master_t *master;
  lcec_slave_t *slave;
  char name[HAL_NAME_LEN + 1];
  ec_pdo_entry_reg_t *pdo_entry_regs;
  lcec_slave_sdoconf_t *sdo_config;

  // connect to the HAL
  if ((comp_id = hal_init (LCEC_MODULE_NAME)) < 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "hal_init() failed\n");
    goto fail0;
  }

  // parse configuration
  if ((slave_count = lcec_parse_config()) < 0) {
    goto fail1;
  }

  // init global hal data
  if ((global_hal_data = lcec_init_master_hal(LCEC_MODULE_NAME)) == NULL) {
    goto fail2;
  }

  // initialize masters
  for (master = first_master; master != NULL; master = master->next) {
    // request ethercat master
    if (!(master->master = ecrt_request_master(master->index))) {
      rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "requesting master %s (index %d) failed\n", master->name, master->index);
      goto fail2;
    }

#ifdef __KERNEL__
    // register callbacks
    ecrt_master_callbacks(master->master, lcec_request_lock, lcec_release_lock, master);
#endif

    // create domain
    if (!(master->domain = ecrt_master_create_domain(master->master))) {
      rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "master %s domain creation failed\n", master->name);
      goto fail2;
    }

    // initialize slaves
    pdo_entry_regs = master->pdo_entry_regs;
    for (slave = master->first_slave; slave != NULL; slave = slave->next) {
      // read slave config
      if (!(slave->config = ecrt_master_slave_config(master->master, 0, slave->index, slave->vid, slave->pid))) {
        rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "fail to read slave %s.%s configuration\n", master->name, slave->name);
        goto fail2;
      }

      // initialize sdos
      if (slave->sdo_config != NULL) {
        for (sdo_config = slave->sdo_config; sdo_config->index != 0xffff; sdo_config = (lcec_slave_sdoconf_t *) &sdo_config->data[sdo_config->length]) {
          if (sdo_config->subindex == LCEC_CONF_SDO_COMPLETE_SUBIDX) {
            if (ecrt_slave_config_complete_sdo(slave->config, sdo_config->index, &sdo_config->data[0], sdo_config->length) != 0) {
              rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "fail to configure slave %s.%s sdo %04x (complete)\n", master->name, slave->name, sdo_config->index);
            }
          } else {
            if (ecrt_slave_config_sdo(slave->config, sdo_config->index, sdo_config->subindex, &sdo_config->data[0], sdo_config->length) != 0) {
              rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "fail to configure slave %s.%s sdo %04x:%02x\n", master->name, slave->name, sdo_config->index, sdo_config->subindex);
            }
          }
        }
      }

      // setup pdos
      if (slave->proc_init != NULL) {
        if ((slave->proc_init(comp_id, slave, pdo_entry_regs)) != 0) {
          goto fail2;
        }
      }
      pdo_entry_regs += slave->pdo_entry_count;

      // configure dc for this slave
      if (slave->dc_conf != NULL) {
        ecrt_slave_config_dc(slave->config, slave->dc_conf->assignActivate,
          slave->dc_conf->sync0Cycle, slave->dc_conf->sync0Shift,
          slave->dc_conf->sync1Cycle, slave->dc_conf->sync1Shift);
        rtapi_print_msg (RTAPI_MSG_DBG, LCEC_MSG_PFX "configuring DC for slave %s.%s: assignActivate=x%x sync0Cycle=%d sync0Shift=%d sync1Cycle=%d sync1Shift=%d\n",
          master->name, slave->name, slave->dc_conf->assignActivate,
          slave->dc_conf->sync0Cycle, slave->dc_conf->sync0Shift,
          slave->dc_conf->sync1Cycle, slave->dc_conf->sync1Shift);
      }

      // Configure the slave's watchdog times.
      if (slave->wd_conf != NULL) {
        ecrt_slave_config_watchdog(slave->config, slave->wd_conf->divider, slave->wd_conf->intervals);
      }

      // configure slave
      if (slave->sync_info != NULL) {
        if (ecrt_slave_config_pdos(slave->config, EC_END, slave->sync_info)) {
          rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "fail to configure slave %s.%s\n", master->name, slave->name);
          goto fail2;
        }
      }

      // export state pins
      if ((slave->hal_state_data = lcec_init_slave_state_hal(master->name, slave->name)) == NULL) {
        goto fail2;
      }
    }

    // terminate POD entries
    pdo_entry_regs->index = 0;

    // register PDO entries
    if (ecrt_domain_reg_pdo_entry_list(master->domain, master->pdo_entry_regs)) {
      rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "master %s PDO entry registration failed\n", master->name);
      goto fail2;
    }

    // activating master
    if (ecrt_master_activate(master->master)) {
      rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "failed to activate master %s\n", master->name);
      goto fail2;
    }

    // Get internal process data for domain
    master->process_data = ecrt_domain_data(master->domain);
    master->process_data_len = ecrt_domain_size(master->domain);

    // init hal data
    rtapi_snprintf(name, HAL_NAME_LEN, "%s.%s", LCEC_MODULE_NAME, master->name);
    if ((master->hal_data = lcec_init_master_hal(name)) == NULL) {
      goto fail2;
    }

    // export read function
    rtapi_snprintf(name, HAL_NAME_LEN, "%s.%s.read", LCEC_MODULE_NAME, master->name);
    if (hal_export_funct(name, lcec_read_master, master, 0, 0, comp_id) != 0) {
      rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "master %s read funct export failed\n", master->name);
      goto fail2;
    }
    // export write function
    rtapi_snprintf(name, HAL_NAME_LEN, "%s.%s.write", LCEC_MODULE_NAME, master->name);
    if (hal_export_funct(name, lcec_write_master, master, 0, 0, comp_id) != 0) {
      rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "master %s write funct export failed\n", master->name);
      goto fail2;
    }
  }

  // export read-all function
  rtapi_snprintf(name, HAL_NAME_LEN, "%s.read-all", LCEC_MODULE_NAME);
  if (hal_export_funct(name, lcec_read_all, NULL, 0, 0, comp_id) != 0) {
    rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "read-all funct export failed\n");
    goto fail2;
  }
  // export write-all function
  rtapi_snprintf(name, HAL_NAME_LEN, "%s.write-all", LCEC_MODULE_NAME);
  if (hal_export_funct(name, lcec_write_all, NULL, 0, 0, comp_id) != 0) {
    rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "write-all funct export failed\n");
    goto fail2;
  }

  rtapi_print_msg(RTAPI_MSG_INFO, LCEC_MSG_PFX "installed driver for %d slaves\n", slave_count);
  hal_ready (comp_id);
  return 0;

fail2:
  lcec_clear_config();
fail1:
  hal_exit(comp_id);
fail0:
  return -EINVAL;
}

void rtapi_app_exit(void) {
  lcec_master_t *master;

  // deactivate all masters
  for (master = first_master; master != NULL; master = master->next) {
    ecrt_master_deactivate(master->master);
  }

  lcec_clear_config();
  hal_exit(comp_id);
}

int lcec_parse_config(void) {
  int shmem_id;
  void *shmem_ptr;
  LCEC_CONF_HEADER_T *header;
  size_t length;
  void *conf;
  int slave_count;
  const lcec_typelist_t *type;
  lcec_master_t *master;
  lcec_slave_t *slave;
  lcec_slave_dc_t *dc;
  lcec_slave_watchdog_t *wd;
  ec_pdo_entry_reg_t *pdo_entry_regs;
  LCEC_CONF_TYPE_T conf_type;
  LCEC_CONF_MASTER_T *master_conf;
  LCEC_CONF_SLAVE_T *slave_conf;
  LCEC_CONF_DC_T *dc_conf;
  LCEC_CONF_WATCHDOG_T *wd_conf;
  LCEC_CONF_SYNCMANAGER_T *sm_conf;
  LCEC_CONF_PDO_T *pdo_conf;
  LCEC_CONF_PDOENTRY_T *pe_conf;
  LCEC_CONF_SDOCONF_T *sdo_conf;
  ec_pdo_entry_info_t *generic_pdo_entries;
  ec_pdo_info_t *generic_pdos;
  ec_sync_info_t *generic_sync_managers;
  lcec_generic_pin_t *generic_hal_data;
  hal_pin_dir_t generic_hal_dir;
  lcec_slave_sdoconf_t *sdo_config;

  // initialize list
  first_master = NULL;
  last_master = NULL;

  // try to get config header
  shmem_id = rtapi_shmem_new(LCEC_CONF_SHMEM_KEY, comp_id, sizeof(LCEC_CONF_HEADER_T));
  if (shmem_id < 0) {
    rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "couldn't allocate user/RT shared memory\n");
    goto fail0;
  }
  if (rtapi_shmem_getptr(shmem_id, &shmem_ptr) < 0 ) {
    rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "couldn't map user/RT shared memory\n");
    goto fail1;
  }

  // check magic, get length and close shmem
  header = shmem_ptr;
  if (header->magic != LCEC_CONF_SHMEM_MAGIC) {
    rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "lcec_conf is not loaded\n");
    goto fail1;
  }
  length = header->length;
  rtapi_shmem_delete(shmem_id, comp_id);

  // reopen shmem with proper size
  shmem_id = rtapi_shmem_new(LCEC_CONF_SHMEM_KEY, comp_id, sizeof(LCEC_CONF_HEADER_T) + length);
  if (shmem_id < 0) {
    rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "couldn't allocate user/RT shared memory\n");
    goto fail0;
  }
  if (rtapi_shmem_getptr(shmem_id, &shmem_ptr) < 0 ) {
    rtapi_print_msg (RTAPI_MSG_ERR, LCEC_MSG_PFX "couldn't map user/RT shared memory\n");
    goto fail1;
  }

  // get pointer to config
  conf = shmem_ptr + sizeof(LCEC_CONF_HEADER_T);

  // process config items
  slave_count = 0;
  master = NULL;
  slave = NULL;
  generic_pdo_entries = NULL;
  generic_pdos = NULL;
  generic_sync_managers = NULL;
  generic_hal_data = NULL;
  generic_hal_dir = 0;
  sdo_config = NULL;
  while((conf_type = ((LCEC_CONF_NULL_T *)conf)->confType) != lcecConfTypeNone) {
    // get type
    switch (conf_type) {
      case lcecConfTypeMaster:
        // get config token
        master_conf = (LCEC_CONF_MASTER_T *)conf;
        conf += sizeof(LCEC_CONF_MASTER_T);

        // alloc master memory
        master = kzalloc(sizeof(lcec_master_t), GFP_KERNEL);
        if (master == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Unable to allocate master %d structure memory\n", master_conf->index);
          goto fail2;
        }

        // initialize master
        master->index = master_conf->index;
        strncpy(master->name, master_conf->name, LCEC_CONF_STR_MAXLEN);
        master->name[LCEC_CONF_STR_MAXLEN - 1] = 0;
        master->mutex = 0;
        master->app_time = 0;
        master->app_time_period = master_conf->appTimePeriod;
        master->sync_ref_cnt = 0;
        master->sync_ref_cycles = master_conf->refClockSyncCycles;

        // add master to list
        LCEC_LIST_APPEND(first_master, last_master, master);
        break;

      case lcecConfTypeSlave:
        // get config token
        slave_conf = (LCEC_CONF_SLAVE_T *)conf;
        conf += sizeof(LCEC_CONF_SLAVE_T);

        // check for master
        if (master == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Master node for slave missing\n");
          goto fail2;
        }

        // check for valid slave type
        if (slave_conf->type == lcecSlaveTypeGeneric) {
          type = NULL;
        } else {
          for (type = types; type->type != slave_conf->type && type->type != lcecSlaveTypeInvalid; type++);
          if (type->type == lcecSlaveTypeInvalid) {
            rtapi_print_msg(RTAPI_MSG_WARN, LCEC_MSG_PFX "Invalid slave type %d\n", slave_conf->type);
            continue;
          }
        }

        // create new slave
        slave = kzalloc(sizeof(lcec_slave_t), GFP_KERNEL);
        if (slave == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Unable to allocate slave %s.%s structure memory\n", master->name, slave_conf->name);
          goto fail2;
        }

        // initialize slave
        generic_pdo_entries = NULL;
        generic_pdos = NULL;
        generic_sync_managers = NULL;
        generic_hal_data = NULL;
        generic_hal_dir = 0;
        sdo_config = NULL;

        slave->index = slave_conf->index;
        strncpy(slave->name, slave_conf->name, LCEC_CONF_STR_MAXLEN);
        slave->name[LCEC_CONF_STR_MAXLEN - 1] = 0;
        slave->master = master;

        // add slave to list
        LCEC_LIST_APPEND(master->first_slave, master->last_slave, slave);

        if (type != NULL) {
          // normal slave
          slave->vid = type->vid;
          slave->pid = type->pid;
          slave->pdo_entry_count = type->pdo_entry_count;
          slave->proc_init = type->proc_init;
        } else {
          // generic slave
          slave->vid = slave_conf->vid;
          slave->pid = slave_conf->pid;
          slave->pdo_entry_count = slave_conf->pdoMappingCount;
          slave->proc_init = lcec_generic_init;

          // alloc hal memory
          if ((generic_hal_data = hal_malloc(sizeof(lcec_generic_pin_t) * slave_conf->pdoMappingCount)) == NULL) {
            rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "hal_malloc() for slave %s.%s failed\n", master->name, slave_conf->name);
            goto fail2;
          }
          memset(generic_hal_data, 0, sizeof(lcec_generic_pin_t) * slave_conf->pdoMappingCount);

          // alloc pdo entry memory
          generic_pdo_entries = kzalloc(sizeof(ec_pdo_entry_info_t) * slave_conf->pdoEntryCount, GFP_KERNEL);
          if (generic_pdo_entries == NULL) {
            rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Unable to allocate slave %s.%s generic pdo entry memory\n", master->name, slave_conf->name);
            goto fail2;
          }

          // alloc pdo memory
          generic_pdos = kzalloc(sizeof(ec_pdo_info_t) * slave_conf->pdoCount, GFP_KERNEL);
          if (generic_pdos == NULL) {
            rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Unable to allocate slave %s.%s generic pdo memory\n", master->name, slave_conf->name);
            goto fail2;
          }

          // alloc sync manager memory
          generic_sync_managers = kzalloc(sizeof(ec_sync_info_t) * (slave_conf->syncManagerCount + 1), GFP_KERNEL);
          if (generic_sync_managers == NULL) {
            rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Unable to allocate slave %s.%s generic sync manager memory\n", master->name, slave_conf->name);
            goto fail2;
          }
          generic_sync_managers->index = 0xff;
        }

        // alloc sdo config memory
        if (slave_conf->sdoConfigLength > 0) {
          sdo_config = kzalloc(slave_conf->sdoConfigLength + sizeof(lcec_slave_sdoconf_t), GFP_KERNEL);
          if (sdo_config == NULL) {
            rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Unable to allocate slave %s.%s generic pdo entry memory\n", master->name, slave_conf->name);
            goto fail2;
          }
        }

        slave->hal_data = generic_hal_data;
        slave->generic_pdo_entries = generic_pdo_entries;
        slave->generic_pdos = generic_pdos;
        slave->generic_sync_managers = generic_sync_managers;
        if (slave_conf->configPdos) {
          slave->sync_info = generic_sync_managers;
        }
        slave->sdo_config = sdo_config;
        slave->dc_conf = NULL;
        slave->wd_conf = NULL;

        // update master's POD entry count
        master->pdo_entry_count += slave->pdo_entry_count;

        // update slave count
        slave_count++;
        break;

      case lcecConfTypeDcConf:
        // get config token
        dc_conf = (LCEC_CONF_DC_T *)conf;
        conf += sizeof(LCEC_CONF_DC_T);

        // check for slave
        if (slave == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Slave node for dc config missing\n");
          goto fail2;
        }

        // check for double dc config
        if (slave->dc_conf != NULL) {
          rtapi_print_msg(RTAPI_MSG_WARN, LCEC_MSG_PFX "Double dc config for slave %s.%s\n", master->name, slave->name);
          continue;
        }

        // create new dc config
        dc = kzalloc(sizeof(lcec_slave_dc_t), GFP_KERNEL);
        if (dc == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Unable to allocate slave %s.%s dc config memory\n", master->name, slave->name);
          goto fail2;
        }

        // initialize dc conf
        dc->assignActivate = dc_conf->assignActivate;
        dc->sync0Cycle = dc_conf->sync0Cycle;
        dc->sync0Shift = dc_conf->sync0Shift;
        dc->sync1Cycle = dc_conf->sync1Cycle;
        dc->sync1Shift = dc_conf->sync1Shift;

        // add to slave
        slave->dc_conf = dc;
        break;

      case lcecConfTypeWatchdog:
        // get config token
        wd_conf = (LCEC_CONF_WATCHDOG_T *)conf;
        conf += sizeof(LCEC_CONF_WATCHDOG_T);

        // check for slave
        if (slave == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Slave node for watchdog config missing\n");
          goto fail2;
        }

        // check for double wd config
        if (slave->wd_conf != NULL) {
          rtapi_print_msg(RTAPI_MSG_WARN, LCEC_MSG_PFX "Double watchdog config for slave %s.%s\n", master->name, slave->name);
          continue;
        }

        // create new wd config
        wd = kzalloc(sizeof(lcec_slave_watchdog_t), GFP_KERNEL);
        if (wd == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Unable to allocate slave %s.%s watchdog config memory\n", master->name, slave->name);
          goto fail2;
        }

        // initialize wd conf
        wd->divider = wd_conf->divider;
        wd->intervals = wd_conf->intervals;

        // add to slave
        slave->wd_conf = wd;
        break;

      case lcecConfTypeSyncManager:
        // get config token
        sm_conf = (LCEC_CONF_SYNCMANAGER_T *)conf;
        conf += sizeof(LCEC_CONF_SYNCMANAGER_T);

        // check for syncmanager
        if (generic_sync_managers == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Sync manager for generic device missing\n");
          goto fail2;
        }

        // check for pdos
        if (generic_pdos == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "PDOs for generic device missing\n");
          goto fail2;
        }

        // initialize sync manager
        generic_sync_managers->index = sm_conf->index;
        generic_sync_managers->dir = sm_conf->dir;
        generic_sync_managers->n_pdos = sm_conf->pdoCount;
        generic_sync_managers->pdos = sm_conf->pdoCount == 0 ? NULL : generic_pdos;

        // get hal direction
        switch (sm_conf->dir) {
          case EC_DIR_INPUT:
            generic_hal_dir = HAL_OUT;
            break;
          case EC_DIR_OUTPUT:
            generic_hal_dir = HAL_IN;
            break;
          default:
            generic_hal_dir = 0;
        }

        // next syncmanager
        generic_sync_managers++;
        generic_sync_managers->index = 0xff;
        break;

      case lcecConfTypePdo:
        // get config token
        pdo_conf = (LCEC_CONF_PDO_T *)conf;
        conf += sizeof(LCEC_CONF_PDO_T);

        // check for pdos
        if (generic_pdos == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "PDOs for generic device missing\n");
          goto fail2;
        }

        // check for pdos entries
        if (generic_pdo_entries == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "PDO entries for generic device missing\n");
          goto fail2;
        }

        // initialize pdo
        generic_pdos->index = pdo_conf->index;
        generic_pdos->n_entries = pdo_conf->pdoEntryCount;
        generic_pdos->entries = pdo_conf->pdoEntryCount == 0 ? NULL : generic_pdo_entries;

        // next pdo
        generic_pdos++;
        break;

      case lcecConfTypePdoEntry:
        // get config token
        pe_conf = (LCEC_CONF_PDOENTRY_T *)conf;
        conf += sizeof(LCEC_CONF_PDOENTRY_T);

        // check for pdos entries
        if (generic_pdo_entries == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "PDO entries for generic device missing\n");
          goto fail2;
        }

        // check for hal data
        if (generic_hal_data == NULL) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "HAL data for generic device missing\n");
          goto fail2;
        }

        // check for hal dir
        if (generic_hal_dir == 0) {
          rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "HAL direction for generic device missing\n");
          goto fail2;
        }

        // initialize pdo entry
        generic_pdo_entries->index = pe_conf->index;
        generic_pdo_entries->subindex = pe_conf->subindex;
        generic_pdo_entries->bit_length = pe_conf->bitLength;

        // initialize hal data
        if (pe_conf->halPin[0] != 0) {
          strncpy(generic_hal_data->name, pe_conf->halPin, LCEC_CONF_STR_MAXLEN);
          generic_hal_data->name[LCEC_CONF_STR_MAXLEN - 1] = 0;
          generic_hal_data->type = pe_conf->halType;
          generic_hal_data->dir = generic_hal_dir;
          generic_hal_data->pdo_idx = pe_conf->index;
          generic_hal_data->pdo_sidx = pe_conf->subindex;
          generic_hal_data->pdo_len = pe_conf->bitLength;
          generic_hal_data->scale = pe_conf->scale;
          generic_hal_data++;
        }

        // next pdo entry
        generic_pdo_entries++;
        break;

      case lcecConfTypeSdoConfig:
        // get config token
        sdo_conf = (LCEC_CONF_SDOCONF_T *)conf;
        conf += sizeof(LCEC_CONF_SDOCONF_T) + sdo_conf->length;

        // copy attributes
        sdo_config->index = sdo_conf->index;
        sdo_config->subindex = sdo_conf->subindex;
        sdo_config->length = sdo_conf->length;

        // copy data
        memcpy(sdo_config->data, sdo_conf->data, sdo_config->length);

        sdo_config = (lcec_slave_sdoconf_t *) &sdo_config->data[sdo_config->length];
        sdo_config->index = 0xffff;
        break;

      default:
        rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Unknown config item type\n");
        goto fail2;
    }
  }

  // close shmem
  rtapi_shmem_delete(shmem_id, comp_id);

  // allocate PDO entity memory
  for (master = first_master; master != NULL; master = master->next) {
    pdo_entry_regs = kzalloc(sizeof(ec_pdo_entry_reg_t) * (master->pdo_entry_count + 1), GFP_KERNEL);
    if (pdo_entry_regs == NULL) {
      rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "Unable to allocate master %s PDO entry memory\n", master->name);
      goto fail2;
    }
    master->pdo_entry_regs = pdo_entry_regs;
  }

  return slave_count;

fail2:
  lcec_clear_config();
fail1:
  rtapi_shmem_delete(shmem_id, comp_id);
fail0:
  return -1;
}

void lcec_clear_config(void) {
  lcec_master_t *master, *prev_master;
  lcec_slave_t *slave, *prev_slave;

  // iterate all masters
  master = last_master;
  while (master != NULL) {
    prev_master = master->prev;

    // iterate all masters
    slave = master->last_slave;
    while (slave != NULL) {
      prev_slave = slave->prev;

      // cleanup slave
      if (slave->proc_cleanup != NULL) {
        slave->proc_cleanup(slave);
      }

      // free slave
      if (slave->sdo_config != NULL) {
        kfree(slave->sdo_config);
      }
      if (slave->generic_pdo_entries != NULL) {
        kfree(slave->generic_pdo_entries);
      }
      if (slave->generic_pdos != NULL) {
        kfree(slave->generic_pdos);
      }
      if (slave->generic_sync_managers != NULL) {
        kfree(slave->generic_sync_managers);
      }
      if (slave->dc_conf != NULL) {
        kfree(slave->dc_conf);
      }
      if (slave->wd_conf != NULL) {
        kfree(slave->wd_conf);
      }
      kfree(slave);
      slave = prev_slave;
    }

    // release master
    if (master->master) {
      ecrt_release_master(master->master);
    }

    // free PDO entry memory
    if (master->pdo_entry_regs != NULL) {
      kfree(master->pdo_entry_regs);
    }

    // free master
    kfree(master);
    master = prev_master;
  }
}

void lcec_request_lock(void *data) {
  lcec_master_t *master = (lcec_master_t *) data;
  rtapi_mutex_get(&master->mutex);
}

void lcec_release_lock(void *data) {
  lcec_master_t *master = (lcec_master_t *) data;
  rtapi_mutex_give(&master->mutex);
}

lcec_master_data_t *lcec_init_master_hal(const char *pfx) {
  lcec_master_data_t *hal_data;

  // alloc hal data
  if ((hal_data = hal_malloc(sizeof(lcec_master_data_t))) == NULL) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "hal_malloc() for %s failed\n", pfx);
    return NULL;
  }
  memset(hal_data, 0, sizeof(lcec_master_data_t));

  // export pins
  if (hal_pin_u32_newf(HAL_OUT, &(hal_data->slaves_responding), comp_id, "%s.slaves-responding", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.slaves-responding failed\n", pfx);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_init), comp_id, "%s.state-init", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.state-init failed\n", pfx);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_preop), comp_id, "%s.state-preop", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.state-preop failed\n", pfx);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_safeop), comp_id, "%s.state-safeop", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.state-safeop failed\n", pfx);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_op), comp_id, "%s.state-op", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.state-op failed\n", pfx);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->link_up), comp_id, "%s.link-up", pfx) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.link-up failed\n", pfx);
    return NULL;
  }

  // initialize pins
  *(hal_data->slaves_responding) = 0;
  *(hal_data->state_init) = 0;
  *(hal_data->state_preop) = 0;
  *(hal_data->state_safeop) = 0;
  *(hal_data->state_op) = 0;
  *(hal_data->link_up) = 0;

  return hal_data;
}

lcec_slave_state_t *lcec_init_slave_state_hal(char *master_name, char *slave_name) {
  lcec_slave_state_t *hal_data;

  // alloc hal data
  if ((hal_data = hal_malloc(sizeof(lcec_slave_state_t))) == NULL) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "hal_malloc() for %s.%s.%s failed\n", LCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  memset(hal_data, 0, sizeof(lcec_master_data_t));

  // export pins
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->online), comp_id, "%s.%s.%s.slave-online", LCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.slaves-online failed\n", LCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->operational), comp_id, "%s.%s.%s.slave-oper", LCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.slaves-oper failed\n", LCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_init), comp_id, "%s.%s.%s.slave-state-init", LCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.state-state-init failed\n", LCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_preop), comp_id, "%s.%s.%s.slave-state-preop", LCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.state-state-preop failed\n", LCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_safeop), comp_id, "%s.%s.%s.slave-state-safeop", LCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.state-state-safeop failed\n", LCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }
  if (hal_pin_bit_newf(HAL_OUT, &(hal_data->state_op), comp_id, "%s.%s.%s.slave-state-op", LCEC_MODULE_NAME, master_name, slave_name) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "exporting pin %s.%s.%s.state-state-op failed\n", LCEC_MODULE_NAME, master_name, slave_name);
    return NULL;
  }

  // initialize pins
  *(hal_data->online) = 0;
  *(hal_data->operational) = 0;
  *(hal_data->state_init) = 0;
  *(hal_data->state_preop) = 0;
  *(hal_data->state_safeop) = 0;
  *(hal_data->state_op) = 0;

  return hal_data;
}

void lcec_update_master_hal(lcec_master_data_t *hal_data, ec_master_state_t *ms) {
  *(hal_data->slaves_responding) = ms->slaves_responding;
  *(hal_data->state_init) = (ms->al_states & 0x01) != 0;
  *(hal_data->state_preop) = (ms->al_states & 0x02) != 0;
  *(hal_data->state_safeop) = (ms->al_states & 0x04) != 0;
  *(hal_data->state_op) = (ms->al_states & 0x08) != 0;
  *(hal_data->link_up) = ms->link_up;
}

void lcec_update_slave_state_hal(lcec_slave_state_t *hal_data, ec_slave_config_state_t *ss) {
  *(hal_data->online) = ss->online;
  *(hal_data->operational) = ss->operational;
  *(hal_data->state_init) = (ss->al_state & 0x01) != 0;
  *(hal_data->state_preop) = (ss->al_state & 0x02) != 0;
  *(hal_data->state_safeop) = (ss->al_state & 0x04) != 0;
  *(hal_data->state_op) = (ss->al_state & 0x08) != 0;
}

void lcec_read_all(void *arg, long period) {
  lcec_master_t *master;

  // initialize global state
  global_ms.slaves_responding = 0;
  global_ms.al_states = 0;
  global_ms.link_up = (first_master != NULL);

  // process slaves
  for (master = first_master; master != NULL; master = master->next) {
    lcec_read_master(master, period);
  }

  // update global state pins
  lcec_update_master_hal(global_hal_data, &global_ms);
}

void lcec_write_all(void *arg, long period) {
  lcec_master_t *master;

  // process slaves
  for (master = first_master; master != NULL; master = master->next) {
    lcec_write_master(master, period);
  }
}

void lcec_read_master(void *arg, long period) {
  lcec_master_t *master = (lcec_master_t *) arg;
  lcec_slave_t *slave;
  ec_master_state_t ms;

  // receive process data & master state
  rtapi_mutex_get(&master->mutex);
  ecrt_master_receive(master->master);
  ecrt_domain_process(master->domain);
  ecrt_master_state(master->master, &ms);
  rtapi_mutex_give(&master->mutex);

  // update state pins
  lcec_update_master_hal(master->hal_data, &ms);

  // update global state
  global_ms.slaves_responding += ms.slaves_responding;
  global_ms.al_states |= ms.al_states;
  global_ms.link_up = global_ms.link_up && ms.link_up;

  // process slaves
  for (slave = master->first_slave; slave != NULL; slave = slave->next) {
    // get slaves state
    rtapi_mutex_get(&master->mutex);
    ecrt_slave_config_state(slave->config, &slave->state);
    rtapi_mutex_give(&master->mutex);
    lcec_update_slave_state_hal(slave->hal_state_data, &slave->state);

    // process read function
    if (slave->proc_read != NULL) {
      slave->proc_read(slave, period);
    }
  }
}

void lcec_write_master(void *arg, long period) {
  lcec_master_t *master = (lcec_master_t *) arg;
  lcec_slave_t *slave;

  // process slaves
  for (slave = master->first_slave; slave != NULL; slave = slave->next) {
    if (slave->proc_write != NULL) {
      slave->proc_write(slave, period);
    }
  }

  // send process data
  rtapi_mutex_get(&master->mutex);

  // update application time
  master->app_time += master->app_time_period;
  ecrt_master_application_time(master->master, master->app_time);

  // sync ref clock to master
  if (master->sync_ref_cnt == 0) {
    master->sync_ref_cnt = master->sync_ref_cycles;
    ecrt_master_sync_reference_clock(master->master);
  }
  master->sync_ref_cnt--;

  // sync slaves to ref clock
  ecrt_master_sync_slave_clocks(master->master);

  // send domain data
  ecrt_domain_queue(master->domain);
  ecrt_master_send(master->master);
  rtapi_mutex_give(&master->mutex);
}

ec_sdo_request_t *lcec_read_sdo(struct lcec_slave *slave, uint16_t index, uint8_t subindex, size_t size) {
  lcec_master_t *master = slave->master;
  ec_sdo_request_t *sdo;
  ec_request_state_t sdo_state;
  unsigned long jiffies_start;

  // create request
  if (!(sdo = ecrt_slave_config_create_sdo_request(slave->config, index, subindex, size))) {
  rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "slave %s.%s: Failed to create SDO request (0x%04x:0x%02x)\n", master->name, slave->name, index, subindex);
      return NULL;
  }

  // set timeout
  ecrt_sdo_request_timeout(sdo, LCEC_SDO_REQ_TIMEOUT);

  // send request
  ecrt_sdo_request_read(sdo);

  // wait for completition (master's time out does not work here. why???)
  jiffies_start = jiffies;
  while ((jiffies -  jiffies_start) < (HZ * LCEC_SDO_REQ_TIMEOUT / 1000) && (sdo_state = ecrt_sdo_request_state(sdo)) == EC_REQUEST_BUSY) {
    schedule();
  }

  // check state
  if (sdo_state != EC_REQUEST_SUCCESS) {
    rtapi_print_msg(RTAPI_MSG_ERR, LCEC_MSG_PFX "slave %s.%s: Failed to execute SDO request (0x%04x:0x%02x)\n", master->name, slave->name, index, subindex);
    return NULL;
  }

  return sdo;
}

