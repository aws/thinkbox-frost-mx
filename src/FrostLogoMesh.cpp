// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
// C++ Mesh File for 3ds Max, created by SaveMeshToCpp Macroscript
// Built from the object Text01

#include "stdafx.h"
#include <mesh.h>

#pragma warning( disable : 4305 )

// Anonymous namespace, to avoid name collisions
namespace {

// 216 vertices
float verts[] = {
    0.133367,    -0.222214,   0.000000,   0.140847,    -0.092204,   0.000000,   0.048027,    -0.093404,   0.000000,
    0.049897,    0.051916,    0.000000,   0.049747,    0.058926,    0.000000,   0.048997,    0.065366,    0.000000,
    0.047667,    0.071246,    0.000000,   0.045747,    0.076546,    0.000000,   0.043247,    0.081266,    0.000000,
    0.040167,    0.085406,    0.000000,   0.036497,    0.088956,    0.000000,   0.028347,    0.093956,    0.000000,
    0.019397,    0.096906,    0.000000,   0.010387,    0.098286,    0.000000,   0.002047,    0.098556,    0.000000,
    -0.004863,   0.098166,    0.000000,   -0.009603,   0.097576,    0.000000,   -0.011433,   0.097266,    0.000000,
    -0.026763,   0.091876,    0.000000,   -0.038383,   0.084416,    0.000000,   -0.046783,   0.075916,    0.000000,
    -0.052483,   0.067386,    0.000000,   -0.055963,   0.059856,    0.000000,   -0.057723,   0.054346,    0.000000,
    -0.058273,   0.051856,    0.000000,   -0.058343,   0.051456,    0.000000,   -0.068563,   0.002186,    0.000000,
    -0.045183,   0.002146,    0.000000,   -0.049333,   -0.046384,   0.000000,   -0.021693,   -0.048144,   0.000000,
    -0.021043,   -0.100524,   0.000000,   -0.133403,   -0.097664,   0.000000,   -0.125823,   -0.208814,   0.000000,
    -0.181023,   -0.281504,   0.000000,   -0.158593,   -0.298534,   0.000000,   -0.110673,   -0.235414,   0.000000,
    -0.014973,   -0.328174,   0.000000,   -0.086913,   -0.389564,   0.000000,   -0.068633,   -0.410984,   0.000000,
    -0.012893,   -0.363404,   0.000000,   -0.012983,   -0.386684,   0.000000,   -0.051523,   -0.424914,   0.000000,
    -0.031693,   -0.444904,   0.000000,   -0.013143,   -0.426504,   0.000000,   -0.013343,   -0.477154,   0.000000,
    0.014807,    -0.477264,   0.000000,   0.014997,    -0.428804,   0.000000,   0.031227,    -0.445154,   0.000000,
    0.051207,    -0.425324,   0.000000,   0.015157,    -0.388984,   0.000000,   0.015257,    -0.365404,   0.000000,
    0.068437,    -0.411534,   0.000000,   0.086877,    -0.390264,   0.000000,   0.015167,    -0.328054,   0.000000,
    0.106437,    -0.239594,   0.000000,   0.151187,    -0.298534,   0.000000,   0.173607,    -0.281504,   0.000000,
    0.129437,    -0.223334,   0.000000,   0.406017,    -0.223674,   0.000000,   0.362937,    -0.201464,   0.000000,
    0.384877,    -0.194454,   0.000000,   0.376307,    -0.167634,   0.000000,   0.327547,    -0.183224,   0.000000,
    0.306587,    -0.172414,   0.000000,   0.371847,    -0.146024,   0.000000,   0.361297,    -0.119924,   0.000000,
    0.275787,    -0.154504,   0.000000,   0.246167,    -0.036644,   0.000000,   0.322867,    -0.036644,   0.000000,
    0.322867,    -0.008494,   0.000000,   0.243347,    -0.008494,   0.000000,   0.274727,    0.116376,    0.000000,
    0.358907,    0.089486,    0.000000,   0.367477,    0.116296,    0.000000,   0.297667,    0.138596,    0.000000,
    0.317507,    0.150786,    0.000000,   0.370297,    0.138186,    0.000000,   0.376837,    0.165576,    0.000000,
    0.351427,    0.171636,    0.000000,   0.394577,    0.198166,    0.000000,   0.379837,    0.222146,    0.000000,
    0.338547,    0.196766,    0.000000,   0.343897,    0.219176,    0.000000,   0.316507,    0.225716,    0.000000,
    0.304627,    0.175916,    0.000000,   0.284527,    0.163566,    0.000000,   0.295707,    0.233076,    0.000000,
    0.267917,    0.237546,    0.000000,   0.254447,    0.153796,    0.000000,   0.128387,    0.189686,    0.000000,
    0.170837,    0.246296,    0.000000,   0.148317,    0.263186,    0.000000,   0.105707,    0.206366,    0.000000,
    0.027067,    0.282586,    0.000000,   0.088257,    0.335246,    0.000000,   0.069897,    0.356586,    0.000000,
    0.014347,    0.308786,    0.000000,   0.014347,    0.332066,    0.000000,   0.052727,    0.370446,    0.000000,
    0.032817,    0.390356,    0.000000,   0.014347,    0.371876,    0.000000,   0.014347,    0.422536,    0.000000,
    -0.013813,   0.422536,    0.000000,   -0.013813,   0.374076,    0.000000,   -0.030093,   0.390356,    0.000000,
    -0.050003,   0.370446,    0.000000,   -0.013813,   0.334256,    0.000000,   -0.013813,   0.310666,    0.000000,
    -0.067173,   0.356586,    0.000000,   -0.085533,   0.335246,    0.000000,   -0.025743,   0.283796,    0.000000,
    -0.109963,   0.202166,    0.000000,   -0.155733,   0.263186,    0.000000,   -0.178253,   0.246296,    0.000000,
    -0.134483,   0.187936,    0.000000,   -0.250313,   0.154956,    0.000000,   -0.264223,   0.234736,    0.000000,
    -0.291963,   0.229896,    0.000000,   -0.279373,   0.157706,    0.000000,   -0.299363,   0.169636,    0.000000,
    -0.312663,   0.222266,    0.000000,   -0.339963,   0.215366,    0.000000,   -0.333563,   0.190036,    0.000000,
    -0.377063,   0.215986,    0.000000,   -0.391483,   0.191806,    0.000000,   -0.349863,   0.166976,    0.000000,
    -0.372193,   0.161336,    0.000000,   -0.365293,   0.134036,    0.000000,   -0.315673,   0.146576,    0.000000,
    -0.295413,   0.134496,    0.000000,   -0.362183,   0.112196,    0.000000,   -0.353273,   0.085496,    0.000000,
    -0.273583,   0.112106,    0.000000,   -0.243273,   -0.008494,   0.000000,   -0.330283,   -0.008494,   0.000000,
    -0.330283,   -0.036644,   0.000000,   -0.246093,   -0.036644,   0.000000,   -0.274883,   -0.151214,   0.000000,
    -0.358013,   -0.118524,   0.000000,   -0.368313,   -0.144724,   0.000000,   -0.300113,   -0.171544,   0.000000,
    -0.320703,   -0.182414,   0.000000,   -0.372563,   -0.166384,   0.000000,   -0.380883,   -0.193284,   0.000000,
    -0.355913,   -0.200994,   0.000000,   -0.400713,   -0.224644,   0.000000,   -0.387573,   -0.249534,   0.000000,
    -0.344713,   -0.226914,   0.000000,   -0.351513,   -0.248924,   0.000000,   -0.324613,   -0.257234,   0.000000,
    -0.309503,   -0.208334,   0.000000,   -0.288643,   -0.197324,   0.000000,   -0.304343,   -0.265944,   0.000000,
    -0.276893,   -0.272224,   0.000000,   -0.257373,   -0.186884,   0.000000,   -0.142283,   -0.219654,   0.000000,
    -0.151793,   -0.080044,   0.000000,   -0.038413,   -0.082924,   0.000000,   -0.038643,   -0.064254,   0.000000,
    -0.067923,   -0.062384,   0.000000,   -0.063863,   -0.014974,   0.000000,   -0.089633,   -0.014924,   0.000000,
    -0.075173,   0.054776,    0.000000,   -0.074283,   0.058766,    0.000000,   -0.071833,   0.066306,    0.000000,
    -0.067223,   0.076156,    0.000000,   -0.059853,   0.087086,    0.000000,   -0.049123,   0.097866,    0.000000,
    -0.034433,   0.107246,    0.000000,   -0.015193,   0.113996,    0.000000,   -0.012463,   0.114506,    0.000000,
    -0.006263,   0.115296,    0.000000,   0.002557,    0.115756,    0.000000,   0.013167,    0.115296,    0.000000,
    0.024737,    0.113286,    0.000000,   0.036427,    0.109126,    0.000000,   0.047397,    0.102206,    0.000000,
    0.052757,    0.097086,    0.000000,   0.057267,    0.091256,    0.000000,   0.060927,    0.084716,    0.000000,
    0.063747,    0.077476,    0.000000,   0.065707,    0.069546,    0.000000,   0.066807,    0.060926,    0.000000,
    0.067047,    0.051636,    0.000000,   0.065407,    -0.076024,   0.000000,   0.159037,    -0.074814,   0.000000,
    0.150837,    -0.217244,   0.000000,   0.260537,    -0.186004,   0.000000,   0.281657,    -0.274394,   0.000000,
    0.309047,    -0.267854,   0.000000,   0.292017,    -0.196574,   0.000000,   0.312707,    -0.207244,   0.000000,
    0.329237,    -0.258944,   0.000000,   0.356057,    -0.250374,   0.000000,   0.348097,    -0.225484,   0.000000,
    0.393117,    -0.248694,   0.000000,   -0.150703,   -0.163804,   0.000000,   -0.232873,   -0.155934,   0.000000,
    -0.194583,   -0.086184,   0.000000,   -0.186863,   -0.136304,   0.000000,   -0.182783,   0.083346,    0.000000,
    -0.188113,   0.038236,    0.000000,   -0.223053,   0.113016,    0.000000,   -0.143523,   0.115446,    0.000000,
    -0.001513,   0.189116,    0.000000,   -0.042493,   0.169516,    0.000000,   0.002047,    0.239006,    0.000000,
    0.046597,    0.173076,    0.000000,   0.142467,    0.116346,    0.000000,   0.224997,    0.114626,    0.000000,
    0.192007,    0.042216,    0.000000,   0.180577,    0.091626,    0.000000,   0.157237,    -0.158544,   0.000000,
    0.194527,    -0.124174,   0.000000,   0.197177,    -0.078824,   0.000000,   0.236487,    -0.151414,   0.000000,
    -0.50000000, 0.48000000,  0.00000000, -0.50000000, 0.50000000,  0.00000000, -0.47999998, 0.50000000,  0.00000000,
    0.50000000,  0.48000000,  0.00000000, 0.50000000,  0.50000000,  0.00000000, 0.47999998,  0.50000000,  0.00000000,
    -0.47999998, -0.50000000, 0.00000000, -0.50000000, -0.50000000, 0.00000000, -0.50000000, -0.48000000, 0.00000000,
    0.47999998,  -0.50000000, 0.00000000, 0.50000000,  -0.50000000, 0.00000000, 0.50000000,  -0.48000000, 0.00000000 };

// 222 faces
int faces[] = { 25,  26,  27,  1, 3, 30,  31,  32,  1, 3, 0,   1,   2,   1, 3, 57,  0,   2,   1, 1, 27,  28,  29,  1, 3,
                43,  44,  45,  1, 3, 43,  45,  46,  1, 2, 36,  37,  38,  1, 3, 36,  38,  39,  1, 2, 50,  51,  52,  1, 3,
                50,  52,  53,  1, 2, 55,  56,  57,  1, 3, 54,  55,  57,  1, 1, 32,  33,  34,  1, 3, 32,  34,  35,  1, 2,
                41,  42,  43,  1, 3, 40,  41,  43,  1, 1, 47,  48,  49,  1, 3, 46,  47,  49,  1, 1, 43,  46,  49,  1, 0,
                40,  43,  49,  1, 0, 40,  49,  50,  1, 2, 39,  40,  50,  1, 1, 39,  50,  53,  1, 0, 36,  39,  53,  1, 0,
                54,  57,  2,   1, 0, 30,  32,  35,  1, 0, 30,  35,  36,  1, 2, 30,  36,  53,  1, 0, 30,  53,  54,  1, 2,
                30,  54,  2,   1, 0, 29,  30,  2,   1, 1, 29,  2,   3,   1, 2, 27,  29,  3,   1, 0, 25,  27,  3,   1, 0,
                24,  25,  3,   1, 1, 23,  24,  3,   1, 1, 22,  23,  3,   1, 1, 21,  22,  3,   1, 1, 20,  21,  3,   1, 1,
                19,  20,  3,   1, 1, 18,  19,  3,   1, 1, 17,  18,  3,   1, 1, 16,  17,  3,   1, 1, 15,  16,  3,   1, 1,
                14,  15,  3,   1, 1, 13,  14,  3,   1, 1, 13,  3,   4,   1, 2, 12,  13,  4,   1, 1, 12,  4,   5,   1, 2,
                11,  12,  5,   1, 1, 10,  11,  5,   1, 1, 10,  5,   6,   1, 2, 10,  6,   7,   1, 2, 10,  7,   8,   1, 2,
                10,  8,   9,   1, 6, 96,  197, 198, 1, 2, 212, 58,  59,  1, 2, 208, 206, 207, 1, 2, 215, 212, 59,  1, 1,
                204, 66,  67,  1, 2, 207, 204, 67,  1, 1, 96,  97,  201, 1, 1, 197, 96,  201, 1, 0, 159, 160, 161, 1, 3,
                156, 157, 158, 1, 3, 156, 158, 159, 1, 2, 156, 159, 161, 1, 0, 183, 184, 185, 1, 3, 64,  65,  66,  1, 3,
                63,  64,  66,  1, 1, 141, 142, 143, 1, 3, 141, 143, 144, 1, 2, 98,  99,  100, 1, 3, 97,  98,  100, 1, 1,
                76,  77,  78,  1, 3, 75,  76,  78,  1, 1, 81,  82,  83,  1, 3, 81,  83,  84,  1, 2, 93,  94,  95,  1, 3,
                93,  95,  96,  1, 2, 148, 149, 150, 1, 3, 147, 148, 150, 1, 1, 122, 123, 124, 1, 3, 122, 124, 125, 1, 2,
                152, 153, 154, 1, 3, 151, 152, 154, 1, 1, 192, 193, 194, 1, 3, 191, 192, 194, 1, 1, 119, 120, 121, 1, 3,
                119, 121, 122, 1, 2, 60,  61,  62,  1, 3, 59,  60,  62,  1, 1, 215, 59,  62,  1, 0, 215, 62,  63,  1, 2,
                215, 63,  66,  1, 0, 129, 130, 131, 1, 3, 129, 131, 132, 1, 2, 126, 127, 128, 1, 3, 125, 126, 128, 1, 1,
                122, 125, 128, 1, 0, 119, 122, 128, 1, 0, 119, 128, 129, 1, 2, 118, 119, 129, 1, 1, 115, 116, 117, 1, 3,
                115, 117, 118, 1, 2, 187, 188, 189, 1, 3, 187, 189, 190, 1, 2, 72,  73,  74,  1, 3, 71,  72,  74,  1, 1,
                145, 146, 147, 1, 3, 144, 145, 147, 1, 1, 144, 147, 150, 1, 0, 141, 144, 150, 1, 0, 141, 150, 151, 1, 2,
                140, 141, 151, 1, 1, 137, 138, 139, 1, 3, 137, 139, 140, 1, 2, 103, 104, 105, 1, 3, 103, 105, 106, 1, 2,
                90,  91,  92,  1, 3, 89,  90,  92,  1, 1, 85,  86,  87,  1, 3, 85,  87,  88,  1, 2, 107, 108, 109, 1, 3,
                107, 109, 110, 1, 2, 134, 135, 136, 1, 3, 133, 134, 136, 1, 1, 101, 102, 103, 1, 3, 100, 101, 103, 1, 1,
                100, 103, 106, 1, 0, 97,  100, 106, 1, 0, 97,  106, 107, 1, 2, 68,  69,  70,  1, 3, 67,  68,  70,  1, 1,
                207, 67,  70,  1, 0, 79,  80,  81,  1, 3, 78,  79,  81,  1, 1, 78,  81,  84,  1, 0, 75,  78,  84,  1, 0,
                75,  84,  85,  1, 2, 74,  75,  85,  1, 1, 111, 112, 113, 1, 3, 111, 113, 114, 1, 2, 194, 195, 58,  1, 3,
                140, 151, 154, 1, 0, 137, 140, 154, 1, 0, 74,  85,  88,  1, 0, 71,  74,  88,  1, 0, 118, 129, 132, 1, 0,
                115, 118, 132, 1, 0, 214, 215, 66,  1, 1, 214, 66,  204, 1, 0, 137, 154, 155, 1, 2, 71,  88,  89,  1, 2,
                114, 115, 132, 1, 1, 210, 211, 208, 1, 3, 203, 200, 201, 1, 3, 203, 201, 97,  1, 0, 198, 199, 196, 1, 3,
                212, 213, 214, 1, 3, 206, 208, 209, 1, 2, 197, 201, 202, 1, 2, 182, 183, 185, 1, 1, 156, 161, 162, 1, 2,
                191, 194, 58,  1, 0, 97,  107, 110, 1, 0, 203, 97,  110, 1, 0, 203, 110, 111, 1, 2, 203, 111, 114, 1, 0,
                203, 114, 132, 1, 0, 202, 203, 132, 1, 1, 202, 132, 133, 1, 2, 197, 202, 133, 1, 0, 197, 133, 136, 1, 0,
                197, 136, 137, 1, 2, 197, 137, 155, 1, 0, 196, 197, 155, 1, 1, 196, 155, 156, 1, 2, 198, 196, 156, 1, 0,
                96,  198, 156, 1, 0, 181, 182, 185, 1, 1, 210, 208, 207, 1, 0, 210, 207, 70,  1, 0, 210, 70,  71,  1, 2,
                209, 210, 71,  1, 1, 209, 71,  89,  1, 0, 206, 209, 89,  1, 0, 206, 89,  92,  1, 0, 206, 92,  93,  1, 2,
                206, 93,  96,  1, 0, 205, 206, 96,  1, 1, 205, 96,  156, 1, 0, 180, 181, 185, 1, 1, 190, 191, 58,  1, 1,
                156, 162, 163, 1, 2, 205, 156, 163, 1, 0, 205, 163, 164, 1, 2, 205, 164, 165, 1, 2, 205, 165, 166, 1, 2,
                205, 166, 167, 1, 2, 205, 167, 168, 1, 2, 205, 168, 169, 1, 2, 204, 205, 169, 1, 1, 204, 169, 170, 1, 2,
                204, 170, 171, 1, 2, 204, 171, 172, 1, 2, 204, 172, 173, 1, 2, 204, 173, 174, 1, 2, 204, 174, 175, 1, 2,
                204, 175, 176, 1, 2, 204, 176, 177, 1, 2, 214, 204, 177, 1, 0, 214, 177, 178, 1, 2, 214, 178, 179, 1, 2,
                214, 179, 180, 1, 2, 214, 180, 185, 1, 0, 212, 214, 185, 1, 0, 212, 185, 186, 1, 2, 212, 186, 187, 1, 2,
                58,  212, 187, 1, 0, 190, 58,  187, 1, 0, 217, 216, 218, 1, 7, 220, 221, 219, 1, 7, 223, 222, 224, 1, 7,
                226, 227, 225, 1, 7 };

} // anonymous namespace

void BuildMesh_FrostLogoMesh( Mesh& outMesh ) {
    int vertCount = sizeof( verts ) / sizeof( verts[0] ) / 3;
    int faceCount = sizeof( faces ) / sizeof( faces[0] ) / 5;

    outMesh.freeAllVData();
    outMesh.FreeAll();

    outMesh.setNumVerts( vertCount );
    outMesh.setNumFaces( faceCount );

    for( int i = 0; i < vertCount; ++i )
        outMesh.setVert( i, &verts[i * 3] );

    for( int i = 0; i < faceCount; ++i )
        memcpy( &outMesh.faces[i], &faces[i * 5], sizeof( Face ) );

    outMesh.InvalidateGeomCache();
    outMesh.InvalidateTopologyCache();
    outMesh.buildBoundingBox();
}

void BuildMesh_FrostDemoLogoMesh( Mesh& outMesh ) {
    int vertCount = sizeof( verts ) / sizeof( verts[0] ) / 3;
    int faceCount = sizeof( faces ) / sizeof( faces[0] ) / 5;

    outMesh.freeAllVData();
    outMesh.FreeAll();

    outMesh.setNumVerts( vertCount );
    outMesh.setNumFaces( faceCount );

    for( int i = 0; i < vertCount; ++i )
        outMesh.setVert( i, &verts[i * 3] );

    for( int i = 0; i < faceCount; ++i )
        memcpy( &outMesh.faces[i], &faces[i * 5], sizeof( Face ) );

    outMesh.InvalidateGeomCache();
    outMesh.InvalidateTopologyCache();
    outMesh.buildBoundingBox();
}
