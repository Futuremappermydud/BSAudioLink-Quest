#include "AssetBundleManager.hpp"

#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "UnityEngine/AssetBundleRequest.hpp"
#include "UnityEngine/AssetBundleCreateRequest.hpp"

#include "assets.hpp"

DEFINE_TYPE(AudioLink, AssetBundleManager);

extern Logger& getLogger();

namespace AudioLink {
    SafePtr<AssetBundleManager> AssetBundleManager::instance;
    
    AssetBundleManager* AssetBundleManager::get_instance() {
        if (!instance) {
            instance = AssetBundleManager::New_ctor();
        }
        return instance.ptr();
    }

    UnityEngine::Material* AssetBundleManager::get_material() {
        return _material;
    }

    UnityEngine::RenderTexture* AssetBundleManager::get_renderTexture() {
        return _renderTexture;
    }

    void AssetBundleManager::StartLoad(std::function<void()> onFinished) {
        if (_bundle && _bundle->m_CachedPtr.m_value) {
            onFinished();
            return;
        }

        getLogger().info("AssetBundleManager StartLoad");
        GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(LoadAsync(onFinished)));
    }

    void AssetBundleManager::RestartGame() {
        getLogger().info("AssetBundleManager RestartGame");
        if (_bundle && _bundle->m_CachedPtr.m_value) {
            _bundle->Unload(true);
        }

        if (_material && _material->m_CachedPtr.m_value) {
            UnityEngine::Object::DestroyImmediate(_material);
        }

        if (_renderTexture && _renderTexture->m_CachedPtr.m_value) {
            UnityEngine::Object::DestroyImmediate(_renderTexture);
        }
    }
    
    custom_types::Helpers::Coroutine AssetBundleManager::LoadAsync(std::function<void()> onFinished) {
        getLogger().info("AssetBundleManager LoadAsync");
        using AssetBundle_LoadFromMemoryAsync = function_ptr_t<UnityEngine::AssetBundleCreateRequest*, ArrayW<uint8_t>, int>;
        static AssetBundle_LoadFromMemoryAsync assetBundle_LoadFromMemoryAsync = reinterpret_cast<AssetBundle_LoadFromMemoryAsync>(il2cpp_functions::resolve_icall("UnityEngine.AssetBundle::LoadFromMemoryAsync_Internal"));

        auto req = assetBundle_LoadFromMemoryAsync(IncludedAssets::Bundle, 0);
        req->set_allowSceneActivation(true);
        co_yield reinterpret_cast<System::Collections::IEnumerator*>(req);
        getLogger().info("AssetBundleManager Bundle Loaded");
        _bundle = req->get_assetBundle();
        UnityEngine::Object::DontDestroyOnLoad(_bundle);

        if (!_bundle && !_bundle->m_CachedPtr.m_value) {
            co_return;
        }
        
        auto matReq = _bundle->LoadAssetAsync<UnityEngine::Material*>("assets/audiolink/materials/mat_audiolink.mat");
        matReq->set_allowSceneActivation(true);
        co_yield reinterpret_cast<System::Collections::IEnumerator*>(matReq);
        getLogger().info("AssetBundleManager Material Loaded");
        // FIXME: check if need for instantiation
        _material = reinterpret_cast<UnityEngine::Material*>(matReq->get_asset());
        UnityEngine::Object::DontDestroyOnLoad(_material);

        auto texReq = _bundle->LoadAssetAsync<UnityEngine::RenderTexture*>("assets/audiolink/rendertextures/rt_audiolink.asset");
        texReq->set_allowSceneActivation(true);
        co_yield reinterpret_cast<System::Collections::IEnumerator*>(texReq);
        getLogger().info("AssetBundleManager RenderTexture Loaded");
        // FIXME: check if need for instantiation
        _renderTexture = reinterpret_cast<UnityEngine::RenderTexture*>(texReq->get_asset());
        UnityEngine::Object::DontDestroyOnLoad(_renderTexture);

        getLogger().info("Loaded material: %p", _material);
        getLogger().info("Loaded texture: %p", _renderTexture);

        getLogger().info("AssetBundleManager onFinished");
        onFinished();
        co_return;
    }
}