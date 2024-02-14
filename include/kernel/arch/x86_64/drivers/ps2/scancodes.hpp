// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <cstdint>

// definitely not from linux

#define NUM_KEYS 256
#define MAX_NUM_FUNC 256
#define MAX_NUM_KEYMAPS 256

#define KEY(t, v) (((t) << 8) | (v))
#define KEY_TYPE(x) ((x) >> 8)
#define KEY_VALUE(x) ((x) & 0xff)

#define KG_SHIFT 0
#define KG_CTRL 2
#define KG_ALT 3
#define KG_ALTGR 1
#define KG_SHIFTL 4
#define KG_KANASHIFT 4
#define KG_SHIFTR 5
#define KG_CTRLL 6
#define KG_CTRLR 7

#define KEY_TYPE_LATIN 0
#define KEY_TYPE_FN 1
#define KEY_TYPE_SPEC 2
#define KEY_TYPE_PAD 3
#define KEY_TYPE_DEAD 4
#define KEY_TYPE_CONS 5
#define KEY_TYPE_CUR 6
#define KEY_TYPE_SHIFT 7
#define KEY_TYPE_META 8
#define KEY_TYPE_ASCII 9
#define KEY_TYPE_LOCK 10
#define KEY_TYPE_LETTER 11
#define KEY_TYPE_SLOCK 12
#define KEY_TYPE_DEAD2 13
#define KEY_TYPE_BRL 14

#define KEY_SHIFT KEY(KEY_TYPE_SHIFT, KG_SHIFT)
#define KEY_CTRL KEY(KEY_TYPE_SHIFT, KG_CTRL)
#define KEY_ALT KEY(KEY_TYPE_SHIFT, KG_ALT)
#define KEY_ALTGR KEY(KEY_TYPE_SHIFT, KG_ALTGR)
#define KEY_SHIFTL KEY(KEY_TYPE_SHIFT, KG_SHIFTL)
#define KEY_SHIFTR KEY(KEY_TYPE_SHIFT, KG_SHIFTR)
#define KEY_CTRLL KEY(KEY_TYPE_SHIFT, KG_CTRLL)
#define KEY_CTRLR KEY(KEY_TYPE_SHIFT, KG_CTRLR)

#define KEY_P0 KEY(KEY_TYPE_PAD, 0)
#define KEY_P1 KEY(KEY_TYPE_PAD, 1)
#define KEY_P2 KEY(KEY_TYPE_PAD, 2)
#define KEY_P3 KEY(KEY_TYPE_PAD, 3)
#define KEY_P4 KEY(KEY_TYPE_PAD, 4)
#define KEY_P5 KEY(KEY_TYPE_PAD, 5)
#define KEY_P6 KEY(KEY_TYPE_PAD, 6)
#define KEY_P7 KEY(KEY_TYPE_PAD, 7)
#define KEY_P8 KEY(KEY_TYPE_PAD, 8)
#define KEY_P9 KEY(KEY_TYPE_PAD, 9)

#define KEY_PCOMMA KEY(KEY_TYPE_PAD, 15)
#define KEY_PDOT KEY(KEY_TYPE_PAD, 16)

#define KEY_FIND KEY(KEY_TYPE_FN, 20)
#define KEY_INSERT KEY(KEY_TYPE_FN, 21)
#define KEY_REMOVE KEY(KEY_TYPE_FN, 22)
#define KEY_SELECT KEY(KEY_TYPE_FN, 23)
#define KEY_PGUP KEY(KEY_TYPE_FN, 24)
#define KEY_PGDN KEY(KEY_TYPE_FN, 25)

#define KEY_DOWN KEY(KEY_TYPE_CUR, 0)
#define KEY_LEFT KEY(KEY_TYPE_CUR, 1)
#define KEY_RIGHT KEY(KEY_TYPE_CUR, 2)
#define KEY_UP KEY(KEY_TYPE_CUR, 3)

extern uint16_t plain_map[NUM_KEYS];
extern uint16_t shift_map[NUM_KEYS];
extern uint16_t altgr_map[NUM_KEYS];
extern uint16_t ctrl_map[NUM_KEYS];
extern uint16_t shift_ctrl_map[NUM_KEYS];
extern uint16_t alt_map[NUM_KEYS];
extern uint16_t ctrl_alt_map[NUM_KEYS];

extern uint16_t *key_maps[MAX_NUM_KEYMAPS];

extern char *func_table[MAX_NUM_FUNC];