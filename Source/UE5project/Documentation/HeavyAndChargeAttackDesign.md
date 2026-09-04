# 강공격 및 차지 공격 설계

## 목표

- 좌클릭은 약공격, Shift+좌클릭은 강공격으로 판정한다.
- 공격 버튼이 눌린 순간 공격 종류를 확정하며, 이후 Shift를 먼저 놓아도 약공격으로 바뀌지 않는다.
- 즉발 강공격과 `Begin -> Loop -> Attack/End` 형식의 차지 강공격을 같은 공격 실행 흐름에 편입한다.
- 약공격, 강공격, 이후 추가할 달리기/점프 공격은 모두 `Action.Attack` 생명주기를 공유한다.
- 입력 함수와 실행 함수는 기존 프로젝트 규칙인 `~Input -> Execute~` 명명 방식을 유지한다.

## 입력과 공격 선택

| 입력 | 확정 공격 이름 | 비고 |
| --- | --- | --- |
| 좌클릭 | `DefaultCombo` | 이 경로에서만 처형을 먼저 시도 |
| Shift+좌클릭 | `HeavyAttack` | Shift 상태는 좌클릭 Started 시점에 스냅샷 |

향후 달리기/점프 공격은 입력 함수를 늘리지 않고 공격 이름을 결정하는 Resolver에 조건을 추가한다.

`CharacterStatusComponent`의 버퍼는 Action 태그만 저장하므로 `PlayerBase`가 `PendingAttackName`을 별도로 보관한다. 버퍼가 소비될 때 공격 이름을 다시 계산하지 않는다.

## 책임 분리

### PlayerBase

- Enhanced Input Started/Completed/Canceled 수신
- 공격 이름 확정 및 `Action.Attack` 요청
- 차지 시간과 보류된 전환 요청 관리
- 스태미나 사전 검사와 실제 공격 커밋 시 소비

### AttackComponent

- 공격 데이터와 콤보 인덱스 해석
- 동일 몽타주의 차지 준비 세션 재생
- 준비 세션을 실제 공격 세션으로 커밋
- Attack/End 섹션 전환, 피격 트레이스, 몽타주 종료 및 강제 중단 정리
- 커밋 시 확정된 공격 배율 보관

## 데이터 구조

`FBaseAttackData::SectionName`은 실제 타격이 발생하는 Attack 섹션이다. 하나의 차지 공격도 논리적으로 하나의 공격 데이터이다.

```cpp
FBaseAttackData
  SectionName                 // 실제 Attack 섹션
  AttackModifiers             // 비차지 공격 배율
  bCanCharge
  ChargeSettings
    BeginSectionName
    LoopSectionName
    EndSectionName
    MaxChargeDuration
    MinimumModifiers
    MaximumModifiers
```

`bCanCharge == false`이면 `AttackModifiers`를 사용한다. `true`이면 0~1 차지 비율로 최소/최대 배율을 보간한다.

기존의 개별 스칼라 배율 필드는 제거했으며, 모든 공격 배율은 `FAttackModifiers`를 통해서만 설정한다.

## 차지 생명주기

```text
Started
  -> Action.Attack 승인
  -> Begin 재생, 차지 시간 측정 시작
  -> Begin 종료 후 Loop

Completed 또는 최대 차지 시간
  -> Attack 요청 저장
  -> Begin 중이면 Loop 진입 시 커밋
  -> Loop 중이면 즉시 커밋
  -> 실제 Attack 섹션 재생

Canceled
  -> End 요청 저장
  -> Begin 중이면 Begin 종료 후 End
  -> Loop 중이면 즉시 End
```

`Completed`는 버튼을 정상적으로 놓은 경우다. 최대 차지 도달은 입력 이벤트를 위조하지 않고 같은 Attack 전환 요청을 내부에서 만든다.

Enhanced Input의 `Canceled`는 입력 트리거 생명주기일 뿐 피격/사망과 동일하지 않다. 피격, 사망, 처형, 강제 Action 전환은 기존 `ExitActionRuntime` 경로로 차지 준비를 즉시 중단하며 End 섹션을 재생하지 않는다.

## 스태미나와 배율 확정

- 차지 시작 전 최소 차지 배율 기준 스태미나를 보유했는지 확인한다.
- 실제 Attack 섹션으로 커밋할 때 차지 비율을 고정한다.
- 고정된 배율로 스태미나를 소비하고, 이후 버튼 상태나 시간이 바뀌어도 피해/Poise/Stance 배율은 변하지 않는다.
- 커밋 시 필요한 스태미나가 부족하면 공격을 실행하지 않고 End로 종료한다.

## 몽타주 요구사항

- 즉발 공격: 기존처럼 `SectionName`만 필요하다.
- 차지 공격: 같은 몽타주 안에 Begin, Loop, Attack, End 섹션이 모두 있어야 한다.
- Begin의 다음 섹션은 Loop, Loop는 자기 자신으로 연결한다.
- 런타임에서도 연결을 설정해 에셋의 실수로 인한 조기 종료를 방지한다.
- 별도 Anim Notify 없이 현재 몽타주 섹션이 Loop로 바뀐 시점을 감지해 보류 요청을 처리한다.

## 콤보 입력 윈도우

- 콤보는 새로운 Action 전환이 아니라 현재 `Action.Attack` 안에서 같은 `AttackName`의 다음 인덱스로 진행하는 동작이다.
- `ANS_AttackComboWindow`는 `AttackComponent`의 콤보 입력 허용 구간만 열고 닫으며 공용 Action Window 태그를 사용하지 않는다.
- 윈도우가 열린 동안 현재 `AttackName`과 입력으로 요청된 `AttackName`이 같고 `ComboIndex + 1`이 존재하면 즉시 다음 공격을 실행한다.
- 윈도우 직전 입력은 공격 이름을 짧게 보관했다가 Notify Begin에서 조건을 다시 확인하고 즉시 소비한다.
- 마지막 콤보 단계는 다음 인덱스가 없으므로 Notify가 잘못 배치되어도 콤보가 실행되지 않는다.
- 일반 `Window.Attack`은 강·약 구분 없이 새로운 공격 Action을 허용하는 회복 구간으로 사용한다.

## 이번 구현 범위

- 약공격/강공격 선택과 입력 스냅샷
- 차지 데이터, 준비/커밋/End 흐름
- 차지 비율 기반 Damage/Poise/Stance/Stamina 배율
- 액션 버퍼와 강제 중단 연동

- Shift 달리기 바인딩을 제거하고 Space 탭은 회피, 일정 시간 홀드는 달리기로 처리한다.
- 승마도 플레이어의 같은 `Dodge` Input Action을 공유한다. 승마에는 아직 회피 동작이 없으므로 Space 탭은 소비만 하고, 홀드 시에만 승마 Sprint를 활성화한다.

달리기/점프 공격 Resolver 자체는 이번 공격 기능을 안정화한 뒤 별도 단계에서 추가한다.
