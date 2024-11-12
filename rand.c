/***********************************************************
	rand.c -- rand()
***********************************************************/

#define RAND_MAX  32767
static unsigned long next;

int rand(void)
{
	next = next * 1103515245L + 12345;
	return (unsigned int)(next / 65536L) % (unsigned int)(RAND_MAX + 1);
}

void srand(unsigned seed)
{
	next = seed;
}
