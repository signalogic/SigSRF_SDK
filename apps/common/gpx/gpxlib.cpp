/*
 gpx functions
 
 Copyright (c) 2022 Signalogic, Dallas, Texas

 Description
   gpx track processing:
     -read/write gpx files in XML format
     -do-noising: lowpass filtering with dynamic coefficient adjustment, extreme distance and altitude excursion detection, GPS dropout detection
     -road recognition: snap-to-road, uses OpenCV

 Revision History
  Created Mar 2022 JHB
  Modified Jun 2022 JHB, fix bug in subtract 1 after mktime() and add 1 after localtime()
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "gpx/gpxlib.h"
#include "dsstring.h"

#ifdef __cplusplus
namespace gpx {
#endif

/* note: distance and bearing formulas from https://www.movable-type.co.uk/scripts/latlong.html */

float gpx_distance(float lat1, float lon1, float lat2, float lon2) {  /* input in degrees, output in meters */

   lat1 *= M_PI/180;
   lon1 *= M_PI/180;
   lat2 *= M_PI/180;
   lon2 *= M_PI/180;

   return EARTH_CIRCUMFERENCE * acos((sin(lat1) * sin(lat2)) + cos(lat1) * cos(lat2) * cos(lon2 - lon1));  /* Haversine forumula */
}

float gpx_bearing(float lat1, float lon1, float lat2, float lon2) {  /* input in degrees, output in radians from -pi/2 to pi/2 */

   lat1 *= M_PI/180;
   lon1 *= M_PI/180;
   lat2 *= M_PI/180;
   lon2 *= M_PI/180;

   return atan2(cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(lon2 - lon1), sin(lon2 - lon1) * cos(lat2));
}

float gpx_y(float lat1, float lat2) {  /* approximation for vertical distance between 2 gps points, valid only for very short distances */

   return EARTH_CIRCUMFERENCE*(lat2-lat1)/180;  /* in m */
}

float gpx_x(float lon1, float lon2) {  /* approximation for horizontal distance between 2 gps points, valid only for very short distances */

   return EARTH_CIRCUMFERENCE*(lon2-lon1)/360;  /* in m */
}

void get_val(char* p, float& val) {

   if (*p == '"') p++;
   
   val = atof((const char*)p);
}               

int read_gpx_point(FILE* fp, GPX_POINT& gpx_point, unsigned int uFlags) {

enum {
  TAG_SEARCH,
  TRKPT
};

char line[1024] = "";
char *p, *p2, *pStr;
int state = TAG_SEARCH;
bool fPointFound = false;

static char tag[1024] = "";

//#define GPX_READ_DEBUG
#ifdef GPX_READ_DEBUG
static int frame_count = 0;
frame_count++;
#endif
 
   while (!fPointFound && (pStr = fgets(line, sizeof(line), fp))) {

      str_remove_whitespace(line);
      str_remove_linebreaks(line);

      #ifdef VALIDATION 
      int valid = 0;
      #endif

      if (state == TAG_SEARCH) {

         #ifdef VALIDATION 
         valid++;
         #endif

         if (strstr(line, "<trkpt")) state = TRKPT;
         /* else if (strstr(line, "xxx")) state = XXX  <-- add states if needed */

         if (state != TAG_SEARCH) strcat(tag, line);
      }

      switch (state) {

         case TRKPT:

            if ((p2 = strstr(tag, "/trkpt>"))) {  /* end of tag, process trkpt */

               #ifdef GPX_READ_DEBUG
               if (frame_count < 20) printf(" inside trkpt, tag = %s \n", tag);
               #endif

               if ((p = strstr(tag, "lat="))) {
                  #ifdef VALIDATION
                  valid++;
                  #endif
                  get_val(p+4, gpx_point.lat);
               }

               if ((p = strstr(tag, "lon="))) {
                  #ifdef VALIDATION 
                  valid++;
                  #endif
                  get_val(p+4, gpx_point.lon);
               }

               if ((p = strstr(tag, "<ele>"))) {  /* look for elevation (optional in trkpt tag) */
                  #ifdef VALIDATION 
                  valid++;
                  #endif
                  get_val(p+5, gpx_point.elev);
               }

               if ((p = strstr(tag, "<time>"))) {  /* look for time (optional in trkpt tag) */

                  #ifdef VALIDATION 
                  valid++;
                  #endif

                  struct tm t = { 0 };
                  time_t t1;
                  static time_t t0 = 0;
                  static bool fFirstPoint = true;

//#define DATE_TIME_DEBUG

                  sscanf(p+6, "%04d-%02d-%02dT%02d:%02d:%02d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec);
                  t.tm_year -= 1900;  /* note these adjustments (which are required, per mktime() doc) are mirrored in write_gpx_point() after call to localtime() */
                  t.tm_mon -= 1;
#ifdef DATE_TIME_DEBUG
  static bool fOnce = false;
  if (!fOnce) {
     printf("before mktime t.tm_year = %d, t.tm_mday = %d \n", t.tm_year, t.tm_mday);
     fOnce = true;
  }
#endif
                  t1 = mktime(&t);  /* t1 and t0 are raw time values, in sec since 1 Jan 1900 */

                  if ((uFlags & DS_GPX_INIT) && fFirstPoint) {
                     t0 = t1;
                     fFirstPoint = false;
                  }

                  gpx_point.time_rel = (float)(t1 - t0);
                  gpx_point.time = (double)t1;
               }

               fPointFound = true;  /* break out of while loop */
               strcpy(tag, p2+7);  /* re-initialize tag with anything left over after /trkpt>  */

               #ifdef GPX_READ_DEBUG
               if (frame_count < 20) printf(" inside trkpt, lat=%f, lon=%f, elev=%f, time=%f, time rel=%f \n", gpx_point.lat, gpx_point.lon, gpx_point.elev, gpx_point.time, gpx_point.time_rel);
               #endif
            }
            else strcat(tag, line);  /* to-do: needs a limit on amount of cat */

            break;
      }
   
      #ifdef VALIDATION  /* first pass at adding validation, for example to find errors that prevent Strava or other GPX upload, JHB Sep 2022. To-do:
                            check XML tag formats
                            look for out-of-range entry (e.g. hours not between 0 and 23, minutes 0 and 59, etc)
                          */

      if (valid < 5) printf(" less than 5 items line %s \n", line);
      #endif
   }

   if (!fPointFound || !pStr) return 0;
   else return 1;  /* to-do: modify return values to indicate "point found" vs other legit gpx data with no trkpts */
}

int write_gpx_point(FILE* fp, GPX_POINT& gpx_point, unsigned int uFlags) {

char line[1024] = "";
struct tm* timeinfo;
time_t t;

   strcpy(line, "    <trkpt ");
   strcat(line, "lat=");
   sprintf(&line[strlen(line)], "\"%f\"", gpx_point.lat);

   strcat(line, " ");

   strcat(line, "lon=");
   sprintf(&line[strlen(line)], "\"%f\"", gpx_point.lon);

   strcat(line, ">");
   
   strcat(line, "<ele>");
   sprintf(&line[strlen(line)], "%f", gpx_point.elev);
   strcat(line, "</ele>");

   strcat(line, "<time>");
   t = (time_t)gpx_point.time;
   timeinfo = localtime(&t);
   if (timeinfo->tm_isdst > 0) {  /* take into account local time DST vs. UTC which doesn't have DST or other winter/summer variation */
      t -= 3600;  /* if local timezone DST is in effect, subtract an hour */
      timeinfo = localtime(&t);  /* note - if there is an efficient way of knowing when DST is in effect, thus avoiding two localtime() calls, I couldn't find it. This thread https://stackoverflow.com/questions/70449323/offset-calculation-for-day-light-savingdst-without-using-localtime calls both gmtime and mktime so it can't be any faster, JHB Mar2022 */
   }

#ifdef DATE_TIME_DEBUG
  static bool fOnce = false;
  if (!fOnce) {
     printf("after localtime() timeinfo->tm_year = %d, timeinfo->tm_mday = %d \n", timeinfo->tm_year, timeinfo->tm_mday);
     fOnce = true;
  }
#endif

   timeinfo->tm_year += 1900;  /* note these adjustments (which are required, per localtime() doc) are mirrored in read_gpx_point() before call to mktime() */
   timeinfo->tm_mon += 1;

   sprintf(&line[strlen(line)], "%04d-%02d-%02dT%02d:%02d:%02d", timeinfo->tm_year, timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
   if (gpx_point.time_zone == 0) strcat(line, "Z");  /* to-do: handle timezones other than UTC */
   strcat(line, "</time>");

   strcat(line, "</trkpt>\n");
   
   fwrite(line, strlen(line), 1, fp);

   return 1;
}

int read_gpx_frame(FILE* fp, GPX_POINT gpx_points[], int nPoints) {

int i, ret_val = 0;
static bool fFirstFrame = true;

   for (i=0; i<nPoints; i++) {

      unsigned int uFlags = fFirstFrame ? DS_GPX_INIT : 0;
      ret_val = gpx::read_gpx_point(fp, gpx_points[i], uFlags);
      fFirstFrame = false;
      if (ret_val <= 0) break;
   }

   if (ret_val < 0) return ret_val;  /* return error code */
   else return i;  /* return number of points read */
}

int write_gpx_frame(FILE* fp, GPX_POINT gpx_points[], int nPoints) {

int i;

   for (i=0; i<nPoints; i++) {

      if (!write_gpx_point(fp, gpx_points[i], 0)) return 0;
   }

   return 1;
}

#ifdef __cplusplus
}  /* namespace */
#endif