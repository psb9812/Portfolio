## 모니터링 서버

- 사용 기술 : C++, IOCP, MySQL
- NetServerLib_v3 적용했다. (정적 라이브러리)
- 역할
  - 각 서버의 모니터링 데이터를 수집하여 모니터링 클라에게 송신하고, 주기적으로 모니터링 DB에 데이터를 저장한다.
    
- 채팅, 로그인 서버와 연동하여 분산 서버 프로젝트에서 활용했다.
