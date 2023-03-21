#include <pthread.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h> //sleep

#define BLU     "\x1b[34m"
#define RED     "\x1b[31m"
#define CYN     "\x1B[36m"
#define RES     "\x1b[0m"

typedef struct {
    int x;
    int y;
} Pair;

typedef struct {
    char** map;
    int rows;
    int cols;
} Map;

typedef struct {
    char occupies;
    char cntrls;
    bool status;
} Team_Args;

static size_t bufp = 0;
static size_t bufsize;
static Pair *buf;
static Map *map;
pthread_mutex_t lock;

void *supervisor_function(void *vargp);
void *team_function(void *vargp);

bool has_valid_seats(int argc, int r_size, int b_size, int rows, int cols);
bool correct_num_args(int argc);
bool args_are_positive_ints (int r_size, int b_size, int rows, int cols);
bool bases_fit_on_board(int r_size, int b_size, int rows, int cols);
void init_map();
void init_list(Pair list[], size_t n);
Pair random_pair();
void random_pair_list(Pair list[], size_t n);
bool contains(Pair p);
void init_team(Pair list[], size_t n, char c);
void print_map();
bool is_game_over();
void missile_strike(Pair p, char c);
void flip_neighbors(Pair p, char team);
bool controls_majority_neighbors(Pair p, char occupies);
bool is_valid_neighbor(Pair p);
void write_to_file();
void game_over_stats();

int main(int argc, char * argv[]) {
    /* initialize variables*/
    srand(time(NULL));
    char *ptr;
    size_t r_size = (int) strtod(argv[1], &ptr);
    size_t b_size = (int) strtod(argv[2], &ptr);
    int rows = (int) strtod(argv[3], &ptr);
    int cols = (int) strtod(argv[4], &ptr);
    Pair *R_list, *B_list;
    pthread_t supervisor;
    pthread_mutex_init(&lock, NULL);


    /* ensure input is valid */
    if(!has_valid_seats(argc, r_size, b_size, rows, cols)) {
        exit(EXIT_FAILURE);
    }

    /* allocate storage */
    bufsize = r_size + b_size;
    R_list = malloc(sizeof(Pair) * r_size);
    B_list = malloc(sizeof(Pair) * b_size);
    buf = malloc(sizeof(Pair) * bufsize);
    map = malloc(sizeof *map);
    map->rows = rows;
    map->cols = cols;
    map->map = (char **)malloc(rows*sizeof(int*));
    for (int i = 0; i < rows; i++) {
        map->map[i] = (char*)malloc(cols*sizeof(cols));
    }


    /* initialize game */
    init_map(*map);
    random_pair_list(R_list, r_size);
    random_pair_list(B_list, b_size);
    init_team(R_list, r_size, 'R');
    init_team(B_list, b_size, 'B');


    /* initialize threads */
    pthread_create(&supervisor, NULL, supervisor_function, NULL);
    pthread_join(supervisor, NULL);


    /* deallocate storage */
    pthread_mutex_destroy(&lock);
    free(R_list);
    free(B_list);
    free(buf);
    for (int i = 0; i < rows; i++) {
        free(map->map[i]);
    }
    free(map->map);
    free(map);
    return EXIT_SUCCESS;
}

void *supervisor_function(void *vargp) {
    pthread_t red_team;
    pthread_t blue_team;
    Team_Args *team_args;
    bool game_status = false;

    do {
        team_args = malloc(sizeof(*team_args));
        team_args->occupies = 'r';
        team_args->cntrls = 'R';
        team_args->status = game_status;
        pthread_create(&red_team, NULL, &team_function, team_args);


        team_args = malloc(sizeof(*team_args));
        team_args->occupies = 'b';
        team_args->cntrls = 'B';
        team_args->status = game_status;
        pthread_create(&blue_team, NULL, &team_function, team_args);

        pthread_join(red_team, NULL);
        pthread_join(blue_team, NULL);
        sleep((rand() % 3) + 1);
        print_map();
        game_status = is_game_over();
    } while (!game_status);
    game_over_stats();
    
    return NULL;
}

void *team_function(void* arg) {
    // pthread_mutex_lock(&lock);
    Team_Args *team_args = arg;

    char cntrls = team_args->cntrls;
    char occupies = team_args->occupies;
    bool game_over = team_args->status;

    if (!game_over) {
        Pair p = random_pair();
        missile_strike(p, occupies);
    }
    free(arg);
    // pthread_mutex_unlock(&lock);
    return NULL;
}

/* ensures the user-specified args are valid */
bool  has_valid_seats(int argc, int r_size, int b_size, int rows, int cols) {
    if (!correct_num_args(argc) ||
    !args_are_positive_ints(r_size, b_size, rows, cols) ||
    !bases_fit_on_board(r_size, b_size, rows, cols)) return false;
    return true;
}

/* makes sure the user entered the correct number of args*/
bool correct_num_args(int argc) {
  if (argc != 5) {
    printf("Invalid number of parameters\n"
           "Correct usage is ./lab3 T1_Soldiers T2_Soldiers Rows Columns\n");
    return false;
  }
  return true;
}

/* makes sure the user-specified args are positive integers*/
bool args_are_positive_ints (int r_size, int b_size, int rows, int cols) {
    if ((r_size <= 0) || (b_size <= 0) || (rows <= 0) || (cols <= 0)) {
        printf("One or more parameters was invalid.\n"
        "All parameters should be positive integers.\n");
        return false;
    }
    return true;
}

/* makes sure the user-specified num bases fits on the board */
bool bases_fit_on_board(int r_size, int b_size, int rows, int cols) {
    if ((r_size + b_size) > (rows * cols)) {
        printf("The sum of T1_Soldiers and T2_Soldiers cannot exceed the total spaces on the board!\n");
        return false;
    }
    return true;
}

/* create map, fill it with 'x's */
void init_map() {
    for (int i = 0; i < map->rows; i++) {
        for (int j = 0; j < map->cols; j++) {
            map->map[i][j] = 'x';
        }
    }
}

/* set every pair in list to origin, up to n elements */
void init_list(Pair list[], size_t n) {
    size_t i;
    for (i = 0; i < n; i++)
        list[i].x = list[i].y = 0;
}

Pair random_pair() {
    Pair p;
    p.x = rand() % map->rows;
    p.y = rand() % map->cols;
    return p;
}

/* set every pair in list to random coordinates, up to n elements
 * check buf for uniqueness */
void random_pair_list(Pair list[], size_t n) {
    Pair p;
    size_t i;

    for (i = 0; i < n; i++) {
        do {
            p = random_pair();
        } while (contains(p));
        list[i] = p;
    }
}

/* check if buf contains pair p */
bool contains(Pair p) {
    size_t i;

    for (i = 0; i < bufsize; i++) {
        Pair bp = buf[i];
        if (bp.x == p.x && bp.y == p.y) {
            return true;
        }
    }

    if (bufp < bufsize) {
        buf[bufp++] = p;
    }

    return false;
}

/* populates the map with home bases from each team's randomly generated list */
void init_team(Pair list[], size_t list_length, char c) {
    for (int i = 0; i < list_length; i++) {
        int x = list[i].x;
        int y = list[i].y;
        map->map[x][y] = c;
    }
}

/* output map to screen*/
void print_map() {
    char c;
    for (int i = 0; i < map->rows; i++) {
        for (int j = 0; j < map->cols; j++) {
            c = map->map[i][j];
            if (c == 'r' || c == 'R') printf("%s", RED);
            else if (c == 'b' || c == 'B') printf("%s", BLU);
            else printf("%s", CYN);
            printf("%c", map->map[i][j]);
        }
        printf("\n");
    }
    printf("%s", RES);
    printf("\n");
    for(int i =0; i < map->rows; i++) printf("=");
    printf("\n");
}

/* check if the game has finished*/
bool is_game_over() {
    for (int i = 0; i < map->rows; i++) {
        for (int j = 0; j < map->cols; j++) {
            if (map->map[i][j] == 'x') return false;
        }
    }
    return true;
}

/* takes an X,Y coordinate and alters the board depending on what was on it*/
void missile_strike(Pair p, char c) {
    printf("%c launches a missile at (%d, %d): \n", c, p.y+1, p.x+1);

    /* if it's a home territory, disregard it*/
    if (map->map[p.x][p.y] == 'R' || map->map[p.x][p.y] == 'B') {
        // printf("Attack failed. Space is already controlled.\n");
        return;
    }

    // printf("Attack successful. ");
    /* if it belonged to the team that launched the missile, it is now unoccupied */
    if (map->map[p.x][p.y] == c) {
        // printf("Unfortunately it was against their own territory...\n");
        pthread_mutex_lock(&lock);
        map->map[p.x][p.y] = 'x';
        write_to_file();
        pthread_mutex_unlock(&lock);
    }

    /* otherwise it (was) the other teams. now its this one's*/
    else {
        pthread_mutex_lock(&lock);
        map->map[p.x][p.y] = c;
        write_to_file();
        pthread_mutex_unlock(&lock);
        if (controls_majority_neighbors(p, c)) {
            flip_neighbors(p, c);
        } else {
        }
    }
}

void flip_neighbors(Pair p, char occupies) {
    Pair p2;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            p2.x = p.x + i - 1;
            p2.y = p.y + j - 1;
            if (is_valid_neighbor(p2)) {
                /* ignore controlled spaces */
                if (map->map[p2.x][p2.y] != 'R' && map->map[p2.x][p2.y] != 'B') {
                    /* flip everything else */
                    pthread_mutex_lock(&lock);
                    map->map[p2.x][p2.y] = occupies;
                    write_to_file();
                    pthread_mutex_unlock(&lock);
                }
            }
        }
    }
}

bool controls_majority_neighbors(Pair p, char team) {
    char cntrls;
    if (team == 'r') cntrls = 'R';
    else cntrls = 'B';
    int total = 0;
    Pair p2;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            p2.x = p.x + i - 1;
            p2.y = p.y + j - 1;
            
            /* if the space is on the map*/
            if (is_valid_neighbor(p2)) {
                
                /* ...and it has an owner */
                if (map->map[p2.x][p2.y] != 'x') {
                    
                    /* increment the score if the space if it belongs to <team>*/
                    if(map->map[p2.x][p2.y] == cntrls || map->map[p2.x][p2.y] == team) total++;
                    
                    /* otherwise it belongs to the opponent. decrement the score. */
                    else total--;
                }
            }
        }
    }

    /* if total is positive, team controls more spaces than opponent*/
    return (total > 0);
}

bool is_valid_neighbor(Pair p) {
    return (p.x >= 0 && p.x < map->rows &&
            p.y >= 0 && p.y < map->cols);
}

void write_to_file() {
    FILE *fp;
    fp = fopen("out.bin", "w"); /* Open the file for writing */
    if (fp == NULL){
        printf("Error: file out.bin cannot be opened\n");
        exit(1);
    }
    fwrite(map->map, sizeof(map->map), map->rows*map->cols, fp); 
}

void game_over_stats() {
    int net = 0; //if positive, red wins. if negative, blue wins
    char c;

    printf("Game over.\n");
    for (int i = 0; i < map->rows; i++) {
        for (int j = 0; j < map->rows; j++) {
           c = map->map[i][j];
           if (c == 'r' || c == 'R') net++;
           else if (c == 'b' || c == 'B') net--;
        }
    }
    if (net < 0) printf("Blue team wins.\n");
    if (net == 0) printf("The game is a draw\n");
    else if (net > 0) printf("Red team wins.\n");
}