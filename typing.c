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
#include <errno.h> 

#define GOTO(x,y) printf("\033[%d;%dH", (y), (x))
#define MOVE_UP(y) printf("\033[%dA",(y))
#define MOVE_DOWN(y) printf("\033[%dB",(y))
#define MOVE_RIGHT(x) printf("\033[%dC",(x))
#define MOVE_LEFT(x) printf("\033[%dD",(x))
#define CTRL_KEY(k) ((k) & 0x1f)
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

struct typing_config{
  char *langauge;
  char *mode;
  double average_wpm;
  int words;
  double average_erros;
  int erros;
  double average_word_erros;
  int word_erros;
  int attemps;
};

struct typing_config config;

char *langauges[] = {
  "en",
  "de",
};
enum{NUM_LANGUGES= sizeof(langauges) / sizeof(langauges[0])};


char *modes[] = {
  "sentencs",
  "quotes",
};
enum{NUM_MODES= sizeof(modes) / sizeof(modes[0])};



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
    case CTRL_KEY('q'): 
      disableRawMode();
      exit(1);
    default:
      return c;
  }
}

void menu(){
  int pos, pos2;
  char c, l;

  printf("%s", ANSI_CLEAR_SCREEN);
  printf("\e[?25h");
  printf("LANGAUGE| \nMODE\t| ");
  MOVE_UP(1);

  pos = 0;
  
  while(1){

    c = read_input();

    switch(c){
      case 'w':
        if(pos > 0){
          pos--;
          MOVE_UP(1);
        }
        continue;
      case 's':
        if(pos < 2){
          pos++;
          MOVE_DOWN(1);
        }
        continue;
      case '\n':

        pos2 = 0;
        if(pos == 0){
          printf("%s",ANSI_CLEAR_SCREEN);
          for(int i=0; i<NUM_LANGUGES;i++) printf("%s\n", langauges[i]);
          GOTO(strlen(langauges[0])+1,0);

          while(1){
            l = read_input();
            
            switch(l){

              case 'w':
                if(pos2> 0){
                  pos2--;
                  MOVE_UP(1);
                }
                continue;

              case 's':
                if(pos2< 2){
                  pos2++;
                  MOVE_DOWN(1);
                }
                continue;

              case '\n':
                config.langauge = langauges[pos2];

              case '\x1b':
                printf("%s", ANSI_CLEAR_SCREEN);
                return;
            }
          }
        }else if(pos == 1){
          printf("%s",ANSI_CLEAR_SCREEN);
          for(int i=0; i<NUM_MODES; i++) printf("%s\n", modes[i]);
          GOTO(strlen(modes[0])+1,0);

          while(1){
            l = read_input();

            switch(l){

              case 'w':
                if(pos2> 0){
                  pos2--;
                  MOVE_UP(1);
                }
                continue;

              case 's':
                if(pos2 < 2){
                  pos2++;
                  MOVE_DOWN(1);
                }
                continue;

              case '\n':
                config.mode = modes[pos2];

              case '\x1b': 
                printf("%s", ANSI_CLEAR_SCREEN);
                return;
            }
          }
        }
        continue;

      case '\x1b': 
        printf("%s", ANSI_CLEAR_SCREEN);
        printf("\e[?25h");
        return;
    }
  }

}




int count_words(const char sentence[]){
    int counted;
    const char* it;
    int inword;

    it = sentence;
    inword = 0;
    counted = 0;

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

  config.words += wpm;
  config.average_wpm = (double)config.words / config.attemps;

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

  config.attemps++;
  config.word_erros += word_mistakes;
  config.average_word_erros = (double)config.word_erros / config.attemps;

  return word_mistakes;
}

void display_stats(int wpm, int typing_erros, int word_erros){
  printf("%s", ANSI_CLEAR_SCREEN);
  printf("\n%s----------------------------------------------%s\n",ANSI_COLOR_BLUE,ANSI_COLOR_RESET);
  printf("WPM: %d\n",wpm);
  printf("%sTYPING ERROS:: %d\n",ANSI_COLOR_RED, typing_erros);
  printf("%sTYPING ERROS:: %d\n",ANSI_COLOR_MAGENTA, word_erros);
  printf("%sAVERAGE WPM: %.2lf\n",ANSI_COLOR_GREEN, config.average_wpm);
  printf("%sAVERAGE WORD ERROS: %.2lf\n",ANSI_COLOR_GREEN, config.average_word_erros);
  printf("%sAVERAGE ERROS: %.2lf\n",ANSI_COLOR_GREEN, config.average_erros);
  printf("%s----------------------------------------------%s",ANSI_COLOR_BLUE,ANSI_COLOR_RESET);
}

int randint(int n) {
  int r;
  int end; 
  end = RAND_MAX / n; 

  if ((n - 1) == RAND_MAX) {
    return rand();
  }else{
    assert (n <= RAND_MAX);

    assert (end > 0);
    end *= n;

    while((r = rand()) >= end);

    return r % n;
  }
}

char *get_sentence(){
  char *fname, line[200][256]; 
  FILE *fptr;
  int i, tot;

  tot = 0;
  i = 0;

  if(strcmp(config.langauge ,"en") == 0){
    if(strcmp(config.mode, "sentence") == 0){
      fname = "english_sentences.txt";
    }else if(strcmp(config.mode, "quote") == 0){
      fname = "sentences.txt";
    }
  }else if(strcmp(config.langauge ,"de") == 0){
    if(strcmp(config.mode, "sentence") == 0){
      fname = "english_sentences.txt";
    }else if(strcmp(config.mode, "quote") == 0){
      fname = "sentences.txt";
    }
  }


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
  int text_pos, words, wpm, typing_erros, word_erros;
  time_t clock_start, clock_end; 
  double total_time;

  game_state = 'y';

  while(game_state == 'y'){

    printf("%s", ANSI_CLEAR_SCREEN);

    text = get_sentence();
    text_len = strlen(text);
    text_pos = 0;
    total_input = (char*) malloc((text_len+128)*sizeof(char));;
    typing_erros = 0;
    words = count_words(text);
    clock_start = time(NULL);
    

    printf("%s%c%s%s", ANSI_COLOR_YELLOW, text[0], ANSI_COLOR_RESET,&(text[text_pos+1]));

    for(;;){

      if(text_pos == text_len) break;

      input = read_input();
      if(input == '\x1b') menu();
      total_input[text_pos] = input; 
      if(text_pos != 0 && input == BACKSPACE){
        text_pos--;

        for(int i=0; i<text_len-text_pos; i++) printf("\b");
        printf("%s%c",ANSI_COLOR_YELLOW, text[text_pos]);
        printf("%s%s",ANSI_COLOR_RESET, &(text[text_pos+1]));

        if(typing_erros > 0) typing_erros--;

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
    config.erros += typing_erros;
    config.average_erros = (double)config.erros/config.attemps;

    display_stats(wpm, typing_erros, word_erros);

    printf("\n\nWanna play another round: y | n");
    game_state = getchar();
  }
}


void init(){
  config.langauge = "en";
  config.mode = "sentence";
  config.average_wpm= 0;
  config.words = 0;
  config.average_erros = 0;
  config.erros = 0;
  config.average_word_erros = 0;
  config.word_erros = 0;
  config.attemps = 0;
}

int main(){
  init();
  enableRawMode();
  mainloop();
  disableRawMode();

  return 0;
}

