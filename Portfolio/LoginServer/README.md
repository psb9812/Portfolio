## 로그인 서버

- 사용 기술 : C++, IOCP, Redis, MySQL
- NetServerLib_v3 적용했다. (정적 라이브러리)
- 설명
  - 로그인 부하 분산
  - accountDB를 조회하여 유효한 로그인 요청인지 판단 후 Redis에 로그인 토큰을 저장한다.
