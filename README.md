# DeviceDriverStudy
# 임베디드 리눅스 ioctl & 타이머 인터럽트

과목: 리눅스
날짜: 2023년 5월 16일
주요 학습 내용: 디바이스 드라이버
책 이름: ARM으로 배우는 임베디드 리눅스
텍스트: 책 한번 읽어보기 1장 내용

## 서론

- 구조체
- 함수포인트 c책에 있음
- 이런 식으로 구조체 생성과 동시에 .owner =~~ 로 초기화 해줄 수 있음

![Untitled](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/9d6c6156-dd3b-43e8-af80-d26cc65cc4f2)

## 본론

- 주번호 부번호
- 부번호가 없는 것은 하드디스크 전체를 뜻함
![Untitled 1](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/800342f4-bd95-4b6f-bd0d-1e59a99bcd49)
![Untitled 2](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/6d155178-5027-4518-a62d-633b74f6cf0e)

- 디바이스 드라이버간의 호출관계
![Untitled 3](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/562a12cb-2fac-4320-85a6-dbe4b9aabbdd)

- misc 의 주번호 확인

![Untitled 4](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/03272534-ddaf-426b-91c4-dee192c958bc)

---

### ioctl 함수

- 디바이스의 설정값을 바꿀 때 쓰는 함수

![Untitled 5](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/314b486f-3001-4b2a-8875-1c02fe4454a0)

- _*IO(IOCTLTEST-MAGIC, 0)*
    - _IO 앞에 2bit, 00, 01, 10, 11로 앞의 2비트 값
    - 매개변수가 없으므로 14비트에는 모두 0이 들어감
    - *IOCTLTEST-MAGIC 매직넘버로 위에 매크로로 ‘t’값을 넣어줘서 ascii코드 값인 0x74값이 들어가서 8bit에 표현함*
    - 나머지 구분번호는 0으로 8비트 표현
    - 위의 코드값을 2진수로 표현하면 00000000000000000111010000000000  으로 총 32비트 구성으로 되어있다

![Untitled 6](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/9d099ba7-1dd5-4686-8e57-8c613e5856e6)
![Untitled 7](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/ce5464b9-64e6-42fc-ac3c-bb32625b3572)


- ioctl.h 파일

![Untitled 8](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/a19d2c78-2c02-4292-a9a3-58203b6f0164)

- /home/udooer/kernel/linux_kernel-3.14-1.0.x-udoo/ 에서 vi .config 파일 확인

![Untitled 9](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/298cec73-1a7d-4120-84b8-d2face496624)

- 커널 타이머에서 사용하는 주파수

- 타이머 인터럽트
- get_jiffies_64() 는 전역변수 값을 읽어오는 함수
- 최소단위가 10ms 커널타이머보다 더 짧은 시간은 표현할 수 없음


![Untitled 10](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/ab3e7d54-6c00-4cf1-9e1e-3491bdad4c89)
![Untitled 11](https://github.com/Bchain91/DeviceDriverStudy/assets/123140313/a592d4bd-6356-41bc-90b1-05f36d17375c)



