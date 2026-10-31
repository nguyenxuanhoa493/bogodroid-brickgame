#include "gplay.h"
#include "javac.h"
#include "jnibridge.h"
#include "logging.h"
#include "toml++/toml.hpp"
#include <cstdlib>
#include <typeinfo>

// Only the Unity-based projects ship unity.h; the GameMaker/Limbo loaders
// compile this file too, so the erased UnityPlayerActivity overloads are
// conditional instead of unconditional.
#if __has_include("unity.h")
#include "unity.h"
#define BD_GPLAY_HAVE_UNITY_ACTIVITY 1
#endif

extern toml::table config;

using namespace jnivm::com::google::android::vending::licensing;
using namespace jnivm::com::billingclient::api;
using namespace jnivm::bitter::jnibridge;

AESObfuscator::AESObfuscator(std::shared_ptr<FakeJni::JByteArray>,
                             std::shared_ptr<FakeJni::JString>,
                             std::shared_ptr<FakeJni::JString>) {}

ServerManagedPolicy::ServerManagedPolicy(
    std::shared_ptr<jnivm::android::content::Context>,
    std::shared_ptr<Obfuscator>) {}

APKExpansionPolicy::APKExpansionPolicy(
    std::shared_ptr<jnivm::android::content::Context>,
    std::shared_ptr<Obfuscator>) {}

LicenseChecker::LicenseChecker(
    std::shared_ptr<jnivm::android::content::Context>,
    std::shared_ptr<Policy>,
    std::shared_ptr<FakeJni::JString>) {}

void LicenseChecker::checkAccess(std::shared_ptr<LicenseCheckerCallback> callback)
{
    if (!callback) {
        BD_LOG("GPLAY", "LVL checkAccess received a null callback");
        return;
    }

    if (config["google_play"]["offline"].value_or<bool>(false)) {
        BD_LOG("GPLAY", "offline LVL entitlement enabled; returning LICENSED");
        callback->allow(Policy::LICENSED);
        return;
    }

    BD_LOG("GPLAY", "Google Play unavailable and offline entitlement disabled");
    callback->applicationError(3); // ERROR_NOT_MARKET_MANAGED
}

void LicenseChecker::onDestroy() {}

jnivm::com::google::android::play::core::tasks::Task::Task(std::shared_ptr<FakeJni::JObject> res)
{
    this->result = res;
}

// Unity's Play Games plugin wraps its C# callbacks in AndroidJavaProxy objects
// whose Java interface is com.google.android.gms.tasks.OnSuccessListener etc.
// Those are JNIBridge proxies, not the play-core listener types, so the
// callbacks are forwarded by name through the same invoker the engine uses.
// The fake task is already complete: delivering on registration is what the
// real (already finished) Java task would do.
static void bd_task_deliver(std::shared_ptr<FakeJni::JObject> listener,
                            const char* className, const char* methodName,
                            const char* methodSig,
                            std::shared_ptr<FakeJni::JObject> arg)
{
    if (!listener) {
        BD_LOG("GPLAY", "%s.%s: no listener", className, methodName);
        return;
    }
    auto proxy = std::dynamic_pointer_cast<JNIBridgeProxy>(listener);
    if (!proxy) {
        BD_LOG("GPLAY", "%s.%s: listener is %s, not a proxy; dropped",
               className, methodName, typeid(*listener).name());
        return;
    }
    BD_LOG("GPLAY", "%s.%s: delivering", className, methodName);
    proxy->deliver(className, methodName, methodSig,
                   std::static_pointer_cast<jnivm::java::lang::Object>(arg));
}

std::shared_ptr<jnivm::com::google::android::play::core::tasks::Task> jnivm::com::google::android::play::core::tasks::Task::addOnCompleteListener(std::shared_ptr<FakeJni::JObject> listener)
{
    this->onCompleteListener = listener;
    bd_task_deliver(listener, "com/google/android/gms/tasks/OnCompleteListener",
                    "onComplete", "(Lcom/google/android/gms/tasks/Task;)V",
                    std::static_pointer_cast<FakeJni::JObject>(shared_from_this()));
    return std::static_pointer_cast<Task>(shared_from_this());
}

std::shared_ptr<jnivm::com::google::android::play::core::tasks::Task> jnivm::com::google::android::play::core::tasks::Task::addOnSuccessListener(std::shared_ptr<FakeJni::JObject> listener)
{
    this->onSuccessListener = listener;
    bd_task_deliver(listener, "com/google/android/gms/tasks/OnSuccessListener",
                    "onSuccess", "(Ljava/lang/Object;)V", this->result);
    return std::static_pointer_cast<Task>(shared_from_this());
}

std::shared_ptr<jnivm::com::google::android::play::core::tasks::Task> jnivm::com::google::android::play::core::tasks::Task::addOnFailureListener(std::shared_ptr<FakeJni::JObject> listener)
{
    // Nothing fails here: every fake task completes successfully.
    this->onFailureListener = listener;
    return std::static_pointer_cast<Task>(shared_from_this());
}

bool jnivm::com::google::android::play::core::tasks::Task::isSuccessful()
{
    return true;
}

std::shared_ptr<FakeJni::JObject> jnivm::com::google::android::play::core::tasks::Task::getResult()
{
    return this->result;
}



std::shared_ptr<jnivm::com::google::android::play::core::review::ReviewManager> jnivm::com::google::android::play::core::review::ReviewManagerFactory::create(std::shared_ptr<jnivm::android::content::Context> context)
{
    return std::make_shared<jnivm::com::google::android::play::core::review::ReviewManager>();
}

std::shared_ptr<jnivm::com::google::android::gms::games::AchievementsClient> jnivm::com::google::android::gms::games::PlayGames::getAchievementsClient(std::shared_ptr<jnivm::android::app::Activity> activity)
{
    return std::make_shared<jnivm::com::google::android::gms::games::AchievementsClient>();
}

std::shared_ptr<jnivm::com::google::android::gms::games::EventsClient> jnivm::com::google::android::gms::games::PlayGames::getEventsClient(std::shared_ptr<jnivm::android::app::Activity> activity)
{
    return std::make_shared<jnivm::com::google::android::gms::games::EventsClient>();
}

std::shared_ptr<jnivm::com::google::android::gms::games::GamesSignInClient> jnivm::com::google::android::gms::games::PlayGames::getGamesSignInClient(std::shared_ptr<jnivm::android::app::Activity> activity)
{
    return std::make_shared<jnivm::com::google::android::gms::games::GamesSignInClient>();
}

std::shared_ptr<jnivm::com::google::android::gms::games::LeaderboardsClient> jnivm::com::google::android::gms::games::PlayGames::getLeaderboardsClient(std::shared_ptr<jnivm::android::app::Activity> activity)
{
    return std::make_shared<jnivm::com::google::android::gms::games::LeaderboardsClient>();
}

std::shared_ptr<jnivm::com::google::android::gms::games::PlayerStatsClient> jnivm::com::google::android::gms::games::PlayGames::getPlayerStatsClient(std::shared_ptr<jnivm::android::app::Activity> activity)
{
    return std::make_shared<jnivm::com::google::android::gms::games::PlayerStatsClient>();
}

std::shared_ptr<jnivm::com::google::android::gms::games::PlayersClient> jnivm::com::google::android::gms::games::PlayGames::getPlayersClient(std::shared_ptr<jnivm::android::app::Activity> activity)
{
    return std::make_shared<jnivm::com::google::android::gms::games::PlayersClient>();
}

std::shared_ptr<jnivm::com::google::android::gms::games::SnapshotsClient> jnivm::com::google::android::gms::games::PlayGames::getSnapshotsClient(std::shared_ptr<jnivm::android::app::Activity> activity)
{
    return std::make_shared<jnivm::com::google::android::gms::games::SnapshotsClient>();
}

BEGIN_NATIVE_DESCRIPTOR(Obfuscator)
    END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(AESObfuscator) {
    FakeJni::Constructor<AESObfuscator, std::shared_ptr<FakeJni::JByteArray>,
                         std::shared_ptr<FakeJni::JString>, std::shared_ptr<FakeJni::JString>> {}
},
    END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(Policy)
    END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(ServerManagedPolicy) {
    FakeJni::Constructor<ServerManagedPolicy,
                         std::shared_ptr<jnivm::android::content::Context>,
                         std::shared_ptr<Obfuscator>> {}
},
    END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(APKExpansionPolicy) {
    FakeJni::Constructor<APKExpansionPolicy,
                         std::shared_ptr<jnivm::android::content::Context>,
                         std::shared_ptr<Obfuscator>> {}
},
    END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(LicenseCheckerCallback)
    { FakeJni::Function<&LicenseCheckerCallback::allow> {}, "allow", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&LicenseCheckerCallback::dontAllow> {}, "dontAllow", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&LicenseCheckerCallback::applicationError> {}, "applicationError", FakeJni::JMethodID::PUBLIC },
    END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(LicenseChecker) {
    FakeJni::Constructor<LicenseChecker,
                         std::shared_ptr<jnivm::android::content::Context>,
                         std::shared_ptr<Policy>, std::shared_ptr<FakeJni::JString>> {}
},
    { FakeJni::Function<&LicenseChecker::checkAccess> {}, "checkAccess", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&LicenseChecker::onDestroy> {}, "onDestroy", FakeJni::JMethodID::PUBLIC },
    END_NATIVE_DESCRIPTOR

void jnivm::com::google::android::gms::games::PlayGamesSdk::initialize(std::shared_ptr<jnivm::android::content::Context> context)
{
    // Initialize the Play Games SDK with the provided context
}

std::shared_ptr<jnivm::com::google::android::play::core::tasks::Task> jnivm::com::google::android::gms::games::GamesSignInClient::isAuthenticated()
{
    return std::make_shared<jnivm::com::google::android::play::core::tasks::Task>(std::make_shared<jnivm::com::google::android::gms::games::AuthenticationResult>());
}

bool jnivm::com::google::android::gms::games::AuthenticationResult::isAuthenticated()
{
    // No Play Games account on this device: the plugin's Authenticate() ends
    // with "not signed in" instead of throwing, and the game keeps booting.
    return false;
}

// Google Play Billing: inert objects only. See the comment in gplay.h.
std::shared_ptr<BillingClientBuilder> BillingClient::newBuilder(std::shared_ptr<jnivm::android::content::Context> context)
{
    BD_LOG("GPLAY", "billing newBuilder(%p) -> inert builder", context.get());
    return std::make_shared<BillingClientBuilder>();
}

#ifdef BD_GPLAY_HAVE_UNITY_ACTIVITY
std::shared_ptr<jnivm::Object> BillingClient::newBuilder(
    std::shared_ptr<jnivm::com::unity3d::player::UnityPlayerActivity> activity)
{
    BD_LOG("GPLAY", "billing newBuilder(activity=%p, erased) -> inert builder", activity.get());
    return std::static_pointer_cast<jnivm::Object>(std::make_shared<BillingClientBuilder>());
}
#endif

std::shared_ptr<BillingClientBuilder> BillingClientBuilder::setListener(std::shared_ptr<FakeJni::JObject> listener)
{
    BD_LOG("GPLAY", "billing setListener(%p)", listener.get());
    return std::static_pointer_cast<BillingClientBuilder>(shared_from_this());
}

std::shared_ptr<BillingClientBuilder> BillingClientBuilder::enablePendingPurchases(std::shared_ptr<PendingPurchasesParams> params)
{
    BD_LOG("GPLAY", "billing enablePendingPurchases(%p)", params.get());
    return std::static_pointer_cast<BillingClientBuilder>(shared_from_this());
}

std::shared_ptr<BillingClient> BillingClientBuilder::build()
{
    BD_LOG("GPLAY", "billing build() -> inert BillingClient");
    return std::make_shared<BillingClient>();
}

std::shared_ptr<jnivm::Object> BillingClientBuilder::enablePendingPurchasesNoArg()
{
    BD_LOG("GPLAY", "billing enablePendingPurchases(erased)");
    return std::static_pointer_cast<jnivm::Object>(shared_from_this());
}

std::shared_ptr<jnivm::Object> BillingClientBuilder::buildErased()
{
    BD_LOG("GPLAY", "billing build(erased) -> inert BillingClient");
    return std::static_pointer_cast<jnivm::Object>(std::make_shared<BillingClient>());
}

// Diagnostic switch for games whose boot waits on Unity IAP reaching
// "connected": with BD_BILLING_OK=1 the store reports OK, isReady() is true and
// the Billing 5 listener callbacks answer with empty product/purchase lists, so
// initialization completes instead of retrying a store that cannot exist here.
// Without the switch every call still reports "store unavailable" honestly.
static bool bd_billing_ok()
{
    static const bool ok = std::getenv("BD_BILLING_OK") != nullptr;
    return ok;
}

static std::shared_ptr<FakeJni::JObject> bd_empty_list()
{
    return std::static_pointer_cast<FakeJni::JObject>(
        std::make_shared<jnivm::java::util::ArrayList>());
}

static std::shared_ptr<BillingResult> bd_ok_result(const char* message)
{
    auto result = std::make_shared<BillingResult>();
    result->responseCode = 0; // BILLING_OK
    result->debugMessage = std::make_shared<FakeJni::JString>(message);
    return result;
}

void BillingClient::startConnection(std::shared_ptr<FakeJni::JObject> listener)
{
    // No Play Store on this device. Report setup failure straight away: a
    // never-completing listener leaves Unity IAP awaiting a connection that
    // cannot happen, which is worse than an honest "store unavailable".
    // (BD_BILLING_OK trades that honesty for a bootable game, see above.)
    const bool ok = bd_billing_ok();
    BD_LOG("GPLAY", "billing startConnection(%p): delivering %s", listener.get(),
           ok ? "OK (BD_BILLING_OK)" : "BILLING_UNAVAILABLE");
    auto proxy = std::dynamic_pointer_cast<JNIBridgeProxy>(listener);
    if (!proxy) {
        if (listener)
            BD_LOG("GPLAY", "billing listener is %s, not a proxy", typeid(*listener).name());
        return;
    }
    auto result = ok ? bd_ok_result("offline stub: store ready")
                     : std::make_shared<BillingResult>();
    if (!ok) result->debugMessage = std::make_shared<FakeJni::JString>("offline: no Play Store");
    proxy->deliver("com/android/billingclient/api/BillingClientStateListener",
                   "onBillingSetupFinished",
                   "(Lcom/android/billingclient/api/BillingResult;)V", result);
}

bool BillingClient::isReady()
{
    return bd_billing_ok();
}

void BillingClient::endConnection() {}

void BillingClient::queryPurchasesAsync(std::shared_ptr<FakeJni::JObject>,
                                        std::shared_ptr<FakeJni::JObject> listener)
{
    if (!bd_billing_ok()) { BD_LOG("GPLAY", "billing queryPurchasesAsync ignored"); return; }
    auto proxy = std::dynamic_pointer_cast<JNIBridgeProxy>(listener);
    if (!proxy) { BD_LOG("GPLAY", "queryPurchases listener is not a proxy"); return; }
    BD_LOG("GPLAY", "billing queryPurchasesAsync -> empty list");
    proxy->deliver("com/android/billingclient/api/PurchasesResponseListener",
                   "onQueryPurchasesResponse",
                   "(Lcom/android/billingclient/api/BillingResult;Ljava/util/List;)V",
                   bd_ok_result("offline stub: no purchases"), bd_empty_list());
}

void BillingClient::queryProductDetailsAsync(std::shared_ptr<FakeJni::JObject>,
                                             std::shared_ptr<FakeJni::JObject> listener)
{
    if (!bd_billing_ok()) { BD_LOG("GPLAY", "billing queryProductDetailsAsync ignored"); return; }
    auto proxy = std::dynamic_pointer_cast<JNIBridgeProxy>(listener);
    if (!proxy) { BD_LOG("GPLAY", "product details listener is not a proxy"); return; }
    BD_LOG("GPLAY", "billing queryProductDetailsAsync -> empty list");
    proxy->deliver("com/android/billingclient/api/ProductDetailsResponseListener",
                   "onProductDetailsResponse",
                   "(Lcom/android/billingclient/api/BillingResult;Ljava/util/List;)V",
                   bd_ok_result("offline stub: no products"), bd_empty_list());
}

void BillingClient::acknowledgePurchase(std::shared_ptr<FakeJni::JObject>,
                                        std::shared_ptr<FakeJni::JObject> listener)
{
    if (!bd_billing_ok()) { BD_LOG("GPLAY", "billing acknowledgePurchase ignored"); return; }
    auto proxy = std::dynamic_pointer_cast<JNIBridgeProxy>(listener);
    if (!proxy) return;
    proxy->deliver("com/android/billingclient/api/AcknowledgePurchaseResponseListener",
                   "onAcknowledgePurchaseResponse",
                   "(Lcom/android/billingclient/api/BillingResult;)V",
                   bd_ok_result("offline stub: acknowledged"));
}

void BillingClient::consumeAsync(std::shared_ptr<FakeJni::JObject>,
                                 std::shared_ptr<FakeJni::JObject> listener)
{
    if (!bd_billing_ok()) { BD_LOG("GPLAY", "billing consumeAsync ignored"); return; }
    auto proxy = std::dynamic_pointer_cast<JNIBridgeProxy>(listener);
    if (!proxy) return;
    proxy->deliver("com/android/billingclient/api/ConsumeResponseListener",
                   "onConsumeResponse",
                   "(Lcom/android/billingclient/api/BillingResult;Ljava/lang/String;)V",
                   bd_ok_result("offline stub: consumed"),
                   std::static_pointer_cast<jnivm::java::lang::Object>(
                       std::make_shared<FakeJni::JString>("")));
}

std::shared_ptr<BillingResult> BillingClient::launchBillingFlow(
    std::shared_ptr<jnivm::android::app::Activity>, std::shared_ptr<FakeJni::JObject>)
{
    if (bd_billing_ok()) { BD_LOG("GPLAY", "billing launchBillingFlow -> OK"); return bd_ok_result("offline stub"); }
    BD_LOG("GPLAY", "billing launchBillingFlow ignored");
    return std::make_shared<BillingResult>();
}

jint BillingResult::getResponseCode()
{
    return responseCode;
}

jint BillingResult::getOnPurchasesUpdatedSubResponseCode()
{
    return subResponseCode;
}

std::shared_ptr<FakeJni::JString> BillingResult::getDebugMessage()
{
    return debugMessage;
}

std::shared_ptr<PendingPurchasesParamsBuilder> PendingPurchasesParams::newBuilder()
{
    return std::make_shared<PendingPurchasesParamsBuilder>();
}

std::shared_ptr<PendingPurchasesParamsBuilder> PendingPurchasesParamsBuilder::enableOneTimeProducts()
{
    return std::static_pointer_cast<PendingPurchasesParamsBuilder>(shared_from_this());
}

std::shared_ptr<PendingPurchasesParams> PendingPurchasesParamsBuilder::build()
{
    return std::make_shared<PendingPurchasesParams>();
}

BEGIN_NATIVE_DESCRIPTOR(BillingClient) {
    FakeJni::Constructor<BillingClient> {},
},
    { FakeJni::Function<static_cast<std::shared_ptr<BillingClientBuilder> (*)(
          std::shared_ptr<jnivm::android::content::Context>)>(
          &BillingClient::newBuilder)> {}, "newBuilder", FakeJni::JMethodID::STATIC },
#ifdef BD_GPLAY_HAVE_UNITY_ACTIVITY
    { FakeJni::Function<static_cast<std::shared_ptr<jnivm::Object> (*)(
          std::shared_ptr<jnivm::com::unity3d::player::UnityPlayerActivity>)>(
          &BillingClient::newBuilder)> {}, "newBuilder", FakeJni::JMethodID::STATIC },
#endif
    { FakeJni::Function<&BillingClient::startConnection> {}, "startConnection", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingClient::isReady> {}, "isReady", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingClient::endConnection> {}, "endConnection", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingClient::queryPurchasesAsync> {}, "queryPurchasesAsync", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingClient::queryProductDetailsAsync> {}, "queryProductDetailsAsync", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingClient::acknowledgePurchase> {}, "acknowledgePurchase", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingClient::consumeAsync> {}, "consumeAsync", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingClient::launchBillingFlow> {}, "launchBillingFlow", FakeJni::JMethodID::PUBLIC },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(BillingClientBuilder) { FakeJni::Constructor<BillingClientBuilder> {} },
    { FakeJni::Function<&BillingClientBuilder::setListener> {}, "setListener", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingClientBuilder::enablePendingPurchases> {}, "enablePendingPurchases", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingClientBuilder::enablePendingPurchasesNoArg> {}, "enablePendingPurchases", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingClientBuilder::build> {}, "build", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingClientBuilder::buildErased> {}, "build", FakeJni::JMethodID::PUBLIC },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(BillingResult) { FakeJni::Constructor<BillingResult> {} },
    { FakeJni::Function<&BillingResult::getResponseCode> {}, "getResponseCode", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingResult::getOnPurchasesUpdatedSubResponseCode> {}, "getOnPurchasesUpdatedSubResponseCode", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&BillingResult::getDebugMessage> {}, "getDebugMessage", FakeJni::JMethodID::PUBLIC },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(PendingPurchasesParams) { FakeJni::Constructor<PendingPurchasesParams> {} },
    { FakeJni::Function<&PendingPurchasesParams::newBuilder> {}, "newBuilder", FakeJni::JMethodID::STATIC },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(PendingPurchasesParamsBuilder) { FakeJni::Constructor<PendingPurchasesParamsBuilder> {} },
    { FakeJni::Function<&PendingPurchasesParamsBuilder::enableOneTimeProducts> {}, "enableOneTimeProducts", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&PendingPurchasesParamsBuilder::build> {}, "build", FakeJni::JMethodID::PUBLIC },
    END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::play::core::review::ReviewManager) { FakeJni::Constructor<ReviewManager> {} },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::play::core::review::ReviewManagerFactory) { FakeJni::Constructor<ReviewManagerFactory> {} },
    { FakeJni::Function<&ReviewManagerFactory::create> {}, "create", FakeJni::JMethodID::STATIC },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::play::core::tasks::Task) { FakeJni::Constructor<Task, std::shared_ptr<FakeJni::JObject>> {} },
    { FakeJni::Function<&Task::addOnCompleteListener> {}, "addOnCompleteListener", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&Task::addOnSuccessListener> {}, "addOnSuccessListener", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&Task::addOnFailureListener> {}, "addOnFailureListener", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&Task::isSuccessful> {}, "isSuccessful", FakeJni::JMethodID::PUBLIC },
    { FakeJni::Function<&Task::getResult> {}, "getResult", FakeJni::JMethodID::PUBLIC },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::play::core::tasks::OnCompleteListener) { FakeJni::Constructor<OnCompleteListener> {} },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::play::core::tasks::OnSuccessListener) { FakeJni::Constructor<OnSuccessListener> {} },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::play::core::tasks::OnFailureListener) { FakeJni::Constructor<OnFailureListener> {} },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::gms::games::AchievementsClient) { FakeJni::Constructor<AchievementsClient> {} },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::gms::games::EventsClient) { FakeJni::Constructor<EventsClient> {} },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::gms::games::AuthenticationResult) { FakeJni::Constructor<AuthenticationResult> {} },
    { FakeJni::Function<&AuthenticationResult::isAuthenticated> {}, "isAuthenticated", FakeJni::JMethodID::PUBLIC },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::gms::games::GamesSignInClient) { FakeJni::Constructor<GamesSignInClient> {} },
    { FakeJni::Function<&GamesSignInClient::isAuthenticated> {}, "isAuthenticated", FakeJni::JMethodID::PUBLIC },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::gms::games::LeaderboardsClient) { FakeJni::Constructor<LeaderboardsClient> {} },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::gms::games::PlayerStatsClient) { FakeJni::Constructor<PlayerStatsClient> {} },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::gms::games::PlayersClient) { FakeJni::Constructor<PlayersClient> {} },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::gms::games::SnapshotsClient) { FakeJni::Constructor<SnapshotsClient> {} },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::gms::games::PlayGames) {
        FakeJni::Constructor<PlayGames> {},
    },
    { FakeJni::Function<&PlayGames::getAchievementsClient> {}, "getAchievementsClient", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&PlayGames::getEventsClient> {}, "getEventsClient", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&PlayGames::getGamesSignInClient> {}, "getGamesSignInClient", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&PlayGames::getLeaderboardsClient> {}, "getLeaderboardsClient", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&PlayGames::getPlayerStatsClient> {}, "getPlayerStatsClient", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&PlayGames::getPlayersClient> {}, "getPlayersClient", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&PlayGames::getSnapshotsClient> {}, "getSnapshotsClient", FakeJni::JMethodID::STATIC },
    END_NATIVE_DESCRIPTOR

    BEGIN_NATIVE_DESCRIPTOR(jnivm::com::google::android::gms::games::PlayGamesSdk) {
        FakeJni::Constructor<PlayGamesSdk> {},
    },
    { FakeJni::Function<&PlayGamesSdk::initialize> {}, "initialize", FakeJni::JMethodID::STATIC },
    END_NATIVE_DESCRIPTOR

void InitJNIGooglePlayClasses(FakeJni::Jvm* vm)
{
    verbose("JBRIDGE", "Initializing Play Games JNI Classes");
    vm->registerClass<Obfuscator>();
    vm->registerClass<AESObfuscator>();
    vm->registerClass<Policy>();
    vm->registerClass<ServerManagedPolicy>();
    vm->registerClass<APKExpansionPolicy>();
    vm->registerClass<LicenseCheckerCallback>();
    vm->registerClass<LicenseChecker>();
    vm->registerClass<jnivm::com::google::android::play::core::review::ReviewManager>();
    vm->registerClass<jnivm::com::google::android::play::core::review::ReviewManagerFactory>();

    vm->registerClass<jnivm::com::google::android::play::core::tasks::Task>();
    vm->registerClass<jnivm::com::google::android::play::core::tasks::OnCompleteListener>();
    vm->registerClass<jnivm::com::google::android::play::core::tasks::OnSuccessListener>();
    vm->registerClass<jnivm::com::google::android::play::core::tasks::OnFailureListener>();

    vm->registerClass<jnivm::com::google::android::gms::games::AchievementsClient>();
    vm->registerClass<jnivm::com::google::android::gms::games::EventsClient>();
    vm->registerClass<jnivm::com::google::android::gms::games::AuthenticationResult>();
    vm->registerClass<jnivm::com::google::android::gms::games::GamesSignInClient>();
    vm->registerClass<jnivm::com::google::android::gms::games::LeaderboardsClient>();
    vm->registerClass<jnivm::com::google::android::gms::games::PlayerStatsClient>();
    vm->registerClass<jnivm::com::google::android::gms::games::PlayersClient>();
    vm->registerClass<jnivm::com::google::android::gms::games::SnapshotsClient>();
    vm->registerClass<jnivm::com::google::android::gms::games::PlayGames>();
    vm->registerClass<jnivm::com::google::android::gms::games::PlayGamesSdk>();

    vm->registerClass<BillingClient>();
    vm->registerClass<BillingClientBuilder>();
    vm->registerClass<BillingResult>();
    vm->registerClass<PendingPurchasesParams>();
    vm->registerClass<PendingPurchasesParamsBuilder>();
}
