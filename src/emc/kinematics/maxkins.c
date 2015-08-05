/********************************************************************
* Description: maxkins.c
*   Kinematics for Chris Radek's tabletop 5 axis mill named 'max'.
*   This mill has a tilting head (B axis) and horizontal rotary
*   mounted to the table (C axis).
*
* Author: Chris Radek
* License: GPL Version 2
*
* Copyright (c) 2007 Chris Radek
********************************************************************/

#include "kinematics.h"		/* these decls */
#include "posemath.h"
#include "hal.h"
#include "rtapi.h"
#include "rtapi_math.h"

#define d2r(d) ((d)*PM_PI/180.0)
#define r2d(r) ((r)*180.0/PM_PI)

#ifndef hypot
#define hypot(a,b) (sqrt((a)*(a)+(b)*(b)))
#endif

struct haldata {
    hal_float_t *enable;
    hal_float_t *yshift;
    hal_float_t *zshift;
} *haldata;

int kinematicsForward(const double *joints,
		      EmcPose * pos,
		      const KINEMATICS_FORWARD_FLAGS * fflags,
		      KINEMATICS_INVERSE_FLAGS * iflags)
{
    // A correction
    double yzr = hypot(joints[1] - *(haldata->yshift), joints[2] - *(haldata->zshift));
    double yztheta = atan2(joints[2] - *(haldata->zshift), joints[1] - *(haldata->yshift)) - d2r(joints[3]);

    pos->tran.x = joints[0];
    if (*(haldata->enable)==1.0){
        pos->tran.y = *(haldata->yshift) + yzr * cos(yztheta);
        pos->tran.z = *(haldata->zshift) + yzr * sin(yztheta);
    }else{
        pos->tran.y = joints[1];
        pos->tran.z = joints[2];
    }

    pos->a = joints[3];
    pos->b = joints[4];
    pos->c = joints[5];
    pos->u = joints[6];
    pos->v = joints[7];
    pos->w = joints[8];

    return 0;
}

int kinematicsInverse(const EmcPose * pos,
		      double *joints,
		      const KINEMATICS_INVERSE_FLAGS * iflags,
		      KINEMATICS_FORWARD_FLAGS * fflags)
{
    // A correction
    double yzr = hypot(pos->tran.y - *(haldata->yshift), pos->tran.z - *(haldata->zshift));
    double yztheta = atan2(pos->tran.z - *(haldata->zshift), pos->tran.y - *(haldata->yshift)) + d2r(pos->a);


    joints[0] = pos->tran.x;
    if (*(haldata->enable)==1.0){
        joints[1] = *(haldata->yshift) + yzr * cos(yztheta);
        joints[2] = *(haldata->zshift) + yzr * sin(yztheta);
    }else{
        joints[1] = pos->tran.y;
        joints[2] = pos->tran.z;
    }

    joints[3] = pos->a;
    joints[4] = pos->b;
    joints[5] = pos->c;
    joints[6] = pos->u;
    joints[7] = pos->v;
    joints[8] = pos->w;

    return 0;
}

/* implemented for these kinematics as giving joints preference */
int kinematicsHome(EmcPose * world,
		   double *joint,
		   KINEMATICS_FORWARD_FLAGS * fflags,
		   KINEMATICS_INVERSE_FLAGS * iflags)
{
    *fflags = 0;
    *iflags = 0;

    return kinematicsForward(joint, world, fflags, iflags);
}

KINEMATICS_TYPE kinematicsType()
{
    return KINEMATICS_BOTH;
}

#include "rtapi.h"		/* RTAPI realtime OS API */
#include "rtapi_app.h"		/* RTAPI realtime module decls */

EXPORT_SYMBOL(kinematicsType);
EXPORT_SYMBOL(kinematicsInverse);
EXPORT_SYMBOL(kinematicsForward);
MODULE_LICENSE("GPL");

int comp_id;
int rtapi_app_main(void) {
    int result;
    comp_id = hal_init("maxkins");
    if(comp_id < 0) return comp_id;

    haldata = hal_malloc(sizeof(struct haldata));

    result = hal_pin_float_new("maxkins.enable", HAL_IO, &(haldata->enable), comp_id);
    if(result < 0) goto error;
    /* shift from machine zero to rotational center of a axes */
    result = hal_pin_float_new("maxkins.a_axes_y_shift", HAL_IN, &(haldata->yshift), comp_id);
    if(result < 0) goto error;
    result = hal_pin_float_new("maxkins.a_axes_z_shift", HAL_IN, &(haldata->zshift), comp_id);
    if(result < 0) goto error;

    *(haldata->enable) = 1.0;
    *(haldata->yshift) = 0.0;
    *(haldata->zshift) = 0.0;

    hal_ready(comp_id);
    return 0;

error:
    hal_exit(comp_id);
    return result;
}

void rtapi_app_exit(void) { hal_exit(comp_id); }
