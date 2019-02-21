#pragma once
#include <math.h>
#include "bonus.h"

#define _MAX_PATH 260 //全路径的最大长度,一般文件名不可能超出该长度
#define SCORE_MAX INFINITY
#define SCORE_MIN -INFINITY

#define max(a, b) (((a) > (b)) ? (a) : (b))


const wchar_t *wcscasechr(const wchar_t* s, wchar_t c);
int has_match(const wchar_t *needle, const wchar_t *haystack);
static void precompute_bonus(const wchar_t *haystack, score_t *match_bonus);
score_t match_positions(const wchar_t *needle, const wchar_t *haystack, size_t *positions);
inline score_t cb(wchar_t last_ch, wchar_t ch);

