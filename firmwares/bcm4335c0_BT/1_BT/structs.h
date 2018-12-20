/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Copyright (c) 2016 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 **************************************************************************/

#ifndef STRUCTS_H
#define STRUCTS_H


struct dvdn_struct;

/* 1 */
struct bloc_struct
{
  int struct_id;
  int field_4;
  int list1_count;
  int list2_count;
  int *list1_head;
  int *list2_head;
  int field_18;
  int element_size;
  int field_20;
  int field_24;
  struct bloc_struct *prev;
  struct bloc_struct *next;
};

/* 2 */
struct __attribute__((packed)) __attribute__((aligned(1))) hci_cmd_buffer
{
  short cmd_code;
  char pkt_len;
  char payload[16];
};

/* 3 */
struct thrd_struct
{
  int struct_id;
  int field_4;
  int ptr_to_last_64byte_of_EF_buf;
  int EF_buffer;
  int EF_buffer_end;
  int EF_buffer_len;
  int field_18;
  int field_1C;
  struct thrd_struct *secondary_prev;
  struct thrd_struct *secondary_next;
  char *thread_name;
  int field_2C;
  int maybe_state;
  int field_34;
  int field_38;
  int maybe_thread_id;
  int field_40;
  int thread_func;
  int field_48;
  int bitmask2;
  int field_50;
  int field_54;
  struct thrd_struct *self_ptr3;
  int field_5C;
  int field_60;
  int field_64;
  int some_func_ptr;
  struct dvdn_struct *dvdn_struct_ptr;
  struct thrd_struct *third_prev;
  struct thrd_struct *third_next;
  int bitmask1;
  int pointer_to_bitmask;
  int field_80;
  int field_84;
  struct thrd_struct *prev;
  struct thrd_struct *next;
};

/* 4 */
struct dvdn_struct
{
  int struct_id;
  int field_4;
  int some_bitmask_mostly_zero;
  int field_C;
  struct thrd_struct *thrd_struct_ptr;
  int some_counter;
  struct dvdn_struct *prev;
  struct dvdn_struct *next;
};

/* 5 */
struct byte_struct
{
  int struct_id;
  int field_4;
  int field_8;
  int field_C;
  int field_10;
  int field_14;
  int field_18;
  int field_1C;
  struct thrd_struct *ptr_to_thrd_struct1;
  struct thrd_struct *ptr_to_thrd_struct2;
  int field_28;
  struct byte_struct *prev;
  struct byte_struct *next;
};


#include "../structs.common.h"

#endif /*STRUCTS_H */
