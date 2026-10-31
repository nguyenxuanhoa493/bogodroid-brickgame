#ifndef __APPLOVIN_H__
#define __APPLOVIN_H__

#include "android.h"
#include "baron/baron.h"
#include "javac.h"

void InitJNIAppLovinClasses(FakeJni::Jvm* vm);

// Delivers the MAX SDK events queued by the stubs. Called once per frame from
// the Choreographer proxy so callbacks arrive after the Java call that caused
// them has returned, like the real SDK's background-thread dispatch.
void BDAppLovinFlushEvents();

namespace jnivm {
namespace com {
    namespace applovin {
        namespace mediation {
            namespace unity {

                // The AppLovin MAX Unity bridge
                // (com.applovin.mediation.unity.MaxUnityPlugin). The C# plugin
                // calls these statics and receives every SDK/ad callback as a
                // JSON string on the BackgroundCallback proxy.
                class MaxUnityPlugin : public FakeJni::JObject {
                public:
                    DEFINE_CLASS_NAME("com/applovin/mediation/unity/MaxUnityPlugin")

                    static void setBackgroundCallback(std::shared_ptr<FakeJni::JObject> callback);
                    static void setSdkKey(std::shared_ptr<FakeJni::JString> sdkKey);
                    static void initializeSdk(std::shared_ptr<FakeJni::JString> adUnitIds,
                                              std::shared_ptr<FakeJni::JString> metadata);
                    static bool isInitialized();
                    static bool isVerboseLoggingEnabled();
                    static bool isDisableAllLogs();
                    static void setUserId(std::shared_ptr<FakeJni::JString> userId);
                    static void setSegmentCollection(std::shared_ptr<FakeJni::JString> json);
                    static void setHasUserConsent(bool hasUserConsent);
                    static bool hasUserConsent();
                    static bool isUserConsentSet();
                    static void setDoNotSell(bool doNotSell);
                    static bool isDoNotSell();
                    static bool isDoNotSellSet();
                    static bool isTablet();
                    static bool isPhysicalDevice();
                    static bool isMuted();

                    static void loadInterstitial(std::shared_ptr<FakeJni::JString> adUnitId);
                    static bool isInterstitialReady(std::shared_ptr<FakeJni::JString> adUnitId);
                    static void showInterstitial(std::shared_ptr<FakeJni::JString> adUnitId,
                                                 std::shared_ptr<FakeJni::JString> placement,
                                                 std::shared_ptr<FakeJni::JString> customData);

                    static void loadRewardedAd(std::shared_ptr<FakeJni::JString> adUnitId);
                    static bool isRewardedAdReady(std::shared_ptr<FakeJni::JString> adUnitId);
                    static void showRewardedAd(std::shared_ptr<FakeJni::JString> adUnitId,
                                               std::shared_ptr<FakeJni::JString> placement,
                                               std::shared_ptr<FakeJni::JString> customData);

                    static void loadAppOpenAd(std::shared_ptr<FakeJni::JString> adUnitId);
                    static bool isAppOpenAdReady(std::shared_ptr<FakeJni::JString> adUnitId);
                    static void showAppOpenAd(std::shared_ptr<FakeJni::JString> adUnitId,
                                              std::shared_ptr<FakeJni::JString> placement,
                                              std::shared_ptr<FakeJni::JString> customData);
                };

            }
        }
    }
}
}
#endif
