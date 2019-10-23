#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "simplerandom.h"
#include "list.h"

#define MAX_SIZE	5

struct enode {
	struct list_head en_node;
	int operands_cnt;	/* operands cnt, remember there is another result */
	int operands_num[MAX_SIZE];
	char operators[MAX_SIZE];

	int resolved_cur;	/* current resoved value of sub_expression */

	unsigned char hide_mode;	/* hide mode: 0->nohide, 1->hide_operator
					   2->hide_operands, 3->hide_a_digit_in_an_operands,
					   4->hide_equal_sign, 5->hide_result */
	int hide_pos;			/* which operand or operator to hide */
	int hide_digit;			/* which digit to hide for an operand */
};

uint32_t real_random_data[8];
static SimpleRandomKISS_t rng_kiss;

void prepare_random_data(char *random_data_addr, unsigned int len)
{
	FILE *fp;
	fp = fopen("/dev/urandom", "r");
	assert(fp != NULL);

	fread(random_data_addr, 1, len, fp);

	fclose(fp);
}

int get_digit(int num, int where)
{
	assert(num >= pow(10, where));
	return (num%(int)pow(10, (where+1))) / where;
}

/* get a rand integer at [from, from+len) */
int rand_int(int from, int len)
{
	return from + simplerandom_kiss_next(&rng_kiss)%len;
}

/* get a float number which is >= from */
float rand_float(int from)
{
	return rand() / (double)(RAND_MAX);
}

static struct list_head en_list;
static char *ops = "+-=";

/* is number n is ok to be used as next operand in enode en,
 * there has been cur_cnt operands resolved in en*/
int is_operand_ok(int n, struct enode *en, int  cur_cnt)
{
	int i=0;

	if ((en->operators[i]=='-') && (en->resolved_cur<n))
		return 0;

	/* is ok */
	return 1;
}

/* print the enode */
void print_en(struct enode *pen, FILE *fpw)
{
	char buf[64] = {0};

	if (pen->hide_pos != 0)
		sprintf(buf, "%d", pen->operands_num[0]);
	else
		sprintf(buf, "%s", "__");

	int i=0;
	for (i=0; i<(pen->operands_cnt-1); i++) {
		//printf("hide_pos: %d, %d\n", pen->hide_pos, i);
		if (pen->hide_pos == (2*i+1))
			sprintf(buf, "%s%s", buf, "__");
		else
			sprintf(buf, "%s%c", buf, pen->operators[i]);
		if (pen->hide_pos == (2*i+2))
			sprintf(buf, "%s%s", buf, "__");
		else
			sprintf(buf, "%s%d", buf, pen->operands_num[i+1]);
	}

	fprintf(fpw, "%s\n", buf);
}

void print_rule()
{
	FILE *fpr = fopen("rule.txt", "w+");
	assert(fpr != NULL);

	fprintf(fpr, "１，任何数，加上０，或者减去０，都等于它本身！\n");
	fprintf(fpr, "	例子：\n");
	fprintf(fpr, "		5+0=5	0+7=7	9-0=9\n");
	fprintf(fpr, "２，任何数，减去它自己，都等于０！\n");
	fprintf(fpr, "	例子：\n");
	fprintf(fpr, "		4-4=0	8-8=0	0-0=0\n");
	fprintf(fpr, "３，任意两个数相加，交互位置，结果保持不变！\n");
	fprintf(fpr, "	例子：\n");
	fprintf(fpr, "		3+4=4+3=7	5+9=9+5=14\n");
	fprintf(fpr, "４，两个数相加，拆小凑整，计算结果！\n");
	fprintf(fpr, "	例子：\n");
	fprintf(fpr, "		8+5		26+37\n");
	fprintf(fpr, "		=8+(2+3)	=(23+3)+37\n");
	fprintf(fpr, "		=(8+2)+3	=23+(3+37)\n");
	fprintf(fpr, "		=13		=63\n");
	fprintf(fpr, "５，两个数相减，按减数去拆分被减数，多出来的数就是结果！\n");
	fprintf(fpr, "	例子：\n");
	fprintf(fpr, "		8-5		37-24\n");
	fprintf(fpr, "		=(3+5)-5	=(13+24)-24\n");
	fprintf(fpr, "		=3+0		=13+(24-24)\n");
	fprintf(fpr, "		=3		=13\n");
	fprintf(fpr, "６，数在等号两边可移动，一边加上这个数，移过去后就是减去这个数！\n");
	fprintf(fpr, "	例子：\n");
	fprintf(fpr, "		8-5=3		37-24=13\n");
	fprintf(fpr, "		8=3+5		37=13+24\n");
	fprintf(fpr, "		8-3=5		37-13=24\n");
	fprintf(fpr, "		8-3-5=0		37-13-24=0\n");
	fprintf(fpr, "		0=3+5-8\n");
	fprintf(fpr, "７，等号两边可以同时加上或者减去一个相同的数！\n");
	fprintf(fpr, "	例子：\n");
	fprintf(fpr, "		8-5=3			37-24=13\n");
	fprintf(fpr, "		8-5-1=3-1		37-24-4=13-4\n");
	fprintf(fpr, "		8-5-1+13=3-1+13		37-24-4+7=13-4+7\n");

	/* do clean work */
	fclose(fpr);
}

int main(int argc, char **argv)
{
	int c = 0;
	int howmany=100;	/* default 100 */
	int maxnum=100;		/* default inside [0-100) */
	int borrow_carry = 0;	/* default no borrow or carry */
	int two_three = 2;	/* default two operands */
	int minus_plus = 0;	/* all plus operators */
	int hide_mode = 0;	/* default no hide */
	char *fout = "kl.txt";

	while ((c = getopt(argc, argv, "m:n:t:o:bh:f:")) != -1)
	{
		switch (c)
		{
		case 'm':	// how many
			howmany = atoi(optarg);
			break;
		case 'n':	// inside [0~n)
			maxnum = atoi(optarg);
			break;
		case 't':
			two_three = atoi(optarg);
			break;
		case 'o':
			minus_plus = atoi(optarg);
		case 'b':	// borrow or carry
			borrow_carry = 1;
			break;
		case 'h':
			hide_mode = atoi(optarg);
			break;
		case 'f':
			fout = optarg;
			break;
		default:
			printf("usage: ./kidsmath -m 100 -n 20 -b -f all.txt\n\
options: \n\
	-m	how many equations\n\
	-n	inside [0,n) \n\
	-t	how many operands \n\
	-o	all plus(0) or all minus(1) or mixed(2) \n\
	-b	borrow or carry \n\
	-f	write output to file \n");
			return 1;
		}
	}

	/* generate equations */
	int i=0, nextn=0;
	struct enode *pen = NULL;
	INIT_LIST_HEAD(&en_list);

	prepare_random_data(real_random_data, sizeof(real_random_data));
	simplerandom_kiss_seed_array(&rng_kiss, real_random_data, 8, true);

	while (howmany--) {
		//usleep(10);
		pen = malloc(sizeof(struct enode));

		memset(pen, '0', sizeof(struct enode));

		/* decide operators */
		if ((minus_plus == 0) || (minus_plus == 1)) {
			for (i=0; i<(two_three-1); i++)
				pen->operators[i] = ops[minus_plus];
		} else {
			for (i=0; i<(two_three-1); i++)
				pen->operators[i] = ops[(rand_int(0,100)%2)];
		}
		pen->operators[i] = ops[2];

		/* decide operands */
		pen->operands_cnt = two_three + 1;
		pen->resolved_cur = pen->operands_num[0] = rand_int(0, maxnum);
		for (i=0; i<(two_three-1); i++) {
			while (!is_operand_ok(nextn=rand_int(0, maxnum), pen, i+1)) ;
			pen->operands_num[i+1] = nextn;
			switch (pen->operators[i]) {
			case '+':
				pen->resolved_cur += pen->operands_num[i+1];
				break;
			case '-':
				pen->resolved_cur -= pen->operands_num[i+1];
				break;
			default:
				assert(0);
				break;
			}
		}
		pen->operands_num[pen->operands_cnt-1] = pen->resolved_cur;

		/* hide mode */
		int total = 2*pen->operands_cnt-1;
		pen->hide_mode = hide_mode;
		switch (pen->hide_mode) {
		case 0:	/* no hide */
			break;
		case 1: /* hide operator */
			while (((pen->hide_pos=rand_int(0, total-2)) % 2) == 0) ;
			break;
		case 2: /* hide operand */
			while (((pen->hide_pos=rand_int(0, total-2)) % 2) == 1) ;
			break;
		case 3: /* hide a digit in an operand */
			break;
		case 4: /* hide equal sign */
			pen->hide_pos = total-2;
			break;
		case 5: /* hide result */
			pen->hide_pos = total-1;
			break;
		default:
			assert(0);
			break;
		}

		list_add_tail(&pen->en_node, &en_list);
	}

	/* output those equations */
	FILE *fpw = fopen(fout, "w+");
	assert(fpw != NULL);
	pen = NULL;
	list_for_each_entry(pen, &en_list, en_node) {
		print_en(pen, fpw);
	}


	/* do clean work */
	fclose(fpw);

	print_rule();


	printf("%d, %f\n", rand_int(0,100), rand_float(1));
	return 0;
}

