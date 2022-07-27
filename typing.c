#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <pwd.h> 
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <termios.h>            
#include <time.h>
#include <assert.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_CLEAR_SCREEN "\e[1;1H\e[2J"

enum Keys{
  BACKSPACE = 127,
  ARROW_LEFT = 37,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
};


void die(const char* error_message){
  perror(error_message);
  exit(1);
}

void enableRawMode(){
  static struct termios oldt, newt;

    /*tcgetattr gets the parameters of the current terminal
    STDIN_FILENO will tell tcgetattr that it should write the settings
    of stdin to oldt*/
    tcgetattr( STDIN_FILENO, &oldt);
    /*now the settings will be copied*/
    newt = oldt;

    /*ICANON normally takes care that one line at a time will be processed
    that means it will return if it sees a "\n" or an EOF or an EOL*/
    newt.c_lflag &= ~(ICANON | ECHO);          

    /*Those new settings will be set to STDIN
    TCSANOW tells tcsetattr to change attributes immediately. */
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
    printf("\e[?25l");
}

void disableRawMode(){
  printf("\e[?25h");
}

char read_input(){
  char c;
  c = getchar();
  switch(c){
    case '\x1b': 
      disableRawMode();
      exit(1);
    default:
      return c;
  }

}

int count_words(const char sentence[]){
    int counted = 0; // result

    // state:
    const char* it = sentence;
    int inword = 0;

    do switch(*it){
        case '\0': 
        case ' ': 
        case '\t': 
        case '\n': 
        case '\r': 
          if(inword){
            inword = 0; counted++;
          }
          break;

      default: 
          inword = 1;

    }while(*it++);

    return counted;
}

int compare(char input, char character){
  if(input == character){
    printf("%s%c",ANSI_COLOR_GREEN, character);
    printf("%s",ANSI_COLOR_RESET);
    return 0;
  }else{
    printf("%s%c",ANSI_COLOR_RED, character);
    printf("%s",ANSI_COLOR_RESET);
    return 1;
  }
}

int calculate_wpm(double duration, int words, int word_mistakes){
  int wpm = (int) (words-word_mistakes)  * 60 / duration;

  return wpm;
}

int calculate_word_mistakes(char *input, char *text){
  int word_mistakes;
  int input_position, text_position;

  word_mistakes = 0;
  input_position = 0;
  text_position = 0;
  
  while(input_position != strlen(input)){
    if(input[input_position] != text[text_position]){
      word_mistakes++;
      while(input[input_position] != ' '){
        input_position++;
      }
      while(text[text_position] != ' '){
        text_position++;
      }
    }
    input_position++;
    text_position++;
  }
  printf("\nmistakes: %d\n", word_mistakes);
  return word_mistakes;
}

void display_stats(int wpm, int typing_erros, int word_erros){
  printf("%s", ANSI_CLEAR_SCREEN);
  printf("\n%s----------------------------------------------%s\n",ANSI_COLOR_BLUE,ANSI_COLOR_RESET);
  printf("WPM: %d\n",wpm);
  printf("%sTYPING ERROS:: %d\n",ANSI_COLOR_RED, typing_erros);
  printf("%s----------------------------------------------%s",ANSI_COLOR_BLUE,ANSI_COLOR_RESET);
}

int randint(int n) {
  if ((n - 1) == RAND_MAX) {
    return rand();
  }else{
    // Supporting larger values for n would requires an even more
    // elaborate implementation that combines multiple calls to rand()
    assert (n <= RAND_MAX);

    // Chop off all of the values that would cause skew...
    int end = RAND_MAX / n; // truncate skew
    assert (end > 0);
    end *= n;

    // ... and ignore results from rand() that fall above that limit.
    // (Worst case the loop condition should succeed 50% of the time,
    // so we can expect to bail out of this loop pretty quickly.)
    int r;
    while ((r = rand()) >= end);

    return r % n;
  }
}

char *get_sentence(){
  char line[200][256];
  char *fname = "sentences.txt";
  FILE *fptr = NULL; 
  int i = 0;
  int tot = 0;

  fptr = fopen(fname, "r");
  while(fgets(line[i], 200, fptr)) 
  {
      line[i][strlen(line[i]) - 1] = '\0';
      i++;
  }
  tot = i;

  return line[randint(200)];
}



void mainloop(){
  char *text, *total_input, input, game_state;
  size_t text_len; 
  int text_pos, words, wpm, typing_erros, word_erros, random_number;
  time_t clock_start, clock_end; 
  double total_time;

  game_state = 'y';

  while(game_state == 'y'){

    printf("%s", ANSI_CLEAR_SCREEN);

    text = get_sentence();
    text_len = strlen(text);
    total_input = (char*) malloc((text_len+128)*sizeof(char));;
    text_pos = 0;
    typing_erros = 0;
    words = count_words(text);
    clock_start = time(NULL);
    

    printf("%s%c%s%s", ANSI_COLOR_YELLOW, text[0], ANSI_COLOR_RESET,&(text[text_pos+1]));

    for(;;){

      if(text_pos == text_len) break;

      input = read_input();
      total_input[text_pos] = input; 
      if(text_pos != 0 && input == BACKSPACE){
        text_pos--;

        for(int i=0; i<text_len-text_pos; i++) printf("\b");
        printf("%s%c",ANSI_COLOR_YELLOW, text[text_pos]);
        printf("%s%s",ANSI_COLOR_RESET, &(text[text_pos+1]));

        typing_erros--;

        continue;
      }

      for(int i=0; i<text_len-text_pos; i++) printf("\b");
      typing_erros += compare(input, text[text_pos]);
      text_pos++;
      printf("%s%c",ANSI_COLOR_YELLOW, text[text_pos]);
      printf("%s%s",ANSI_COLOR_RESET, &(text[text_pos+1]));
    }
    
    clock_end = time(NULL);
    total_time = (double)(clock_end - clock_start);

    word_erros = calculate_word_mistakes(total_input, text);
    wpm = calculate_wpm(total_time, words, word_erros);

    display_stats(wpm, typing_erros, word_erros);

    printf("\n\nWanna play another round: y | n");
    game_state = getchar();
  }
}


void init(){
  enableRawMode();
  mainloop();
  disableRawMode();
}

int main(){
  init();
  return 0;
}