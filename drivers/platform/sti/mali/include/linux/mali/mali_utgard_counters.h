/*
 * Copyright (C) 2010-2013 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _MALI_UTGARD_COUNTERS_H_
#define _MALI_UTGARD_COUNTERS_H_

typedef struct {
	void *unused;
} mali_cinstr_counter_info;

typedef enum {
	MALI_CINSTR_COUNTER_SOURCE_EGL      =     0,
	MALI_CINSTR_COUNTER_SOURCE_OPENGLES =  1000,
	MALI_CINSTR_COUNTER_SOURCE_OPENVG   =  2000,
	MALI_CINSTR_COUNTER_SOURCE_GP       =  3000,
	MALI_CINSTR_COUNTER_SOURCE_PP       =  4000,
} cinstr_counter_source;

#define MALI_CINSTR_EGL_FIRST_COUNTER MALI_CINSTR_COUNTER_SOURCE_EGL
#define MALI_CINSTR_EGL_LAST_COUNTER (MALI_CINSTR_COUNTER_SOURCE_EGL + 999)

#define MALI_CINSTR_GLES_FIRST_COUNTER MALI_CINSTR_COUNTER_SOURCE_OPENGLES
#define MALI_CINSTR_GLES_LAST_COUNTER (MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 999)

#define MALI_CINSTR_VG_FIRST_COUNTER MALI_CINSTR_COUNTER_SOURCE_OPENVG
#define MALI_CINSTR_VG_LAST_COUNTER (MALI_CINSTR_COUNTER_SOURCE_OPENVG + 999)

#define MALI_CINSTR_GP_FIRST_COUNTER MALI_CINSTR_COUNTER_SOURCE_GP
#define MALI_CINSTR_GP_LAST_COUNTER (MALI_CINSTR_COUNTER_SOURCE_GP + 999)

#define MALI_CINSTR_PP_FIRST_COUNTER MALI_CINSTR_COUNTER_SOURCE_PP
#define MALI_CINSTR_PP_LAST_COUNTER (MALI_CINSTR_COUNTER_SOURCE_PP + 999)

typedef enum {
	/* EGL counters */

	MALI_CINSTR_EGL_BLIT_TIME                                            = MALI_CINSTR_COUNTER_SOURCE_EGL + 0,

	/* Last counter in the EGL set */
	MALI_CINSTR_EGL_MAX_COUNTER                                           = MALI_CINSTR_COUNTER_SOURCE_EGL + 1,

	/* GLES counters */

	MALI_CINSTR_GLES_DRAW_ELEMENTS_CALLS                                 = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 0,
	MALI_CINSTR_GLES_DRAW_ELEMENTS_NUM_INDICES                           = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 1,
	MALI_CINSTR_GLES_DRAW_ELEMENTS_NUM_TRANSFORMED                       = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 2,
	MALI_CINSTR_GLES_DRAW_ARRAYS_CALLS                                   = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 3,
	MALI_CINSTR_GLES_DRAW_ARRAYS_NUM_TRANSFORMED                         = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 4,
	MALI_CINSTR_GLES_DRAW_POINTS                                         = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 5,
	MALI_CINSTR_GLES_DRAW_LINES                                          = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 6,
	MALI_CINSTR_GLES_DRAW_LINE_LOOP                                      = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 7,
	MALI_CINSTR_GLES_DRAW_LINE_STRIP                                     = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 8,
	MALI_CINSTR_GLES_DRAW_TRIANGLES                                      = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 9,
	MALI_CINSTR_GLES_DRAW_TRIANGLE_STRIP                                 = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 10,
	MALI_CINSTR_GLES_DRAW_TRIANGLE_FAN                                   = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 11,
	MALI_CINSTR_GLES_NON_VBO_DATA_COPY_TIME                              = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 12,
	MALI_CINSTR_GLES_UNIFORM_BYTES_COPIED_TO_MALI                        = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 13,
	MALI_CINSTR_GLES_UPLOAD_TEXTURE_TIME                                 = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 14,
	MALI_CINSTR_GLES_UPLOAD_VBO_TIME                                     = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 15,
	MALI_CINSTR_GLES_NUM_FLUSHES                                         = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 16,
	MALI_CINSTR_GLES_NUM_VSHADERS_GENERATED                              = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 17,
	MALI_CINSTR_GLES_NUM_FSHADERS_GENERATED                              = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 18,
	MALI_CINSTR_GLES_VSHADER_GEN_TIME                                    = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 19,
	MALI_CINSTR_GLES_FSHADER_GEN_TIME                                    = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 20,
	MALI_CINSTR_GLES_INPUT_TRIANGLES                                     = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 21,
	MALI_CINSTR_GLES_VXCACHE_HIT                                         = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 22,
	MALI_CINSTR_GLES_VXCACHE_MISS                                        = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 23,
	MALI_CINSTR_GLES_VXCACHE_COLLISION                                   = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 24,
	MALI_CINSTR_GLES_CULLED_TRIANGLES                                    = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 25,
	MALI_CINSTR_GLES_CULLED_LINES                                        = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 26,
	MALI_CINSTR_GLES_BACKFACE_TRIANGLES                                  = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 27,
	MALI_CINSTR_GLES_GBCLIP_TRIANGLES                                    = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 28,
	MALI_CINSTR_GLES_GBCLIP_LINES                                        = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 29,
	MALI_CINSTR_GLES_TRIANGLES_DRAWN                                     = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 30,
	MALI_CINSTR_GLES_DRAWCALL_TIME                                       = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 31,
	MALI_CINSTR_GLES_TRIANGLES_COUNT                                     = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 32,
	MALI_CINSTR_GLES_INDEPENDENT_TRIANGLES_COUNT                         = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 33,
	MALI_CINSTR_GLES_STRIP_TRIANGLES_COUNT                               = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 34,
	MALI_CINSTR_GLES_FAN_TRIANGLES_COUNT                                 = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 35,
	MALI_CINSTR_GLES_LINES_COUNT                                         = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 36,
	MALI_CINSTR_GLES_INDEPENDENT_LINES_COUNT                             = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 37,
	MALI_CINSTR_GLES_STRIP_LINES_COUNT                                   = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 38,
	MALI_CINSTR_GLES_LOOP_LINES_COUNT                                    = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 39,
	MALI_CINSTR_GLES_POINTS_COUNT                                        = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 40,

	/* Last counter in the GLES set */
	MALI_CINSTR_GLES_MAX_COUNTER                                         = MALI_CINSTR_COUNTER_SOURCE_OPENGLES + 41,

	/* OpenVG counters */

	MALI_CINSTR_VG_MASK_COUNTER                                          = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 0,
	MALI_CINSTR_VG_CLEAR_COUNTER                                         = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 1,
	MALI_CINSTR_VG_APPEND_PATH_COUNTER                                   = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 2,
	MALI_CINSTR_VG_APPEND_PATH_DATA_COUNTER                              = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 3,
	MALI_CINSTR_VG_MODIFY_PATH_COORDS_COUNTER                            = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 4,
	MALI_CINSTR_VG_TRANSFORM_PATH_COUNTER                                = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 5,
	MALI_CINSTR_VG_INTERPOLATE_PATH_COUNTER                              = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 6,
	MALI_CINSTR_VG_PATH_LENGTH_COUNTER                                   = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 7,
	MALI_CINSTR_VG_POINT_ALONG_PATH_COUNTER                              = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 8,
	MALI_CINSTR_VG_PATH_BOUNDS_COUNTER                                   = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 9,
	MALI_CINSTR_VG_PATH_TRANSFORMED_BOUNDS_COUNTER                       = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 10,
	MALI_CINSTR_VG_DRAW_PATH_COUNTER                                     = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 11,
	MALI_CINSTR_VG_CLEAR_IMAGE_COUNTER                                   = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 12,
	MALI_CINSTR_VG_IMAGE_SUB_DATA_COUNTER                                = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 13,
	MALI_CINSTR_VG_GET_IMAGE_SUB_DATA_COUNTER                            = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 14,
	MALI_CINSTR_VG_COPY_IMAGE_COUNTER                                    = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 15,
	MALI_CINSTR_VG_DRAW_IMAGE_COUNTER                                    = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 16,
	MALI_CINSTR_VG_SET_PIXELS_COUNTER                                    = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 17,
	MALI_CINSTR_VG_WRITE_PIXELS_COUNTER                                  = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 18,
	MALI_CINSTR_VG_GET_PIXELS_COUNTER                                    = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 19,
	MALI_CINSTR_VG_READ_PIXELS_COUNTER                                   = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 20,
	MALI_CINSTR_VG_COPY_PIXELS_COUNTER                                   = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 21,
	MALI_CINSTR_VG_COLOR_MATRIX_COUNTER                                  = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 22,
	MALI_CINSTR_VG_CONVOLVE_COUNTER                                      = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 23,
	MALI_CINSTR_VG_SEPARABLE_CONVOLVE_COUNTER                            = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 24,
	MALI_CINSTR_VG_GAUSSIAN_BLUR_COUNTER                                 = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 25,
	MALI_CINSTR_VG_LOOKUP_COUNTER                                        = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 26,
	MALI_CINSTR_VG_LOOKUP_SINGLE_COUNTER                                 = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 27,
	MALI_CINSTR_VG_CONTEXT_CREATE_COUNTER                                = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 28,
	MALI_CINSTR_VG_STROKED_CUBICS_COUNTER                                = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 29,
	MALI_CINSTR_VG_STROKED_QUADS_COUNTER                                 = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 30,
	MALI_CINSTR_VG_STROKED_ARCS_COUNTER                                  = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 31,
	MALI_CINSTR_VG_STROKED_LINES_COUNTER                                 = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 32,
	MALI_CINSTR_VG_FILLED_CUBICS_COUNTER                                 = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 33,
	MALI_CINSTR_VG_FILLED_QUADS_COUNTER                                  = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 34,
	MALI_CINSTR_VG_FILLED_ARCS_COUNTER                                   = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 35,
	MALI_CINSTR_VG_FILLED_LINES_COUNTER                                  = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 36,
	MALI_CINSTR_VG_DRAW_PATH_CALLS_COUNTER                               = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 37,
	MALI_CINSTR_VG_TRIANGLES_COUNTER                                     = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 38,
	MALI_CINSTR_VG_VERTICES_COUNTER                                      = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 39,
	MALI_CINSTR_VG_INDICES_COUNTER                                       = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 40,
	MALI_CINSTR_VG_FILLED_PATHS_COUNTER                                  = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 41,
	MALI_CINSTR_VG_STROKED_PATHS_COUNTER                                 = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 42,
	MALI_CINSTR_VG_FILL_EXTRACT_COUNTER                                  = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 43,
	MALI_CINSTR_VG_DRAW_FILLED_PATH_COUNTER                              = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 44,
	MALI_CINSTR_VG_STROKE_EXTRACT_COUNTER                                = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 45,
	MALI_CINSTR_VG_DRAW_STROKED_PATH_COUNTER                             = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 46,
	MALI_CINSTR_VG_DRAW_PAINT_COUNTER                                    = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 47,
	MALI_CINSTR_VG_DATA_STRUCTURES_COUNTER                               = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 48,
	MALI_CINSTR_VG_MEM_PATH_COUNTER                                      = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 49,
	MALI_CINSTR_VG_RSW_COUNTER                                           = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 50,

	/* Last counter in the VG set */
	MALI_CINSTR_VG_MAX_COUNTER                                           = MALI_CINSTR_COUNTER_SOURCE_OPENVG + 51,

	/* Mali GP counters */

	MALI_CINSTR_GP_DEPRECATED_0                                          = MALI_CINSTR_COUNTER_SOURCE_GP + 0,
	MALI_CINSTR_GP_ACTIVE_CYCLES_GP                                      = MALI_CINSTR_COUNTER_SOURCE_GP + 1,
	MALI_CINSTR_GP_ACTIVE_CYCLES_VERTEX_SHADER                           = MALI_CINSTR_COUNTER_SOURCE_GP + 2,
	MALI_CINSTR_GP_ACTIVE_CYCLES_VERTEX_STORER                           = MALI_CINSTR_COUNTER_SOURCE_GP + 3,
	MALI_CINSTR_GP_ACTIVE_CYCLES_VERTEX_LOADER                           = MALI_CINSTR_COUNTER_SOURCE_GP + 4,
	MALI_CINSTR_GP_CYCLES_VERTEX_LOADER_WAITING_FOR_VERTEX_SHADER        = MALI_CINSTR_COUNTER_SOURCE_GP + 5,
	MALI_CINSTR_GP_NUMBER_OF_WORDS_READ                                  = MALI_CINSTR_COUNTER_SOURCE_GP + 6,
	MALI_CINSTR_GP_NUMBER_OF_WORDS_WRITTEN                               = MALI_CINSTR_COUNTER_SOURCE_GP + 7,
	MALI_CINSTR_GP_NUMBER_OF_READ_BURSTS                                 = MALI_CINSTR_COUNTER_SOURCE_GP + 8,
	MALI_CINSTR_GP_NUMBER_OF_WRITE_BURSTS                                = MALI_CINSTR_COUNTER_SOURCE_GP + 9,
	MALI_CINSTR_GP_NUMBER_OF_VERTICES_PROCESSED                          = MALI_CINSTR_COUNTER_SOURCE_GP + 10,
	MALI_CINSTR_GP_NUMBER_OF_VERTICES_FETCHED                            = MALI_CINSTR_COUNTER_SOURCE_GP + 11,
	MALI_CINSTR_GP_NUMBER_OF_PRIMITIVES_FETCHED                          = MALI_CINSTR_COUNTER_SOURCE_GP + 12,
	MALI_CINSTR_GP_RESERVED_13                                           = MALI_CINSTR_COUNTER_SOURCE_GP + 13,
	MALI_CINSTR_GP_NUMBER_OF_BACKFACE_CULLINGS_DONE                      = MALI_CINSTR_COUNTER_SOURCE_GP + 14,
	MALI_CINSTR_GP_NUMBER_OF_COMMANDS_WRITTEN_TO_TILES                   = MALI_CINSTR_COUNTER_SOURCE_GP + 15,
	MALI_CINSTR_GP_NUMBER_OF_MEMORY_BLOCKS_ALLOCATED                     = MALI_CINSTR_COUNTER_SOURCE_GP + 16,
	MALI_CINSTR_GP_RESERVED_17                                           = MALI_CINSTR_COUNTER_SOURCE_GP + 17,
	MALI_CINSTR_GP_RESERVED_18                                           = MALI_CINSTR_COUNTER_SOURCE_GP + 18,
	MALI_CINSTR_GP_NUMBER_OF_VERTEX_LOADER_CACHE_MISSES                  = MALI_CINSTR_COUNTER_SOURCE_GP + 19,
	MALI_CINSTR_GP_RESERVED_20                                           = MALI_CINSTR_COUNTER_SOURCE_GP + 20,
	MALI_CINSTR_GP_RESERVED_21                                           = MALI_CINSTR_COUNTER_SOURCE_GP + 21,
	MALI_CINSTR_GP_ACTIVE_CYCLES_VERTEX_SHADER_COMMAND_PROCESSOR         = MALI_CINSTR_COUNTER_SOURCE_GP + 22,
	MALI_CINSTR_GP_ACTIVE_CYCLES_PLBU_COMMAND_PROCESSOR                  = MALI_CINSTR_COUNTER_SOURCE_GP + 23,
	MALI_CINSTR_GP_ACTIVE_CYCLES_PLBU_LIST_WRITER                        = MALI_CINSTR_COUNTER_SOURCE_GP + 24,
	MALI_CINSTR_GP_ACTIVE_CYCLES_THROUGH_THE_PREPARE_LIST_COMMANDS       = MALI_CINSTR_COUNTER_SOURCE_GP + 25,
	MALI_CINSTR_GP_RESERVED_26                                           = MALI_CINSTR_COUNTER_SOURCE_GP + 26,
	MALI_CINSTR_GP_ACTIVE_CYCLES_PRIMITIVE_ASSEMBLY                      = MALI_CINSTR_COUNTER_SOURCE_GP + 27,
	MALI_CINSTR_GP_ACTIVE_CYCLES_PLBU_VERTEX_FETCHER                     = MALI_CINSTR_COUNTER_SOURCE_GP + 28,
	MALI_CINSTR_GP_RESERVED_29                                           = MALI_CINSTR_COUNTER_SOURCE_GP + 29,
	MALI_CINSTR_GP_ACTIVE_CYCLES_BOUNDINGBOX_AND_COMMAND_GENERATOR       = MALI_CINSTR_COUNTER_SOURCE_GP + 30,
	MALI_CINSTR_GP_RESERVED_31                                           = MALI_CINSTR_COUNTER_SOURCE_GP + 31,
	MALI_CINSTR_GP_ACTIVE_CYCLES_SCISSOR_TILE_ITERATOR                   = MALI_CINSTR_COUNTER_SOURCE_GP + 32,
	MALI_CINSTR_GP_ACTIVE_CYCLES_PLBU_TILE_ITERATOR                      = MALI_CINSTR_COUNTER_SOURCE_GP + 33,
	MALI_CINSTR_GP_JOB_COUNT                                             = MALI_CINSTR_COUNTER_SOURCE_GP + 900,

	/* Mali PP counters */

	MALI_CINSTR_PP_ACTIVE_CLOCK_CYCLES_COUNT                             = MALI_CINSTR_COUNTER_SOURCE_PP + 0,
	MALI_CINSTR_PP_TOTAL_CLOCK_CYCLES_COUNT_REMOVED                      = MALI_CINSTR_COUNTER_SOURCE_PP + 1,
	MALI_CINSTR_PP_TOTAL_BUS_READS                                       = MALI_CINSTR_COUNTER_SOURCE_PP + 2,
	MALI_CINSTR_PP_TOTAL_BUS_WRITES                                      = MALI_CINSTR_COUNTER_SOURCE_PP + 3,
	MALI_CINSTR_PP_BUS_READ_REQUEST_CYCLES_COUNT                         = MALI_CINSTR_COUNTER_SOURCE_PP + 4,
	MALI_CINSTR_PP_BUS_WRITE_REQUEST_CYCLES_COUNT                        = MALI_CINSTR_COUNTER_SOURCE_PP + 5,
	MALI_CINSTR_PP_BUS_READ_TRANSACTIONS_COUNT                           = MALI_CINSTR_COUNTER_SOURCE_PP + 6,
	MALI_CINSTR_PP_BUS_WRITE_TRANSACTIONS_COUNT                          = MALI_CINSTR_COUNTER_SOURCE_PP + 7,
	MALI_CINSTR_PP_RESERVED_08                                           = MALI_CINSTR_COUNTER_SOURCE_PP + 8,
	MALI_CINSTR_PP_TILE_WRITEBACK_WRITES                                 = MALI_CINSTR_COUNTER_SOURCE_PP + 9,
	MALI_CINSTR_PP_STORE_UNIT_WRITES                                     = MALI_CINSTR_COUNTER_SOURCE_PP + 10,
	MALI_CINSTR_PP_RESERVED_11                                           = MALI_CINSTR_COUNTER_SOURCE_PP + 11,
	MALI_CINSTR_PP_PALETTE_CACHE_READS                                   = MALI_CINSTR_COUNTER_SOURCE_PP + 12,
	MALI_CINSTR_PP_TEXTURE_CACHE_UNCOMPRESSED_READS                      = MALI_CINSTR_COUNTER_SOURCE_PP + 13,
	MALI_CINSTR_PP_POLYGON_LIST_READS                                    = MALI_CINSTR_COUNTER_SOURCE_PP + 14,
	MALI_CINSTR_PP_RSW_READS                                             = MALI_CINSTR_COUNTER_SOURCE_PP + 15,
	MALI_CINSTR_PP_VERTEX_CACHE_READS                                    = MALI_CINSTR_COUNTER_SOURCE_PP + 16,
	MALI_CINSTR_PP_UNIFORM_REMAPPING_READS                               = MALI_CINSTR_COUNTER_SOURCE_PP + 17,
	MALI_CINSTR_PP_PROGRAM_CACHE_READS                                   = MALI_CINSTR_COUNTER_SOURCE_PP + 18,
	MALI_CINSTR_PP_VARYING_READS                                         = MALI_CINSTR_COUNTER_SOURCE_PP + 19,
	MALI_CINSTR_PP_TEXTURE_DESCRIPTORS_READS                             = MALI_CINSTR_COUNTER_SOURCE_PP + 20,
	MALI_CINSTR_PP_TEXTURE_DESCRIPTORS_REMAPPING_READS                   = MALI_CINSTR_COUNTER_SOURCE_PP + 21,
	MALI_CINSTR_PP_TEXTURE_CACHE_COMPRESSED_READS                        = MALI_CINSTR_COUNTER_SOURCE_PP + 22,
	MALI_CINSTR_PP_LOAD_UNIT_READS                                       = MALI_CINSTR_COUNTER_SOURCE_PP + 23,
	MALI_CINSTR_PP_POLYGON_COUNT                                         = MALI_CINSTR_COUNTER_SOURCE_PP + 24,
	MALI_CINSTR_PP_PIXEL_RECTANGLE_COUNT                                 = MALI_CINSTR_COUNTER_SOURCE_PP + 25,
	MALI_CINSTR_PP_LINES_COUNT                                           = MALI_CINSTR_COUNTER_SOURCE_PP + 26,
	MALI_CINSTR_PP_POINTS_COUNT                                          = MALI_CINSTR_COUNTER_SOURCE_PP + 27,
	MALI_CINSTR_PP_STALL_CYCLES_POLYGON_LIST_READER                      = MALI_CINSTR_COUNTER_SOURCE_PP + 28,
	MALI_CINSTR_PP_STALL_CYCLES_TRIANGLE_SETUP                           = MALI_CINSTR_COUNTER_SOURCE_PP + 29,
	MALI_CINSTR_PP_QUAD_RASTERIZED_COUNT                                 = MALI_CINSTR_COUNTER_SOURCE_PP + 30,
	MALI_CINSTR_PP_FRAGMENT_RASTERIZED_COUNT                             = MALI_CINSTR_COUNTER_SOURCE_PP + 31,
	MALI_CINSTR_PP_FRAGMENT_REJECTED_FRAGMENT_KILL_COUNT                 = MALI_CINSTR_COUNTER_SOURCE_PP + 32,
	MALI_CINSTR_PP_FRAGMENT_REJECTED_FWD_FRAGMENT_KILL_COUNT             = MALI_CINSTR_COUNTER_SOURCE_PP + 33,
	MALI_CINSTR_PP_FRAGMENT_PASSED_ZSTENCIL_COUNT                        = MALI_CINSTR_COUNTER_SOURCE_PP + 34,
	MALI_CINSTR_PP_PATCHES_REJECTED_EARLY_Z_STENCIL_COUNT                = MALI_CINSTR_COUNTER_SOURCE_PP + 35,
	MALI_CINSTR_PP_PATCHES_EVALUATED                                     = MALI_CINSTR_COUNTER_SOURCE_PP + 36,
	MALI_CINSTR_PP_INSTRUCTION_COMPLETED_COUNT                           = MALI_CINSTR_COUNTER_SOURCE_PP + 37,
	MALI_CINSTR_PP_INSTRUCTION_FAILED_RENDEZVOUS_COUNT                   = MALI_CINSTR_COUNTER_SOURCE_PP + 38,
	MALI_CINSTR_PP_INSTRUCTION_FAILED_VARYING_MISS_COUNT                 = MALI_CINSTR_COUNTER_SOURCE_PP + 39,
	MALI_CINSTR_PP_INSTRUCTION_FAILED_TEXTURE_MISS_COUNT                 = MALI_CINSTR_COUNTER_SOURCE_PP + 40,
	MALI_CINSTR_PP_INSTRUCTION_FAILED_LOAD_MISS_COUNT                    = MALI_CINSTR_COUNTER_SOURCE_PP + 41,
	MALI_CINSTR_PP_INSTRUCTION_FAILED_TILE_READ_MISS_COUNT               = MALI_CINSTR_COUNTER_SOURCE_PP + 42,
	MALI_CINSTR_PP_INSTRUCTION_FAILED_STORE_MISS_COUNT                   = MALI_CINSTR_COUNTER_SOURCE_PP + 43,
	MALI_CINSTR_PP_RENDEZVOUS_BREAKAGE_COUNT                             = MALI_CINSTR_COUNTER_SOURCE_PP + 44,
	MALI_CINSTR_PP_PIPELINE_BUBBLES_CYCLE_COUNT                          = MALI_CINSTR_COUNTER_SOURCE_PP + 45,
	MALI_CINSTR_PP_TEXTURE_MAPPER_MULTIPASS_COUNT                        = MALI_CINSTR_COUNTER_SOURCE_PP + 46,
	MALI_CINSTR_PP_TEXTURE_MAPPER_CYCLE_COUNT                            = MALI_CINSTR_COUNTER_SOURCE_PP + 47,
	MALI_CINSTR_PP_VERTEX_CACHE_HIT_COUNT                                = MALI_CINSTR_COUNTER_SOURCE_PP + 48,
	MALI_CINSTR_PP_VERTEX_CACHE_MISS_COUNT                               = MALI_CINSTR_COUNTER_SOURCE_PP + 49,
	MALI_CINSTR_PP_VARYING_CACHE_HIT_COUNT                               = MALI_CINSTR_COUNTER_SOURCE_PP + 50,
	MALI_CINSTR_PP_VARYING_CACHE_MISS_COUNT                              = MALI_CINSTR_COUNTER_SOURCE_PP + 51,
	MALI_CINSTR_PP_VARYING_CACHE_CONFLICT_MISS_COUNT                     = MALI_CINSTR_COUNTER_SOURCE_PP + 52,
	MALI_CINSTR_PP_TEXTURE_CACHE_HIT_COUNT                               = MALI_CINSTR_COUNTER_SOURCE_PP + 53,
	MALI_CINSTR_PP_TEXTURE_CACHE_MISS_COUNT                              = MALI_CINSTR_COUNTER_SOURCE_PP + 54,
	MALI_CINSTR_PP_TEXTURE_CACHE_CONFLICT_MISS_COUNT                     = MALI_CINSTR_COUNTER_SOURCE_PP + 55,
	MALI_CINSTR_PP_PALETTE_CACHE_HIT_COUNT                               = MALI_CINSTR_COUNTER_SOURCE_PP + 56, /* Mali 200 only */
	MALI_CINSTR_PP_PALETTE_CACHE_MISS_COUNT                              = MALI_CINSTR_COUNTER_SOURCE_PP + 57, /* Mali 200 only */
	MALI_CINSTR_PP_COMPRESSED_TEXTURE_CACHE_HIT_COUNT                    = MALI_CINSTR_COUNTER_SOURCE_PP + 56, /* Mali 400 class only */
	MALI_CINSTR_PP_COMPRESSED_TEXTURE_CACHE_MISS_COUNT                   = MALI_CINSTR_COUNTER_SOURCE_PP + 57, /* Mali 400 class only */
	MALI_CINSTR_PP_LOAD_STORE_CACHE_HIT_COUNT                            = MALI_CINSTR_COUNTER_SOURCE_PP + 58,
	MALI_CINSTR_PP_LOAD_STORE_CACHE_MISS_COUNT                           = MALI_CINSTR_COUNTER_SOURCE_PP + 59,
	MALI_CINSTR_PP_PROGRAM_CACHE_HIT_COUNT                               = MALI_CINSTR_COUNTER_SOURCE_PP + 60,
	MALI_CINSTR_PP_PROGRAM_CACHE_MISS_COUNT                              = MALI_CINSTR_COUNTER_SOURCE_PP + 61,
	MALI_CINSTR_PP_JOB_COUNT                                             = MALI_CINSTR_COUNTER_SOURCE_PP + 900,
} cinstr_counters_m200_t;

#endif /*_MALI_UTGARD_COUNTERS_H_*/
