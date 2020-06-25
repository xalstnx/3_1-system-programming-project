# 3학년 1학기 시스템프로그래밍 프로젝트

## 요구사항
- foreground and background execution (&)
- multiple commands separated by semicolons
- history command
- shell redirection (>, >>, >|, <)
- shell pipe (ls –la | more)
- Multiple pipe (ls | grep “^d” | more)
- cd command

## makefile


## 1.foreground and background execution(&)

lookupBackground()로 &가 있는지 체크

execute에서 bg에 &의 유무 저장

bg 가 true 이면(background 실행이면) bg 를 0 으로 바꿔줌
bg 가 false 이면(foreground 실행이면) waitpid로 프로세스가 끝나는 것을 기다림 

sleep 100 에 &를 붙일 경우 background에서 실행되어 바로 다음 line을 받는다.
하지만 &를 붙이지 않을 경우 sleep 100이 foreground에서 실행되어 100을 기다리게 된다.

## 2.multiple commands separated by semicolons

strstr()로 ;이 있는 argument를 체크하고 token에 ;을 떼어낸 argument를 넣어줌
위 코드를 쉽게 예제로 설명 해보면
who > names.txt; ls; pwd 라는 line이 입력되면 parse()로 arguments배열에 strtok()로
“ ”로 나누어 저장하면 
arguments[0] = who
arguments[1] = >
arguments[2] = names.txt;
arguments[3] = ls;
arguments[4] = pwd
가 될 것이다.

s = 2 
beforesemiindex = 0
semiindex = 2
beforesemiargs[0] = arguments[0] = who
beforesemiargs[1] = arguments[1] = >
beforesemiargs[2] = token = names.txt
semiindex = 3
execute(beforesemiargs)로 who > names.txt 가 실행된다.

반복문 for로 돌아와서
s = 3
beforesemiindex = 3
semiindex = 3
beforesemiargs[0] = token = ls
semiindex = 4
execute(beforesemiargs)로 ls 가 실행된다.
이후에는 ; 이 없으므로
lastargs[0] = arguments[argument_count-1] = pwd
execute(lastargs) 로 pwd가 실행된다.

### ls; pwd; date를 실행한 화면


## 3.history command




### history 실행화면, !번호 실행화면


### 방향키 위, 아래 입력


방향키 위, 아래로 이전에 사용한 line을 불러오기 위해 termios를 사용하여 
Canonical / Non Canonical 모드를 만듦.

## 4.shell redirection (>, >>, >|, <)



‘>’, ‘>|’ 일 경우 O_WRONLY | O_CREAT | O_TRUNC을 flag로 주어 덮어쓰도록 한다.
‘<’ 일 경우 O_RDONLY로 준다.
‘>>’ 일 경우 O_WRONLY | O_CREAT | O_APPEND로 주어 이어쓰게 한다.

### shell redirection 실행 화면


### set [+|-]o noclobber / set [+|-]C 로 옵션 설정



## 5.shell pipe (ls –la | more)


ispipe()로 전달받은 arguments에 | 가 있는지 체크한후 있으면 pipeexecute()로 실행함

### ls –la | more 실행 화면


## 6.multiple pipe (ls | grep “^d” | more)
구현 실패

## 7.cd command

execute() 내부에 위 코드를 추가하여 cd를 입력받으면 chdir로 디렉토리를 변경함

### cd 실행 화면
