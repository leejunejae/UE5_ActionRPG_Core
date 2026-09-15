# 무기 장착 조합 이행 가이드

## 데이터 책임

- `WeaponDataAsset`은 물리 무기 분류(`WeaponCategory`), 허용 손(`AllowedSlot`), 파지 능력(`GripType`, `bCanEquipOffHand`)과 외형/Trail/Audio를 소유한다.
- `PlayerAnimSetDataAsset`은 주무기 분류 + 보조무기 분류 + 현재 파지 방식에 대응하는 `CombatStyleRules`를 소유한다.
- 플레이어 공격·피격·애니메이션 데이터는 `CombatStyle` 태그를 키로 사용한다.
- NPC의 공격 패턴과 공격 애니메이션 선택은 기존 NPC 전용 데이터가 계속 소유한다. NPC의 `OffHandWeaponData`는 외형, Trail, 무기 동작음 소스만 제공한다.

## 기존 콘텐츠 호환

기존 `EWeaponType`, `AnimList`, `AttackContextMap`, `HitReactionMap`, `DefaultProfiles`, `SubMesh`, `SubConfig`는 직렬화된 에셋을 깨뜨리지 않기 위한 폴백이다. 새 필드를 채우지 않은 기존 에셋은 이전 무기 유형을 `CombatStyle`과 물리 분류로 변환해 계속 동작한다.

기존 검·방패 묶음 에셋을 독립 장비로 옮길 때는 다음 순서로 작업한다.

1. 검 에셋에 `WeaponCategory = Sword`, `GripType = OneHanded` 또는 `Versatile`, `AllowedSlot = MainHand`, `bCanEquipOffHand = true`를 설정한다.
2. 방패용 `WeaponDataAsset`과 무기 데이터 테이블 행을 만들고 `WeaponCategory = Shield`, `AllowedSlot = OffHand`를 설정한다.
3. `PlayerAnimSetDataAsset.CombatStyleRules`에 `Sword + Shield + OneHanded -> CombatStyle.Sword.Shield` 규칙을 추가한다.
4. 같은 에셋의 `CombatStyleAnimList`, 플레이어 공격 데이터의 `CombatStyleAttackContextMap`, 피격 데이터의 `CombatStyleHitReactionMap`에 대응 태그 항목을 추가한다.
5. 플레이어 기본 장비라면 `DefaultOffHandWeaponKey`에 방패 행 키를 설정한다.
6. 런타임 확인이 끝난 뒤에만 기존 검 에셋의 `HasSubWeapon` 데이터를 제거한다.

## 현재 범위

- 플레이어는 독립 주무기/보조무기, 장비 무게, 공격 소스별 피해 데이터, Trail/Audio, 미리보기 UI를 지원한다.
- 파지 상태 변경은 데이터와 전투 프로필을 갱신하며, 양손 파지로 바꾸면 독립 보조 슬롯을 해제한다. 기존 `HasSubWeapon` 묶음 에셋만 이행 기간에 메시 가시성으로 처리한다. 전환 입력과 전환 몽타주는 후속 구현 범위다.
- 별도의 장비 저장/불러오기 구현은 현재 프로젝트 소스에 없으므로 이번 변경에서 저장 포맷 마이그레이션은 발생하지 않는다.
