## 수신 패킷을 싱글 스레드로 처리하는 구조의 채팅 서버 프로젝트

- 사용 기술 : C++, IOCP
- 설명
    - 유저들의 채팅 메시지 처리를 담당한다.
    - 한 유저가 채팅을 입력하면 주변 9개의 섹터에 브로드 캐스팅하여 채팅 메시지를 전달한다.
    - 유저의 섹터 이동을 처리한다.
    - 메시지 처리를 싱글 스레드가 처리하여 동기화 처리하지 않아도 된다.
