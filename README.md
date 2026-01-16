#🕵️‍♂️ SHATTERED MIND
Unreal Engine 5.6 기반 3인칭 미스터리 수사 게임 > "폐쇄된 정신병원, 그곳에 남겨진 단서를 통해 진실을 파헤치는 형사의 이야기"

(▲ 여기에 게임 타이틀 화면이나 대표 스크린샷 이미지 주소를 넣으세요)

##📝 Project Overview (프로젝트 개요)
개발 기간: 2025.09 ~ 2025.11 (약 2개월)

개발 인원: 1인 개발 (기획, 프로그래밍, 레벨 디자인)

개발 환경: Unreal Engine 5.6, Visual Studio 2022 (C++)

장르: 3인칭 어드벤처 / 미스터리

핵심 키워드: Investigation, Puzzle, Story-driven

##📺 Gameplay Video
(▲ 클릭 시 유튜브 영상으로 이동합니다. YOUR_VIDEO_ID 부분을 유튜브 영상 ID로 교체하세요)

##⚙️ Key Features & Tech Stack (핵심 기술 구현)
1. 확장성 있는 상호작용 시스템 (Interaction Architecture)
LineTrace & Interface 기반 설계: LineTraceSingleByChannel을 사용하여 플레이어 시선 기준의 정밀한 객체 감지를 구현했습니다.

Decoupling(의존성 분리): IInteractionInterface를 도입하여 플레이어 클래스가 특정 액터(NPC, 아이템, 문 등)에 강하게 결합되는 것을 방지하고, 상호작용 대상의 확장성을 확보했습니다.

Visual Feedback: PostProcess Material을 활용하여 상호작용 가능한 객체에 아웃라인(Outline) 효과를 동적으로 적용했습니다.

2. Context-Sensitive Input & Camera (상황별 입력 제어)
동적 입력 매핑: 일반 탐색 모드와 퍼즐(자물쇠) 모드 간의 부드러운 전환을 위해 SetViewTargetWithBlend와 입력 모드 스위칭 로직을 구현했습니다.

마우스 휠 활용: 동일한 마우스 휠 입력이 상황에 따라 '카메라 줌(Zoom)' 또는 '자물쇠 회전(Rotate Lock)'으로 다르게 동작하도록 분기 처리하여 직관적인 UX를 제공했습니다.

3. 데이터 기반 UI 시스템 (Data-Driven UI)
Observer Pattern 응용: 단서 획득 시 데이터를 갱신하고, 델리게이트(Delegate)를 통해 UI 위젯(수첩, HUD)에 즉시 변경 사항을 전파하는 구조입니다.

Manager Class: UnlockMemoWrap 함수 등을 통해 데이터 무결성을 검증(중복 해금 방지)한 후 UI를 업데이트하도록 설계했습니다.

##🛠️ Trouble Shooting (문제 해결)
🚀 NPC 상호작용 안정성 확보 (Interaction Stability)
문제 상황 (Issue) > NPC가 플레이어를 바라보기 위해 회전하거나 애니메이션을 재생할 때, Mesh의 불규칙한 형태 때문에 LineTrace 판정이 끊기거나 불안정한 현상이 발생했습니다.

해결 방법 (Solution) > **'판정과 시각화의 분리(Decoupling Logic from Visuals)'**를 적용했습니다.

NPC의 SkeletalMesh는 충돌 판정(Trace)에서 제외(Ignore)시킵니다.

형태가 일정한 CapsuleComponent를 별도로 두어 상호작용 판정 전용(QueryOnly)으로 설정했습니다.

물리적 밀림 방지를 위해 PhysicsInteraction을 비활성화하여 안정적인 대화 환경을 구축했습니다.

C++

// NPC.cpp - 충돌체 설정 예시
void ANPC::SetupCollisionForMovingNPC()
{
    // 판정 안정화를 위한 캡슐 컴포넌트 설정
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 물리 연산 제외, 쿼리만 수행
    Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 캐릭터가 끼이는 현상 방지
}

##📂 Source Code Structure
APolice: 메인 캐릭터 컨트롤러. 입력 처리 및 상호작용 레이캐스팅 담당.

ANPC: FSM 기반은 아니지만, 상태에 따라 대화 및 회전 로직 수행.

APickUp: 아이템 및 단서 객체. 인터페이스 구현체.

UI_System: PoliceMemoWidget, PoliceHUD 등 데이터 바인딩 기반 위젯 클래스.

📬 Contact
Email: namsoeun012@gmail.com

Portfolio: 
