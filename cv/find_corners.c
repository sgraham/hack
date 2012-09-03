#include "find_corners.h"

#include "graymap.h"

#include <math.h>
#include <stdlib.h>

static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }

static bool intersect(float point[2], float l1[2], float l2[2]) {
  float nx1 = cos(l1[0]);
  float ny1 = sin(l1[0]);

  float nx2 = cos(l2[0]);
  float ny2 = sin(l2[0]);

  float d = nx1 * ny2 - nx2 * ny1;
  if (fabs(d) < 0.0001f)
    return false;

  // fabs() due to mod 180 in line orientation.
  point[0] = fabs((ny2 * l1[1] - ny1 * l2[1]) / d);
  point[1] = fabs((nx1 * l2[1] - nx2 * l1[1]) / d);

  return true;
}

bool find_corners(graymap_t* graymap, float corners[4][2]) {
  const int kNumAngles = 360;
  const int kNumRadii = graymap->h;

  float kSin[kNumAngles];
  float kCos[kNumAngles];
  for (int a = 0; a < kNumAngles; ++a) {
    float rho = a * M_PI / kNumAngles;
    kSin[a] = sin(rho);
    kCos[a] = cos(rho);
  }

  const float kMaxRadius = sqrt(graymap->w*graymap->w + graymap->h*graymap->h);
  unsigned* houghmap = calloc(kNumRadii * kNumAngles, sizeof(unsigned));
  for (int y = 0; y < graymap->h; ++y) {
    for (int x = 0; x < graymap->w; ++x) {
      if (graymap->data[y*graymap->w + x] == 255) continue;

      for (int a = 0; a < kNumAngles; ++a) {
        float nx = kCos[a];
        float ny = kSin[a];
        float r = fabs(nx*x + ny*y);
        unsigned ri = r * (kNumRadii - 1) / kMaxRadius;
        houghmap[ri*kNumAngles + a]++;
      }
    }
  }

  float lines[4][2];
  bool line_set[2] = {};

  unsigned maxhough = 1;
  for (int i = 0; i < kNumRadii*kNumAngles; ++i)
    if (houghmap[i] > maxhough)
      maxhough = houghmap[i];

#if 0
  graymap_t* alloc_graymap(int w, int h);
  void free_graymap(graymap_t*);
  bool save_graymap_to_pgm(const char* filename, const graymap_t*);

  graymap_t* grayhough = alloc_graymap(kNumAngles, kNumRadii);
  for (int ri = 0, i = 0; ri < kNumRadii; ++ri)
    for (int ai = 0; ai < kNumAngles; ++ai, ++i)
      grayhough->data[i] = houghmap[i] * 255 / maxhough;
  save_graymap_to_pgm("hough.pgm", grayhough);
  free_graymap(grayhough);
#endif

  for (int ri = 0, i = 0; ri < kNumRadii; ++ri) {
    for (int ai = 0; ai < kNumAngles; ++ai, ++i) {
      if (houghmap[i] > 75 * maxhough / 100) {
        // Candidate! Find max in neighborhood, zero out remainder.
        const int k = 31;
        unsigned best = houghmap[i], besta = ai, bestr = ri;
        for (int rd = imax(ri - k/2, 0);
             rd <= imin(ri + k/2, kNumRadii - 1);
             ++rd) {
          for (int ad = ai - k/2; ad <= ai + k/2; ++ad) {
            int aii = ad < 0 ? ad + kNumAngles : ad;
            if (houghmap[rd*kNumAngles + aii] > best) {
              best = houghmap[rd*kNumAngles + aii];
              besta = aii;
              bestr = rd;
            }
            houghmap[rd*kNumAngles + aii] = 0;
          }
        }

        // XXX: could use kCos / kSin
        float deg = besta * M_PI / kNumAngles;
        float radius = bestr * kMaxRadius / (kNumRadii - 1);

        if (!line_set[0]) {
          for (int j = 0; j < 2; ++j) {
            lines[j][0] = deg; 
            lines[j][1] = radius; 
          }
          line_set[0] = true;
        } else {
          if (fabs(deg - lines[0][0]) < 10 * M_PI / 180) {
            if (radius > lines[1][1]) {
              lines[1][0] = deg;
              lines[1][1] = radius;
            }
          } else {
            if (!line_set[1]) {
              for (int j = 2; j < 4; ++j) {
                lines[j][0] = deg; 
                lines[j][1] = radius; 
              }
              line_set[1] = true;
            } else {
              if (radius > lines[3][1]) {
                lines[3][0] = deg;
                lines[3][1] = radius;
              }
            }
          }
        }
      }
    }
  }

#if 1
  int printf(const char*, ...);
  for (int i = 0; i < 4; ++i) {
    printf("angle %frad, radius %f\n", lines[i][0], lines[i][1]);
  }
#endif

  free(houghmap);

  return intersect(corners[0], lines[0], lines[2]) &&
         intersect(corners[1], lines[0], lines[3]) &&
         intersect(corners[2], lines[1], lines[2]) &&
         intersect(corners[3], lines[1], lines[3]);
}