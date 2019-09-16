# Linear Wireless Sensor Network 환경에서 TDMA
**https://www.nsnam.org/**
NS-3 오픈소스를 활용하여 시뮬레이션을 구축하였으며, 수정한 일부분만 github에 올려두었습니다.

## TDMA 특징
* 각 Node가 주어진 시간에 전송한다
* 전송이 실패하진 않으나 아무것도 전송되지 않는 시간을 기다려야한다
* 전송할 데이터가 많이 없는 상황에서는 효율이 좋다

## 주요 코드
* src/network/utils/simple-net-device.cc
