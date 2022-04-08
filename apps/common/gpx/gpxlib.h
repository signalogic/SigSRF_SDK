/*
  gpxlib.h

  Description
   API, flag, and constant definitions for gpx track processing. See gpxlib.cpp for more info

  Copyright (C) 2022 Signalogic, Dallas, Texas

  Revision History
   Created Mar 2022 JHB
 */

#ifndef _GPXLIB_H_
#define _GPXLIB_H_

#define EARTH_CIRCUMFERENCE       (6371*1000)  /* circumference of earth, in m */
#define NUM_GPX_POINTS_PER_FRAME  64           /* default number of GPX points per frame, for matrix / signal processing purposes */

#define GPS_FS_DEFAULT            1            /* GPS sampling rate default value of 1 Hz if no -Fn command line entry */

/* flags gpx read/write APIs */

#define DS_GPX_INIT               0x100

typedef struct {

  float lat;       /* in degrees */
  float lon;
  float elev;      /* in m */
  double time;     /* record time */
  float time_rel;  /* relative time, starting from 0 */
  int time_zone;   /* zero = UTC */

} GPX_POINT;

#ifdef __cplusplus
extern "C" {
namespace gpx {
#endif

int read_gpx_point(FILE* fp, GPX_POINT* gpx_point, unsigned int uFlags);  /* read point from gpx file */
int write_gpx_frame(FILE* fp, GPX_POINT gpx_points[], int nPoints);  /* write point to gpx file */
int read_gpx_frame(FILE* fp, GPX_POINT gpx_points[], int nPoints);  /* read points from gpx file (number of points specified by nPoints) */

float gpx_distance(float lat1, float lon1, float lat2, float lon2);  /* get Haversine distance between 2 points. Inputs in degrees, output in m */
float gpx_bearing(float lat1, float lon1, float lat2, float lon2);  /* get bearing between 2 points. Inputs in degrees, output in radians from -pi/2 to pi/2 */

#ifdef __cplusplus
}  /* namespace */
}  /* extern "C" */
#endif

#endif  /* _GPXLIB_H_ */
