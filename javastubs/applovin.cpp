#include "applovin.h"
#include "jnibridge.h"
#include "logging.h"
#include <mutex>
#include <set>
#include <string>
#include <typeinfo>
#include <vector>

using namespace jnivm::com::applovin::mediation::unity;
using namespace jnivm::bitter::jnibridge;

namespace {

std::mutex g_lock;
std::vector<std::string> g_pending;
std::shared_ptr<FakeJni::JObject> g_callback;
bool g_sdk_initialized = false;
bool g_consent_set = false;
bool g_user_consent = false;
bool g_dns_set = false;
bool g_do_not_sell = false;
std::set<std::string> g_interstitial_ready;
std::set<std::string> g_rewarded_ready;
std::set<std::string> g_appopen_ready;

void queue_event(std::string json)
{
    std::lock_guard<std::mutex> lock(g_lock);
    g_pending.push_back(std::move(json));
}

// Mirrors MaxUnityAdManager.getAdInfo(): the C# side reads every field by name
// and tolerates empty values, so a fixed skeleton is enough.
std::string ad_event(const char* name, const std::string& adUnitId, const char* format)
{
    std::string json;
    json.reserve(256);
    json += "{\"name\":\"" + std::string(name) + "\"";
    json += ",\"adUnitId\":\"" + adUnitId + "\"";
    json += ",\"adFormat\":\"" + std::string(format) + "\"";
    json += ",\"networkName\":\"\"";
    json += ",\"networkPlacement\":\"\"";
    json += ",\"creativeId\":\"\"";
    json += ",\"placement\":\"\"";
    json += ",\"revenue\":\"0\"";
    json += ",\"revenuePrecision\":\"\"";
    json += ",\"latencyMillis\":\"0\"";
    json += ",\"dspName\":\"\"";
    json += ",\"waterfallInfo\":{}";
    json += "}";
    return json;
}

std::string id_of(const std::shared_ptr<FakeJni::JString>& value)
{
    return value ? value->c_str() : std::string();
}

} // namespace

void BDAppLovinFlushEvents()
{
    std::shared_ptr<FakeJni::JObject> callback;
    {
        std::lock_guard<std::mutex> lock(g_lock);
        if (g_pending.empty())
            return;
        callback = g_callback;
    }
    if (!callback) {
        BD_LOG("APPLOVIN", "dropping %zu queued event(s): no background callback",
               g_pending.size());
        std::lock_guard<std::mutex> lock(g_lock);
        g_pending.clear();
        return;
    }
    auto proxy = std::dynamic_pointer_cast<JNIBridgeProxy>(callback);
    if (!proxy) {
        BD_LOG("APPLOVIN", "background callback is %s, not a proxy; dropping events",
               typeid(*callback).name());
        std::lock_guard<std::mutex> lock(g_lock);
        g_pending.clear();
        return;
    }

    // A handler may queue further events (show -> displayed -> hidden); drain a
    // bounded batch so a misbehaving callback cannot spin this loop forever.
    for (int i = 0; i < 32; ++i) {
        std::string json;
        {
            std::lock_guard<std::mutex> lock(g_lock);
            if (g_pending.empty())
                return;
            json = g_pending.front();
            g_pending.erase(g_pending.begin());
        }
        BD_LOG("APPLOVIN", "event %s", json.c_str());
        proxy->deliver("com/applovin/mediation/unity/MaxUnityAdManager$BackgroundCallback",
                       "onEvent", "(Ljava/lang/String;)V",
                       std::make_shared<FakeJni::JString>(json));
    }
}

void MaxUnityPlugin::setBackgroundCallback(std::shared_ptr<FakeJni::JObject> callback)
{
    std::lock_guard<std::mutex> lock(g_lock);
    g_callback = callback;
    BD_LOG("APPLOVIN", "setBackgroundCallback(%p)", callback.get());
}

void MaxUnityPlugin::setSdkKey(std::shared_ptr<FakeJni::JString> sdkKey)
{
    BD_LOG("APPLOVIN", "setSdkKey(%s)", id_of(sdkKey).c_str());
}

void MaxUnityPlugin::initializeSdk(std::shared_ptr<FakeJni::JString> adUnitIds,
                                   std::shared_ptr<FakeJni::JString> metadata)
{
    BD_LOG("APPLOVIN", "initializeSdk(adUnits='%s', config='%s')",
           id_of(adUnitIds).c_str(), id_of(metadata).c_str());
    g_sdk_initialized = true;
    queue_event("{\"name\":\"OnSdkInitializedEvent\""
                ",\"consentFlowUserGeography\":\"0\""
                ",\"consentDialogState\":\"0\""
                ",\"countryCode\":\"US\""
                ",\"isSuccessfullyInitialized\":\"true\""
                ",\"isTestModeEnabled\":false}");
}

bool MaxUnityPlugin::isInitialized() { return g_sdk_initialized; }

bool MaxUnityPlugin::isVerboseLoggingEnabled() { return false; }

bool MaxUnityPlugin::isDisableAllLogs() { return false; }

void MaxUnityPlugin::setUserId(std::shared_ptr<FakeJni::JString> userId)
{
    BD_LOG("APPLOVIN", "setUserId(%s)", id_of(userId).c_str());
}

void MaxUnityPlugin::setSegmentCollection(std::shared_ptr<FakeJni::JString>) {}

void MaxUnityPlugin::setHasUserConsent(bool hasUserConsent)
{
    g_consent_set = true;
    g_user_consent = hasUserConsent;
}

bool MaxUnityPlugin::hasUserConsent() { return g_user_consent; }

bool MaxUnityPlugin::isUserConsentSet() { return g_consent_set; }

void MaxUnityPlugin::setDoNotSell(bool doNotSell)
{
    g_dns_set = true;
    g_do_not_sell = doNotSell;
}

bool MaxUnityPlugin::isDoNotSell() { return g_do_not_sell; }

bool MaxUnityPlugin::isDoNotSellSet() { return g_dns_set; }

bool MaxUnityPlugin::isTablet() { return false; }

bool MaxUnityPlugin::isPhysicalDevice() { return true; }

bool MaxUnityPlugin::isMuted() { return true; }

void MaxUnityPlugin::loadInterstitial(std::shared_ptr<FakeJni::JString> adUnitId)
{
    const std::string id = id_of(adUnitId);
    BD_LOG("APPLOVIN", "loadInterstitial(%s): reporting loaded", id.c_str());
    g_interstitial_ready.insert(id);
    queue_event(ad_event("OnInterstitialLoadedEvent", id, "INTERSTITIAL"));
}

bool MaxUnityPlugin::isInterstitialReady(std::shared_ptr<FakeJni::JString> adUnitId)
{
    return g_interstitial_ready.count(id_of(adUnitId)) != 0;
}

void MaxUnityPlugin::showInterstitial(std::shared_ptr<FakeJni::JString> adUnitId,
                                      std::shared_ptr<FakeJni::JString> placement,
                                      std::shared_ptr<FakeJni::JString>)
{
    const std::string id = id_of(adUnitId);
    // A show with nothing loaded still has to answer: games chain
    // show -> (Displayed/Failed) -> Hidden and hold the splash until the ad
    // closes. Returning silently leaves that wait unfinished forever.
    g_interstitial_ready.erase(id);
    BD_LOG("APPLOVIN", "showInterstitial(%s, placement='%s'): displayed+hidden",
           id.c_str(), id_of(placement).c_str());
    queue_event(ad_event("OnInterstitialDisplayedEvent", id, "INTERSTITIAL"));
    queue_event(ad_event("OnInterstitialHiddenEvent", id, "INTERSTITIAL"));
}

void MaxUnityPlugin::loadRewardedAd(std::shared_ptr<FakeJni::JString> adUnitId)
{
    const std::string id = id_of(adUnitId);
    BD_LOG("APPLOVIN", "loadRewardedAd(%s): reporting loaded", id.c_str());
    g_rewarded_ready.insert(id);
    queue_event(ad_event("OnRewardedAdLoadedEvent", id, "REWARDED"));
}

bool MaxUnityPlugin::isRewardedAdReady(std::shared_ptr<FakeJni::JString> adUnitId)
{
    return g_rewarded_ready.count(id_of(adUnitId)) != 0;
}

void MaxUnityPlugin::showRewardedAd(std::shared_ptr<FakeJni::JString> adUnitId,
                                    std::shared_ptr<FakeJni::JString> placement,
                                    std::shared_ptr<FakeJni::JString>)
{
    const std::string id = id_of(adUnitId);
    g_rewarded_ready.erase(id);
    BD_LOG("APPLOVIN", "showRewardedAd(%s, placement='%s'): displayed+rewarded+hidden",
           id.c_str(), id_of(placement).c_str());
    queue_event(ad_event("OnRewardedAdDisplayedEvent", id, "REWARDED"));
    queue_event(ad_event("OnRewardedAdReceivedRewardEvent", id, "REWARDED"));
    queue_event(ad_event("OnRewardedAdHiddenEvent", id, "REWARDED"));
}

void MaxUnityPlugin::loadAppOpenAd(std::shared_ptr<FakeJni::JString> adUnitId)
{
    const std::string id = id_of(adUnitId);
    BD_LOG("APPLOVIN", "loadAppOpenAd(%s): reporting loaded", id.c_str());
    g_appopen_ready.insert(id);
    queue_event(ad_event("OnAppOpenAdLoadedEvent", id, "APPOPEN"));
}

bool MaxUnityPlugin::isAppOpenAdReady(std::shared_ptr<FakeJni::JString> adUnitId)
{
    return g_appopen_ready.count(id_of(adUnitId)) != 0;
}

void MaxUnityPlugin::showAppOpenAd(std::shared_ptr<FakeJni::JString> adUnitId,
                                   std::shared_ptr<FakeJni::JString> placement,
                                   std::shared_ptr<FakeJni::JString>)
{
    const std::string id = id_of(adUnitId);
    g_appopen_ready.erase(id);
    BD_LOG("APPLOVIN", "showAppOpenAd(%s, placement='%s'): displayed+hidden",
           id.c_str(), id_of(placement).c_str());
    queue_event(ad_event("OnAppOpenAdDisplayedEvent", id, "APPOPEN"));
    queue_event(ad_event("OnAppOpenAdHiddenEvent", id, "APPOPEN"));
}

BEGIN_NATIVE_DESCRIPTOR(jnivm::com::applovin::mediation::unity::MaxUnityPlugin)
    { FakeJni::Function<&MaxUnityPlugin::setBackgroundCallback> {}, "setBackgroundCallback", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::setSdkKey> {}, "setSdkKey", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::initializeSdk> {}, "initializeSdk", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isInitialized> {}, "isInitialized", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isVerboseLoggingEnabled> {}, "isVerboseLoggingEnabled", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isDisableAllLogs> {}, "isDisableAllLogs", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::setUserId> {}, "setUserId", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::setSegmentCollection> {}, "setSegmentCollection", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::setHasUserConsent> {}, "setHasUserConsent", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::hasUserConsent> {}, "hasUserConsent", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isUserConsentSet> {}, "isUserConsentSet", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::setDoNotSell> {}, "setDoNotSell", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isDoNotSell> {}, "isDoNotSell", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isDoNotSellSet> {}, "isDoNotSellSet", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isTablet> {}, "isTablet", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isPhysicalDevice> {}, "isPhysicalDevice", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isMuted> {}, "isMuted", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::loadInterstitial> {}, "loadInterstitial", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isInterstitialReady> {}, "isInterstitialReady", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::showInterstitial> {}, "showInterstitial", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::loadRewardedAd> {}, "loadRewardedAd", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isRewardedAdReady> {}, "isRewardedAdReady", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::showRewardedAd> {}, "showRewardedAd", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::loadAppOpenAd> {}, "loadAppOpenAd", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::isAppOpenAdReady> {}, "isAppOpenAdReady", FakeJni::JMethodID::STATIC },
    { FakeJni::Function<&MaxUnityPlugin::showAppOpenAd> {}, "showAppOpenAd", FakeJni::JMethodID::STATIC },
    END_NATIVE_DESCRIPTOR

void InitJNIAppLovinClasses(FakeJni::Jvm* vm)
{
    verbose("JBRIDGE", "Initializing AppLovin MAX JNI Classes");
    vm->registerClass<jnivm::com::applovin::mediation::unity::MaxUnityPlugin>();
}
