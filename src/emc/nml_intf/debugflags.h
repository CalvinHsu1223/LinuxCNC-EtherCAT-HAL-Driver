// factored out from emcglb.h so subsystems not requiring the
// emcglb.h defines may include them as well



#define EMC_DEBUG_CONFIG            0x00000002
#define EMC_DEBUG_VERSIONS          0x00000008
#define EMC_DEBUG_TASK_ISSUE        0x00000010
#define EMC_DEBUG_NML               0x00000040
#define EMC_DEBUG_MOTION_TIME       0x00000080
#define EMC_DEBUG_INTERP            0x00000100
#define EMC_DEBUG_RCS               0x00000200
#define EMC_DEBUG_INTERP_LIST       0x00000800
#define EMC_DEBUG_IOCONTROL         0x00001000
#define EMC_DEBUG_OWORD             0x00002000
#define EMC_DEBUG_REMAP             0x00004000
#define EMC_DEBUG_PYTHON            0x00008000
#define EMC_DEBUG_NAMEDPARAM        0x00010000
#define EMC_DEBUG_GDBONSIGNAL       0x00020000
#define EMC_DEBUG_PYTHON_TASK       0x00040000

// not interpreted by EMC.
#define EMC_DEBUG_USER1             0x10000000
#define EMC_DEBUG_USER2             0x20000000

#define EMC_DEBUG_UNCONDITIONAL     0x40000000  // always logged
#define EMC_DEBUG_ALL               0x7FFFFFFF	/* it's an int for %i to work
						 */
// debug prefix flags
#define LOG_TIME  1
#define LOG_PID   2
#define LOG_FILENAME 4 // and line
