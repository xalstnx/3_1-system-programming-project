#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <termio.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
//2016003736 소프트웨어학부 컴퓨터전공 이민수
#define MAX_LINE_LENGTH       1 << 10
#define MAX_ARG_LENGTH       1 << 10
#define MAX_ARG_SIZE           100
#define MAX_HIST_SIZE           20

#define INPUT_REDIRECTION     ">"
#define OUTPUT_REDIRECTION   "<"
#define APPEND_REDIRECTION   ">>"
#define INPUT_REDIRECTION_OW	">|"
#define PIPELINE                "|"
#define BACKGROUND           "&"

#define ENTER                  10
#define QUIT                   4
#define BACKSPACE             8
#define ESC                    27
#define LSB                    91

#define STATE_NONCANONICAL  0
#define STATE_CANONICAL      1

#define DELIMS                " \t\r\n"
#define SEMI			";"

void set_input_mode(void);
void reset_input_mode(void);
int prompt(char*);

void init_history(char**);
void get_history(char**, int);
void print_history(char**, int);
void clear_history(char**);

bool parse(char*, char**, size_t*);
int lookupRedirection(char**, size_t, int*, int*);
bool lookupBackground(char**, size_t);
bool execute(char**, size_t);
void pipeexecute(char**, size_t);
int ispipe(char **, size_t);

char curpath[MAX_LINE_LENGTH];

struct termios saved_tty;

char* hist[MAX_HIST_SIZE];
int current_cursor = 0;
int setnoclobber = 0;

int main(int argc, char** argv)
{
  char line[MAX_LINE_LENGTH];

  init_history(hist);  //history 초기화
  set_input_mode();

  while(true)
  {
    char* arguments[MAX_ARG_LENGTH];
    char eraseexclmarkline[MAX_LINE_LENGTH];
    size_t argument_count = 0;
    bool issemi = false;
    int semiindex = 0;
    int beforesemiindex = 0;
    int exclmarknum = 0;

    if(prompt(line) == 0) continue;

    if(strcmp(line, "exit") == 0){  //line이 exit이면 반복문 종료
      exit(EXIT_SUCCESS);
      break;
    }
    if(strcmp(line, "set -C") == 0 || strcmp(line, "set -o noclobber") == 0){ //line이 set -C나 set -o noclobber이면 '>'이 덮어쓰기가 안되도록 설정
      //printf("> can not overwrite\n");
      setnoclobber = 1;
      continue;
    }
    else if(strcmp(line, "set +C") == 0 || strcmp(line, "set +o noclobber") == 0){ //line이 set +C나 set +o noclobber이면 '>'이 덮어쓰기가 되도록 설정
      //printf("> can overwrite\n");
      setnoclobber = 0;
      continue;
    }

    hist[current_cursor] = strdup(line);  // history에 현재 들어온 라인 저장
    current_cursor = (current_cursor + 1) % MAX_HIST_SIZE;// history에서의 현재 위치를 바꿔줌
    
    if(strncmp(line, "!", 1) == 0){  //line 의 첫글자가 ! 이면 history에 저장된 해당 번호번째 라인 실행 
      char* token;
      token = strtok(line, "!");  //line에서 !를 떼어내어 token에 저장
      exclmarknum = atoi(token);  //token을 정수형을 바꿔줌
      
      if(hist[exclmarknum-1] != NULL){  //해당 번호의 history가 비어있는지 확인
	      memset(line, '\0', MAX_LINE_LENGTH); // line 초기화
        strcpy(line, hist[exclmarknum-1]);  // line에 해당 번호의 history 복사 
      }else{
	      printf("no %dth line in history\n", exclmarknum);
      }
    }
    

    parse(line, arguments, &argument_count); //들어온 line을 arguments에 델리미터로 나누어 저장함 argument_count는 저장한 arguments의 수

    if(ispipe(arguments, argument_count) == 1){  //만약 | 가 있으면 pipeexecute를 실행함
      pipeexecute(arguments, argument_count);  
    }else{                                      // | 가 없을때
	    for(int s = 0;s<argument_count;s++){  
	      char* token;
	      if(strstr(arguments[s], SEMI)!=NULL){    // ; 이 있으면
		      token = strtok(arguments[s], SEMI);    // argument에서 ;을 떼어냄
	      	char* beforesemiargs[MAX_ARG_LENGTH];
		      issemi = true;
		      beforesemiindex = semiindex;
		      semiindex = s;
		      for(int b = beforesemiindex;b<semiindex-1;b++)
		        beforesemiargs[b-beforesemiindex] = arguments[b];
		      beforesemiargs[semiindex-beforesemiindex] = token;
		      if((strcmp(beforesemiargs[semiindex-beforesemiindex], "history") == 0))
		        print_history(hist, current_cursor); 
		      else
		        execute(beforesemiargs, (size_t)(semiindex-beforesemiindex+1));
		      semiindex++;
	      }
	    }

	    if(issemi == true){        // ;이 있으면 line의 마지막 argument를 실행
	      char* lastargs[MAX_ARG_LENGTH];
	      lastargs[0] = arguments[argument_count-1];
	      if((strcmp(lastargs[0], "history") == 0))
		      print_history(hist, current_cursor); 
	      else
		      execute(lastargs, 1);
	    }
	    else{
	      
	      if(argument_count == 0) continue;

	      if(strcmp(arguments[0], "exit") == 0){   // exit를 입력하면 반복문 빠져나옴
		      exit(EXIT_SUCCESS);
		      break;
	      }
	      else if((strcmp(arguments[0], "history") == 0))   // history를 입력하면 history 출력
		      print_history(hist, current_cursor);    
	      else  
		      execute(arguments, argument_count);
	    }
    }

  }
  reset_input_mode();
  clear_history(hist); // history 비우기

  return EXIT_SUCCESS;
}

void init_history(char** hist_buf) // history 초기화
{
  int i;
  for(i=0; i<MAX_HIST_SIZE; ++i) hist_buf[i] = NULL;
}

void print_history(char** hist_buf, int cursor) // history 출력
{
  int i = cursor;
  int n = 1;

  do
  {
    if(hist_buf[i])
      fprintf(stdout, "%2d: %s\n", n++, hist_buf[i]);

    i = (i + 1) % MAX_HIST_SIZE;
  }
  while(i != cursor);
}

void clear_history(char** hist_buf)  // history 비우기
{
  int i;

  for(i=0; i<MAX_HIST_SIZE; ++i)
  {
    free(hist_buf[i]);
    hist_buf[i] = NULL;
  }
}

void set_input_mode(void)
{
  struct termios tty;

  if(!isatty(STDIN_FILENO))
  {
    perror("isatty");

    exit(EXIT_FAILURE);
  }

  tcgetattr(STDIN_FILENO, &saved_tty);   // termios 구조체 초기화 현재 값을 saved_tty에 저장
  atexit(reset_input_mode);

  tcgetattr(STDIN_FILENO, &tty);        // termios 구조체 초기화
  tty.c_lflag &= ~(ICANON | ECHO);      // lflag에 not (echo 가능 또는 정규입력처리 가능) 을 추가함
  tty.c_cc[VMIN] = 1;                   // ◦쉘 스크립트가 한 번에 한 문자씩 처리하도록 설정
  tty.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty);
}

void reset_input_mode(void)             // 저장되어 있던 tty로 다시 inputmode를 변경
{
  tcsetattr(STDIN_FILENO, TCSANOW, &saved_tty);
}

int prompt(char* line)
{
  int n = 0;
  int len = 0;
  char c;

  int cur = current_cursor;

  fflush(NULL);

  fprintf(stdout, "[%s]$ ", getcwd(curpath, MAX_LINE_LENGTH));  // 현재 작업 위치를 표시해줌

  while(c = fgetc(stdin))
  {
    if(c == QUIT)
    {
      fputc('\n', stdout);
      return 0;
    }

    if(c == ENTER)
    {
      fputc('\n', stdout);
      break;      
    }

    if((c != BACKSPACE) && (c <= 26)) break;  // 입력받은 문자가 backspace가 아니고 알파벳이라면 반복문을 빠져나옴

    switch(c)
    {
      case BACKSPACE:       //입력받은 문자가 backspace이면 
        if(n == 0) break;

        fputc('\b', stdout);
        fputc(' ', stdout);
        fputc('\b', stdout);

        line[--n] = (char) 0; // line에서 한글자를 지움

        break;
      case  ESC:
        if((c = fgetc(stdin)) != LSB) break;

        switch(fgetc(stdin))
        {
          case 'A':  // 위방향키 눌렀을때
            --cur;   // cur을 1감소 시킴
            if(cur < 0 ) cur = current_cursor - 1;

            fprintf(stdout, "\r%80s", " ");
            fprintf(stdout, "\r[%s]$ %s", getcwd(curpath, MAX_LINE_LENGTH), hist[cur]);

            memset(line, '\0', MAX_LINE_LENGTH); // line 초기화 
            strcpy(line, hist[cur]);        // line에 history에서 cur번째 것을 복사해줌
            n = strlen(line);

            break;

          case 'B':  // 아래방향키 눌렀을때
            ++cur;   // cur을 1증가 시킴
              if(cur >= current_cursor) cur = 0;

              fprintf(stdout, "\r%80s", " ");
              fprintf(stdout, "\r[%s]$ %s", getcwd(curpath, MAX_LINE_LENGTH), hist[cur]);

              memset(line, '\0', MAX_LINE_LENGTH);  // line 초기화 
              strcpy(line, hist[cur]);  // line에 history에서 cur번째 것을 복사해줌
              n = strlen(line);

              break;
        }

        break;

      default:
        fputc(c, stdout);
        line[n++] = (char) c;

        break;
    }
  }

  line[n] = '\0';

  len = strlen(line);

  if(len == 0) return 0;

  for(n=0; n<len; ++n)
    if(line[n] != ' ' && line[n] != '\t') return 1;

  return 0;
}

int lookupRedirection(char** argv, size_t argc, int* flag, int* kinds)    // redirection이 있는지 찾음
{
  int i;

  for(i=0; i<(int)argc; ++i)
  {
    if(strcmp(argv[i], INPUT_REDIRECTION) == 0)  // '>' 이 있는지 체크 
    {
      *flag = O_WRONLY | O_CREAT | O_TRUNC;
      *kinds = 1;
      break;
    }
    
    if(strcmp(argv[i], APPEND_REDIRECTION) == 0)  // '>>' 이 있는지 체크
    {
      *flag = O_WRONLY | O_CREAT | O_APPEND;
      *kinds = 2;
      break;
    }
    
    if(strcmp(argv[i], OUTPUT_REDIRECTION) == 0)  // '<' 이 있는지 체크
    {
      *flag = O_RDONLY;
      *kinds = 3;
      break;
    }
    
    if(strcmp(argv[i], INPUT_REDIRECTION_OW) == 0)  // '>|' 이 있는지 체크 
    {
      *flag = O_WRONLY | O_CREAT | O_TRUNC;
      *kinds = 4;
      break;
    }
  }

  return i;
}

bool lookupBackground(char** argv, size_t argc)   // '&' 이 있는지 체크
{
  return (strcmp(argv[argc - 1], BACKGROUND) == 0);
}

bool parse(char* line, char** argv, size_t* argc)  // line을 delimiter로 각각 한 단어나 redirection으로 떼어내고 argv배열에 저장
{
  size_t n = 0;
  char* temp = strtok(line, DELIMS);

  if(temp == NULL) return false;

  while(temp != NULL)
  {
    argv[n++] = temp;

    temp = strtok(NULL, DELIMS);
  }
  argv[n] = NULL;

  *argc = n;

  return true;
}

bool execute(char** argv, size_t argc)   // arguments들을 처리함 
{
  char* params[MAX_ARG_LENGTH];

  int fd = -1;
  int flag = 0;
  int kinds = 0;
  int idx = lookupRedirection(argv, argc, &flag, &kinds);

  bool bg = lookupBackground(argv, argc);

  pid_t pid;
  int status;

  int i, n;

  if((pid = fork()) == -1)
  {
    perror("fork");

    return false;
  }
  else if(pid == 0)
  {
    for(i=0; i<idx; ++i)
    {
      if(strcmp(argv[i], BACKGROUND) == 0) break;  // '&' 전까지를 params에 저장

      params[i] = argv[i];
    }

    if(flag > 0)    // redirection이 있을경우
    {
      if(kinds == 1 && setnoclobber == 1){    // '>'이고 덮어쓰기가 안될경우 아래코드를 프린트함
	      printf("%s: cannot overwrite existing file\n", argv[i+1]);  
      }else{
        if(kinds == 1 || kinds == 2 || kinds == 4){   // '>' 이거나 '>>' 이거나 '>|' 일경우 stdout으로 처리 
          if((fd = open(argv[i+1], flag, 0644)) == -1)
          {
            perror("open");

            return false;
          }
	
	        if(close(STDOUT_FILENO) == -1)
          {
            perror("close");
        
            return false;
          }   

          if(dup2(fd, STDOUT_FILENO) == -1)
          {
            perror("dup2");

            return false;
          }

          if(close(fd) == -1)
          {
            perror("close");

            return false;
          }
        }else if(kinds == 3){   // '<' 일경우 stdin으로 처리
      	  if((fd = open(argv[2], O_RDONLY)) == -1)
          {
            perror("open");

            return false;
          }

	        if(close(STDIN_FILENO) == -1)
          {
            perror("close");
        
            return false;
          }   

          if(dup2(fd, STDIN_FILENO) == -1)
          {
            perror("dup2");
  
            return false;
          }

	        if(close(fd) == -1)
          {
            perror("close");
  
            return false;
          }
        }
      }
    }

    if(strcmp(argv[0], "cd") == 0){   // cd를 입력받으면 
	    chdir(argv[1]);                 // 현재 디렉토리 변경 
	    getcwd(curpath, MAX_LINE_LENGTH);
	    return false;
    }else{          // 그 외의 것들은 execvp로 처리함
      if(execvp(argv[0], params) == -1)
      {
        perror("execvp");

        return false;
      }
    }
    exit(EXIT_SUCCESS);
  }
  else
  {
    if(bg == true)
    {
      bg = 0;
      return true;
    }
    else
    {
      if((pid = waitpid(pid, &status, 0)) == -1) // 프로세스가 끝나는것을 기다림
      {
        perror("waitpid");
      
        return false;
      }

      return true;
    }
  }
}

void pipeexecute(char** argv, size_t argc)  // '|' 가 있을 경우 실행
{
  int n, i, j, m;
  int exist = 0;
  int subpid;
  int fds[2];
  char **argv2 = (char **)malloc(16*sizeof(char *));
  
  for(n=0;n<argc;n++){      // '|' 가 있는지 확인
    if(strcmp(argv[n], PIPELINE) == 0){
      exist = 1;
      break;
    }
  }
  
  m = n;                // '|'가 있는 위치

  if(exist == 1){       //'|'가 있으면
    argv[n] == NULL;    // '|' 없앰
    for(i=n+1, j=0;i<argc;i++, j++){
      argv2[j] = (char *)malloc(strlen(argv[i])+1);
      strcpy(argv2[j], argv[i]);    // argv2에 argv의 '|'뒤에 있는 argument들을 복사
      argv[i] = NULL;               // argv의 '|'뒤에 있는 argument들을 없앰
    }
  }
  else{
    argv2 = argv;
  }

  if(exist == 1){       //'|'가 있으면
    pipe(fds);          // 파이프 생성
    if((subpid = fork()) == -1){
      perror("fork");
    }else if(subpid){
      dup2(fds[1], STDOUT_FILENO);  // stdout을 fds[1]로 write
      close(fds[0]);
      close(fds[1]);
      execute(argv, (size_t)(m));   // '|' 이전의 argument 처리
      exit(EXIT_SUCCESS);
    }

    dup2(fds[0], STDIN_FILENO);     // stdin을 fds[0]로 write
    close(fds[0]); 
    close(fds[1]);
  }
  execute(argv2, (size_t)(j));      // '|' 이후의 argument 처리
  exit(EXIT_SUCCESS);
  
}

int ispipe(char **argv, size_t argc){   // '|' 가 있는지 확인
  int exist = 0;
  for(int n=0;n<argc;n++){
    if(strcmp(argv[n], PIPELINE) == 0){
      exist = 1;
      break;
    }
  }
  return exist;
}
