/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 **you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include "bsp.h"

#define imgw 500
#define imgh 500

#if 1
uint8_t img[imgh*imgw] = {
#include "test9.include"
};
#define ix1 210
#define iy1 208
#define ix2 293
#define iy2 212
#define ix3 224
#define iy3 267
#define ix4 211
#define iy4 297
#define ix5 295
#define iy5 302
#else
uint8_t img[imgh*imgw] = {
    #include "test8.include"
};
#define ix1 210
#define iy1 208
#define ix2 308
#define iy2 213
#define ix3 254
#define iy3 264
#define ix4 212
#define iy4 296
#define ix5 304
#define iy5 309
#endif

uint8_t dst[128*128];

typedef struct {
    volatile double ax, ay, a1;
    volatile double bx, by, b1;
} trans_matrix_t;

trans_matrix_t cal_matrix(
    double x1, double y1, double x2, double y2, double x3, double y3,
    double u1, double v1, double u2, double v2, double u3, double v3
){
    trans_matrix_t ret;
    ret.ax = (u2*y1 - u3*y1 - u1*y2 + u3*y2 + u1*y3 - u2*y3)/(x2*y1 - x3*y1 - x1*y2 + x3*y2 + x1*y3 - x2*y3);
    ret.bx = (v2*y1 - v3*y1 - v1*y2 + v3*y2 + v1*y3 - v2*y3)/(x2*y1 - x3*y1 - x1*y2 + x3*y2 + x1*y3 - x2*y3);
    ret.ay = (u3*(-x1 + x2) + u2*(x1 - x3) + u1*(-x2 + x3))/(x3*(y1 - y2) + x1*(y2 - y3) + x2*(-y1 + y3));
    ret.by = (v3*(-x1 + x2) + v2*(x1 - x3) + v1*(-x2 + x3))/(x3*(y1 - y2) + x1*(y2 - y3) + x2*(-y1 + y3));
    ret.a1 = (u3*x2*y1 - u2*x3*y1 - u3*x1*y2 + u1*x3*y2 + u2*x1*y3 - u1*x2*y3)/(x2*y1 - x3*y1 - x1*y2 + x3*y2 + x1*y3 - x2*y3);
    ret.b1 = (v3*x2*y1 - v2*x3*y1 - v3*x1*y2 + v1*x3*y2 + v2*x1*y3 - v1*x2*y3)/(x2*y1 - x3*y1 - x1*y2 + x3*y2 + x1*y3 - x2*y3);
    return ret;
}

uint8_t get_pixel(double x, double y, uint8_t* src){
    if(y<0) return 0;
    if(y>=imgh) return 0;
    if(x<0) return 0;
    if(x>=imgw) return 0;
    uint32_t xi = x;
    uint32_t yi = y;
    return src[yi*imgw + xi];
}

void fill(uint8_t* dst, uint8_t* src, trans_matrix_t f1, trans_matrix_t f2, trans_matrix_t f3, trans_matrix_t f4){
    trans_matrix_t* pf;
    for(uint32_t iy=0; iy<128; iy++){
        for(uint32_t jx=0; jx<128; jx++){
            if(jx>iy){
                if((iy+jx)<128){
                    pf = &f1;
                }else{
                    pf = &f3;
                }
            }else{
                if((iy+jx)<128){
                    pf = &f2;
                }else{
                    pf = &f4;
                }
            }
            volatile double x = pf->ax * jx + pf->ay * iy + pf->a1;
            volatile double y = pf->bx * jx + pf->by * iy + pf->b1;
            dst[iy*128 + jx] = get_pixel(x, y, src);
        }
    }
}

void cal_and_fill(
    uint8_t* dst, uint8_t* src,
    double p1x, double p1y,
    double p2x, double p2y,
    double p3x, double p3y,
    double p4x, double p4y,
    double p5x, double p5y
){
    volatile double
    g1x=32, g1y=32,
    g2x=96, g2y=32,
    g3x=64, g3y=64,
    g4x=32, g4y=96,
    g5x=96, g5y=96;

    trans_matrix_t f1 = cal_matrix(g1x, g1y, g2x, g2y, g3x, g3y, p1x, p1y, p2x, p2y, p3x, p3y);
    trans_matrix_t f2 = cal_matrix(g1x, g1y, g3x, g3y, g4x, g4y, p1x, p1y, p3x, p3y, p4x, p4y);
    trans_matrix_t f3 = cal_matrix(g2x, g2y, g5x, g5y, g3x, g3y, p2x, p2y, p5x, p5y, p3x, p3y);
    trans_matrix_t f4 = cal_matrix(g3x, g3y, g5x, g5y, g4x, g4y, p3x, p3y, p5x, p5y, p4x, p4y);

    fill(dst, src, f1, f2, f3, f4);
}


int main()
{
    uint64_t start_t = read_csr(mcycle);
    cal_and_fill(
        dst, img,
        ix1, iy1,
        ix2, iy2,
        ix3, iy3,
        ix4, iy4,
        ix5, iy5
    );
    uint64_t end_t = read_csr(mcycle);

    for(uint32_t i=0; i<128*128; i++){
        if(i%64 == 0)printf("\n");
        printf("%d, ", dst[i]);
    }

    printf("\ncycles: %lu, time(ms): %lf\n", end_t-start_t, (end_t-start_t)/400000.0f);
    while(1);
}
