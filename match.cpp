#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <locale>
#include "bonus.h"
#include "match.h"

std::locale _loc;

const wchar_t *wcscasechr(const wchar_t* s, wchar_t c)
{
	const wchar_t accept[3] = { c,toupper(c),0 };  //不分大小写
	return wcspbrk(s, accept); //如 s为"hello",c为'e',此时打印返回值为"ello"
}

//************************************
// Method:    has_match 若目标子串的每个字符都在目标字符串里按相同顺序出现,那么说明可以进行模糊匹配,如 "adw" 和 "adobe dream  weaver"
// Returns:   int 若匹配成功,则返回1,否则为0
// Parameter: const wchar_t * needle 子串指针
// Parameter: const wchar_t * haystack 目标字符串指针
// Note: 本函数不负责检查传入字符串的合法性,请传入0结尾的字符串
//************************************
int has_match(const wchar_t *needle, const wchar_t *haystack) //const char* ,指针将被改变,但指向的字符串不可变
{
	while (*needle)
	{
		wchar_t cur = *needle;
		needle++;

		haystack = wcscasechr(haystack, cur);
		//注意haystack被改变了,因为前面匹配过的的字符串已经不再处理了

		if (!haystack)//一旦cur在haystack中不存在,说明构不成fuzzy的条件,直接退出
			return 0;
		haystack++;
	}
	return 1;
}

inline score_t cb(wchar_t last_ch, wchar_t ch)
{
	score_t score = 0;
	if (last_ch == 0x20) //' '
	{
		return SCORE_MATCH_WORD;
	}
	else if (ispunct(last_ch, _loc)) //标点符号后的字符分数增加
	{
		if (last_ch == L'.')
			score = SCORE_MATCH_DOT;
		else
			score = SCORE_MATCH_WORD;
	}
	else if ((last_ch >96)&&(last_ch<123)&&(ch>64)&&(ch<91)) // 小写后的大写字母分数增加
	{
		score = SCORE_MATCH_CAPITAL;
	}
	return score;
}

static void precompute_bonus(const wchar_t *haystack, score_t *match_bonus) {
	/* Which positions are beginning of words */
	int m = wcslen(haystack);
	wchar_t last_ch = ' ';
	for (int i = 0; i < m; i++) {
		wchar_t ch = haystack[i];
		match_bonus[i] = cb(last_ch, ch);
		last_ch = ch;
	}
}


//************************************
// Method:    match_positions  给定子串和父串,返回两者的匹配的分数,(可选)并返回匹配到子串的字符在父串中的位置
// Returns:   score_t 匹配的得分,越高越匹配
// Parameter: const wchar_t * needle 字串
// Parameter: const wchar_t * haystack 父串
// Parameter: size_t * positions 包含着父串中,字串各个字符的位置,请照如下设置,比如子串为"hello',那么定义 size_t positions[strlen(hello)],然后match_positions(子,父,&positions)
// 注:调用时必须传入的两个字符串必须有着字串-父串的关系,即 has_match(子串,父串) 得返回1才能继续调用该函数
//************************************
score_t match_positions(const wchar_t *needle, const wchar_t *haystack, size_t *positions)
{
	if (!*needle)
		return SCORE_MIN;

	if (0==has_match(needle, haystack))
		return SCORE_MIN;

	int n = wcslen(needle);
	int m = wcslen(haystack);

	if (n == m)
	{
		//由于has_match已经表明字串父串有匹配了,那么若两者长度相等,那么一定是完全匹配的
		if (positions)
		{
			for (int i = 0; i < n; i++)
				positions[i] = i;
		}	
		return SCORE_MAX;
	}
	if (m>_MAX_PATH)
	{
		//父串太长了,不处理
		return SCORE_MIN;
	}

	wchar_t* lower_needle = (wchar_t*)malloc(sizeof(wchar_t)*n);   // wchar_t lower_needle[n]
	wchar_t* lower_haystack = (wchar_t*)malloc(sizeof(wchar_t)*m); //wchar_t lower_haystack[m]


	for (int i = 0; i < n; i++)
		lower_needle[i] = tolower(needle[i]);

	for (int i = 0; i < m; i++)
		lower_haystack[i] = tolower(haystack[i]);

	score_t* match_bonus = (score_t*)malloc(sizeof(score_t)*m); // score_t match_bonus[m]
	score_t** D,**M;

	/*
	 * D[][] Stores the best score for this position ending with a match.
	 * M[][] Stores the best possible score at this position.
	 */
	D = (score_t**)malloc(sizeof(score_t)*n); // D[n][m]
	M = (score_t**)malloc(sizeof(score_t*)*n); // M[n][m]


	for (int i =0;i<n;i++)
	{
		D[i] = (score_t*)malloc(sizeof(score_t)*m);
		M[i] = (score_t*)malloc(sizeof(score_t)*m);
	}



	precompute_bonus(haystack, match_bonus);

	for (int i = 0; i < n; i++) {
		score_t prev_score = SCORE_MIN;
		score_t gap_score = i == n - 1 ? SCORE_GAP_TRAILING : SCORE_GAP_INNER;

		for (int j = 0; j < m; j++) {
			if (lower_needle[i] == lower_haystack[j]) {
				score_t score = SCORE_MIN;
				if (!i) {
					score = (j * SCORE_GAP_LEADING) + match_bonus[j];
				}
				else if (j) { /* i > 0 && j > 0*/
					score = max(
						M[i - 1][j - 1] + match_bonus[j],

						/* consecutive match, doesn't stack with match_bonus */
						D[i - 1][j - 1] + SCORE_MATCH_CONSECUTIVE);
				}
				D[i][j] = score;
				M[i][j] = prev_score = max(score, prev_score + gap_score);
			}
			else {
				D[i][j] = SCORE_MIN;
				M[i][j] = prev_score = prev_score + gap_score;
			}
		}
	}

	/* backtrace to find the positions of optimal matching */
	if (positions) {
		int match_required = 0;
		for (int i = n - 1, j = m - 1; i >= 0; i--) {
			for (; j >= 0; j--) {
				/*
				 * There may be multiple paths which result in
				 * the optimal weight.
				 *
				 * For simplicity, we will pick the first one
				 * we encounter, the latest in the candidate
				 * string.
				 */
				if (D[i][j] != SCORE_MIN &&
					(match_required || D[i][j] == M[i][j])) {
					/* If this score was determined using
					 * SCORE_MATCH_CONSECUTIVE, the
					 * previous character MUST be a match
					 */
					match_required =
						i && j &&
						M[i][j] == D[i - 1][j - 1] + SCORE_MATCH_CONSECUTIVE;
					positions[i] = j--;
					break;
				}
			}
		}
	}
	score_t score = M[n - 1][m - 1];
	
	free(match_bonus);
	free(lower_needle);
	free(lower_haystack);

	for (int i = 0; i < n; i++)
	{
		free(D[i]);
		free(M[i]);
	}

	free(D);
	free(M);

	return score;

}
