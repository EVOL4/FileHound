#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <locale>
#include "bonus.h"
#include "match.h"

std::locale _loc;

const wchar_t *wcscasechr(const wchar_t* s, wchar_t c)
{
	const wchar_t accept[3] = { c,toupper(c),0 };  //���ִ�Сд
	return wcspbrk(s, accept); //�� sΪ"hello",cΪ'e',��ʱ��ӡ����ֵΪ"ello"
}

//************************************
// Method:    has_match ��Ŀ���Ӵ���ÿ���ַ�����Ŀ���ַ����ﰴ��ͬ˳�����,��ô˵�����Խ���ģ��ƥ��,�� "adw" �� "adobe dream  weaver"
// Returns:   int ��ƥ��ɹ�,�򷵻�1,����Ϊ0
// Parameter: const wchar_t * needle �Ӵ�ָ��
// Parameter: const wchar_t * haystack Ŀ���ַ���ָ��
// Note: �������������鴫���ַ����ĺϷ���,�봫��0��β���ַ���
//************************************
int has_match(const wchar_t *needle, const wchar_t *haystack) //const char* ,ָ�뽫���ı�,��ָ����ַ������ɱ�
{
	while (*needle)
	{
		wchar_t cur = *needle;
		needle++;

		haystack = wcscasechr(haystack, cur);
		//ע��haystack���ı���,��Ϊǰ��ƥ����ĵ��ַ����Ѿ����ٴ�����

		if (!haystack)//һ��cur��haystack�в�����,˵��������fuzzy������,ֱ���˳�
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
	else if (ispunct(last_ch, _loc)) //�����ź���ַ���������
	{
		if (last_ch == L'.')
			score = SCORE_MATCH_DOT;
		else
			score = SCORE_MATCH_WORD;
	}
	else if ((last_ch >96)&&(last_ch<123)&&(ch>64)&&(ch<91)) // Сд��Ĵ�д��ĸ��������
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
// Method:    match_positions  �����Ӵ��͸���,�������ߵ�ƥ��ķ���,(��ѡ)������ƥ�䵽�Ӵ����ַ��ڸ����е�λ��
// Returns:   score_t ƥ��ĵ÷�,Խ��Խƥ��
// Parameter: const wchar_t * needle �ִ�
// Parameter: const wchar_t * haystack ����
// Parameter: size_t * positions �����Ÿ�����,�ִ������ַ���λ��,������������,�����Ӵ�Ϊ"hello',��ô���� size_t positions[strlen(hello)],Ȼ��match_positions(��,��,&positions)
// ע:����ʱ���봫��������ַ������������ִ�-�����Ĺ�ϵ,�� has_match(�Ӵ�,����) �÷���1���ܼ������øú���
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
		//����has_match�Ѿ������ִ�������ƥ����,��ô�����߳������,��ôһ������ȫƥ���
		if (positions)
		{
			for (int i = 0; i < n; i++)
				positions[i] = i;
		}	
		return SCORE_MAX;
	}
	if (m>_MAX_PATH)
	{
		//����̫����,������
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
