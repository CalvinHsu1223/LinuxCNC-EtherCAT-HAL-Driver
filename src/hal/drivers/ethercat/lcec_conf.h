//
//    Copyright (C) 2011 Sascha Ittner <sascha.ittner@modusoft.de>
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
#ifndef _LCEC_CONF_H_
#define _LCEC_CONF_H_

#include "hal.h"
#include "ecrt.h"

#define LCEC_MODULE_NAME "lcec"

#define LCEC_CONF_SHMEM_KEY   0xACB572C7
#define LCEC_CONF_SHMEM_MAGIC 0x036ED5A3

#define LCEC_CONF_STR_MAXLEN 32

typedef enum {
  lcecConfTypeNone,
  lcecConfTypeMaster,
  lcecConfTypeSlave,
  lcecConfTypeDcConf,
  lcecConfTypeWatchdog,
  lcecConfTypeSyncManager,
  lcecConfTypePdo,
  lcecConfTypePdoEntry,
  lcecConfTypeSdoConfig,
  lcecConfTypeSdoDataRaw
} LCEC_CONF_TYPE_T;

typedef enum {
  lcecSlaveTypeInvalid,
  lcecSlaveTypeGeneric,
  lcecSlaveTypeEK1100,
  lcecSlaveTypeEL1002,
  lcecSlaveTypeEL1004,
  lcecSlaveTypeEL1008,
  lcecSlaveTypeEL1012,
  lcecSlaveTypeEL1014,
  lcecSlaveTypeEL1018,
  lcecSlaveTypeEL1024,
  lcecSlaveTypeEL1034,
  lcecSlaveTypeEL1084,
  lcecSlaveTypeEL1088,
  lcecSlaveTypeEL1094,
  lcecSlaveTypeEL1098,
  lcecSlaveTypeEL1104,
  lcecSlaveTypeEL1114,
  lcecSlaveTypeEL1124,
  lcecSlaveTypeEL1134,
  lcecSlaveTypeEL1144,
  lcecSlaveTypeEL1808,
  lcecSlaveTypeEL1809,
  lcecSlaveTypeEL2002,
  lcecSlaveTypeEL2004,
  lcecSlaveTypeEL2008,
  lcecSlaveTypeEL2022,
  lcecSlaveTypeEL2024,
  lcecSlaveTypeEL2032,
  lcecSlaveTypeEL2034,
  lcecSlaveTypeEL2042,
  lcecSlaveTypeEL2084,
  lcecSlaveTypeEL2088,
  lcecSlaveTypeEL2124,
  lcecSlaveTypeEL2808,
  lcecSlaveTypeEL2809,
  lcecSlaveTypeEL3102,
  lcecSlaveTypeEL3112,
  lcecSlaveTypeEL3122,
  lcecSlaveTypeEL3142,
  lcecSlaveTypeEL3152,
  lcecSlaveTypeEL3162,
  lcecSlaveTypeEL4002,
  lcecSlaveTypeEL4012,
  lcecSlaveTypeEL4022,
  lcecSlaveTypeEL4032,
  lcecSlaveTypeEL4102,
  lcecSlaveTypeEL4112,
  lcecSlaveTypeEL4122,
  lcecSlaveTypeEL4132,
  lcecSlaveTypeEL5101,
  lcecSlaveTypeEL5151,
  lcecSlaveTypeEL5152,
  lcecSlaveTypeEL2521,
  lcecSlaveTypeEL7342,
  lcecSlaveTypeEL9505,
  lcecSlaveTypeEL9508,
  lcecSlaveTypeEL9510,
  lcecSlaveTypeEL9512,
  lcecSlaveTypeEL9515,
  lcecSlaveTypeStMDS5k
} LCEC_SLAVE_TYPE_T;

typedef struct {
  uint32_t magic;
  size_t length;
} LCEC_CONF_HEADER_T;

typedef struct {
  LCEC_CONF_TYPE_T confType;
  int index;
  uint32_t appTimePeriod;
  int refClockSyncCycles;
  char name[LCEC_CONF_STR_MAXLEN];
} LCEC_CONF_MASTER_T;

typedef struct {
  LCEC_CONF_TYPE_T confType;
  int index;
  LCEC_SLAVE_TYPE_T type;
  uint32_t vid;
  uint32_t pid;
  int configPdos;
  unsigned int syncManagerCount;
  unsigned int pdoCount;
  unsigned int pdoEntryCount;
  unsigned int pdoMappingCount;
  size_t sdoConfigLength;
  char name[LCEC_CONF_STR_MAXLEN];
} LCEC_CONF_SLAVE_T;

typedef struct {
  LCEC_CONF_TYPE_T confType;
  uint16_t assignActivate;
  uint32_t sync0Cycle;
  uint32_t sync0Shift;
  uint32_t sync1Cycle;
  uint32_t sync1Shift;
} LCEC_CONF_DC_T;

typedef struct {
  LCEC_CONF_TYPE_T confType;
  uint16_t divider;
  uint16_t intervals;
} LCEC_CONF_WATCHDOG_T;

typedef struct {
  LCEC_CONF_TYPE_T confType;
  uint8_t index;
  ec_direction_t dir;
  unsigned int pdoCount;
} LCEC_CONF_SYNCMANAGER_T;

typedef struct {
  LCEC_CONF_TYPE_T confType;
  uint16_t index;
  unsigned int pdoEntryCount;
} LCEC_CONF_PDO_T;

typedef struct {
  LCEC_CONF_TYPE_T confType;
  uint16_t index;
  uint8_t subindex;
  uint8_t bitLength;
  float scale;
  hal_type_t halType;
  char halPin[LCEC_CONF_STR_MAXLEN];
} LCEC_CONF_PDOENTRY_T;

typedef struct {
  LCEC_CONF_TYPE_T confType;
} LCEC_CONF_NULL_T;

#define LCEC_CONF_SDO_COMPLETE_SUBIDX -1

typedef struct {
  LCEC_CONF_TYPE_T confType;
  uint16_t index;
  int16_t subindex;
  size_t length;
  uint8_t data[];
} LCEC_CONF_SDOCONF_T;

#endif
