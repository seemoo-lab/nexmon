/* bench-slope.c - for libgcrypt
 * Copyright (C) 2013 Jussi Kivilinna <jussi.kivilinna@iki.fi>
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _GCRYPT_IN_LIBGCRYPT
# include "../src/gcrypt-int.h"
# include "../compat/libcompat.h"
#else
# include <gcrypt.h>
#endif

#ifndef STR
#define STR(v) #v
#define STR2(v) STR(v)
#endif

#define PGM "bench-slope"
#include "t-common.h"

static int verbose;
static int csv_mode;
static int include_slow;
static int unaligned_mode;
static int num_measurement_repetitions;

/* CPU Ghz value provided by user, allows constructing cycles/byte and other
   results.  */
static double cpu_ghz = -1;

/* Attempt to autodetect CPU Ghz. */
static int auto_ghz;

/* Whether we are running as part of the regression test suite.  */
static int in_regression_test;

/* The name of the currently printed section.  */
static char *current_section_name;
/* The name of the currently printed algorithm.  */
static char *current_algo_name;
/* The name of the currently printed mode.  */
static char *current_mode_name;


/* Currently used CPU Ghz (either user input or auto-detected. */
static double bench_ghz;

/* Current accuracy of auto-detected CPU Ghz. */
static double bench_ghz_diff;

static int in_fips_mode;

/*************************************** Default parameters for measurements. */

/* Start at small buffer size, to get reasonable timer calibration for fast
 * implementations (AES-NI etc). Sixteen selected to support the largest block
 * size of current set cipher blocks. */
#define BUF_START_SIZE			16

/* From ~0 to ~4kbytes give comparable results with results from academia
 * (SUPERCOP). */
#define BUF_END_SIZE			(BUF_START_SIZE + 4096)

/* With 128 byte steps, we get (4096)/64 = 64 data points. */
#define BUF_STEP_SIZE			64

/* Number of repeated measurements at each data point. The median of these
 * measurements is selected as data point further analysis. */
#define NUM_MEASUREMENT_REPETITIONS	64

/* Target accuracy for auto-detected CPU Ghz. */
#define AUTO_GHZ_TARGET_DIFF		(5e-5)

/**************************************************** High-resolution timers. */

/* This benchmarking module needs needs high resolution timer.  */
#undef NO_GET_NSEC_TIME
#if defined(_WIN32)
struct nsec_time
{
  LARGE_INTEGER perf_count;
};

static void
get_nsec_time (struct nsec_time *t)
{
  BOOL ok;

  ok = QueryPerformanceCounter (&t->perf_count);
  assert (ok);
}

static double
get_time_nsec_diff (struct nsec_time *start, struct nsec_time *end)
{
  static double nsecs_per_count = 0.0;
  double nsecs;

  if (nsecs_per_count == 0.0)
    {
      LARGE_INTEGER perf_freq;
      BOOL ok;

      /* Get counts per second. */
      ok = QueryPerformanceFrequency (&perf_freq);
      assert (ok);

      nsecs_per_count = 1.0 / perf_freq.QuadPart;
      nsecs_per_count *= 1000000.0 * 1000.0;	/* sec => nsec */

      assert (nsecs_per_count > 0.0);
    }

  nsecs = end->perf_count.QuadPart - start->perf_count.QuadPart;	/* counts */
  nsecs *= nsecs_per_count;	/* counts * (nsecs / count) => nsecs */

  return nsecs;
}
#elif defined(HAVE_CLOCK_GETTIME)
struct nsec_time
{
  struct timespec ts;
};

static void
get_nsec_time (struct nsec_time *t)
{
  int err;

  err = clock_gettime (CLOCK_REALTIME, &t->ts);
  assert (err == 0);
}

static double
get_time_nsec_diff (struct nsec_time *start, struct nsec_time *end)
{
  double nsecs;

  nsecs = end->ts.tv_sec - start->ts.tv_sec;
  nsecs *= 1000000.0 * 1000.0;	/* sec => nsec */

  /* This way we don't have to care if tv_nsec unsigned or signed. */
  if (end->ts.tv_nsec >= start->ts.tv_nsec)
    nsecs += end->ts.tv_nsec - start->ts.tv_nsec;
  else
    nsecs -= start->ts.tv_nsec - end->ts.tv_nsec;

  return nsecs;
}
#elif defined(HAVE_GETTIMEOFDAY)
struct nsec_time
{
  struct timeval tv;
};

static void
get_nsec_time (struct nsec_time *t)
{
  int err;

  err = gettimeofday (&t->tv, NULL);
  assert (err == 0);
}

static double
get_time_nsec_diff (struct nsec_time *start, struct nsec_time *end)
{
  double nsecs;

  nsecs = end->tv.tv_sec - start->tv.tv_sec;
  nsecs *= 1000000;		/* sec => µsec */

  /* This way we don't have to care if tv_usec unsigned or signed. */
  if (end->tv.tv_usec >= start->tv.tv_usec)
    nsecs += end->tv.tv_usec - start->tv.tv_usec;
  else
    nsecs -= start->tv.tv_usec - end->tv.tv_usec;

  nsecs *= 1000;		/* µsec => nsec */

  return nsecs;
}
#else
#define NO_GET_NSEC_TIME 1
#endif


/* If no high resolution timer found, provide dummy bench-slope.  */
#ifdef NO_GET_NSEC_TIME


int
main (void)
{
  /* No nsec timer => SKIP test. */
  return 77;
}


#else /* !NO_GET_NSEC_TIME */


/********************************************** Slope benchmarking framework. */

struct bench_obj
{
  const struct bench_ops *ops;

  unsigned int num_measure_repetitions;
  unsigned int min_bufsize;
  unsigned int max_bufsize;
  unsigned int step_size;

  void *priv;
  void *hd;
};

typedef int (*const bench_initialize_t) (struct bench_obj * obj);
typedef void (*const bench_finalize_t) (struct bench_obj * obj);
typedef void (*const bench_do_run_t) (struct bench_obj * obj, void *buffer,
				      size_t buflen);

struct bench_ops
{
  bench_initialize_t initialize;
  bench_finalize_t finalize;
  bench_do_run_t do_run;
};


static double
safe_div (double x, double y)
{
  union
  {
    double d;
    char buf[sizeof(double)];
  } u_neg_zero, u_y;

  if (y != 0)
    return x / y;

  u_neg_zero.d = -0.0;
  u_y.d = y;
  if (memcmp(u_neg_zero.buf, u_y.buf, sizeof(double)) == 0)
    return -DBL_MAX;

  return DBL_MAX;
}


static double
get_slope (double (*const get_x) (unsigned int idx, void *priv),
	   void *get_x_priv, double y_points[], unsigned int npoints,
	   double *overhead)
{
  double sumx, sumy, sumx2, sumy2, sumxy;
  unsigned int i;
  double b, a;

  sumx = sumy = sumx2 = sumy2 = sumxy = 0;

  if (npoints <= 1)
    {
      /* No slope with zero or one point. */
      return 0;
    }

  for (i = 0; i < npoints; i++)
    {
      double x, y;

      x = get_x (i, get_x_priv);	/* bytes */
      y = y_points[i];			/* nsecs */

      sumx += x;
      sumy += y;
      sumx2 += x * x;
      /*sumy2 += y * y;*/
      sumxy += x * y;
    }

  b = safe_div(npoints * sumxy - sumx * sumy, npoints * sumx2 - sumx * sumx);

  if (overhead)
    {
      a = safe_div(sumy - b * sumx, npoints);
      *overhead = a;		/* nsecs */
    }

  return b;			/* nsecs per byte */
}


double
get_bench_obj_point_x (unsigned int idx, void *priv)
{
  struct bench_obj *obj = priv;
  return (double) (obj->min_bufsize + (idx * obj->step_size));
}


unsigned int
get_num_measurements (struct bench_obj *obj)
{
  unsigned int buf_range = obj->max_bufsize - obj->min_bufsize;
  unsigned int num = buf_range / obj->step_size + 1;

  while (obj->min_bufsize + (num * obj->step_size) > obj->max_bufsize)
    num--;

  return num + 1;
}


static int
double_cmp (const void *_a, const void *_b)
{
  const double *a, *b;

  a = _a;
  b = _b;

  if (*a > *b)
    return 1;
  if (*a < *b)
    return -1;
  return 0;
}


double
do_bench_obj_measurement (struct bench_obj *obj, void *buffer, size_t buflen,
			  double *measurement_raw,
			  unsigned int loop_iterations)
{
  const unsigned int num_repetitions = obj->num_measure_repetitions;
  const bench_do_run_t do_run = obj->ops->do_run;
  struct nsec_time start, end;
  unsigned int rep, loop;
  double res;

  if (num_repetitions < 1 || loop_iterations < 1)
    return 0.0;

  for (rep = 0; rep < num_repetitions; rep++)
    {
      get_nsec_time (&start);

      for (loop = 0; loop < loop_iterations; loop++)
	do_run (obj, buffer, buflen);

      get_nsec_time (&end);

      measurement_raw[rep] = get_time_nsec_diff (&start, &end);
    }

  /* Return median of repeated measurements. */
  qsort (measurement_raw, num_repetitions, sizeof (measurement_raw[0]),
	 double_cmp);

  if (num_repetitions % 2 == 1)
    return measurement_raw[num_repetitions / 2];

  res = measurement_raw[num_repetitions / 2]
    + measurement_raw[num_repetitions / 2 - 1];
  return res / 2;
}


unsigned int
adjust_loop_iterations_to_timer_accuracy (struct bench_obj *obj, void *buffer,
					  double *measurement_raw)
{
  const double increase_thres = 3.0;
  double tmp, nsecs;
  unsigned int loop_iterations;
  unsigned int test_bufsize;

  test_bufsize = obj->min_bufsize;
  if (test_bufsize == 0)
    test_bufsize += obj->step_size;

  loop_iterations = 0;
  do
    {
      /* Increase loop iterations until we get other results than zero.  */
      nsecs =
	do_bench_obj_measurement (obj, buffer, test_bufsize,
				  measurement_raw, ++loop_iterations);
    }
  while (nsecs < 1.0 - 0.1);
  do
    {
      /* Increase loop iterations until we get reasonable increase for elapsed time.  */
      tmp =
	do_bench_obj_measurement (obj, buffer, test_bufsize,
				  measurement_raw, ++loop_iterations);
    }
  while (tmp < nsecs * (increase_thres - 0.1));

  return loop_iterations;
}


/* Benchmark and return linear regression slope in nanoseconds per byte.  */
double
slope_benchmark (struct bench_obj *obj)
{
  unsigned int num_measurements;
  double *measurements = NULL;
  double *measurement_raw = NULL;
  double slope, overhead;
  unsigned int loop_iterations, midx, i;
  unsigned char *real_buffer = NULL;
  unsigned char *buffer;
  size_t cur_bufsize;
  int err;

  err = obj->ops->initialize (obj);
  if (err < 0)
    return -1;

  num_measurements = get_num_measurements (obj);
  measurements = calloc (num_measurements, sizeof (*measurements));
  if (!measurements)
    goto err_free;

  measurement_raw =
    calloc (obj->num_measure_repetitions, sizeof (*measurement_raw));
  if (!measurement_raw)
    goto err_free;

  if (num_measurements < 1 || obj->num_measure_repetitions < 1 ||
      obj->max_bufsize < 1 || obj->min_bufsize > obj->max_bufsize)
    goto err_free;

  real_buffer = malloc (obj->max_bufsize + 128 + unaligned_mode);
  if (!real_buffer)
    goto err_free;
  /* Get aligned buffer */
  buffer = real_buffer;
  buffer += 128 - ((uintptr_t)real_buffer & (128 - 1));
  if (unaligned_mode)
    buffer += unaligned_mode; /* Make buffer unaligned */

  for (i = 0; i < obj->max_bufsize; i++)
    buffer[i] = 0x55 ^ (-i);

  /* Adjust number of loop iterations up to timer accuracy.  */
  loop_iterations = adjust_loop_iterations_to_timer_accuracy (obj, buffer,
							      measurement_raw);

  /* Perform measurements */
  for (midx = 0, cur_bufsize = obj->min_bufsize;
       cur_bufsize <= obj->max_bufsize; cur_bufsize += obj->step_size, midx++)
    {
      measurements[midx] =
	do_bench_obj_measurement (obj, buffer, cur_bufsize, measurement_raw,
				  loop_iterations);
      measurements[midx] /= loop_iterations;
    }

  assert (midx == num_measurements);

  slope =
    get_slope (&get_bench_obj_point_x, obj, measurements, num_measurements,
	       &overhead);

  free (measurement_raw);
  free (measurements);
  free (real_buffer);
  obj->ops->finalize (obj);

  return slope;

err_free:
  if (measurement_raw)
    free (measurement_raw);
  if (measurements)
    free (measurements);
  if (real_buffer)
    free (real_buffer);
  obj->ops->finalize (obj);

  return -1;
}

/********************************************* CPU frequency auto-detection. */

static volatile size_t vone = 1;

static int
auto_ghz_init (struct bench_obj *obj)
{
  obj->min_bufsize = 16;
  obj->max_bufsize = 64 + obj->min_bufsize;
  obj->step_size = 8;
  obj->num_measure_repetitions = 16;

  return 0;
}

static void
auto_ghz_free (struct bench_obj *obj)
{
  (void)obj;
}

static void
auto_ghz_bench (struct bench_obj *obj, void *buf, size_t buflen)
{
  size_t one = vone;
  size_t two = one + vone;

  (void)obj;
  (void)buf;

  buflen *= 1024;

  /* Turbo frequency detection benchmark. Without CPU turbo-boost, this
   * function will give cycles/iteration result 1024.0 on high-end CPUs.
   * With turbo, result will be less and can be used detect turbo-clock. */

  /* Auto-ghz operation takes two CPU cycles to perform. Variables are
   * generated through volatile object and therefore compiler is unable
   * to optimize these operations to immediate values. */
#ifdef HAVE_GCC_ASM_VOLATILE_MEMORY
  /* Auto-ghz operation takes two CPU cycles to perform. Memory barriers
   * are used to prevent compiler from optimizing this loop away. */
  #define AUTO_GHZ_OPERATION \
	asm volatile ("":"+r"(buflen),"+r"(one),"+r"(two)::"memory"); \
	buflen ^= one; \
	asm volatile ("":"+r"(buflen),"+r"(one),"+r"(two)::"memory"); \
	buflen -= two
#else
  /* TODO: Needs alternative way of preventing compiler optimizations.
   *       Mix of XOR and subtraction appears to do the trick for now. */
  #define AUTO_GHZ_OPERATION \
	buflen ^= one; \
	buflen -= two
#endif

#define AUTO_GHZ_OPERATION_2 \
	AUTO_GHZ_OPERATION; \
	AUTO_GHZ_OPERATION

#define AUTO_GHZ_OPERATION_4 \
	AUTO_GHZ_OPERATION_2; \
	AUTO_GHZ_OPERATION_2

#define AUTO_GHZ_OPERATION_8 \
	AUTO_GHZ_OPERATION_4; \
	AUTO_GHZ_OPERATION_4

#define AUTO_GHZ_OPERATION_16 \
	AUTO_GHZ_OPERATION_8; \
	AUTO_GHZ_OPERATION_8

#define AUTO_GHZ_OPERATION_32 \
	AUTO_GHZ_OPERATION_16; \
	AUTO_GHZ_OPERATION_16

#define AUTO_GHZ_OPERATION_64 \
	AUTO_GHZ_OPERATION_32; \
	AUTO_GHZ_OPERATION_32

#define AUTO_GHZ_OPERATION_128 \
	AUTO_GHZ_OPERATION_64; \
	AUTO_GHZ_OPERATION_64

  do
    {
      /* 1024 auto-ghz operations per loop, total 2048 instructions. */
      AUTO_GHZ_OPERATION_128; AUTO_GHZ_OPERATION_128;
      AUTO_GHZ_OPERATION_128; AUTO_GHZ_OPERATION_128;
      AUTO_GHZ_OPERATION_128; AUTO_GHZ_OPERATION_128;
      AUTO_GHZ_OPERATION_128; AUTO_GHZ_OPERATION_128;
    }
  while (buflen);
}

static struct bench_ops auto_ghz_detect_ops = {
  &auto_ghz_init,
  &auto_ghz_free,
  &auto_ghz_bench
};


double
get_auto_ghz (void)
{
  struct bench_obj obj = { 0 };
  double nsecs_per_iteration;
  double cycles_per_iteration;

  obj.ops = &auto_ghz_detect_ops;

  nsecs_per_iteration = slope_benchmark (&obj);

  cycles_per_iteration = nsecs_per_iteration * cpu_ghz;

  /* Adjust CPU Ghz so that cycles per iteration would give '1024.0'. */

  return safe_div(cpu_ghz * 1024, cycles_per_iteration);
}


double
do_slope_benchmark (struct bench_obj *obj)
{
  unsigned int try_count = 0;
  double ret;

  if (!auto_ghz)
    {
      /* Perform measurement without autodetection of CPU frequency. */

      do
        {
	  ret = slope_benchmark (obj);
        }
      while (ret <= 0 && try_count++ <= 4);

      bench_ghz = cpu_ghz;
      bench_ghz_diff = 0;
    }
  else
    {
      double target_diff = AUTO_GHZ_TARGET_DIFF;
      double cpu_auto_ghz_before;
      double cpu_auto_ghz_after;
      double nsecs_per_iteration;
      double diff;

      /* Perform measurement with CPU frequency autodetection. */

      do
        {
          /* Repeat measurement until CPU turbo frequency has stabilized. */

	  if ((++try_count % 4) == 0)
	    {
	      /* Too much frequency instability on the system, relax target
	       * accuracy. */
	      target_diff *= 2;
	    }

          cpu_auto_ghz_before = get_auto_ghz ();

          nsecs_per_iteration = slope_benchmark (obj);

          cpu_auto_ghz_after = get_auto_ghz ();

          diff = 1.0 - safe_div(cpu_auto_ghz_before, cpu_auto_ghz_after);
          diff = diff < 0 ? -diff : diff;
        }
      while ((nsecs_per_iteration <= 0 || diff > target_diff)
	     && try_count < 1000);

      ret = nsecs_per_iteration;

      bench_ghz = (cpu_auto_ghz_before + cpu_auto_ghz_after) / 2;
      bench_ghz_diff = diff;
    }

  return ret;
}


/********************************************************** Printing results. */

static void
double_to_str (char *out, size_t outlen, double value)
{
  const char *fmt;

  if (value < 1.0)
    fmt = "%.3f";
  else if (value < 100.0)
    fmt = "%.2f";
  else if (value < 1000.0)
    fmt = "%.1f";
  else
    fmt = "%.0f";

  snprintf (out, outlen, fmt, value);
}

static void
bench_print_result_csv (double nsecs_per_byte)
{
  double cycles_per_byte, mbytes_per_sec;
  char nsecpbyte_buf[16];
  char mbpsec_buf[16];
  char cpbyte_buf[16];
  char mhz_buf[16];
  char mhz_diff_buf[32];

  strcpy (mhz_diff_buf, "");
  *cpbyte_buf = 0;
  *mhz_buf = 0;

  double_to_str (nsecpbyte_buf, sizeof (nsecpbyte_buf), nsecs_per_byte);

  /* If user didn't provide CPU speed, we cannot show cycles/byte results.  */
  if (bench_ghz > 0.0)
    {
      cycles_per_byte = nsecs_per_byte * bench_ghz;
      double_to_str (cpbyte_buf, sizeof (cpbyte_buf), cycles_per_byte);
      double_to_str (mhz_buf, sizeof (mhz_buf), bench_ghz * 1000);
      if (auto_ghz && bench_ghz_diff * 1000 >= 1)
	{
	  snprintf(mhz_diff_buf, sizeof(mhz_diff_buf), ",%.0f,Mhz-diff",
		   bench_ghz_diff * 1000);
	}
    }

  mbytes_per_sec =
      safe_div(1000.0 * 1000.0 * 1000.0, nsecs_per_byte * 1024 * 1024);
  double_to_str (mbpsec_buf, sizeof (mbpsec_buf), mbytes_per_sec);

  /* We print two empty fields to allow for future enhancements.  */
  if (auto_ghz)
    {
      printf ("%s,%s,%s,,,%s,ns/B,%s,MiB/s,%s,c/B,%s,Mhz%s\n",
              current_section_name,
              current_algo_name? current_algo_name : "",
              current_mode_name? current_mode_name : "",
              nsecpbyte_buf,
              mbpsec_buf,
              cpbyte_buf,
              mhz_buf,
              mhz_diff_buf);
    }
  else
    {
      printf ("%s,%s,%s,,,%s,ns/B,%s,MiB/s,%s,c/B\n",
              current_section_name,
              current_algo_name? current_algo_name : "",
              current_mode_name? current_mode_name : "",
              nsecpbyte_buf,
              mbpsec_buf,
              cpbyte_buf);
    }
}

static void
bench_print_result_std (double nsecs_per_byte)
{
  double cycles_per_byte, mbytes_per_sec;
  char nsecpbyte_buf[16];
  char mbpsec_buf[16];
  char cpbyte_buf[16];
  char mhz_buf[16];
  char mhz_diff_buf[32];

  strcpy (mhz_diff_buf, "");

  double_to_str (nsecpbyte_buf, sizeof (nsecpbyte_buf), nsecs_per_byte);

  /* If user didn't provide CPU speed, we cannot show cycles/byte results.  */
  if (bench_ghz > 0.0)
    {
      cycles_per_byte = nsecs_per_byte * bench_ghz;
      double_to_str (cpbyte_buf, sizeof (cpbyte_buf), cycles_per_byte);
      double_to_str (mhz_buf, sizeof (mhz_buf), bench_ghz * 1000);
      if (auto_ghz && bench_ghz_diff * 1000 >= 0.5)
	{
	  snprintf(mhz_diff_buf, sizeof(mhz_diff_buf), "±%.0f",
		   bench_ghz_diff * 1000);
	}
    }
  else
    {
      strcpy (cpbyte_buf, "-");
      strcpy (mhz_buf, "-");
    }

  mbytes_per_sec =
      safe_div(1000.0 * 1000.0 * 1000.0, nsecs_per_byte * 1024 * 1024);
  double_to_str (mbpsec_buf, sizeof (mbpsec_buf), mbytes_per_sec);

  if (auto_ghz)
    {
      printf ("%9s ns/B %9s MiB/s %9s c/B %9s%s\n",
              nsecpbyte_buf, mbpsec_buf, cpbyte_buf, mhz_buf, mhz_diff_buf);
    }
  else
    {
      printf ("%9s ns/B %9s MiB/s %9s c/B\n",
              nsecpbyte_buf, mbpsec_buf, cpbyte_buf);
    }
}

static void
bench_print_result (double nsecs_per_byte)
{
  if (csv_mode)
    bench_print_result_csv (nsecs_per_byte);
  else
    bench_print_result_std (nsecs_per_byte);
}

static void
bench_print_result_nsec_per_iteration (double nsecs_per_iteration)
{
  double cycles_per_iteration;
  char nsecpiter_buf[16];
  char cpiter_buf[16];
  char mhz_buf[16];

  strcpy(cpiter_buf, csv_mode ? "" : "-");
  strcpy(mhz_buf, csv_mode ? "" : "-");

  double_to_str (nsecpiter_buf, sizeof (nsecpiter_buf), nsecs_per_iteration);

  /* If user didn't provide CPU speed, we cannot show cycles/iter results.  */
  if (bench_ghz > 0.0)
    {
      cycles_per_iteration = nsecs_per_iteration * bench_ghz;
      double_to_str (cpiter_buf, sizeof (cpiter_buf), cycles_per_iteration);
      double_to_str (mhz_buf, sizeof (mhz_buf), bench_ghz * 1000);
    }

  if (csv_mode)
    {
      if (auto_ghz)
        printf ("%s,%s,%s,,,,,,,,,%s,ns/iter,%s,c/iter,%s,Mhz\n",
                current_section_name,
                current_algo_name ? current_algo_name : "",
                current_mode_name ? current_mode_name : "",
                nsecpiter_buf,
                cpiter_buf,
                mhz_buf);
      else
        printf ("%s,%s,%s,,,,,,,,,%s,ns/iter,%s,c/iter\n",
                current_section_name,
                current_algo_name ? current_algo_name : "",
                current_mode_name ? current_mode_name : "",
                nsecpiter_buf,
                cpiter_buf);
    }
  else
    {
      if (auto_ghz)
        printf ("%14s %13s %9s\n", nsecpiter_buf, cpiter_buf, mhz_buf);
      else
        printf ("%14s %13s\n", nsecpiter_buf, cpiter_buf);
    }
}


static void
bench_print_result_skipped (void)
{
  if (auto_ghz)
    printf ("%14s %13s %9s\n", "<<skipped>>", "-", "-");
  else
    printf ("%14s %13s\n", "<<skipped>>", "-");
}

static void
bench_print_section (const char *section_name, const char *print_name)
{
  if (csv_mode)
    {
      gcry_free (current_section_name);
      current_section_name = gcry_xstrdup (section_name);
    }
  else
    printf ("%s:\n", print_name);
}

static void
bench_print_header (int algo_width, const char *algo_name)
{
  if (csv_mode)
    {
      gcry_free (current_algo_name);
      current_algo_name = gcry_xstrdup (algo_name);
    }
  else
    {
      if (algo_width < 0)
        printf (" %-*s | ", -algo_width, algo_name);
      else
        printf (" %-*s | ", algo_width, algo_name);

      if (auto_ghz)
        printf ("%14s %15s %13s %9s\n", "nanosecs/byte", "mebibytes/sec",
                "cycles/byte", "auto Mhz");
      else
        printf ("%14s %15s %13s\n", "nanosecs/byte", "mebibytes/sec",
                "cycles/byte");
    }
}

static void
bench_print_header_nsec_per_iteration (int algo_width, const char *algo_name)
{
  if (csv_mode)
    {
      gcry_free (current_algo_name);
      current_algo_name = gcry_xstrdup (algo_name);
    }
  else
    {
      if (algo_width < 0)
        printf (" %-*s | ", -algo_width, algo_name);
      else
        printf (" %-*s | ", algo_width, algo_name);

      if (auto_ghz)
        printf ("%14s %13s %9s\n", "nanosecs/iter", "cycles/iter", "auto Mhz");
      else
        printf ("%14s %13s\n", "nanosecs/iter", "cycles/iter");
    }
}

static void
bench_print_algo (int algo_width, const char *algo_name)
{
  if (csv_mode)
    {
      gcry_free (current_algo_name);
      current_algo_name = gcry_xstrdup (algo_name);
    }
  else
    {
      if (algo_width < 0)
        printf (" %-*s | ", -algo_width, algo_name);
      else
        printf (" %-*s | ", algo_width, algo_name);
    }
}

static void
bench_print_mode (int width, const char *mode_name)
{
  if (csv_mode)
    {
      gcry_free (current_mode_name);
      current_mode_name = gcry_xstrdup (mode_name);
    }
  else
    {
      if (width < 0)
        printf (" %-*s | ", -width, mode_name);
      else
        printf (" %*s | ", width, mode_name);
      fflush (stdout);
    }
}

static void
bench_print_footer (int algo_width)
{
  if (!csv_mode)
    printf (" %-*s =\n", algo_width, "");
}


/********************************************************* Cipher benchmarks. */

struct bench_cipher_mode
{
  int mode;
  const char *name;
  struct bench_ops *ops;

  int algo;
};


static void
bench_set_cipher_key (gcry_cipher_hd_t hd, int keylen)
{
  char *key;
  int err, i;

  key = malloc (keylen);
  if (!key)
    {
      fprintf (stderr, PGM ": couldn't allocate %d bytes\n", keylen);
      gcry_cipher_close (hd);
      exit (1);
    }

  for (i = 0; i < keylen; i++)
    key[i] = 0x33 ^ (11 - i);

  err = gcry_cipher_setkey (hd, key, keylen);
  free (key);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_setkey failed: %s\n",
                gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }
}


static int
bench_encrypt_init (struct bench_obj *obj)
{
  struct bench_cipher_mode *mode = obj->priv;
  gcry_cipher_hd_t hd;
  int err, keylen;

  obj->min_bufsize = BUF_START_SIZE;
  obj->max_bufsize = BUF_END_SIZE;
  obj->step_size = BUF_STEP_SIZE;
  obj->num_measure_repetitions = num_measurement_repetitions;

  err = gcry_cipher_open (&hd, mode->algo, mode->mode, 0);
  if (err)
    {
      fprintf (stderr, PGM ": error opening cipher `%s'\n",
	       gcry_cipher_algo_name (mode->algo));
      exit (1);
    }

  keylen = gcry_cipher_get_algo_keylen (mode->algo);
  if (mode->mode == GCRY_CIPHER_MODE_SIV)
    {
      keylen *= 2;
    }

  if (keylen)
    {
      bench_set_cipher_key (hd, keylen);
    }
  else
    {
      fprintf (stderr, PGM ": failed to get key length for algorithm `%s'\n",
	       gcry_cipher_algo_name (mode->algo));
      gcry_cipher_close (hd);
      exit (1);
    }

  obj->hd = hd;

  return 0;
}

static void
bench_encrypt_free (struct bench_obj *obj)
{
  gcry_cipher_hd_t hd = obj->hd;

  gcry_cipher_close (hd);
}

static void
bench_encrypt_do_bench (struct bench_obj *obj, void *buf, size_t buflen)
{
  gcry_cipher_hd_t hd = obj->hd;
  int err;

  err = gcry_cipher_reset (hd);
  if (!err)
    err = gcry_cipher_encrypt (hd, buf, buflen, buf, buflen);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_encrypt failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }
}

static void
bench_decrypt_do_bench (struct bench_obj *obj, void *buf, size_t buflen)
{
  gcry_cipher_hd_t hd = obj->hd;
  int err;

  err = gcry_cipher_reset (hd);
  if (!err)
    err = gcry_cipher_decrypt (hd, buf, buflen, buf, buflen);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_encrypt failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }
}

static struct bench_ops encrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_encrypt_do_bench
};

static struct bench_ops decrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_decrypt_do_bench
};


static int
bench_xts_encrypt_init (struct bench_obj *obj)
{
  struct bench_cipher_mode *mode = obj->priv;
  gcry_cipher_hd_t hd;
  int err, keylen;

  obj->min_bufsize = BUF_START_SIZE;
  obj->max_bufsize = BUF_END_SIZE;
  obj->step_size = BUF_STEP_SIZE;
  obj->num_measure_repetitions = num_measurement_repetitions;

  err = gcry_cipher_open (&hd, mode->algo, mode->mode, 0);
  if (err)
    {
      fprintf (stderr, PGM ": error opening cipher `%s'\n",
	       gcry_cipher_algo_name (mode->algo));
      exit (1);
    }

  /* Double key-length for XTS. */
  keylen = gcry_cipher_get_algo_keylen (mode->algo) * 2;
  if (keylen)
    {
      bench_set_cipher_key (hd, keylen);
    }
  else
    {
      fprintf (stderr, PGM ": failed to get key length for algorithm `%s'\n",
	       gcry_cipher_algo_name (mode->algo));
      gcry_cipher_close (hd);
      exit (1);
    }

  obj->hd = hd;

  return 0;
}

static struct bench_ops xts_encrypt_ops = {
  &bench_xts_encrypt_init,
  &bench_encrypt_free,
  &bench_encrypt_do_bench
};

static struct bench_ops xts_decrypt_ops = {
  &bench_xts_encrypt_init,
  &bench_encrypt_free,
  &bench_decrypt_do_bench
};


static void
bench_ccm_encrypt_do_bench (struct bench_obj *obj, void *buf, size_t buflen)
{
  gcry_cipher_hd_t hd = obj->hd;
  int err;
  char tag[8];
  char nonce[11] = { 0x80, 0x01, };
  u64 params[3];

  gcry_cipher_setiv (hd, nonce, sizeof (nonce));

  /* Set CCM lengths */
  params[0] = buflen;
  params[1] = 0;		/*aadlen */
  params[2] = sizeof (tag);
  err =
    gcry_cipher_ctl (hd, GCRYCTL_SET_CCM_LENGTHS, params, sizeof (params));
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_ctl failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }

  err = gcry_cipher_encrypt (hd, buf, buflen, buf, buflen);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_encrypt failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }

  err = gcry_cipher_gettag (hd, tag, sizeof (tag));
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_gettag failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }
}

static void
bench_ccm_decrypt_do_bench (struct bench_obj *obj, void *buf, size_t buflen)
{
  gcry_cipher_hd_t hd = obj->hd;
  int err;
  char tag[8] = { 0, };
  char nonce[11] = { 0x80, 0x01, };
  u64 params[3];

  gcry_cipher_setiv (hd, nonce, sizeof (nonce));

  /* Set CCM lengths */
  params[0] = buflen;
  params[1] = 0;		/*aadlen */
  params[2] = sizeof (tag);
  err =
    gcry_cipher_ctl (hd, GCRYCTL_SET_CCM_LENGTHS, params, sizeof (params));
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_ctl failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }

  err = gcry_cipher_decrypt (hd, buf, buflen, buf, buflen);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_encrypt failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }

  err = gcry_cipher_checktag (hd, tag, sizeof (tag));
  if (gpg_err_code (err) == GPG_ERR_CHECKSUM)
    err = gpg_error (GPG_ERR_NO_ERROR);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_gettag failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }
}

static void
bench_ccm_authenticate_do_bench (struct bench_obj *obj, void *buf,
				 size_t buflen)
{
  gcry_cipher_hd_t hd = obj->hd;
  int err;
  char tag[8] = { 0, };
  char nonce[11] = { 0x80, 0x01, };
  u64 params[3];
  char data = 0xff;

  gcry_cipher_setiv (hd, nonce, sizeof (nonce));

  /* Set CCM lengths */
  params[0] = sizeof (data);	/*datalen */
  params[1] = buflen;		/*aadlen */
  params[2] = sizeof (tag);
  err =
    gcry_cipher_ctl (hd, GCRYCTL_SET_CCM_LENGTHS, params, sizeof (params));
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_ctl failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }

  err = gcry_cipher_authenticate (hd, buf, buflen);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_authenticate failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }

  err = gcry_cipher_encrypt (hd, &data, sizeof (data), &data, sizeof (data));
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_encrypt failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }

  err = gcry_cipher_gettag (hd, tag, sizeof (tag));
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_gettag failed: %s\n",
	       gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }
}

static struct bench_ops ccm_encrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_ccm_encrypt_do_bench
};

static struct bench_ops ccm_decrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_ccm_decrypt_do_bench
};

static struct bench_ops ccm_authenticate_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_ccm_authenticate_do_bench
};


static void
bench_aead_encrypt_do_bench (struct bench_obj *obj, void *buf, size_t buflen,
			     const char *nonce, size_t noncelen)
{
  gcry_cipher_hd_t hd = obj->hd;
  int err;
  char tag[16];

  gcry_cipher_reset (hd);
  gcry_cipher_setiv (hd, nonce, noncelen);

  gcry_cipher_final (hd);
  err = gcry_cipher_encrypt (hd, buf, buflen, buf, buflen);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_encrypt failed: %s\n",
           gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }

  err = gcry_cipher_gettag (hd, tag, sizeof (tag));
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_gettag failed: %s\n",
           gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }
}

static void
bench_aead_decrypt_do_bench (struct bench_obj *obj, void *buf, size_t buflen,
			     const char *nonce, size_t noncelen)
{
  gcry_cipher_hd_t hd = obj->hd;
  int err;
  char tag[16] = { 0, };

  gcry_cipher_reset (hd);
  gcry_cipher_set_decryption_tag (hd, tag, 16);

  gcry_cipher_setiv (hd, nonce, noncelen);

  gcry_cipher_final (hd);
  err = gcry_cipher_decrypt (hd, buf, buflen, buf, buflen);
  if (gpg_err_code (err) == GPG_ERR_CHECKSUM)
    err = gpg_error (GPG_ERR_NO_ERROR);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_decrypt failed: %s\n",
           gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }

  err = gcry_cipher_checktag (hd, tag, sizeof (tag));
  if (gpg_err_code (err) == GPG_ERR_CHECKSUM)
    err = gpg_error (GPG_ERR_NO_ERROR);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_gettag failed: %s\n",
           gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }
}

static void
bench_aead_authenticate_do_bench (struct bench_obj *obj, void *buf,
				  size_t buflen, const char *nonce,
				  size_t noncelen)
{
  gcry_cipher_hd_t hd = obj->hd;
  int err;
  char tag[16] = { 0, };
  char data = 0xff;

  gcry_cipher_reset (hd);

  if (noncelen > 0)
    {
      err = gcry_cipher_setiv (hd, nonce, noncelen);
      if (err)
	{
	  fprintf (stderr, PGM ": gcry_cipher_setiv failed: %s\n",
	       gpg_strerror (err));
	  gcry_cipher_close (hd);
	  exit (1);
	}
    }

  err = gcry_cipher_authenticate (hd, buf, buflen);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_authenticate failed: %s\n",
           gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }

  gcry_cipher_final (hd);
  err = gcry_cipher_encrypt (hd, &data, sizeof (data), &data, sizeof (data));
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_encrypt failed: %s\n",
           gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }

  err = gcry_cipher_gettag (hd, tag, sizeof (tag));
  if (err)
    {
      fprintf (stderr, PGM ": gcry_cipher_gettag failed: %s\n",
           gpg_strerror (err));
      gcry_cipher_close (hd);
      exit (1);
    }
}


static void
bench_gcm_encrypt_do_bench (struct bench_obj *obj, void *buf,
			    size_t buflen)
{
  char nonce[12] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88 };
  bench_aead_encrypt_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static void
bench_gcm_decrypt_do_bench (struct bench_obj *obj, void *buf,
			    size_t buflen)
{
  char nonce[12] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88 };
  bench_aead_decrypt_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static void
bench_gcm_authenticate_do_bench (struct bench_obj *obj, void *buf,
				 size_t buflen)
{
  char nonce[12] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88 };
  bench_aead_authenticate_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static struct bench_ops gcm_encrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_gcm_encrypt_do_bench
};

static struct bench_ops gcm_decrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_gcm_decrypt_do_bench
};

static struct bench_ops gcm_authenticate_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_gcm_authenticate_do_bench
};


static void
bench_ocb_encrypt_do_bench (struct bench_obj *obj, void *buf,
			    size_t buflen)
{
  char nonce[15] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88,
                     0x00, 0x00, 0x01 };
  bench_aead_encrypt_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static void
bench_ocb_decrypt_do_bench (struct bench_obj *obj, void *buf,
			    size_t buflen)
{
  char nonce[15] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88,
                     0x00, 0x00, 0x01 };
  bench_aead_decrypt_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static void
bench_ocb_authenticate_do_bench (struct bench_obj *obj, void *buf,
				 size_t buflen)
{
  char nonce[15] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88,
                     0x00, 0x00, 0x01 };
  bench_aead_authenticate_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static struct bench_ops ocb_encrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_ocb_encrypt_do_bench
};

static struct bench_ops ocb_decrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_ocb_decrypt_do_bench
};

static struct bench_ops ocb_authenticate_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_ocb_authenticate_do_bench
};


static void
bench_siv_encrypt_do_bench (struct bench_obj *obj, void *buf,
			    size_t buflen)
{
  bench_aead_encrypt_do_bench (obj, buf, buflen, NULL, 0);
}

static void
bench_siv_decrypt_do_bench (struct bench_obj *obj, void *buf,
			    size_t buflen)
{
  bench_aead_decrypt_do_bench (obj, buf, buflen, NULL, 0);
}

static void
bench_siv_authenticate_do_bench (struct bench_obj *obj, void *buf,
				 size_t buflen)
{
  bench_aead_authenticate_do_bench (obj, buf, buflen, NULL, 0);
}

static struct bench_ops siv_encrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_siv_encrypt_do_bench
};

static struct bench_ops siv_decrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_siv_decrypt_do_bench
};

static struct bench_ops siv_authenticate_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_siv_authenticate_do_bench
};


static void
bench_gcm_siv_encrypt_do_bench (struct bench_obj *obj, void *buf,
				size_t buflen)
{
  char nonce[12] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88 };
  bench_aead_encrypt_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static void
bench_gcm_siv_decrypt_do_bench (struct bench_obj *obj, void *buf,
				size_t buflen)
{
  char nonce[12] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88 };
  bench_aead_decrypt_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static void
bench_gcm_siv_authenticate_do_bench (struct bench_obj *obj, void *buf,
				     size_t buflen)
{
  char nonce[12] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88 };
  bench_aead_authenticate_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static struct bench_ops gcm_siv_encrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_gcm_siv_encrypt_do_bench
};

static struct bench_ops gcm_siv_decrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_gcm_siv_decrypt_do_bench
};

static struct bench_ops gcm_siv_authenticate_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_gcm_siv_authenticate_do_bench
};


static void
bench_eax_encrypt_do_bench (struct bench_obj *obj, void *buf,
			    size_t buflen)
{
  char nonce[16] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88,
                     0x00, 0x00, 0x01, 0x00 };
  bench_aead_encrypt_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static void
bench_eax_decrypt_do_bench (struct bench_obj *obj, void *buf,
			    size_t buflen)
{
  char nonce[16] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88,
                     0x00, 0x00, 0x01, 0x00 };
  bench_aead_decrypt_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static void
bench_eax_authenticate_do_bench (struct bench_obj *obj, void *buf,
				 size_t buflen)
{
  char nonce[16] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce,
                     0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88,
                     0x00, 0x00, 0x01, 0x00 };
  bench_aead_authenticate_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static struct bench_ops eax_encrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_eax_encrypt_do_bench
};

static struct bench_ops eax_decrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_eax_decrypt_do_bench
};

static struct bench_ops eax_authenticate_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_eax_authenticate_do_bench
};

static void
bench_poly1305_encrypt_do_bench (struct bench_obj *obj, void *buf,
				 size_t buflen)
{
  char nonce[8] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad };
  bench_aead_encrypt_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static void
bench_poly1305_decrypt_do_bench (struct bench_obj *obj, void *buf,
				 size_t buflen)
{
  char nonce[8] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad };
  bench_aead_decrypt_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static void
bench_poly1305_authenticate_do_bench (struct bench_obj *obj, void *buf,
				      size_t buflen)
{
  char nonce[8] = { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad };
  bench_aead_authenticate_do_bench (obj, buf, buflen, nonce, sizeof(nonce));
}

static struct bench_ops poly1305_encrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_poly1305_encrypt_do_bench
};

static struct bench_ops poly1305_decrypt_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_poly1305_decrypt_do_bench
};

static struct bench_ops poly1305_authenticate_ops = {
  &bench_encrypt_init,
  &bench_encrypt_free,
  &bench_poly1305_authenticate_do_bench
};


static struct bench_cipher_mode cipher_modes[] = {
  {GCRY_CIPHER_MODE_ECB, "ECB enc", &encrypt_ops},
  {GCRY_CIPHER_MODE_ECB, "ECB dec", &decrypt_ops},
  {GCRY_CIPHER_MODE_CBC, "CBC enc", &encrypt_ops},
  {GCRY_CIPHER_MODE_CBC, "CBC dec", &decrypt_ops},
  {GCRY_CIPHER_MODE_CFB, "CFB enc", &encrypt_ops},
  {GCRY_CIPHER_MODE_CFB, "CFB dec", &decrypt_ops},
  {GCRY_CIPHER_MODE_OFB, "OFB enc", &encrypt_ops},
  {GCRY_CIPHER_MODE_OFB, "OFB dec", &decrypt_ops},
  {GCRY_CIPHER_MODE_CTR, "CTR enc", &encrypt_ops},
  {GCRY_CIPHER_MODE_CTR, "CTR dec", &decrypt_ops},
  {GCRY_CIPHER_MODE_XTS, "XTS enc", &xts_encrypt_ops},
  {GCRY_CIPHER_MODE_XTS, "XTS dec", &xts_decrypt_ops},
  {GCRY_CIPHER_MODE_CCM, "CCM enc", &ccm_encrypt_ops},
  {GCRY_CIPHER_MODE_CCM, "CCM dec", &ccm_decrypt_ops},
  {GCRY_CIPHER_MODE_CCM, "CCM auth", &ccm_authenticate_ops},
  {GCRY_CIPHER_MODE_EAX, "EAX enc",  &eax_encrypt_ops},
  {GCRY_CIPHER_MODE_EAX, "EAX dec",  &eax_decrypt_ops},
  {GCRY_CIPHER_MODE_EAX, "EAX auth", &eax_authenticate_ops},
  {GCRY_CIPHER_MODE_GCM, "GCM enc", &gcm_encrypt_ops},
  {GCRY_CIPHER_MODE_GCM, "GCM dec", &gcm_decrypt_ops},
  {GCRY_CIPHER_MODE_GCM, "GCM auth", &gcm_authenticate_ops},
  {GCRY_CIPHER_MODE_OCB, "OCB enc",  &ocb_encrypt_ops},
  {GCRY_CIPHER_MODE_OCB, "OCB dec",  &ocb_decrypt_ops},
  {GCRY_CIPHER_MODE_OCB, "OCB auth", &ocb_authenticate_ops},
  {GCRY_CIPHER_MODE_SIV, "SIV enc", &siv_encrypt_ops},
  {GCRY_CIPHER_MODE_SIV, "SIV dec", &siv_decrypt_ops},
  {GCRY_CIPHER_MODE_SIV, "SIV auth", &siv_authenticate_ops},
  {GCRY_CIPHER_MODE_GCM_SIV, "GCM-SIV enc", &gcm_siv_encrypt_ops},
  {GCRY_CIPHER_MODE_GCM_SIV, "GCM-SIV dec", &gcm_siv_decrypt_ops},
  {GCRY_CIPHER_MODE_GCM_SIV, "GCM-SIV auth", &gcm_siv_authenticate_ops},
  {GCRY_CIPHER_MODE_POLY1305, "POLY1305 enc", &poly1305_encrypt_ops},
  {GCRY_CIPHER_MODE_POLY1305, "POLY1305 dec", &poly1305_decrypt_ops},
  {GCRY_CIPHER_MODE_POLY1305, "POLY1305 auth", &poly1305_authenticate_ops},
  {0},
};


static void
cipher_bench_one (int algo, struct bench_cipher_mode *pmode)
{
  struct bench_cipher_mode mode = *pmode;
  struct bench_obj obj = { 0 };
  double result;
  unsigned int blklen;
  unsigned int keylen;

  mode.algo = algo;

  /* Check if this mode is ok */
  blklen = gcry_cipher_get_algo_blklen (algo);
  if (!blklen)
    return;

  keylen = gcry_cipher_get_algo_keylen (algo);
  if (!keylen)
    return;

  /* Stream cipher? Only test with "ECB" and POLY1305. */
  if (blklen == 1 && (mode.mode != GCRY_CIPHER_MODE_ECB &&
		      mode.mode != GCRY_CIPHER_MODE_POLY1305))
    return;
  if (blklen == 1 && mode.mode == GCRY_CIPHER_MODE_ECB)
    {
      mode.mode = GCRY_CIPHER_MODE_STREAM;
      mode.name = mode.ops == &encrypt_ops ? "STREAM enc" : "STREAM dec";
    }

  /* Poly1305 has restriction for cipher algorithm */
  if (mode.mode == GCRY_CIPHER_MODE_POLY1305 && algo != GCRY_CIPHER_CHACHA20)
    return;

  /* CCM has restrictions for block-size */
  if (mode.mode == GCRY_CIPHER_MODE_CCM && blklen != GCRY_CCM_BLOCK_LEN)
    return;

  /* GCM has restrictions for block-size; not allowed in FIPS mode */
  if (mode.mode == GCRY_CIPHER_MODE_GCM && (in_fips_mode || blklen != GCRY_GCM_BLOCK_LEN))
    return;

  /* XTS has restrictions for block-size */
  if (mode.mode == GCRY_CIPHER_MODE_XTS && blklen != GCRY_XTS_BLOCK_LEN)
    return;

  /* SIV has restrictions for block-size */
  if (mode.mode == GCRY_CIPHER_MODE_SIV && blklen != GCRY_SIV_BLOCK_LEN)
    return;

  /* GCM-SIV has restrictions for block-size */
  if (mode.mode == GCRY_CIPHER_MODE_GCM_SIV && blklen != GCRY_SIV_BLOCK_LEN)
    return;

  /* GCM-SIV has restrictions for key length */
  if (mode.mode == GCRY_CIPHER_MODE_GCM_SIV && !(keylen == 16 || keylen == 32))
    return;

  /* Our OCB implementation has restrictions for block-size.  */
  if (mode.mode == GCRY_CIPHER_MODE_OCB && blklen != GCRY_OCB_BLOCK_LEN)
    return;

  bench_print_mode (14, mode.name);

  obj.ops = mode.ops;
  obj.priv = &mode;

  result = do_slope_benchmark (&obj);

  bench_print_result (result);
}


static void
_cipher_bench (int algo)
{
  const char *algoname;
  int i;

  algoname = gcry_cipher_algo_name (algo);

  bench_print_header (14, algoname);

  for (i = 0; cipher_modes[i].mode; i++)
    cipher_bench_one (algo, &cipher_modes[i]);

  bench_print_footer (14);
}


void
cipher_bench (char **argv, int argc)
{
  int i, algo;

  bench_print_section ("cipher", "Cipher");

  if (argv && argc)
    {
      for (i = 0; i < argc; i++)
        {
          algo = gcry_cipher_map_name (argv[i]);
          if (algo)
            _cipher_bench (algo);
        }
    }
  else
    {
      for (i = 1; i < 400; i++)
        if (!gcry_cipher_test_algo (i))
          _cipher_bench (i);
    }
}


/*********************************************************** Hash benchmarks. */

struct bench_hash_mode
{
  const char *name;
  struct bench_ops *ops;

  int algo;
};


static int
bench_hash_init (struct bench_obj *obj)
{
  struct bench_hash_mode *mode = obj->priv;
  gcry_md_hd_t hd;
  int err;

  obj->min_bufsize = BUF_START_SIZE;
  obj->max_bufsize = BUF_END_SIZE;
  obj->step_size = BUF_STEP_SIZE;
  obj->num_measure_repetitions = num_measurement_repetitions;

  err = gcry_md_open (&hd, mode->algo, 0);
  if (err)
    {
      fprintf (stderr, PGM ": error opening hash `%s'\n",
	       gcry_md_algo_name (mode->algo));
      exit (1);
    }

  obj->hd = hd;

  return 0;
}

static void
bench_hash_free (struct bench_obj *obj)
{
  gcry_md_hd_t hd = obj->hd;

  gcry_md_close (hd);
}

static void
bench_hash_do_bench (struct bench_obj *obj, void *buf, size_t buflen)
{
  gcry_md_hd_t hd = obj->hd;

  gcry_md_reset (hd);
  gcry_md_write (hd, buf, buflen);
  gcry_md_final (hd);
}

static struct bench_ops hash_ops = {
  &bench_hash_init,
  &bench_hash_free,
  &bench_hash_do_bench
};


static struct bench_hash_mode hash_modes[] = {
  {"", &hash_ops},
  {0},
};


static void
hash_bench_one (int algo, struct bench_hash_mode *pmode)
{
  struct bench_hash_mode mode = *pmode;
  struct bench_obj obj = { 0 };
  double result;

  mode.algo = algo;

  if (mode.name[0] == '\0')
    bench_print_algo (-14, gcry_md_algo_name (algo));
  else
    bench_print_algo (14, mode.name);

  obj.ops = mode.ops;
  obj.priv = &mode;

  result = do_slope_benchmark (&obj);

  bench_print_result (result);
}

static void
_hash_bench (int algo)
{
  int i;

  for (i = 0; hash_modes[i].name; i++)
    hash_bench_one (algo, &hash_modes[i]);
}

void
hash_bench (char **argv, int argc)
{
  int i, algo;

  bench_print_section ("hash", "Hash");
  bench_print_header (14, "");

  if (argv && argc)
    {
      for (i = 0; i < argc; i++)
	{
	  algo = gcry_md_map_name (argv[i]);
	  if (algo)
	    _hash_bench (algo);
	}
    }
  else
    {
      for (i = 1; i < 400; i++)
        if (i == GCRY_MD_CSHAKE128 || i == GCRY_MD_CSHAKE256)
          ; /* Skip the bench. */
        else if (!gcry_md_test_algo (i))
	  _hash_bench (i);
    }

  bench_print_footer (14);
}


/************************************************************ MAC benchmarks. */

struct bench_mac_mode
{
  const char *name;
  struct bench_ops *ops;

  int algo;
};


static int
bench_mac_init (struct bench_obj *obj)
{
  struct bench_mac_mode *mode = obj->priv;
  gcry_mac_hd_t hd;
  int err;
  unsigned int keylen;
  void *key;

  obj->min_bufsize = BUF_START_SIZE;
  obj->max_bufsize = BUF_END_SIZE;
  obj->step_size = BUF_STEP_SIZE;
  obj->num_measure_repetitions = num_measurement_repetitions;

  keylen = gcry_mac_get_algo_keylen (mode->algo);
  if (keylen == 0)
    keylen = 32;
  key = malloc (keylen);
  if (!key)
    {
      fprintf (stderr, PGM ": couldn't allocate %d bytes\n", keylen);
      exit (1);
    }
  memset(key, 42, keylen);

  err = gcry_mac_open (&hd, mode->algo, 0, NULL);
  if (err)
    {
      fprintf (stderr, PGM ": error opening mac `%s'\n",
	       gcry_mac_algo_name (mode->algo));
      free (key);
      exit (1);
    }

  err = gcry_mac_setkey (hd, key, keylen);
  if (err)
    {
      fprintf (stderr, PGM ": error setting key for mac `%s'\n",
	       gcry_mac_algo_name (mode->algo));
      free (key);
      exit (1);
    }

  switch (mode->algo)
    {
    default:
      break;
    case GCRY_MAC_POLY1305_AES:
    case GCRY_MAC_POLY1305_CAMELLIA:
    case GCRY_MAC_POLY1305_TWOFISH:
    case GCRY_MAC_POLY1305_SERPENT:
    case GCRY_MAC_POLY1305_SEED:
    case GCRY_MAC_POLY1305_SM4:
    case GCRY_MAC_POLY1305_ARIA:
      gcry_mac_setiv (hd, key, 16);
      break;
    }

  obj->hd = hd;

  free (key);
  return 0;
}

static void
bench_mac_free (struct bench_obj *obj)
{
  gcry_mac_hd_t hd = obj->hd;

  gcry_mac_close (hd);
}

static void
bench_mac_do_bench (struct bench_obj *obj, void *buf, size_t buflen)
{
  gcry_mac_hd_t hd = obj->hd;
  size_t bs;
  char b;

  gcry_mac_reset (hd);
  gcry_mac_write (hd, buf, buflen);
  bs = sizeof(b);
  gcry_mac_read (hd, &b, &bs);
}

static struct bench_ops mac_ops = {
  &bench_mac_init,
  &bench_mac_free,
  &bench_mac_do_bench
};


static struct bench_mac_mode mac_modes[] = {
  {"", &mac_ops},
  {0},
};


static void
mac_bench_one (int algo, struct bench_mac_mode *pmode)
{
  struct bench_mac_mode mode = *pmode;
  struct bench_obj obj = { 0 };
  double result;

  mode.algo = algo;

  if (mode.name[0] == '\0')
    bench_print_algo (-18, gcry_mac_algo_name (algo));
  else
    bench_print_algo (18, mode.name);

  obj.ops = mode.ops;
  obj.priv = &mode;

  result = do_slope_benchmark (&obj);

  bench_print_result (result);
}

static void
_mac_bench (int algo)
{
  int i;

  for (i = 0; mac_modes[i].name; i++)
    mac_bench_one (algo, &mac_modes[i]);
}

void
mac_bench (char **argv, int argc)
{
  int i, algo;

  bench_print_section ("mac", "MAC");
  bench_print_header (18, "");

  if (argv && argc)
    {
      for (i = 0; i < argc; i++)
	{
	  algo = gcry_mac_map_name (argv[i]);
	  if (algo)
	    _mac_bench (algo);
	}
    }
  else
    {
      for (i = 1; i < 600; i++)
	if (!gcry_mac_test_algo (i))
	  _mac_bench (i);
    }

  bench_print_footer (18);
}


/************************************************************ KDF benchmarks. */

struct bench_kdf_mode
{
  struct bench_ops *ops;

  int algo;
  int subalgo;
};


static int
bench_kdf_init (struct bench_obj *obj)
{
  struct bench_kdf_mode *mode = obj->priv;

  if (mode->algo == GCRY_KDF_PBKDF2)
    {
      int n = in_fips_mode ? 1000 : 2;

      obj->min_bufsize = n;
      obj->max_bufsize = n * 32;
      obj->step_size = n;
    }

  obj->num_measure_repetitions = num_measurement_repetitions;

  return 0;
}

static void
bench_kdf_free (struct bench_obj *obj)
{
  (void)obj;
}

static void
bench_kdf_do_bench (struct bench_obj *obj, void *buf, size_t buflen)
{
  struct bench_kdf_mode *mode = obj->priv;
  char keybuf[16];

  (void)buf;

  if (mode->algo == GCRY_KDF_PBKDF2)
    {
      gcry_kdf_derive("qwertyuiop", 10, mode->algo, mode->subalgo,
                      "0123456789ABCDEF", 16,
                      buflen, sizeof(keybuf), keybuf);
    }
}

static struct bench_ops kdf_ops = {
  &bench_kdf_init,
  &bench_kdf_free,
  &bench_kdf_do_bench
};


static void
kdf_bench_one (int algo, int subalgo)
{
  struct bench_kdf_mode mode = { &kdf_ops };
  struct bench_obj obj = { 0 };
  double nsecs_per_iteration;
  char algo_name[32];

  mode.algo = algo;
  mode.subalgo = subalgo;

  switch (subalgo)
    {
    case GCRY_MD_CRC32:
    case GCRY_MD_CRC32_RFC1510:
    case GCRY_MD_CRC24_RFC2440:
    case GCRY_MD_MD4:
      /* Skip CRC32s. */
      return;
    }

  if (gcry_md_get_algo_dlen (subalgo) == 0)
    {
      /* Skip XOFs */
      return;
    }

  *algo_name = 0;

  if (algo == GCRY_KDF_PBKDF2)
    {
      snprintf (algo_name, sizeof(algo_name), "PBKDF2-HMAC-%s",
		gcry_md_algo_name (subalgo));
    }

  bench_print_algo (-24, algo_name);

  obj.ops = mode.ops;
  obj.priv = &mode;

  nsecs_per_iteration = do_slope_benchmark (&obj);
  bench_print_result_nsec_per_iteration (nsecs_per_iteration);
}

void
kdf_bench (char **argv, int argc)
{
  char algo_name[32];
  int i, j;

  bench_print_section ("kdf", "KDF");

  bench_print_header_nsec_per_iteration (24, "");

  if (argv && argc)
    {
      for (i = 0; i < argc; i++)
	{
	  for (j = 1; j < 400; j++)
	    {
              if (i == GCRY_MD_CSHAKE128 || i == GCRY_MD_CSHAKE256)
                continue; /* Skip the bench. */

	      if (gcry_md_test_algo (j))
		continue;

	      snprintf (algo_name, sizeof(algo_name), "PBKDF2-HMAC-%s",
			gcry_md_algo_name (j));

	      if (!strcmp(argv[i], algo_name))
		kdf_bench_one (GCRY_KDF_PBKDF2, j);
	    }
	}
    }
  else
    {
      for (i = 1; i < 400; i++)
        if (i == GCRY_MD_CSHAKE128 || i == GCRY_MD_CSHAKE256)
          ; /* Skip the bench. */
	else if (!gcry_md_test_algo (i))
	  kdf_bench_one (GCRY_KDF_PBKDF2, i);
    }

  bench_print_footer (24);
}


/************************************************************** PK benchmarks. */

enum bench_pk_algo
{
#if USE_RSA
  PK_ALGO_RSA2048 = 0,
  PK_ALGO_RSA3072,
  PK_ALGO_RSA4096,
#endif
#if USE_DSA
  PK_ALGO_DSA2048,
  PK_ALGO_DSA3072,
#endif
  __MAX_PK_ALGO
};

enum bench_pk_operation
{
  PK_OPER_SIGN = 0,
  PK_OPER_VERIFY,
  __MAX_PK_OPER
};

struct bench_pk_oper
{
  enum bench_pk_operation oper;
  const char *name;
  struct bench_ops *ops;

  enum bench_pk_algo algo;
};

struct bench_pk_hd
{
  gcry_sexp_t pub_key;
  gcry_sexp_t sec_key;
  gcry_sexp_t data;
  gcry_sexp_t sig;
};

static const char sample_private_rsa_key_2048[] =
"(private-key \n"
" (rsa \n"
"  (n #00E11B35517B742D97078D46EC5FADC86E846436DBD852B1D83B80C0C629A1"
"AA0F5DBEEFD74B5C8FF420BA5EB586C4EEED96C8A3517E4C4C01B4525B7312D68A34"
"75B2FC78C87B33936260131F5A0AE3CFE2924E85D722C9FA0838F88C30BC8B5B0797"
"735450DE5C3F89B877B0960D285FA76FE64D93C2CA679AB6E59C014D13DBF513A301"
"B993E40C1AAF47171450690A8EA19E72C739C64D1CFCDA4FE0F165FA6F7AC8189C3F"
"CC2AB8E13A009B6B7869842CEEA54C91B249DC7906C8D3FE8FBFEBA88F6E5284A215"
"B2FCF28B31DC296EF26F7532D99EDEBC2D25D96AB3393702E7BAB7877285AA92324A"
"6D4A32E5BAD25B0B67541F526E1AF17331ECBBD971A3#)\n"
"  (e #010001#)\n"
"  (d #16091CCEDA2651E42BEE7AB17878418ECB38282A9CC8AD2889DC36E922468E"
"DB5F6C1CE2BD6CD53377AE4D2DEC4D374728BD4E1447282BA7C4A81F3A6ABBDE1191"
"C7FD70FE33E60B98D3E7381BDC2BD7A263AEE14F94A0D6239B84E452B36BD9A6C465"
"34C6C36738C03FF2BC4BF0D097F4830B8295C9537F5E0B3361A587C23EBB8564BA85"
"81AF7B48B4F65FC29AB0E9140101ED6AE837166910D3BE56B9F5D2816F8130D532C7"
"627C7AA8BF7F290B538B42C7F7BC5AB23F4001C6F5DB0FD8102491C80F1E275C3BD6"
"A3AC87AEBF9E467820A9B4CB7B09021F8C468789102A1D01B359ED6DFE4E056EA2FC"
"3B09F1E46890356D660BB33B479AB6981A445D1F89#)\n"
"  (p #00EFA1B3F6348A80FB8041C97965985DDDF83B2FDB0AE8866C11F984E55C2E"
"57A9DD03101B6DC5BC0C77AEED227B4F77FB62A4D9932ABFCFDC39C43FDEB06C7525"
"0705B2F6D7F48E1C79891F74BBAD3A671F75FFBDA1478D565CF8A4AC3840429874EA"
"87DABF688696BA7AE85341101BE3A8958D76B1021C439F27F49F82EB3ADD#)\n"
"  (q #00F07B823CFBB5F8DFA439EC6CA63BB8E2AA30679DD2D67F42F40AF4777C0F"
"154161E35CD5B789DD98DE7BF435F52EDB5A156F8840C45B00A341A2786A07C2FB37"
"942B384E45FD7CBCCC84A017998ACEC4C90A832EBBF58CB66FDB3D7B8BB447EB8782"
"551AF520281E6DF838013DA9AA1A7278801813ED9B24ADFA33AA9855567F#)\n"
"  (u #5493CEDD291EBB5EFAB1911CA9BAE7C42D9F3FE303C25FBA6674047FCBEDB7"
"7A14A3C8F7176B4BF46AE3C44D85017A4F71C0CAADE6B97507A913FA001D38262772"
"12C78674C1FCC1273865DDA92A844269C086A769D5571D7E988E915339AC758E257F"
"96AC7A073A8EF8D288DE0A9F7F1E5761304CBDFBEB8BF1783F0967484E#)\n"
"  )\n"
" )\n"
;

static const char sample_public_rsa_key_2048[] =
"(public-key \n"
" (rsa \n"
"  (n #00E11B35517B742D97078D46EC5FADC86E846436DBD852B1D83B80C0C629A1"
"AA0F5DBEEFD74B5C8FF420BA5EB586C4EEED96C8A3517E4C4C01B4525B7312D68A34"
"75B2FC78C87B33936260131F5A0AE3CFE2924E85D722C9FA0838F88C30BC8B5B0797"
"735450DE5C3F89B877B0960D285FA76FE64D93C2CA679AB6E59C014D13DBF513A301"
"B993E40C1AAF47171450690A8EA19E72C739C64D1CFCDA4FE0F165FA6F7AC8189C3F"
"CC2AB8E13A009B6B7869842CEEA54C91B249DC7906C8D3FE8FBFEBA88F6E5284A215"
"B2FCF28B31DC296EF26F7532D99EDEBC2D25D96AB3393702E7BAB7877285AA92324A"
"6D4A32E5BAD25B0B67541F526E1AF17331ECBBD971A3#)\n"
"  (e #010001#)\n"
"  )\n"
" )\n"
;

static const char sample_private_rsa_key_3072[] =
"(private-key \n"
" (rsa \n"
"  (n #00AC28969FBE9079BFFC90F55455E629D54E9125657C7543DB104F55001C9E"
"92797333910372D00931C4886BE3DA195ED540E799D5C0AA1EFBBAFB6D687097A3C7"
"FBC4430EFDDED910F66C67F4A11AE7A6925413394A4856426EA1B924849A88A459D2"
"0C963F0FA13DEE3DA3BF41445881A49A03E2417EEF15207C0114E57F0639AAFF32ED"
"92D2C51796F3601337EAB8E82036344F508972E560B57CBEB0413AB48885C9F20B6D"
"AE6D946E2562B3E9B42D4D82B6E3E4CE714E9C7EF8DC35C851013F637C4D658E902D"
"A85A3894C6CC2425D9C7979FD0482343B46B9543C4ACCA1A0FB34132E30F8654E583"
"98D38749FBC63158EA79B73C6C87DDB2BD56A920DF7A4BDE2EF16E28D36A3D76B560"
"7944A6804D4BBDFA92B52B7DADB80A6279D347D2D3124A4F0BAABD420882E00032E1"
"9476AE9E195C2A0FD09A73ACE50757FF250DDDF8F511B222F9510C1F17DFAAD5C8C3"
"F6198F511E69DEBE1257DF6692E9CB232A944F53521469224A3483CFC59AB7D59A5D"
"B6FFF620CF6359914B1905286D03#)\n"
"  (e #010001#)\n"
"  (d #2322235F155649AA6F022C36DA52DEDDABAB7E5CC031F4379C13FC8E49C8E8"
"AE855E942D067CC32B976699D2059BE0D917664C642D6DEA65C80A7090FC4D4DFCCD"
"7A078F632ADBD494DD99B7783B53E40FFFBD6E9724BD09D0A70B7012E9B0920DCC8A"
"8A0CF3851DECE5426A11094020B0F5476EA09C257183D01AAE67896D3D4E92C71369"
"BFBEBE2A2D9FC13C4B9811B3252CB6B5025FE2C4C234E37B77CC61B46CCD422AA7E7"
"0D70D9ABA28181E3A5CD282C67C4B586B51AC5E4C697E939F2780654FF2ED151B72F"
"800EE7EFAC08B44E309BF93A3304927591F34A912618EBA58369C42AC2D5A58476F7"
"A80C000A19278B4938C208294B8E1CEAA17941AF549CEC640C51BEDEA0D51A8C66E3"
"6EAF1DC1F79EFFEFEFB47119A3E434E32EACBB9C75EAF669DD91B54C739BFCCA9504"
"707EBAEA445673255160F3574EBAC5D257E52BF80C43F78D02DDC34CE291F30A9074"
"D5A0E1624F7FDC72D8EB7A4499754BC8CD302B20D7C14920726899E30D2D571BFDF6"
"B39A80CDBFF64DFB2F0E3CC891#)\n"
"  (p #00CD37BD7DAE22FA808DC3CDCBE6A68AD444D692707C5BF564A97563193B30"
"211636A46D1C7523DF06DBDCA83F7D19290F83B132C43BE02E882655ABE94261EC2B"
"99BE4962875B3D8D78CE4A2F2F2B9FA006327E1E924E2AD9BBDC033F1521BA7E7FB8"
"12944CD7DC23E3F26635C9B998974915947EB634607A7C74FE6BE21A07F37836F0FF"
"F6DC44494B23E0098103EF12730D46B5D70A330690534BCF0359F252991A3D205F00"
"A266D02DB68F3C7530F63F7D8052E9DDC8A16CEFA17BA5A23813#)\n"
"  (q #00D6C2982A666A794F2CE1B0C603343994369B702AA01BB5DE28DC8CC009C9"
"07F574875CEA31B8EC1E84AE38762FC33D531FB02EB01BA5D52B6AAD1287B18BA6B0"
"B3B1AF16B8449638E11CF53F5C764A3BE9C78C115936A2C20D44F88745CEFB9AAF10"
"C41557B3D4D7651E6A90E41A13DD9332142898795AD96AB0F3D783797688EAB1ACC8"
"4247CEE427F4C8B015F094B0C470F5ED5C64850B2B34F2EDE4F467D99C484BB61CC2"
"40B34F01B80C424E0CDA72D44B6ED87A56B91E54F989707C7551#)\n"
"  (u #584ED527F71C163D8F552DCE5866CF8DF0FCC269BB0DBB992DFB4FA48C13B7"
"8C984269C08928C211A243194D713FEB2B38134E91AB5F89180BC8A9845E9483561E"
"B28006167DC0EAEDC11C57D9021347F71E4E7ED68E748865B9A55FAAE73DC0860F75"
"6A98CD87C34D2882E1F0985341F6AD4134E749598DB1C696B95306A758C7B9BA8B7F"
"14601117F7B67371DC8CC324F5B5C62AC333434A6752106F09E3699E94DEE5AB00F5"
"D478FE6D002D6473EFBC9A02E4507135ED37EF3F26401D687A#)\n"
"  )\n"
" )\n"
;

static const char sample_public_rsa_key_3072[] =
"(public-key \n"
" (rsa \n"
"  (n #00AC28969FBE9079BFFC90F55455E629D54E9125657C7543DB104F55001C9E"
"92797333910372D00931C4886BE3DA195ED540E799D5C0AA1EFBBAFB6D687097A3C7"
"FBC4430EFDDED910F66C67F4A11AE7A6925413394A4856426EA1B924849A88A459D2"
"0C963F0FA13DEE3DA3BF41445881A49A03E2417EEF15207C0114E57F0639AAFF32ED"
"92D2C51796F3601337EAB8E82036344F508972E560B57CBEB0413AB48885C9F20B6D"
"AE6D946E2562B3E9B42D4D82B6E3E4CE714E9C7EF8DC35C851013F637C4D658E902D"
"A85A3894C6CC2425D9C7979FD0482343B46B9543C4ACCA1A0FB34132E30F8654E583"
"98D38749FBC63158EA79B73C6C87DDB2BD56A920DF7A4BDE2EF16E28D36A3D76B560"
"7944A6804D4BBDFA92B52B7DADB80A6279D347D2D3124A4F0BAABD420882E00032E1"
"9476AE9E195C2A0FD09A73ACE50757FF250DDDF8F511B222F9510C1F17DFAAD5C8C3"
"F6198F511E69DEBE1257DF6692E9CB232A944F53521469224A3483CFC59AB7D59A5D"
"B6FFF620CF6359914B1905286D03#)\n"
"  (e #010001#)\n"
"  )\n"
" )\n"
;

static const char sample_private_rsa_key_4096[] =
"(private-key \n"
" (rsa \n"
"  (n #00C13612FBA6DA535403677448ACA42C7601A556D08678DB1C1AB209D0341E"
"B5D6F9FCA26EA57199471A7C07E5FF1CEEB91202241756E7B6EA55EFAEC44B17A897"
"3337302AE96FE76142C110B1EBB7B40987BA9243D475B2739F75A444ED875384C84E"
"0753178E88CFA2D674474D444098111E0F8931BCB30CA03BF40D6D4D56AFE256E777"
"84DB66E258C1C6853E714DC1329D3441F5134F1F294C2A006C340A9D2AF824CFF457"
"27C5830A0AF32B1EF5C2C3281B22B78DB79079CEB38B3F3E160C7D406B4FD1BC2D9D"
"B8B52C67031D6A30464C1098450080779A87F961AA6E9952A78F8739C2AAD7ACA210"
"AD8CA3D713BD612C770F10C50ACC6908EAD0A03FB988D9671A6F66A232FA1F5FE395"
"8DEE13A69689C5FE9E2724E1AAC21E64FB9F7D95F58452074A33ECA1367B8CE90DFB"
"04F4C4120BAEC00E25DAD6C4EC28C3B54C0F17D19DFB4AD6120412E3B2BC4965BC8D"
"B402E69A0F53D2EE885C7C36CFA9683E42CDE2F261241F7D0528A61FA64E3D1C0180"
"E9369F6329F1F4B04E868E8A28932346013DC3F3444D227553E8F7F90B99D4B807F5"
"CF2675FA492D4C10764407FE2E0A71410401CFFCB6580270033206E0FDB55733E6AE"
"AB32494726C136833D7A1C3AA5AA870825ACDBD707C86A1032E6064CB525B30945EE"
"FF2E65FA6E560BB1C031CFE2C8828ACD226990BFC103FDDD8CD5C7F231E69B64BD97"
"A50EE74877E1#)\n"
"  (e #010001#)\n"
"  (d #0AFCB70DF8CF8FE48655B2E9A7BE50499CFA038553D84F3C25127C77600CB7"
"B2296B77408BDCF96699E2CABF86B1E500C0E83E982B2E28708C01821D3C8349BD8B"
"6BE1D681898662B95D3EEF4C8F18F30A750ECDECFE37C4812165FD861FBCC7F4C211"
"D83A4DE0897A379ACB8136813BD9E157E87E8668B783972A8A3C43252101B7326E26"
"AFCEAFCEC9FA2EE7E84BE526D52F671422372222583D47E289FFBE84A131800D38E0"
"3F4849366A874863EE6193081EAC747D27549DEF5CDAF45364536D148D289EA708C1"
"E65F9CDE0850051317A3EBF22494A9A32FD9FBCBFED7C38E5972F5A869CC5E4F505B"
"D0D47258778657F524A0AE012EB2F1B9B4B27587E889B1D107036C59DCE206409B5E"
"92F157DE97EAF1C41076E6BB012744EE578E19B4F060714DC3131A867D52AF346270"
"E58BE5536FDC48CD59C2062898181617648605600B749DB99BBA2D1E6787FC6F0F2C"
"D6B1FA428C00ABD3B32C372949CB3C19E8D567F1DEEB3ACECFE05984040358E5ADDD"
"921168445DBCFB480CDEE907A418F98003658ACCF041258DCCEA7102CF04CD5A1CD9"
"EB62A7E825B386A0A9045E4E91E3013DAA4858295B092B74C261F4447DC5F1C3514F"
"4FC47E6BA13C15B4C817E3AF087AFC638FA4635FCF7F0CE42BC76BF0A7E11DC029A2"
"E2D70DFBAFFAA49AAD9B7199233A7B9F74D989182705A5C9F1FE35DF6ABDFB482FAA"
"A273E29849#)\n"
"  (p #00C57614F493C9B2FA39F8388DD775189F625FBAA9F310FE9B29050259B719"
"D9601A070BE2CDDFFE29FB7C1EC8228D565330D2CC8DC8D8AB3B3BB52CFDC75700A5"
"E9EBEF69ACDA308E5620F1626B20C84B3B28FC0ABE5BBCE3080196C66736D2BFDDA9"
"17F179126D4C569C7EB48F9353DE62AE49A42A7C03F696D4666558C7A1E23EAA147D"
"7C0621DA5612DCB324355811CE65341168A0FD4159A3B080BE2F96CCFD7B7DEE71B5"
"6C04A68455736E35009D3BBF3231E027F7D5F67CDB57D81E215623B647160716920C"
"90F9685C53197146A4A8ACDBF563C9F944D503AE1916AB481DCCA4E843926D5A5335"
"871E8DC40181CD1453988539E7407EE5F2337A4EC535#)\n"
"  (q #00FA7D71E8A70AA5CF33B125A29F1B8E15EB60B83F6E44879EBF33C7029ADE"
"1BA565BF4862AECA2E4EA39CF8F9621AE6CF5094697CA43AFCE73F633B89DEBD026E"
"218740089FEDE00E16004048216DBDF69F0D66BEAD543B4983ED512585E85D55A6C2"
"7399157BB6B83AAFFA084C2599011A0F2E2228E85AC5457EC84DCB0E20E7B065AEC0"
"CAD67DA1C9E3FC79120F9096A5F57B81D97B81961C85D158C6BF34C1922D8D8C4B6B"
"4A138933A882C657AE94EB7B804BB973291D58DB40B541F02366B42C61AECBB22901"
"0C17AA19E33682617C03B6B20D66335C79933CC25CA5594068BCC94F09BAC7955D79"
"C84EF0F4821127D959286A0F30D06B3B7B760950197D#)\n"
"  (u #3C322D5C68FDF8CF92A68E1EE596FE7ACD1E8287EAC36545036133D29A19FB"
"341186FB83CA59FDEF7F9F25E97C3EB7843C2C7188019C910F1E68735D6FA29C8E04"
"4F4E1D39008C0C52FFA76E271FDA096ADC6A67957A833ECC19E0D02C7336C8DA548A"
"B66EFFE98D3429A8935EF62C56764939DE157BC264F3F37253AE3A86F253BD522A21"
"4C4CD4D48EE639520C58C427F2E1A413BDF696F9335194E81D5FD9C46397AB566754"
"2ADE3818A44818F0C037D5A9DA21CBF60C610AF44A32FF78D4DFC751878C77B8FE92"
"AE40D67E0BC3C52CAE2DCC6AB8E35D93B235CCA908822A0A272A9D068E6FE0447BD0"
"3D485565B133FF42E4BB780DD3EA819261B07F64A6#)\n"
"  )\n"
" )\n"
;

static const char sample_public_rsa_key_4096[] =
"(public-key \n"
" (rsa \n"
"  (n #00C13612FBA6DA535403677448ACA42C7601A556D08678DB1C1AB209D0341E"
"B5D6F9FCA26EA57199471A7C07E5FF1CEEB91202241756E7B6EA55EFAEC44B17A897"
"3337302AE96FE76142C110B1EBB7B40987BA9243D475B2739F75A444ED875384C84E"
"0753178E88CFA2D674474D444098111E0F8931BCB30CA03BF40D6D4D56AFE256E777"
"84DB66E258C1C6853E714DC1329D3441F5134F1F294C2A006C340A9D2AF824CFF457"
"27C5830A0AF32B1EF5C2C3281B22B78DB79079CEB38B3F3E160C7D406B4FD1BC2D9D"
"B8B52C67031D6A30464C1098450080779A87F961AA6E9952A78F8739C2AAD7ACA210"
"AD8CA3D713BD612C770F10C50ACC6908EAD0A03FB988D9671A6F66A232FA1F5FE395"
"8DEE13A69689C5FE9E2724E1AAC21E64FB9F7D95F58452074A33ECA1367B8CE90DFB"
"04F4C4120BAEC00E25DAD6C4EC28C3B54C0F17D19DFB4AD6120412E3B2BC4965BC8D"
"B402E69A0F53D2EE885C7C36CFA9683E42CDE2F261241F7D0528A61FA64E3D1C0180"
"E9369F6329F1F4B04E868E8A28932346013DC3F3444D227553E8F7F90B99D4B807F5"
"CF2675FA492D4C10764407FE2E0A71410401CFFCB6580270033206E0FDB55733E6AE"
"AB32494726C136833D7A1C3AA5AA870825ACDBD707C86A1032E6064CB525B30945EE"
"FF2E65FA6E560BB1C031CFE2C8828ACD226990BFC103FDDD8CD5C7F231E69B64BD97"
"A50EE74877E1#)\n"
"  (e #010001#)\n"
"  )\n"
" )\n"
;

static const char sample_private_dsa_key_2048[] =
"(private-key\n"
"  (dsa\n"
"   (p #00B54636673962B64F7DC23C71ACEF6E7331796F607560B194DFCC0CA370E858A365"
       "A413152FB6EB8C664BD171AC316FE5B381CD084D07377571599880A068EF1382D85C"
       "308B4E9DEAC12D66DE5C4A826EBEB5ED94A62E7301E18927E890589A2F230272A150"
       "C118BC3DC2965AE0D05BE4F65C6137B2BA7EDABB192C3070D202C10AA3F534574970"
       "71454DB8A73DDB6511A5BA98EF1450FD90DE5BAAFC9FD3AC22EBEA612DD075BB7405"
       "D56866D125E33982C046808F7CEBA8E5C0B9F19A6FE451461660A1CBA9EF68891179"
       "0256A573D3B8F35A5C7A0C6C31F2DB90E25A26845252AD9E485EF2D339E7B5890CD4"
       "2F9C9F315ED409171EC35CA04CC06B275577B3#)\n"
"   (q #00DA67989167FDAC4AE3DF9247A716859A30C0CF9C5A6DBA01EABA3481#)\n"
"   (g #48E35DA584A089D05142AA63603FDB00D131B07A0781E2D5A8F9614D2B33D3E40A78"
       "98A9E10CDBB612CF093F95A3E10D09566726F2C12823836B2D9CD974BB695665F3B3"
       "5D219A9724B87F380BD5207EDA0AE38C79E8F18122C3F76E4CEB0ABED3250914987F"
       "B30D4B9E19C04C28A5D4F45560AF586F6A1B41751EAD90AE7F044F4E2A4A50C1F508"
       "4FC202463F478F678B9A19392F0D2961C5391C546EF365368BB46410C9C1CEE96E9F"
       "0C953570C2ED06328B11C90E86E57CAA7FA5ABAA278E22A4C8C08E16EE59F484EC44"
       "2CF55535BAA2C6BEA8833A555372BEFE1E665D3C7DAEF58061D5136331EF4EB61BC3"
       "6EE4425A553AF8885FEA15A88135BE133520#)\n"
"   (y #66E0D1A69D663466F8FEF2B7C0878DAC93C36A2FB2C05E0306A53B926021D4B92A1C"
       "2FA6860061E88E78CBBBA49B0E12700F07DBF86F72CEB2927EDAC0C7E3969C3A47BB"
       "4E0AE93D8BB3313E93CC7A72DFEEE442EFBC81B3B2AEC9D8DCBE21220FB760201D79"
       "328C41C773866587A44B6954767D022A88072900E964089D9B17133603056C985C4F"
       "8A0B648F297F8D2C3CB43E4371DC6002B5B12CCC085BDB2CFC5074A0587566187EE3"
       "E11A2A459BD94726248BB8D6CC62938E11E284C2C183576FBB51749EB238C4360923"
       "79C08CE1C8CD77EB57404CE9B4744395ACF721487450BADE3220576F2F816248B0A7"
       "14A264330AECCB24DE2A1107847B23490897#)\n"
"   (x #477BD14676E22563C5ABA68025CEBA2A48D485F5B2D4AD4C0EBBD6D0#)\n"
"))\n";

static const char sample_public_dsa_key_2048[] =
"(public-key\n"
"  (dsa\n"
"   (p #00B54636673962B64F7DC23C71ACEF6E7331796F607560B194DFCC0CA370E858A365"
       "A413152FB6EB8C664BD171AC316FE5B381CD084D07377571599880A068EF1382D85C"
       "308B4E9DEAC12D66DE5C4A826EBEB5ED94A62E7301E18927E890589A2F230272A150"
       "C118BC3DC2965AE0D05BE4F65C6137B2BA7EDABB192C3070D202C10AA3F534574970"
       "71454DB8A73DDB6511A5BA98EF1450FD90DE5BAAFC9FD3AC22EBEA612DD075BB7405"
       "D56866D125E33982C046808F7CEBA8E5C0B9F19A6FE451461660A1CBA9EF68891179"
       "0256A573D3B8F35A5C7A0C6C31F2DB90E25A26845252AD9E485EF2D339E7B5890CD4"
       "2F9C9F315ED409171EC35CA04CC06B275577B3#)\n"
"   (q #00DA67989167FDAC4AE3DF9247A716859A30C0CF9C5A6DBA01EABA3481#)\n"
"   (g #48E35DA584A089D05142AA63603FDB00D131B07A0781E2D5A8F9614D2B33D3E40A78"
       "98A9E10CDBB612CF093F95A3E10D09566726F2C12823836B2D9CD974BB695665F3B3"
       "5D219A9724B87F380BD5207EDA0AE38C79E8F18122C3F76E4CEB0ABED3250914987F"
       "B30D4B9E19C04C28A5D4F45560AF586F6A1B41751EAD90AE7F044F4E2A4A50C1F508"
       "4FC202463F478F678B9A19392F0D2961C5391C546EF365368BB46410C9C1CEE96E9F"
       "0C953570C2ED06328B11C90E86E57CAA7FA5ABAA278E22A4C8C08E16EE59F484EC44"
       "2CF55535BAA2C6BEA8833A555372BEFE1E665D3C7DAEF58061D5136331EF4EB61BC3"
       "6EE4425A553AF8885FEA15A88135BE133520#)\n"
"   (y #66E0D1A69D663466F8FEF2B7C0878DAC93C36A2FB2C05E0306A53B926021D4B92A1C"
       "2FA6860061E88E78CBBBA49B0E12700F07DBF86F72CEB2927EDAC0C7E3969C3A47BB"
       "4E0AE93D8BB3313E93CC7A72DFEEE442EFBC81B3B2AEC9D8DCBE21220FB760201D79"
       "328C41C773866587A44B6954767D022A88072900E964089D9B17133603056C985C4F"
       "8A0B648F297F8D2C3CB43E4371DC6002B5B12CCC085BDB2CFC5074A0587566187EE3"
       "E11A2A459BD94726248BB8D6CC62938E11E284C2C183576FBB51749EB238C4360923"
       "79C08CE1C8CD77EB57404CE9B4744395ACF721487450BADE3220576F2F816248B0A7"
       "14A264330AECCB24DE2A1107847B23490897#)\n"
"))\n";

static const char sample_private_dsa_key_3072[] =
"(private-key\n"
"  (dsa\n"
"   (p #00BA73E148AEA5E8B64878AF5BE712B8302B9671C5F3EEB7722A9D0D9868D048C938"
       "877C91C335C7819292E69C7D34264F1578E32EC2DA8408DF75D0EB76E0D3030B84B5"
       "62D8EF93AB53BAB6B8A5DE464F5CA87AEA43BDCF0FB0B7815AA3114CFC84FD916A83"
       "B3D5FD78390189332232E9D037D215313FD002FF46C048B66703F87FAE092AAA0988"
       "AC745336EBE672A01DEDBD52395783579B67CF3AE1D6F1602CCCB12154FA0E00AE46"
       "0D9B289CF709194625BCB919B11038DEFC50ADBBA20C3F320078E4E9529B4F6848E2"
       "AB5E6278DB961FE226F2EEBD201E071C48C5BEF98B4D9BEE42C1C7102D893EBF8902"
       "D7A91266340AFD6CE1D09E52282FFF5B97EAFA3886A3FCF84FF76D1E06538D0D8E60"
       "B3332145785E07D29A5965382DE3470D1D888447FA9C00A2373378FC3FA7B9F7D17E"
       "95A6A5AE1397BE46D976EF2C96E89913AC4A09351CA661BF6F67E30407DA846946C7"
       "62D9BAA6B77825097D3E7B886456BB32E3E74516BF3FD93D71B257AA8F723E01CE33"
       "8015353D3778B02B892AF7#)\n"
"   (q #00BFF3F3CC18FA018A5B8155A8695E1E4939660D5E4759322C39D50F3B93E5F68B#)\n"
"   (g #6CCFD8219F5FCE8EF2BEF3262929787140847E38674B1EF8DB20255E212CB6330EC4"
       "DFE8A26AB7ECC5760DEB9BBF59A2B2821D510F1868172222867558B8D204E889C474"
       "7CA30FBF9D8CF41AE5D5BD845174641101593849FF333E6C93A6550931B2B9D56B98"
       "9CAB01729D9D736FA6D24A74D2DDE1E9E648D141473E443DD6BBF0B3CAB64F9FE4FC"
       "134B2EB57437789F75C744DF1FA67FA8A64603E5441BC7ECE29E00BDF262BDC81E8C"
       "7330A18A412DE38E7546D342B89A0AF675A89E6BEF00540EB107A2FE74EA402B0D89"
       "F5C02918DEEEAF8B8737AC866B09B50810AB8D8668834A1B9E1E53866E2B0A926FAB"
       "120A0CDE5B3715FFFE6ACD1AB73588DCC1EC4CE9392FE57F8D1D35811200CB07A0E6"
       "374E2C4B0AEB7E3D077B8545C0E438DCC0F1AE81E186930E99EBC5B91B77E92803E0"
       "21602887851A4FFDB3A7896AC655A0901218C121C5CBB0931E7D5EAC243F37711B5F"
       "D5A62B1B38A83F03D8F6703D8B98DF367FC8A76990335F62173A5391836F0F2413EC"
       "4997AF9EB55C6660B01A#)\n"
"   (y #2320B22434C5DB832B4EC267CC52E78DD5CCFA911E8F0804E7E7F32B186B2D4167AE"
       "4AA6869822E76400492D6A193B0535322C72B0B7AA4A87E33044FDC84BE24C64A053"
       "A37655EE9EABDCDC1FDF63F3F1C677CEB41595DF7DEFE9178D85A3D621B4E4775492"
       "8C0A58D2458D06F9562E4DE2FE6129A64063A99E88E54485B97484A28188C4D33F15"
       "DDC903B6CEA0135E3E3D27B4EA39319696305CE93D7BA7BE00367DBE3AAF43491E71"
       "CBF254744A5567F5D70090D6139E0C990239627B3A1C5B20B6F9F6374B8D8D8A8997"
       "437265BE1E3B4810D4B09254400DE287A0DFFBAEF339E48D422B1D41A37E642BC026"
       "73314701C8FA9792845C129351A87A945A03E6C895860E51D6FB8B7340A94D1A8A7B"
       "FA85AC83B4B14E73AB86CB96C236C8BFB0978B61B2367A7FE4F7891070F56C78D5DD"
       "F5576BFE5BE4F333A4E2664E79528B3294907AADD63F4F2E7AA8147B928D8CD69765"
       "3DB98C4297CB678046ED55C0DBE60BF7142C594603E4D705DC3D17270F9F086EC561"
       "2703D518D8D49FF0EBE6#)\n"
"   (x #00A9FFFC88E67D6F7B810E291C050BAFEA7FC4A75E8D2F16CFED3416FD77607232#)\n"
"))\n";

static const char sample_public_dsa_key_3072[] =
"(public-key\n"
"  (dsa\n"
"   (p #00BA73E148AEA5E8B64878AF5BE712B8302B9671C5F3EEB7722A9D0D9868D048C938"
       "877C91C335C7819292E69C7D34264F1578E32EC2DA8408DF75D0EB76E0D3030B84B5"
       "62D8EF93AB53BAB6B8A5DE464F5CA87AEA43BDCF0FB0B7815AA3114CFC84FD916A83"
       "B3D5FD78390189332232E9D037D215313FD002FF46C048B66703F87FAE092AAA0988"
       "AC745336EBE672A01DEDBD52395783579B67CF3AE1D6F1602CCCB12154FA0E00AE46"
       "0D9B289CF709194625BCB919B11038DEFC50ADBBA20C3F320078E4E9529B4F6848E2"
       "AB5E6278DB961FE226F2EEBD201E071C48C5BEF98B4D9BEE42C1C7102D893EBF8902"
       "D7A91266340AFD6CE1D09E52282FFF5B97EAFA3886A3FCF84FF76D1E06538D0D8E60"
       "B3332145785E07D29A5965382DE3470D1D888447FA9C00A2373378FC3FA7B9F7D17E"
       "95A6A5AE1397BE46D976EF2C96E89913AC4A09351CA661BF6F67E30407DA846946C7"
       "62D9BAA6B77825097D3E7B886456BB32E3E74516BF3FD93D71B257AA8F723E01CE33"
       "8015353D3778B02B892AF7#)\n"
"   (q #00BFF3F3CC18FA018A5B8155A8695E1E4939660D5E4759322C39D50F3B93E5F68B#)\n"
"   (g #6CCFD8219F5FCE8EF2BEF3262929787140847E38674B1EF8DB20255E212CB6330EC4"
       "DFE8A26AB7ECC5760DEB9BBF59A2B2821D510F1868172222867558B8D204E889C474"
       "7CA30FBF9D8CF41AE5D5BD845174641101593849FF333E6C93A6550931B2B9D56B98"
       "9CAB01729D9D736FA6D24A74D2DDE1E9E648D141473E443DD6BBF0B3CAB64F9FE4FC"
       "134B2EB57437789F75C744DF1FA67FA8A64603E5441BC7ECE29E00BDF262BDC81E8C"
       "7330A18A412DE38E7546D342B89A0AF675A89E6BEF00540EB107A2FE74EA402B0D89"
       "F5C02918DEEEAF8B8737AC866B09B50810AB8D8668834A1B9E1E53866E2B0A926FAB"
       "120A0CDE5B3715FFFE6ACD1AB73588DCC1EC4CE9392FE57F8D1D35811200CB07A0E6"
       "374E2C4B0AEB7E3D077B8545C0E438DCC0F1AE81E186930E99EBC5B91B77E92803E0"
       "21602887851A4FFDB3A7896AC655A0901218C121C5CBB0931E7D5EAC243F37711B5F"
       "D5A62B1B38A83F03D8F6703D8B98DF367FC8A76990335F62173A5391836F0F2413EC"
       "4997AF9EB55C6660B01A#)\n"
"   (y #2320B22434C5DB832B4EC267CC52E78DD5CCFA911E8F0804E7E7F32B186B2D4167AE"
       "4AA6869822E76400492D6A193B0535322C72B0B7AA4A87E33044FDC84BE24C64A053"
       "A37655EE9EABDCDC1FDF63F3F1C677CEB41595DF7DEFE9178D85A3D621B4E4775492"
       "8C0A58D2458D06F9562E4DE2FE6129A64063A99E88E54485B97484A28188C4D33F15"
       "DDC903B6CEA0135E3E3D27B4EA39319696305CE93D7BA7BE00367DBE3AAF43491E71"
       "CBF254744A5567F5D70090D6139E0C990239627B3A1C5B20B6F9F6374B8D8D8A8997"
       "437265BE1E3B4810D4B09254400DE287A0DFFBAEF339E48D422B1D41A37E642BC026"
       "73314701C8FA9792845C129351A87A945A03E6C895860E51D6FB8B7340A94D1A8A7B"
       "FA85AC83B4B14E73AB86CB96C236C8BFB0978B61B2367A7FE4F7891070F56C78D5DD"
       "F5576BFE5BE4F333A4E2664E79528B3294907AADD63F4F2E7AA8147B928D8CD69765"
       "3DB98C4297CB678046ED55C0DBE60BF7142C594603E4D705DC3D17270F9F086EC561"
       "2703D518D8D49FF0EBE6#)\n"
"))\n";

/* Keys are kept as samples instead of being generated, as prime search
   time varies too much for slope measurement.  */
static const struct
{
  const char *name;
  const char *sec_key;
  const char *pub_key;
  unsigned int value_bits;   /* Size of value to be signed. */
  int sign_cost;             /* Scales down signing repetitions. */
  int fips_allowed;
} pk_algos[] = {
#if USE_RSA
  { "RSA-2048", sample_private_rsa_key_2048, sample_public_rsa_key_2048,
    2040, 4, 1 },
  { "RSA-3072", sample_private_rsa_key_3072, sample_public_rsa_key_3072,
    3064, 8, 1 },
  { "RSA-4096", sample_private_rsa_key_4096, sample_public_rsa_key_4096,
    4088, 16, 1 },
#endif
#if USE_DSA
  { "DSA-2048", sample_private_dsa_key_2048, sample_public_dsa_key_2048,
    224, 1, 0 },
  { "DSA-3072", sample_private_dsa_key_3072, sample_public_dsa_key_3072,
    256, 2, 0 },
#endif
  { NULL, NULL, NULL, 0, 1, 0 }
};


static const char *
pk_algo_name (int algo)
{
  if (algo < 0 || algo >= __MAX_PK_ALGO)
    return NULL;

  return pk_algos[algo].name;
}


static int
pk_map_name (const char *name)
{
  int i;

  for (i = 0; i < __MAX_PK_ALGO; i++)
    if (!strcmp (pk_algos[i].name, name))
      return i;

  return -1;
}


static int
bench_pk_init (struct bench_obj *obj)
{
  struct bench_pk_oper *oper = obj->priv;
  struct bench_pk_hd *hd;
  gcry_mpi_t x;
  gpg_error_t err;
  int cost;

  cost = oper->oper == PK_OPER_SIGN ? pk_algos[oper->algo].sign_cost : 1;

  obj->min_bufsize = 1;
  obj->max_bufsize = 4;
  obj->step_size = 1;
  obj->num_measure_repetitions =
    num_measurement_repetitions / obj->max_bufsize / cost;
  if (obj->num_measure_repetitions == 0)
    obj->num_measure_repetitions = 1;

  hd = calloc (1, sizeof(*hd));
  if (!hd)
    return -1;

  err = gcry_sexp_sscan (&hd->sec_key, NULL, pk_algos[oper->algo].sec_key,
			 strlen (pk_algos[oper->algo].sec_key));
  if (!err)
    err = gcry_sexp_sscan (&hd->pub_key, NULL, pk_algos[oper->algo].pub_key,
			   strlen (pk_algos[oper->algo].pub_key));
  if (err)
    {
      fprintf (stderr, PGM ": gcry_sexp_sscan failed: %s\n",
	       gpg_strerror (err));
      exit (1);
    }

  x = gcry_mpi_new (pk_algos[oper->algo].value_bits);
  gcry_mpi_randomize (x, pk_algos[oper->algo].value_bits, GCRY_WEAK_RANDOM);
  err = gcry_sexp_build (&hd->data, NULL, "(data (flags raw) (value %m))", x);
  gcry_mpi_release (x);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_sexp_build failed: %s\n",
	       gpg_strerror (err));
      exit (1);
    }

  err = gcry_pk_sign (&hd->sig, hd->data, hd->sec_key);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_pk_sign failed: %s\n",
	       gpg_strerror (err));
      exit (1);
    }

  obj->hd = hd;
  return 0;
}


static void
bench_pk_free (struct bench_obj *obj)
{
  struct bench_pk_hd *hd = obj->hd;

  gcry_sexp_release (hd->sig);
  gcry_sexp_release (hd->data);
  gcry_sexp_release (hd->pub_key);
  gcry_sexp_release (hd->sec_key);
  free (hd);
  obj->hd = NULL;
}


static void
bench_pk_sign_do_bench (struct bench_obj *obj, void *buf, size_t num_iter)
{
  struct bench_pk_hd *hd = obj->hd;
  gcry_sexp_t sig;
  gpg_error_t err;
  size_t i;

  (void)buf;

  for (i = 0; i < num_iter; i++)
    {
      err = gcry_pk_sign (&sig, hd->data, hd->sec_key);
      if (err)
	{
	  fprintf (stderr, PGM ": gcry_pk_sign failed: %s\n",
		   gpg_strerror (err));
	  exit (1);
	}
      gcry_sexp_release (sig);
    }
}


static void
bench_pk_verify_do_bench (struct bench_obj *obj, void *buf, size_t num_iter)
{
  struct bench_pk_hd *hd = obj->hd;
  gpg_error_t err;
  size_t i;

  (void)buf;

  for (i = 0; i < num_iter; i++)
    {
      err = gcry_pk_verify (hd->sig, hd->data, hd->pub_key);
      if (err)
	{
	  fprintf (stderr, PGM ": gcry_pk_verify failed: %s\n",
		   gpg_strerror (err));
	  exit (1);
	}
    }
}


static struct bench_ops pk_sign_ops = {
  &bench_pk_init,
  &bench_pk_free,
  &bench_pk_sign_do_bench
};

static struct bench_ops pk_verify_ops = {
  &bench_pk_init,
  &bench_pk_free,
  &bench_pk_verify_do_bench
};

static struct bench_pk_oper pk_operations[] = {
  { PK_OPER_SIGN,   "sign",   &pk_sign_ops },
  { PK_OPER_VERIFY, "verify", &pk_verify_ops },
  { 0, NULL, NULL }
};


static void
cipher_pk_one (enum bench_pk_algo algo, struct bench_pk_oper *poper)
{
  struct bench_pk_oper oper = *poper;
  struct bench_obj obj = { 0 };
  double result;

  oper.algo = algo;

  bench_print_mode (14, oper.name);

  obj.ops = oper.ops;
  obj.priv = &oper;

  result = do_slope_benchmark (&obj);
  bench_print_result_nsec_per_iteration (result);
}


static void
_pk_bench (int algo)
{
  int i;

  if (in_fips_mode && !pk_algos[algo].fips_allowed)
    return;

  bench_print_header_nsec_per_iteration (14, pk_algo_name (algo));

  for (i = 0; pk_operations[i].name; i++)
    cipher_pk_one (algo, &pk_operations[i]);

  bench_print_footer (14);
}


void
pk_bench (char **argv, int argc)
{
  int i, algo;

  bench_print_section ("pk", "Public-key");

  if (argv && argc)
    {
      for (i = 0; i < argc; i++)
	{
	  algo = pk_map_name (argv[i]);
	  if (algo >= 0)
	    _pk_bench (algo);
	}
    }
  else
    {
      for (i = 0; i < __MAX_PK_ALGO; i++)
	_pk_bench (i);
    }
}


/************************************************************ ECC benchmarks. */

#if USE_ECC
enum bench_ecc_algo
{
  ECC_ALGO_ED25519 = 0,
  ECC_ALGO_ED448,
  ECC_ALGO_X25519,
  ECC_ALGO_X448,
  ECC_ALGO_NIST_P192,
  ECC_ALGO_NIST_P224,
  ECC_ALGO_NIST_P256,
  ECC_ALGO_NIST_P384,
  ECC_ALGO_NIST_P521,
  ECC_ALGO_SECP256K1,
  ECC_ALGO_BRAINP256R1,
  __MAX_ECC_ALGO
};

enum bench_ecc_operation
{
  ECC_OPER_MULT = 0,
  ECC_OPER_KEYGEN,
  ECC_OPER_SIGN,
  ECC_OPER_VERIFY,
  __MAX_ECC_OPER
};

struct bench_ecc_oper
{
  enum bench_ecc_operation oper;
  const char *name;
  struct bench_ops *ops;

  enum bench_ecc_algo algo;
};

struct bench_ecc_mult_hd
{
  gcry_ctx_t ec;
  gcry_mpi_t k, x, y;
  gcry_mpi_point_t G, Q;
};

struct bench_ecc_hd
{
  gcry_sexp_t key_spec;
  gcry_sexp_t data;
  gcry_sexp_t pub_key;
  gcry_sexp_t sec_key;
  gcry_sexp_t sig;
};


static int
ecc_algo_fips_allowed (int algo)
{
  switch (algo)
    {
      case ECC_ALGO_NIST_P224:
      case ECC_ALGO_NIST_P256:
      case ECC_ALGO_NIST_P384:
      case ECC_ALGO_NIST_P521:
      case ECC_ALGO_ED25519:
      case ECC_ALGO_ED448:
        return 1;
      case ECC_ALGO_SECP256K1:
      case ECC_ALGO_BRAINP256R1:
      case ECC_ALGO_X25519:
      case ECC_ALGO_X448:
      case ECC_ALGO_NIST_P192:
      default:
        return 0;
    }
}

static const char *
ecc_algo_name (int algo)
{
  switch (algo)
    {
      case ECC_ALGO_ED25519:
	return "Ed25519";
      case ECC_ALGO_ED448:
	return "Ed448";
      case ECC_ALGO_X25519:
	return "X25519";
      case ECC_ALGO_X448:
	return "X448";
      case ECC_ALGO_NIST_P192:
	return "NIST-P192";
      case ECC_ALGO_NIST_P224:
	return "NIST-P224";
      case ECC_ALGO_NIST_P256:
	return "NIST-P256";
      case ECC_ALGO_NIST_P384:
	return "NIST-P384";
      case ECC_ALGO_NIST_P521:
	return "NIST-P521";
      case ECC_ALGO_SECP256K1:
	return "secp256k1";
      case ECC_ALGO_BRAINP256R1:
	return "brainpoolP256r1";
      default:
	return NULL;
    }
}

static const char *
ecc_algo_curve (int algo)
{
  switch (algo)
    {
      case ECC_ALGO_ED25519:
	return "Ed25519";
      case ECC_ALGO_ED448:
	return "Ed448";
      case ECC_ALGO_X25519:
	return "Curve25519";
      case ECC_ALGO_X448:
	return "X448";
      case ECC_ALGO_NIST_P192:
	return "NIST P-192";
      case ECC_ALGO_NIST_P224:
	return "NIST P-224";
      case ECC_ALGO_NIST_P256:
	return "NIST P-256";
      case ECC_ALGO_NIST_P384:
	return "NIST P-384";
      case ECC_ALGO_NIST_P521:
	return "NIST P-521";
      case ECC_ALGO_SECP256K1:
	return "secp256k1";
      case ECC_ALGO_BRAINP256R1:
	return "brainpoolP256r1";
      default:
	return NULL;
    }
}

static int
ecc_nbits (int algo)
{
  switch (algo)
    {
      case ECC_ALGO_ED25519:
	return 255;
      case ECC_ALGO_ED448:
	return 448;
      case ECC_ALGO_X25519:
	return 255;
      case ECC_ALGO_X448:
	return 448;
      case ECC_ALGO_NIST_P192:
	return 192;
      case ECC_ALGO_NIST_P224:
	return 224;
      case ECC_ALGO_NIST_P256:
	return 256;
      case ECC_ALGO_NIST_P384:
	return 384;
      case ECC_ALGO_NIST_P521:
	return 521;
      case ECC_ALGO_SECP256K1:
	return 256;
      case ECC_ALGO_BRAINP256R1:
	return 256;
      default:
	return 0;
    }
}

static int
ecc_map_name (const char *name)
{
  int i;

  for (i = 0; i < __MAX_ECC_ALGO; i++)
    {
      if (strcmp(ecc_algo_name(i), name) == 0)
	{
	  return i;
	}
    }

  return -1;
}


static int
bench_ecc_mult_init (struct bench_obj *obj)
{
  struct bench_ecc_oper *oper = obj->priv;
  struct bench_ecc_mult_hd *hd;
  int p_size = ecc_nbits (oper->algo);
  gpg_error_t err;
  gcry_mpi_t p;

  obj->min_bufsize = 1;
  obj->max_bufsize = 4;
  obj->step_size = 1;
  obj->num_measure_repetitions =
    num_measurement_repetitions / obj->max_bufsize;

  while (obj->num_measure_repetitions == 0)
    {
      if (obj->max_bufsize == 2)
	{
	  obj->num_measure_repetitions = 2;
	}
      else
	{
	  obj->max_bufsize--;
	  obj->num_measure_repetitions =
	    num_measurement_repetitions / obj->max_bufsize;
	}
    }

  hd = calloc (1, sizeof(*hd));
  if (!hd)
    return -1;

  err = gcry_mpi_ec_new (&hd->ec, NULL, ecc_algo_curve(oper->algo));
  if (err)
    {
      fprintf (stderr, PGM ": gcry_mpi_ec_new failed: %s\n",
	      gpg_strerror (err));
      exit (1);
    }
  hd->G = gcry_mpi_ec_get_point ("g", hd->ec, 1);
  hd->Q = gcry_mpi_point_new (0);
  hd->x = gcry_mpi_new (0);
  hd->y = gcry_mpi_new (0);
  hd->k = gcry_mpi_new (p_size);
  gcry_mpi_randomize (hd->k, p_size, GCRY_WEAK_RANDOM);
  p = gcry_mpi_ec_get_mpi ("p", hd->ec, 1);
  gcry_mpi_mod (hd->k, hd->k, p);
  gcry_mpi_release (p);

  obj->hd = hd;
  return 0;
}

static void
bench_ecc_mult_free (struct bench_obj *obj)
{
  struct bench_ecc_mult_hd *hd = obj->hd;

  gcry_mpi_release (hd->k);
  gcry_mpi_release (hd->y);
  gcry_mpi_release (hd->x);
  gcry_mpi_point_release (hd->Q);
  gcry_mpi_point_release (hd->G);
  gcry_ctx_release (hd->ec);
  free (hd);
  obj->hd = NULL;
}

static void
bench_ecc_mult_do_bench (struct bench_obj *obj, void *buf, size_t num_iter)
{
  struct bench_ecc_oper *oper = obj->priv;
  struct bench_ecc_mult_hd *hd = obj->hd;
  gcry_mpi_t y;
  size_t i;

  (void)buf;

  if (oper->algo == ECC_ALGO_X25519 || oper->algo == ECC_ALGO_X448)
    {
      y = NULL;
    }
  else
    {
      y = hd->y;
    }

  for (i = 0; i < num_iter; i++)
    {
      gcry_mpi_ec_mul (hd->Q, hd->k, hd->G, hd->ec);
      if (gcry_mpi_ec_get_affine (hd->x, y, hd->Q, hd->ec))
	{
	  fprintf (stderr, PGM ": gcry_mpi_ec_get_affine failed\n");
	  exit (1);
	}
    }
}


static int
bench_ecc_init (struct bench_obj *obj)
{
  struct bench_ecc_oper *oper = obj->priv;
  struct bench_ecc_hd *hd;
  int p_size = ecc_nbits (oper->algo);
  gpg_error_t err;
  gcry_mpi_t x;

  obj->min_bufsize = 1;
  obj->max_bufsize = 4;
  obj->step_size = 1;
  obj->num_measure_repetitions =
    num_measurement_repetitions / obj->max_bufsize;

  while (obj->num_measure_repetitions == 0)
    {
      if (obj->max_bufsize == 2)
	{
	  obj->num_measure_repetitions = 2;
	}
      else
	{
	  obj->max_bufsize--;
	  obj->num_measure_repetitions =
	    num_measurement_repetitions / obj->max_bufsize;
	}
    }

  hd = calloc (1, sizeof(*hd));
  if (!hd)
    return -1;

  x = gcry_mpi_new (p_size);
  gcry_mpi_randomize (x, p_size, GCRY_WEAK_RANDOM);

  switch (oper->algo)
    {
      default:
        gcry_mpi_release (x);
        free (hd);
        return -1;

      case ECC_ALGO_ED25519:
        err = gcry_sexp_build (&hd->key_spec, NULL,
                               "(genkey (ecdsa (curve \"Ed25519\")"
                               "(flags eddsa)))");
	if (err)
	  break;
        err = gcry_sexp_build (&hd->data, NULL,
                               "(data (flags eddsa)(hash-algo sha512)"
                               " (value %m))", x);
	break;

      case ECC_ALGO_ED448:
        err = gcry_sexp_build (&hd->key_spec, NULL,
                               "(genkey (ecdsa (curve \"Ed448\")"
                               "(flags eddsa)))");
	if (err)
	  break;
        err = gcry_sexp_build (&hd->data, NULL,
                               "(data (flags eddsa)(hash-algo shake256)"
                               " (value %m))", x);
	break;

      case ECC_ALGO_NIST_P192:
      case ECC_ALGO_NIST_P224:
      case ECC_ALGO_NIST_P256:
      case ECC_ALGO_NIST_P384:
      case ECC_ALGO_NIST_P521:
        err = gcry_sexp_build (&hd->key_spec, NULL,
                               "(genkey (ECDSA (nbits %d)))", p_size);
	if (err)
	  break;
        err = gcry_sexp_build (&hd->data, NULL,
			       "(data (flags raw) (value %m))", x);
	break;
      case ECC_ALGO_BRAINP256R1:
        err = gcry_sexp_build (&hd->key_spec, NULL,
                               "(genkey (ECDSA (curve brainpoolP256r1)))");
	if (err)
	  break;
        err = gcry_sexp_build (&hd->data, NULL,
			       "(data (flags raw) (value %m))", x);
	break;
    }

  gcry_mpi_release (x);

  if (err)
    {
      fprintf (stderr, PGM ": gcry_sexp_build failed: %s\n",
	       gpg_strerror (err));
      exit (1);
    }

  obj->hd = hd;
  return 0;
}

static void
bench_ecc_free (struct bench_obj *obj)
{
  struct bench_ecc_hd *hd = obj->hd;

  gcry_sexp_release (hd->sig);
  gcry_sexp_release (hd->pub_key);
  gcry_sexp_release (hd->sec_key);
  gcry_sexp_release (hd->data);
  gcry_sexp_release (hd->key_spec);
  free (hd);
  obj->hd = NULL;
}

static void
bench_ecc_keygen (struct bench_ecc_hd *hd)
{
  gcry_sexp_t key_pair;
  gpg_error_t err;

  err = gcry_pk_genkey (&key_pair, hd->key_spec);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_pk_genkey failed: %s\n",
		gpg_strerror (err));
      exit (1);
    }

  hd->pub_key = gcry_sexp_find_token (key_pair, "public-key", 0);
  if (!hd->pub_key)
    {
      fprintf (stderr, PGM ": public part missing in key\n");
      exit (1);
    }
  hd->sec_key = gcry_sexp_find_token (key_pair, "private-key", 0);
  if (!hd->sec_key)
    {
      fprintf (stderr, PGM ": private part missing in key\n");
      exit (1);
    }

  gcry_sexp_release (key_pair);
}

static void
bench_ecc_keygen_do_bench (struct bench_obj *obj, void *buf, size_t num_iter)
{
  struct bench_ecc_hd *hd = obj->hd;
  size_t i;

  (void)buf;

  for (i = 0; i < num_iter; i++)
    {
      bench_ecc_keygen (hd);
      gcry_sexp_release (hd->pub_key);
      gcry_sexp_release (hd->sec_key);
    }

  hd->pub_key = NULL;
  hd->sec_key = NULL;
}

static void
bench_ecc_sign_do_bench (struct bench_obj *obj, void *buf, size_t num_iter)
{
  struct bench_ecc_hd *hd = obj->hd;
  gpg_error_t err;
  size_t i;

  (void)buf;

  bench_ecc_keygen (hd);

  for (i = 0; i < num_iter; i++)
    {
      err = gcry_pk_sign (&hd->sig, hd->data, hd->sec_key);
      if (err)
	{
	  fprintf (stderr, PGM ": gcry_pk_sign failed: %s\n",
		  gpg_strerror (err));
	  exit (1);
	}
      gcry_sexp_release (hd->sig);
    }

  gcry_sexp_release (hd->pub_key);
  gcry_sexp_release (hd->sec_key);
  hd->sig = NULL;
  hd->pub_key = NULL;
  hd->sec_key = NULL;
}

static void
bench_ecc_verify_do_bench (struct bench_obj *obj, void *buf, size_t num_iter)
{
  struct bench_ecc_hd *hd = obj->hd;
  gpg_error_t err;
  int i;

  (void)buf;

  bench_ecc_keygen (hd);
  err = gcry_pk_sign (&hd->sig, hd->data, hd->sec_key);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_pk_sign failed: %s\n",
	      gpg_strerror (err));
      exit (1);
    }

  for (i = 0; i < num_iter; i++)
    {
      err = gcry_pk_verify (hd->sig, hd->data, hd->pub_key);
      if (err)
	{
	  fprintf (stderr, PGM ": gcry_pk_verify failed: %s\n",
		  gpg_strerror (err));
	  exit (1);
	}
    }

  gcry_sexp_release (hd->sig);
  gcry_sexp_release (hd->pub_key);
  gcry_sexp_release (hd->sec_key);
  hd->sig = NULL;
  hd->pub_key = NULL;
  hd->sec_key = NULL;
}


static struct bench_ops ecc_mult_ops = {
  &bench_ecc_mult_init,
  &bench_ecc_mult_free,
  &bench_ecc_mult_do_bench
};

static struct bench_ops ecc_keygen_ops = {
  &bench_ecc_init,
  &bench_ecc_free,
  &bench_ecc_keygen_do_bench
};

static struct bench_ops ecc_sign_ops = {
  &bench_ecc_init,
  &bench_ecc_free,
  &bench_ecc_sign_do_bench
};

static struct bench_ops ecc_verify_ops = {
  &bench_ecc_init,
  &bench_ecc_free,
  &bench_ecc_verify_do_bench
};


static struct bench_ecc_oper ecc_operations[] = {
  { ECC_OPER_MULT,   "mult",   &ecc_mult_ops },
  { ECC_OPER_KEYGEN, "keygen", &ecc_keygen_ops },
  { ECC_OPER_SIGN,   "sign",   &ecc_sign_ops },
  { ECC_OPER_VERIFY, "verify", &ecc_verify_ops },
  { 0, NULL, NULL }
};


static void
cipher_ecc_one (enum bench_ecc_algo algo, struct bench_ecc_oper *poper)
{
  struct bench_ecc_oper oper = *poper;
  struct bench_obj obj = { 0 };
  double result;

  if ((algo == ECC_ALGO_X25519 || algo == ECC_ALGO_X448 ||
       algo == ECC_ALGO_SECP256K1) && oper.oper != ECC_OPER_MULT)
    return;

  oper.algo = algo;

  bench_print_mode (14, oper.name);

  obj.ops = oper.ops;
  obj.priv = &oper;

  result = do_slope_benchmark (&obj);
  bench_print_result_nsec_per_iteration (result);
}


static void
_ecc_bench (int algo)
{
  const char *algo_name;
  int i;

  /* Skip not allowed mechanisms */
  if (in_fips_mode && !ecc_algo_fips_allowed (algo))
    return;

  algo_name = ecc_algo_name (algo);

  bench_print_header_nsec_per_iteration (14, algo_name);

  for (i = 0; ecc_operations[i].name; i++)
    cipher_ecc_one (algo, &ecc_operations[i]);

  bench_print_footer (14);
}
#endif


void
ecc_bench (char **argv, int argc)
{
#if USE_ECC
  int i, algo;

  bench_print_section ("ecc", "ECC");

  if (argv && argc)
    {
      for (i = 0; i < argc; i++)
        {
          algo = ecc_map_name (argv[i]);
          if (algo >= 0)
            _ecc_bench (algo);
        }
    }
  else
    {
      for (i = 0; i < __MAX_ECC_ALGO; i++)
        _ecc_bench (i);
    }
#else
  (void)argv;
  (void)argc;
#endif
}

/************************************************************ PQC benchmarks. */

enum bench_pq_algo
{
#if USE_KYBER
  PQ_ALGO_MLKEM512 = 0,
  PQ_ALGO_MLKEM768,
  PQ_ALGO_MLKEM1024,
#endif
  PQ_ALGO_SNTRUP761,
  PQ_ALGO_CM6688128F,
#if USE_DILITHIUM
  PQ_ALGO_MLDSA44,
  PQ_ALGO_MLDSA65,
  PQ_ALGO_MLDSA87,
#endif
  __MAX_PQ_ALGO
};

enum bench_pq_operation
{
  PQ_OPER_KEYGEN = 0,
  PQ_OPER_ENCAP,
  PQ_OPER_DECAP,
  PQ_OPER_SIGN,
  PQ_OPER_VERIFY,
  __MAX_PQ_OPER
};

struct bench_pq_oper
{
  enum bench_pq_operation oper;
  const char *name;
  struct bench_ops *ops;

  enum bench_pq_algo algo;
};

struct bench_pq_kem_hd
{
  int algo;
  size_t pubkey_len;
  size_t seckey_len;
  size_t ciph_len;
  size_t shared_len;
  unsigned char *pubkey;
  unsigned char *seckey;
  unsigned char *ciph;
  unsigned char *shared;
};

struct bench_pq_sig_hd
{
  gcry_sexp_t key_spec;
  gcry_sexp_t data;
  gcry_sexp_t pub_key;
  gcry_sexp_t sec_key;
  gcry_sexp_t sig;
};

/* KEM algorithms use 'kem_algo', signature algorithms use 'sig_name'. */
static const struct
{
  const char *name;
  int kem_algo;
  const char *sig_name;
  size_t pubkey_len;
  size_t seckey_len;
  size_t ciph_len;
  size_t shared_len;
  int keygen_cost;             /* Scales down key generation repetitions. */
  int oper_cost;               /* Scales down other operation repetitions. */
  int keygen_on_demand;        /* Run key generation only when named. */
} pq_algos[] = {
#if USE_KYBER
  { "ML-KEM-512", GCRY_KEM_MLKEM512, NULL,
    GCRY_KEM_MLKEM512_PUBKEY_LEN, GCRY_KEM_MLKEM512_SECKEY_LEN,
    GCRY_KEM_MLKEM512_ENCAPS_LEN, GCRY_KEM_MLKEM512_SHARED_LEN, 1, 1, 0 },
  { "ML-KEM-768", GCRY_KEM_MLKEM768, NULL,
    GCRY_KEM_MLKEM768_PUBKEY_LEN, GCRY_KEM_MLKEM768_SECKEY_LEN,
    GCRY_KEM_MLKEM768_ENCAPS_LEN, GCRY_KEM_MLKEM768_SHARED_LEN, 1, 1, 0 },
  { "ML-KEM-1024", GCRY_KEM_MLKEM1024, NULL,
    GCRY_KEM_MLKEM1024_PUBKEY_LEN, GCRY_KEM_MLKEM1024_SECKEY_LEN,
    GCRY_KEM_MLKEM1024_ENCAPS_LEN, GCRY_KEM_MLKEM1024_SHARED_LEN, 1, 1, 0 },
#endif
  { "sntrup761", GCRY_KEM_SNTRUP761, NULL,
    GCRY_KEM_SNTRUP761_PUBKEY_LEN, GCRY_KEM_SNTRUP761_SECKEY_LEN,
    GCRY_KEM_SNTRUP761_ENCAPS_LEN, GCRY_KEM_SNTRUP761_SHARED_LEN, 1, 1, 0 },
  { "cm6688128f", GCRY_KEM_CM6688128F, NULL,
    GCRY_KEM_CM6688128F_PUBKEY_LEN, GCRY_KEM_CM6688128F_SECKEY_LEN,
    GCRY_KEM_CM6688128F_ENCAPS_LEN, GCRY_KEM_CM6688128F_SHARED_LEN, 16, 1, 1 },
#if USE_DILITHIUM
  { "ML-DSA-44", -1, "dilithium2", 0, 0, 0, 0, 1, 1, 0 },
  { "ML-DSA-65", -1, "dilithium3", 0, 0, 0, 0, 1, 1, 0 },
  { "ML-DSA-87", -1, "dilithium5", 0, 0, 0, 0, 1, 1, 0 },
#endif
  { NULL, -1, NULL, 0, 0, 0, 0, 1, 1, 0 }
};

#define PQ_SIG_SEED_LEN 32
#define PQ_SIG_MSG_LEN 32


static const char *
pq_algo_name (int algo)
{
  if (algo < 0 || algo >= __MAX_PQ_ALGO)
    return NULL;

  return pq_algos[algo].name;
}

static int
pq_algo_is_kem (int algo)
{
  return pq_algos[algo].kem_algo >= 0;
}

static int
pq_map_name (const char *name)
{
  int i;

  for (i = 0; i < __MAX_PQ_ALGO; i++)
    {
      if (strcmp (pq_algo_name (i), name) == 0)
	return i;
    }

  return -1;
}

static void
pq_setup_obj (struct bench_obj *obj)
{
  struct bench_pq_oper *oper = obj->priv;
  int cost;

  if (oper->oper == PQ_OPER_KEYGEN)
    cost = pq_algos[oper->algo].keygen_cost;
  else
    cost = pq_algos[oper->algo].oper_cost;

  obj->min_bufsize = 1;
  obj->max_bufsize = 4;
  obj->step_size = 1;
  obj->num_measure_repetitions =
    num_measurement_repetitions / obj->max_bufsize / cost;

  while (obj->num_measure_repetitions == 0)
    {
      if (obj->max_bufsize == 2)
	{
	  obj->num_measure_repetitions = 1;
	}
      else
	{
	  obj->max_bufsize--;
	  obj->num_measure_repetitions =
	    num_measurement_repetitions / obj->max_bufsize / cost;
	}
    }
}


static void
bench_pq_kem_keypair (struct bench_pq_kem_hd *hd)
{
  gpg_error_t err;

  err = gcry_kem_keypair (hd->algo, hd->pubkey, hd->pubkey_len,
			  hd->seckey, hd->seckey_len);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_kem_keypair failed: %s\n",
	       gpg_strerror (err));
      exit (1);
    }
}

static int
bench_pq_kem_init (struct bench_obj *obj)
{
  struct bench_pq_oper *oper = obj->priv;
  struct bench_pq_kem_hd *hd;

  pq_setup_obj (obj);

  hd = calloc (1, sizeof(*hd));
  if (!hd)
    return -1;

  hd->algo = pq_algos[oper->algo].kem_algo;
  hd->pubkey_len = pq_algos[oper->algo].pubkey_len;
  hd->seckey_len = pq_algos[oper->algo].seckey_len;
  hd->ciph_len = pq_algos[oper->algo].ciph_len;
  hd->shared_len = pq_algos[oper->algo].shared_len;

  hd->pubkey = calloc (1, hd->pubkey_len);
  hd->seckey = calloc (1, hd->seckey_len);
  hd->ciph = calloc (1, hd->ciph_len);
  hd->shared = calloc (1, hd->shared_len);
  if (!hd->pubkey || !hd->seckey || !hd->ciph || !hd->shared)
    {
      free (hd->shared);
      free (hd->ciph);
      free (hd->seckey);
      free (hd->pubkey);
      free (hd);
      return -1;
    }

  obj->hd = hd;
  bench_pq_kem_keypair (hd);
  return 0;
}

static void
bench_pq_kem_free (struct bench_obj *obj)
{
  struct bench_pq_kem_hd *hd = obj->hd;

  free (hd->shared);
  free (hd->ciph);
  free (hd->seckey);
  free (hd->pubkey);
  free (hd);
  obj->hd = NULL;
}

static void
bench_pq_kem_encapsulate (struct bench_pq_kem_hd *hd)
{
  gpg_error_t err;

  err = gcry_kem_encap (hd->algo, hd->pubkey, hd->pubkey_len,
			hd->ciph, hd->ciph_len, hd->shared, hd->shared_len,
			NULL, 0);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_kem_encap failed: %s\n",
	       gpg_strerror (err));
      exit (1);
    }
}

static void
bench_pq_kem_keygen_do_bench (struct bench_obj *obj, void *buf,
			      size_t num_iter)
{
  struct bench_pq_kem_hd *hd = obj->hd;
  size_t i;

  (void)buf;

  for (i = 0; i < num_iter; i++)
    bench_pq_kem_keypair (hd);
}

static void
bench_pq_kem_encap_do_bench (struct bench_obj *obj, void *buf, size_t num_iter)
{
  struct bench_pq_kem_hd *hd = obj->hd;
  size_t i;

  (void)buf;

  for (i = 0; i < num_iter; i++)
    bench_pq_kem_encapsulate (hd);
}

static void
bench_pq_kem_decap_do_bench (struct bench_obj *obj, void *buf, size_t num_iter)
{
  struct bench_pq_kem_hd *hd = obj->hd;
  gpg_error_t err;
  size_t i;

  (void)buf;

  bench_pq_kem_encapsulate (hd);

  for (i = 0; i < num_iter; i++)
    {
      err = gcry_kem_decap (hd->algo, hd->seckey, hd->seckey_len,
			    hd->ciph, hd->ciph_len, hd->shared, hd->shared_len,
			    NULL, 0);
      if (err)
	{
	  fprintf (stderr, PGM ": gcry_kem_decap failed: %s\n",
		   gpg_strerror (err));
	  exit (1);
	}
    }
}


static int
bench_pq_sig_init (struct bench_obj *obj)
{
  struct bench_pq_oper *oper = obj->priv;
  struct bench_pq_sig_hd *hd;
  unsigned char seed[PQ_SIG_SEED_LEN];
  unsigned char msg[PQ_SIG_MSG_LEN];
  gpg_error_t err;

  pq_setup_obj (obj);

  hd = calloc (1, sizeof(*hd));
  if (!hd)
    return -1;

  gcry_randomize (seed, sizeof(seed), GCRY_WEAK_RANDOM);
  gcry_randomize (msg, sizeof(msg), GCRY_WEAK_RANDOM);

  err = gcry_sexp_build (&hd->key_spec, NULL, "(genkey(%s(S%b)))",
			 pq_algos[oper->algo].sig_name,
			 (int)sizeof(seed), seed, NULL);
  if (!err)
    err = gcry_sexp_build (&hd->data, NULL,
			   "(data(raw)(flags no-prefix)(value%b))",
			   (int)sizeof(msg), msg, NULL);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_sexp_build failed: %s\n",
	       gpg_strerror (err));
      exit (1);
    }

  obj->hd = hd;
  return 0;
}

static void
bench_pq_sig_free (struct bench_obj *obj)
{
  struct bench_pq_sig_hd *hd = obj->hd;

  gcry_sexp_release (hd->sig);
  gcry_sexp_release (hd->pub_key);
  gcry_sexp_release (hd->sec_key);
  gcry_sexp_release (hd->data);
  gcry_sexp_release (hd->key_spec);
  free (hd);
  obj->hd = NULL;
}

static void
bench_pq_sig_keygen (struct bench_pq_sig_hd *hd)
{
  gcry_sexp_t key_pair;
  gpg_error_t err;

  err = gcry_pk_genkey (&key_pair, hd->key_spec);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_pk_genkey failed: %s\n",
	       gpg_strerror (err));
      exit (1);
    }

  hd->pub_key = gcry_sexp_find_token (key_pair, "public-key", 0);
  if (!hd->pub_key)
    {
      fprintf (stderr, PGM ": public part missing in key\n");
      exit (1);
    }
  hd->sec_key = gcry_sexp_find_token (key_pair, "private-key", 0);
  if (!hd->sec_key)
    {
      fprintf (stderr, PGM ": private part missing in key\n");
      exit (1);
    }

  gcry_sexp_release (key_pair);
}

static void
bench_pq_sig_keygen_do_bench (struct bench_obj *obj, void *buf,
			      size_t num_iter)
{
  struct bench_pq_sig_hd *hd = obj->hd;
  size_t i;

  (void)buf;

  for (i = 0; i < num_iter; i++)
    {
      bench_pq_sig_keygen (hd);
      gcry_sexp_release (hd->pub_key);
      gcry_sexp_release (hd->sec_key);
    }

  hd->pub_key = NULL;
  hd->sec_key = NULL;
}

static void
bench_pq_sig_sign_do_bench (struct bench_obj *obj, void *buf, size_t num_iter)
{
  struct bench_pq_sig_hd *hd = obj->hd;
  gpg_error_t err;
  size_t i;

  (void)buf;

  bench_pq_sig_keygen (hd);

  for (i = 0; i < num_iter; i++)
    {
      err = gcry_pk_sign (&hd->sig, hd->data, hd->sec_key);
      if (err)
	{
	  fprintf (stderr, PGM ": gcry_pk_sign failed: %s\n",
		   gpg_strerror (err));
	  exit (1);
	}
      gcry_sexp_release (hd->sig);
    }

  gcry_sexp_release (hd->pub_key);
  gcry_sexp_release (hd->sec_key);
  hd->sig = NULL;
  hd->pub_key = NULL;
  hd->sec_key = NULL;
}

static void
bench_pq_sig_verify_do_bench (struct bench_obj *obj, void *buf,
			      size_t num_iter)
{
  struct bench_pq_sig_hd *hd = obj->hd;
  gpg_error_t err;
  size_t i;

  (void)buf;

  bench_pq_sig_keygen (hd);
  err = gcry_pk_sign (&hd->sig, hd->data, hd->sec_key);
  if (err)
    {
      fprintf (stderr, PGM ": gcry_pk_sign failed: %s\n",
	       gpg_strerror (err));
      exit (1);
    }

  for (i = 0; i < num_iter; i++)
    {
      err = gcry_pk_verify (hd->sig, hd->data, hd->pub_key);
      if (err)
	{
	  fprintf (stderr, PGM ": gcry_pk_verify failed: %s\n",
		   gpg_strerror (err));
	  exit (1);
	}
    }

  gcry_sexp_release (hd->sig);
  gcry_sexp_release (hd->pub_key);
  gcry_sexp_release (hd->sec_key);
  hd->sig = NULL;
  hd->pub_key = NULL;
  hd->sec_key = NULL;
}


static struct bench_ops pq_kem_keygen_ops = {
  &bench_pq_kem_init,
  &bench_pq_kem_free,
  &bench_pq_kem_keygen_do_bench
};

static struct bench_ops pq_kem_encap_ops = {
  &bench_pq_kem_init,
  &bench_pq_kem_free,
  &bench_pq_kem_encap_do_bench
};

static struct bench_ops pq_kem_decap_ops = {
  &bench_pq_kem_init,
  &bench_pq_kem_free,
  &bench_pq_kem_decap_do_bench
};

static struct bench_ops pq_sig_keygen_ops = {
  &bench_pq_sig_init,
  &bench_pq_sig_free,
  &bench_pq_sig_keygen_do_bench
};

static struct bench_ops pq_sig_sign_ops = {
  &bench_pq_sig_init,
  &bench_pq_sig_free,
  &bench_pq_sig_sign_do_bench
};

static struct bench_ops pq_sig_verify_ops = {
  &bench_pq_sig_init,
  &bench_pq_sig_free,
  &bench_pq_sig_verify_do_bench
};


static struct bench_pq_oper pq_kem_operations[] = {
  { PQ_OPER_KEYGEN, "keygen", &pq_kem_keygen_ops },
  { PQ_OPER_ENCAP,  "encap",  &pq_kem_encap_ops },
  { PQ_OPER_DECAP,  "decap",  &pq_kem_decap_ops },
  { 0, NULL, NULL }
};

static struct bench_pq_oper pq_sig_operations[] = {
  { PQ_OPER_KEYGEN, "keygen", &pq_sig_keygen_ops },
  { PQ_OPER_SIGN,   "sign",   &pq_sig_sign_ops },
  { PQ_OPER_VERIFY, "verify", &pq_sig_verify_ops },
  { 0, NULL, NULL }
};


static void
cipher_pq_one (enum bench_pq_algo algo, struct bench_pq_oper *poper)
{
  struct bench_pq_oper oper = *poper;
  struct bench_obj obj = { 0 };
  double result;

  oper.algo = algo;

  bench_print_mode (14, oper.name);

  obj.ops = oper.ops;
  obj.priv = &oper;

  result = do_slope_benchmark (&obj);
  bench_print_result_nsec_per_iteration (result);
}


static void
_pq_bench (int algo, int named)
{
  struct bench_pq_oper *operations;
  int i;

  bench_print_header_nsec_per_iteration (14, pq_algo_name (algo));

  operations = pq_algo_is_kem (algo) ? pq_kem_operations : pq_sig_operations;

  for (i = 0; operations[i].name; i++)
    {
      if (!named && !include_slow && operations[i].oper == PQ_OPER_KEYGEN
	  && pq_algos[algo].keygen_on_demand)
	{
	  if (!csv_mode)
	    {
	      bench_print_mode (14, operations[i].name);
	      bench_print_result_skipped ();
	    }
	  continue;
	}
      cipher_pq_one (algo, &operations[i]);
    }

  bench_print_footer (14);
}


void
pq_bench (char **argv, int argc)
{
  int i, algo;

  bench_print_section ("pq", "Post-quantum");

  if (argv && argc)
    {
      for (i = 0; i < argc; i++)
	{
	  algo = pq_map_name (argv[i]);
	  if (algo >= 0)
	    _pq_bench (algo, 1);
	}
    }
  else
    {
      for (i = 0; i < __MAX_PQ_ALGO; i++)
	_pq_bench (i, 0);
    }
}

/************************************************************ MPI benchmarks. */

#define MPI_START_SIZE 64
#define MPI_END_SIZE 1024
#define MPI_STEP_SIZE 8
#define MPI_NUM_STEPS (((MPI_END_SIZE - MPI_START_SIZE) / MPI_STEP_SIZE) + 1)

enum bench_mpi_test
{
  MPI_TEST_ADD = 0,
  MPI_TEST_SUB,
  MPI_TEST_RSHIFT3,
  MPI_TEST_LSHIFT3,
  MPI_TEST_RSHIFT65,
  MPI_TEST_LSHIFT65,
  MPI_TEST_MUL4,
  MPI_TEST_MUL8,
  MPI_TEST_MUL16,
  MPI_TEST_MUL32,
  MPI_TEST_DIV4,
  MPI_TEST_DIV8,
  MPI_TEST_DIV16,
  MPI_TEST_DIV32,
  MPI_TEST_MOD4,
  MPI_TEST_MOD8,
  MPI_TEST_MOD16,
  MPI_TEST_MOD32,
  __MAX_MPI_TEST
};

static const char * const mpi_test_names[] =
{
  "add",
  "sub",
  "rshift3",
  "lshift3",
  "rshift65",
  "lshift65",
  "mul4",
  "mul8",
  "mul16",
  "mul32",
  "div4",
  "div8",
  "div16",
  "div32",
  "mod4",
  "mod8",
  "mod16",
  "mod32",
  NULL,
};

struct bench_mpi_mode
{
  const char *name;
  struct bench_ops *ops;

  enum bench_mpi_test test_id;
};

struct bench_mpi_hd
{
  gcry_mpi_t bytes[MPI_NUM_STEPS + 1];
  gcry_mpi_t y;
};

static int
bench_mpi_init (struct bench_obj *obj)
{
  struct bench_mpi_mode *mode = obj->priv;
  struct bench_mpi_hd *hd;
  int y_bytes;
  int i, j;

  (void)mode;

  obj->min_bufsize = MPI_START_SIZE;
  obj->max_bufsize = MPI_END_SIZE;
  obj->step_size = MPI_STEP_SIZE;
  obj->num_measure_repetitions = num_measurement_repetitions;

  hd = calloc (1, sizeof(*hd));
  if (!hd)
    return -1;

  /* Generate input MPIs for benchmark. */
  for (i = MPI_START_SIZE, j = 0; j < DIM(hd->bytes); i += MPI_STEP_SIZE, j++)
    {
      hd->bytes[j] = gcry_mpi_new (i * 8);
      gcry_mpi_randomize (hd->bytes[j], i * 8, GCRY_WEAK_RANDOM);
      gcry_mpi_set_bit (hd->bytes[j], i * 8 - 1);
    }

  switch (mode->test_id)
    {
      case MPI_TEST_MUL4:
      case MPI_TEST_DIV4:
      case MPI_TEST_MOD4:
	y_bytes = 4;
	break;

      case MPI_TEST_MUL8:
      case MPI_TEST_DIV8:
      case MPI_TEST_MOD8:
	y_bytes = 8;
	break;

      case MPI_TEST_MUL16:
      case MPI_TEST_DIV16:
      case MPI_TEST_MOD16:
	y_bytes = 16;
	break;

      case MPI_TEST_MUL32:
      case MPI_TEST_DIV32:
      case MPI_TEST_MOD32:
	y_bytes = 32;
	break;

      default:
	y_bytes = 0;
	break;
    }

  hd->y = gcry_mpi_new (y_bytes * 8);
  if (y_bytes)
    {
      gcry_mpi_randomize (hd->y, y_bytes * 8, GCRY_WEAK_RANDOM);
      gcry_mpi_set_bit (hd->y, y_bytes * 8 - 1);
    }

  obj->hd = hd;
  return 0;
}

static void
bench_mpi_free (struct bench_obj *obj)
{
  struct bench_mpi_hd *hd = obj->hd;
  int i;

  gcry_mpi_release (hd->y);
  for (i = DIM(hd->bytes) - 1; i >= 0; i--)
    gcry_mpi_release (hd->bytes[i]);

  free(hd);
}

static void
bench_mpi_do_bench (struct bench_obj *obj, void *buf, size_t buflen)
{
  struct bench_mpi_hd *hd = obj->hd;
  struct bench_mpi_mode *mode = obj->priv;
  int bytes_idx = (buflen - MPI_START_SIZE) / MPI_STEP_SIZE;
  gcry_mpi_t x;

  (void)buf;

  x = gcry_mpi_new (2 * (MPI_END_SIZE + 1) * 8);

  switch (mode->test_id)
    {
      case MPI_TEST_ADD:
	gcry_mpi_add (x, hd->bytes[bytes_idx], hd->bytes[bytes_idx]);
	break;

      case MPI_TEST_SUB:
	gcry_mpi_sub (x, hd->bytes[bytes_idx + 1], hd->bytes[bytes_idx]);
	break;

      case MPI_TEST_RSHIFT3:
	gcry_mpi_rshift (x, hd->bytes[bytes_idx], 3);
	break;

      case MPI_TEST_LSHIFT3:
	gcry_mpi_lshift (x, hd->bytes[bytes_idx], 3);
	break;

      case MPI_TEST_RSHIFT65:
	gcry_mpi_rshift (x, hd->bytes[bytes_idx], 65);
	break;

      case MPI_TEST_LSHIFT65:
	gcry_mpi_lshift (x, hd->bytes[bytes_idx], 65);
	break;

      case MPI_TEST_MUL4:
      case MPI_TEST_MUL8:
      case MPI_TEST_MUL16:
      case MPI_TEST_MUL32:
	gcry_mpi_mul (x, hd->bytes[bytes_idx], hd->y);
	break;

      case MPI_TEST_DIV4:
      case MPI_TEST_DIV8:
      case MPI_TEST_DIV16:
      case MPI_TEST_DIV32:
	gcry_mpi_div (x, NULL, hd->bytes[bytes_idx], hd->y, 0);
	break;

      case MPI_TEST_MOD4:
      case MPI_TEST_MOD8:
      case MPI_TEST_MOD16:
      case MPI_TEST_MOD32:
	gcry_mpi_mod (x, hd->bytes[bytes_idx], hd->y);
	break;

      default:
	break;
    }

  gcry_mpi_release (x);
}

static struct bench_ops mpi_ops = {
  &bench_mpi_init,
  &bench_mpi_free,
  &bench_mpi_do_bench
};


static struct bench_mpi_mode mpi_modes[] = {
  {"", &mpi_ops},
  {0},
};


static void
mpi_bench_one (int test_id, struct bench_mpi_mode *pmode)
{
  struct bench_mpi_mode mode = *pmode;
  struct bench_obj obj = { 0 };
  double result;

  mode.test_id = test_id;

  if (mode.name[0] == '\0')
    bench_print_algo (-18, mpi_test_names[test_id]);
  else
    bench_print_algo (18, mode.name);

  obj.ops = mode.ops;
  obj.priv = &mode;

  result = do_slope_benchmark (&obj);

  bench_print_result (result);
}

static void
_mpi_bench (int test_id)
{
  int i;

  for (i = 0; mpi_modes[i].name; i++)
    mpi_bench_one (test_id, &mpi_modes[i]);
}

static int
mpi_match_test(const char *name)
{
  int i;

  for (i = 0; i < __MAX_MPI_TEST; i++)
    if (strcmp(name, mpi_test_names[i]) == 0)
      return i;

  return -1;
}

void
mpi_bench (char **argv, int argc)
{
  int i, test_id;

  bench_print_section ("mpi", "MPI");
  bench_print_header (18, "");

  if (argv && argc)
    {
      for (i = 0; i < argc; i++)
	{
	  test_id = mpi_match_test (argv[i]);
	  if (test_id >= 0)
	    _mpi_bench (test_id);
	}
    }
  else
    {
      for (i = 0; i < __MAX_MPI_TEST; i++)
	_mpi_bench (i);
    }

  bench_print_footer (18);
}

/************************************************************** Main program. */

void
print_help (void)
{
  static const char *help_lines[] = {
    "usage: bench-slope [options] [hash|mac|cipher|kdf|pk|ecc|pq|mpi",
    "                              [algonames]]",
    "",
    " options:",
    "   --cpu-mhz <mhz>           Set CPU speed for calculating cycles",
    "                             per bytes results.  Set as \"auto\"",
    "                             for auto-detection of CPU speed.",
    "   --disable-hwf <features>  Disable hardware acceleration feature(s)",
    "                             for benchmarking.",
    "   --repetitions <n>         Use N repetitions (default "
                                     STR2(NUM_MEASUREMENT_REPETITIONS) ")",
    "   --unaligned               Use unaligned input buffers.",
    "   --no-quick-rng            Use default random number generation",
    "   --include-slow            Include slow benchmarks in default run",
    "   --csv                     Use CSV output format",
    "",
    " notes:",
    "   Slow post-quantum key generation is benchmarked only when algorithm",
    "   is given by name or with '--include-slow'.",
    NULL
  };
  const char **line;

  for (line = help_lines; *line; line++)
    fprintf (stdout, "%s\n", *line);
}


/* Warm up CPU.  */
static void
warm_up_cpu (void)
{
  struct nsec_time start, end;

  if (in_regression_test)
    return;

  get_nsec_time (&start);
  do
    {
      get_nsec_time (&end);
    }
  while (get_time_nsec_diff (&start, &end) < 1000.0 * 1000.0 * 1000.0);
}


int
main (int argc, char **argv)
{
  int last_argc = -1;
  int no_quick_rng = 0;
  char tmp[4];

  if (argc)
    {
      argc--;
      argv++;
    }

  /* We skip this test if we are running under the test suite (no args
     and srcdir defined) and GCRYPT_NO_BENCHMARKS is set.  */
  if (!argc && getenv ("srcdir") && getenv ("GCRYPT_NO_BENCHMARKS"))
    exit (77);

  if (getenv ("GCRYPT_IN_REGRESSION_TEST"))
    {
      in_regression_test = 1;
      num_measurement_repetitions = 2;
    }
  else
    num_measurement_repetitions = NUM_MEASUREMENT_REPETITIONS;

  while (argc && last_argc != argc)
    {
      last_argc = argc;

      if (!strcmp (*argv, "--"))
	{
	  argc--;
	  argv++;
	  break;
	}
      else if (!strcmp (*argv, "--help"))
	{
	  print_help ();
	  exit (0);
	}
      else if (!strcmp (*argv, "--verbose"))
	{
	  verbose++;
	  argc--;
	  argv++;
	}
      else if (!strcmp (*argv, "--debug"))
	{
	  verbose += 2;
	  debug++;
	  argc--;
	  argv++;
	}
      else if (!strcmp (*argv, "--csv"))
	{
	  csv_mode = 1;
	  argc--;
	  argv++;
	}
      else if (!strcmp (*argv, "--no-quick-rng"))
	{
	  no_quick_rng = 1;
	  argc--;
	  argv++;
	}
      else if (!strcmp (*argv, "--include-slow"))
	{
	  include_slow = 1;
	  argc--;
	  argv++;
	}
      else if (!strcmp (*argv, "--unaligned"))
	{
	  unaligned_mode = 1;
	  argc--;
	  argv++;
	}
      else if (!strcmp (*argv, "--disable-hwf"))
	{
	  argc--;
	  argv++;
	  if (argc)
	    {
	      if (gcry_control (GCRYCTL_DISABLE_HWF, *argv, NULL))
		fprintf (stderr,
			 PGM
			 ": unknown hardware feature `%s' - option ignored\n",
			 *argv);
	      argc--;
	      argv++;
	    }
	}
      else if (!strcmp (*argv, "--cpu-mhz"))
	{
	  argc--;
	  argv++;
	  if (argc)
	    {
              if (!strcmp (*argv, "auto"))
                {
                  auto_ghz = 1;
                }
              else
                {
                  cpu_ghz = atof (*argv);
                  cpu_ghz /= 1000;	/* Mhz => Ghz */
                }

	      argc--;
	      argv++;
	    }
	}
      else if (!strcmp (*argv, "--repetitions"))
	{
	  argc--;
	  argv++;
	  if (argc)
	    {
	      num_measurement_repetitions = atof (*argv);
              if (num_measurement_repetitions < 2)
                {
                  fprintf (stderr,
                           PGM
                           ": value for --repetitions too small - using %d\n",
                           NUM_MEASUREMENT_REPETITIONS);
                  num_measurement_repetitions = NUM_MEASUREMENT_REPETITIONS;
                }
	      argc--;
	      argv++;
	    }
	}
    }

  xgcry_control ((GCRYCTL_SET_VERBOSITY, (int) verbose));

  if (!gcry_check_version (GCRYPT_VERSION))
    {
      fprintf (stderr, PGM ": version mismatch; pgm=%s, library=%s\n",
	       GCRYPT_VERSION, gcry_check_version (NULL));
      exit (1);
    }

  if (debug)
    xgcry_control ((GCRYCTL_SET_DEBUG_FLAGS, 1u, 0));

  xgcry_control ((GCRYCTL_DISABLE_SECMEM, 0));
  xgcry_control ((GCRYCTL_INITIALIZATION_FINISHED, 0));

  if (!no_quick_rng)
    xgcry_control ((GCRYCTL_ENABLE_QUICK_RANDOM, 0));

  /* Fill random pool so that first measurement is not different. */
  gcry_randomize (tmp, sizeof(tmp), GCRY_STRONG_RANDOM);

  if (gcry_fips_mode_active ())
    in_fips_mode = 1;

  if (in_regression_test)
    fputs ("Note: " PGM " running in quick regression test mode.\n", stdout);

  if (!argc)
    {
      warm_up_cpu ();
      hash_bench (NULL, 0);
      mac_bench (NULL, 0);
      cipher_bench (NULL, 0);
      kdf_bench (NULL, 0);
      pk_bench (NULL, 0);
      ecc_bench (NULL, 0);
      pq_bench (NULL, 0);
      mpi_bench (NULL, 0);
    }
  else if (!strcmp (*argv, "hash"))
    {
      argc--;
      argv++;

      warm_up_cpu ();
      hash_bench ((argc == 0) ? NULL : argv, argc);
    }
  else if (!strcmp (*argv, "mac"))
    {
      argc--;
      argv++;

      warm_up_cpu ();
      mac_bench ((argc == 0) ? NULL : argv, argc);
    }
  else if (!strcmp (*argv, "cipher"))
    {
      argc--;
      argv++;

      warm_up_cpu ();
      cipher_bench ((argc == 0) ? NULL : argv, argc);
    }
  else if (!strcmp (*argv, "kdf"))
    {
      argc--;
      argv++;

      warm_up_cpu ();
      kdf_bench ((argc == 0) ? NULL : argv, argc);
    }
  else if (!strcmp (*argv, "pk"))
    {
      argc--;
      argv++;

      warm_up_cpu ();
      pk_bench ((argc == 0) ? NULL : argv, argc);
    }
  else if (!strcmp (*argv, "ecc"))
    {
      argc--;
      argv++;

      warm_up_cpu ();
      ecc_bench ((argc == 0) ? NULL : argv, argc);
    }
  else if (!strcmp (*argv, "pq"))
    {
      argc--;
      argv++;

      warm_up_cpu ();
      pq_bench ((argc == 0) ? NULL : argv, argc);
    }
  else if (!strcmp (*argv, "mpi"))
    {
      argc--;
      argv++;

      warm_up_cpu ();
      mpi_bench ((argc == 0) ? NULL : argv, argc);
    }
  else
    {
      fprintf (stderr, PGM ": unknown argument: %s\n", *argv);
      print_help ();
    }

  return 0;
}

#endif /* !NO_GET_NSEC_TIME */
