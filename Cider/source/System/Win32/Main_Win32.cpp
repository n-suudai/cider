
#include "Cider.hpp"

#include <tchar.h>
#include <Windows.h>


int APIENTRY _tWinMain(
    _In_        HINSTANCE hInstance,
    _In_opt_    HINSTANCE hPrevInstance,
    _In_        LPWSTR    lpCmdLine,
    _In_        int       nCmdShow
)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    //Cider::System::Signal<void()> signal;

    //signal.Connect(Cider::Hello);

    //signal();

    auto entityManager = Cider::GameSystem::EntityManager::Instance();

    // エンティティの生成
    auto entityId = entityManager->CreateEntity();

    // コンポーネントの登録
    entityManager->RegisterComponent(entityId, "TestComponentA");

    // 更新イベントの発行
    entityManager->BroadcastEvent(Cider::GameSystem::OnUpdate { 0.0 });

    // コンポーネントの削除
    entityManager->DestroyEntity(entityId);

    // イベントを実行
    entityManager->DispatchEvent();

    CIDER_ASSERT(false, "強制失敗！！");

    return EXIT_SUCCESS;
}

