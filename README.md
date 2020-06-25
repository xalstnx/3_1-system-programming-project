# 3학년 1학기 시스템프로그래밍 프로젝트

## 요구사항
- foreground and background execution (&)
- multiple commands separated by semicolons
- history command
- shell redirection (>, >>, >|, <)
- shell pipe (ls –la | more)
- Multiple pipe (ls | grep “^d” | more)
- cd command

## 1.foreground and background execution(&)
![1](https://user-images.githubusercontent.com/22045179/85659774-daa66b80-b6ef-11ea-9267-79d37bdfe05c.png)

lookupBackground()로 &가 있는지 체크

![2](https://user-images.githubusercontent.com/22045179/85659784-dd08c580-b6ef-11ea-85db-84b37943d960.png)

execute에서 bg에 &의 유무 저장

![3](https://user-images.githubusercontent.com/22045179/85659791-de39f280-b6ef-11ea-9518-672bdcea87ee.png)
![4](https://user-images.githubusercontent.com/22045179/85659799-df6b1f80-b6ef-11ea-874d-35ba5a1e3911.png)

bg 가 true 이면(background 실행이면) bg 를 0 으로 바꿔줌
bg 가 false 이면(foreground 실행이면) waitpid로 프로세스가 끝나는 것을 기다림 

![4](https://user-images.githubusercontent.com/22045179/85659799-df6b1f80-b6ef-11ea-874d-35ba5a1e3911.png)

sleep 100 에 &를 붙일 경우 background에서 실행되어 바로 다음 line을 받는다.
하지만 &를 붙이지 않을 경우 sleep 100이 foreground에서 실행되어 100을 기다리게 된다.

## 2.multiple commands separated by semicolons
![5](https://user-images.githubusercontent.com/22045179/85659803-e09c4c80-b6ef-11ea-9223-2fde8cccce81.png)

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
![6](https://user-images.githubusercontent.com/22045179/85659808-e1cd7980-b6ef-11ea-8bcc-9b42a82bd141.png)

## 3.history command
![7](https://user-images.githubusercontent.com/22045179/85659814-e3973d00-b6ef-11ea-98c3-9f6186c6ed43.png)
![8](https://user-images.githubusercontent.com/22045179/85659823-e4c86a00-b6ef-11ea-960b-43a36464b24f.png)
![9](https://user-images.githubusercontent.com/22045179/85659831-e72ac400-b6ef-11ea-98d9-8f0b2a6330a3.png)

### history 실행화면, !번호 실행화면
![10](https://user-images.githubusercontent.com/22045179/85659846-e8f48780-b6ef-11ea-9743-d932682d8bec.png)

### 방향키 위, 아래 입력
![11](https://user-images.githubusercontent.com/22045179/85659850-ea25b480-b6ef-11ea-854c-bfa2ef226882.png)

방향키 위, 아래로 이전에 사용한 line을 불러오기 위해 termios를 사용하여 
Canonical / Non Canonical 모드를 만듦.
![12](https://user-images.githubusercontent.com/22045179/85659857-ec880e80-b6ef-11ea-9b4c-887ccf1ff58b.png)

## 4.shell redirection (>, >>, >|, <)
![13](https://user-images.githubusercontent.com/22045179/85659872-ef82ff00-b6ef-11ea-907b-28b8bfb513aa.png)
![14](https://user-images.githubusercontent.com/22045179/85659881-f14cc280-b6ef-11ea-8747-a59f42ac22f7.png)
![15](https://user-images.githubusercontent.com/22045179/85659886-f3168600-b6ef-11ea-8ba6-7c2abd2f6bea.png)

‘>’, ‘>|’ 일 경우 O_WRONLY | O_CREAT | O_TRUNC을 flag로 주어 덮어쓰도록 한다.
‘<’ 일 경우 O_RDONLY로 준다.
‘>>’ 일 경우 O_WRONLY | O_CREAT | O_APPEND로 주어 이어쓰게 한다.

### shell redirection 실행 화면
![16](https://user-images.githubusercontent.com/22045179/85659893-f4e04980-b6ef-11ea-9d53-24b1ae3f4fe5.png)

### set [+|-]o noclobber / set [+|-]C 로 옵션 설정
![17](https://user-images.githubusercontent.com/22045179/85659902-f6aa0d00-b6ef-11ea-96ef-9db9232ebc2b.png)
![18](https://user-images.githubusercontent.com/22045179/85659911-f873d080-b6ef-11ea-8a5e-a8f612d071bd.png)

## 5.shell pipe (ls –la | more)
![19](https://user-images.githubusercontent.com/22045179/85659918-fa3d9400-b6ef-11ea-9aa2-d3b0e281126e.png)
![20](https://user-images.githubusercontent.com/22045179/85659925-fc9fee00-b6ef-11ea-9ac5-522b31bd80a4.png)

ispipe()로 전달받은 arguments에 | 가 있는지 체크한후 있으면 pipeexecute()로 실행함

### ls –la | more 실행 화면
![21](https://user-images.githubusercontent.com/22045179/85659930-fe69b180-b6ef-11ea-9420-65d1dd93eb02.png)

## 6.multiple pipe (ls | grep “^d” | more)
구현 실패

## 7.cd command
![22](https://user-images.githubusercontent.com/22045179/85659936-00337500-b6f0-11ea-82fe-68f745977270.png)

execute() 내부에 위 코드를 추가하여 cd를 입력받으면 chdir로 디렉토리를 변경함

### cd 실행 화면
![23](https://user-images.githubusercontent.com/22045179/85659943-0164a200-b6f0-11ea-9d84-dadc2e48f307.png)
