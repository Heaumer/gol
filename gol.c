#include <signal.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DEFH	6
#define DEFW	6

enum state {
	Dead  = -1,
	Alive =  1
};

typedef struct cell Cell;
struct cell {
	enum state s;
	int n;
};

typedef struct st State;
struct st {
	Cell **world;
	int h, w;
	int time;
	int stable;
};

static State state;

/* Copy of the world */
static Cell **newworld = NULL;
	
#define isalive(c)		((c).s == Alive)
#define isdead(c)		!isalive(c)

#define underpop(c)		((c).n < 2)
#define overpop(c)		((c).n > 3)

#define uprising(c)		(isdead(c) && (c).n == 3)
#define stayalive(c)	(isalive(c) && ((c).n == 2 || (c).n == 3))
#define isdying(c)		(isalive(c) && (underpop(c) || overpop(c)))

#define areequals(a,b)	((a).s == (b).s && (a).n == (b).n)

void
printworld(State *s)
{
	int i, j;

	if (s->stable) {
		puts("Stable state reached!");
		exit(1);
	}

	printf("Time: %d\n", s->time);

	for (i = 0; i < s->h; i++) {
		for (j = 0; j < s->w; j++) {
			if (isalive(s->world[i][j]))
				putchar('#');
			else
				putchar(' ');
		}
		putchar('\n');
	}
}

void
delworld(void)
{
	int i;

	for (i = 0; i < state.h; i++) {
		free((state.world)[i]), free(newworld[i]);
	}

	free(state.world), free(newworld);
}

void
initworld(int h, int w)
{
	int i, j;

	state.time = 0;
	state.h = h, state.w = w;
	state.world = malloc(h * sizeof *(state.world));
	newworld = malloc(h * sizeof *newworld);

	for (i = 0; i < h; i++) {
		(state.world)[i] = malloc(w * sizeof **(state.world));
		newworld[i] = malloc(w * sizeof **newworld);
		for (j = 0; j < w; j++) {
			/* newworld[i][j] = (Cell) { Dead, 0 }; */
			newworld[i][j].s = Dead;
			newworld[i][j].n = 0;
		}
	}

	atexit(delworld);
}

void
loadworld(void)
{
	int i, j;

	state.stable = 1;

	for (i = 0; i < state.h; i++)
		for (j = 0; j < state.w; j++) {
			if (!areequals((state.world)[i][j], newworld[i][j]))
				state.stable = 0;
			(state.world)[i][j] = newworld[i][j];
		}
}

#define outofbounds(k, l) (((k) < 0 || (k) >= state.h) || ((l) < 0 || (l) >= state.w))

void
affect(int i, int j, enum state how)
{
	int k, l;

	newworld[i][j].s = how;

	for (k = i-1; k <= i+1; k++)
		for (l = j-1; l <= j+1; l++) {
			if (!outofbounds(k, l) && !(k == i && l == j))
				newworld[k][l].n += how;
		}
}

void
nextstep(void)
{
	int i, j;

	for (i = 0; i < state.h; i++)
		for (j = 0; j < state.w; j++) {
			if (uprising((state.world)[i][j]))
				affect(i, j, Alive);
			else if (isdying((state.world)[i][j]))
				affect(i, j, Dead);
		}

	loadworld();
	state.time++;
}

void
loadfile(FILE *f)
{
	int i, j;
	int c;

	i = j = 0;

	while ( (c = getc(f)) != EOF) {
		if (i > state.h || j > state.w) {
			fprintf(stderr, "error: size does not match with content\n");
			exit(1);
		}
		if (c == '\n') {
			j = 0; i++;
			continue;
		}
		if (c != ' ')
			affect(i, j, Alive);
		j++;
	}
}

void
initfile(const char *file)
{
	char buf[BUFSIZ], *pbuf;
	FILE *f;

	f = fopen(file, "r");

	if (f == NULL) {
		fprintf(stderr, "error: cannot open %s\n", file);
		exit(1);
	}

	/* First line contains size informations : state.h state.w */
	fgets(buf, BUFSIZ, f);

	errno = 0;

	state.h = strtol(buf, &pbuf, 10);
	if (errno != 0)
		state.w = strtol(pbuf, NULL, 10);

	if (errno != 0) {
		fprintf(stderr, "error: first line must contain size informations\n");
		exit(1);
	}

	/* File contains size, but we count from 0 */
	initworld(state.h-1, state.w-1);

	loadfile(f);

	fclose(f);
}

void
sighandle(int s)
{
	exit(0);
}

int
main(int argc, char *argv[])
{
	signal(SIGINT, sighandle);
	signal(SIGTERM, sighandle);

	if (argc >= 2) {
		initfile(argv[1]);
	}
	else {
		initworld(DEFH, DEFW);
		affect(0, 1, Alive);
		affect(1, 2, Alive);
		affect(2, 0, Alive);
		affect(2, 1, Alive);
		affect(2, 2, Alive);
	}
	loadworld();

	do {
		printworld(&state);
		nextstep();
		sleep(1);
	} while (1);

	return 0;
}
